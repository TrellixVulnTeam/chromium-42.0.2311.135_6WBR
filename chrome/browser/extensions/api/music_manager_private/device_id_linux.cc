// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/music_manager_private/device_id.h"

#if defined(OS_FREEBSD)
#include <sys/types.h>
#include <net/if_dl.h>
#endif
#include <sys/socket.h>  // Must be included before ifaddrs.h.
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <map>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/browser_thread.h"

namespace {

using extensions::api::DeviceId;

typedef base::Callback<bool(const void* bytes, size_t size)>
    IsValidMacAddressCallback;

const char kDiskByUuidDirectoryName[] = "/dev/disk/by-uuid";
const char* const kDeviceNames[] = {
  "sda1", "hda1", "dm-0", "xvda1", "sda2", "hda2", "dm-1", "xvda2",
};
// Fedora 15 uses biosdevname feature where Embedded ethernet uses the
// "em" prefix and PCI cards use the p[0-9]c[0-9] format based on PCI
// slot and card information.
const char* const kNetDeviceNamePrefixes[] = {
  "eth", "em", "en", "wl", "ww", "p0", "p1", "p2", "p3", "p4", "p5", "p6",
  "p7", "p8", "p9", "wlan"
};

// Map from device name to disk uuid
typedef std::map<base::FilePath, base::FilePath> DiskEntries;

std::string GetDiskUuid() {
  base::ThreadRestrictions::AssertIOAllowed();

  DiskEntries disk_uuids;
  base::FileEnumerator files(base::FilePath(kDiskByUuidDirectoryName),
                             false,  // Recursive.
                             base::FileEnumerator::FILES);
  do {
    base::FilePath file_path = files.Next();
    if (file_path.empty())
      break;

    base::FilePath target_path;
    if (!base::ReadSymbolicLink(file_path, &target_path))
      continue;

    base::FilePath device_name = target_path.BaseName();
    base::FilePath disk_uuid = file_path.BaseName();
    disk_uuids[device_name] = disk_uuid;
  } while (true);

  // Look for first device name matching an entry of |kDeviceNames|.
  std::string result;
  for (size_t i = 0; i < arraysize(kDeviceNames); i++) {
    DiskEntries::iterator it =
        disk_uuids.find(base::FilePath(kDeviceNames[i]));
    if (it != disk_uuids.end()) {
      DVLOG(1) << "Returning uuid: \"" << it->second.value()
               << "\" for device \"" << it->first.value() << "\"";
      result = it->second.value();
      break;
    }
  }

  // Log failure (at most once) for diagnostic purposes.
  static bool error_logged = false;
  if (result.empty() && !error_logged) {
    error_logged = true;
    LOG(ERROR) << "Could not find appropriate disk uuid.";
    for (DiskEntries::iterator it = disk_uuids.begin();
        it != disk_uuids.end(); ++it) {
      LOG(ERROR) << "  DeviceID=" << it->first.value() << ", uuid="
                 << it->second.value();
    }
  }

  return result;
}

class MacAddressProcessor {
 public:
  explicit MacAddressProcessor(
      const IsValidMacAddressCallback& is_valid_mac_address)
      : is_valid_mac_address_(is_valid_mac_address) {
  }

  bool ProcessInterface(struct ifaddrs *ifaddr,
                        const char* const prefixes[],
                        size_t prefixes_count) {
    const int MAC_LENGTH = 6;
#if defined(OS_FREEBSD)
    struct ifaddrs *ifap, *ifinfo;
#else
    struct ifreq ifinfo;
#endif

    memset(&ifinfo, 0, sizeof(ifinfo));

#if defined(OS_FREEBSD)
    int result = getifaddrs(&ifap);
    if (result != 0)
      return true;
    result = 1; // no MAC found yet
    for (ifinfo = ifap; ifinfo != NULL; ifinfo = ifinfo->ifa_next) {
      struct sockaddr* sa = ifinfo->ifa_addr;
      if (sa->sa_family == AF_LINK &&
        !strncmp(ifinfo->ifa_name, ifaddr->ifa_name,
        sizeof(ifinfo->ifa_name) - 1)) { 
          result = 0;
	  break;
      }
    }

    char mac_address[6];
    strncpy(mac_address, (const char*)LLADDR((struct sockaddr_dl*)ifinfo->ifa_addr), sizeof(mac_address));
#else
    strncpy(ifinfo.ifr_name, ifaddr->ifa_name, sizeof(ifinfo.ifr_name) - 1);
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    int result = ioctl(sd, SIOCGIFHWADDR, &ifinfo);
    close(sd);

    if (result != 0)
      return true;

    const char* mac_address =
        static_cast<const char*>(ifinfo.ifr_hwaddr.sa_data);
#endif
    if (!is_valid_mac_address_.Run(mac_address, MAC_LENGTH))
      return true;

#if defined(OS_FREEBSD)
    if (!IsValidPrefix(ifinfo->ifa_name, prefixes, prefixes_count))
      return true;
#else
    if (!IsValidPrefix(ifinfo.ifr_name, prefixes, prefixes_count))
      return true;
#endif

    // Got one!
    found_mac_address_ =
        base::StringToLowerASCII(base::HexEncode(mac_address, MAC_LENGTH));
    return false;
  }

  std::string mac_address() const { return found_mac_address_; }

 private:
  bool IsValidPrefix(const char* name,
                     const char* const prefixes[],
                     size_t prefixes_count) {
    for (size_t i = 0; i < prefixes_count; i++) {
      if (strncmp(prefixes[i], name, strlen(prefixes[i])) == 0)
        return true;
    }
    return false;
  }

  const IsValidMacAddressCallback& is_valid_mac_address_;
  std::string found_mac_address_;
};

std::string GetMacAddress(
    const IsValidMacAddressCallback& is_valid_mac_address) {
  base::ThreadRestrictions::AssertIOAllowed();

  struct ifaddrs* ifaddrs;
  int rv = getifaddrs(&ifaddrs);
  if (rv < 0) {
    PLOG(ERROR) << "getifaddrs failed " << rv;
    return "";
  }

  MacAddressProcessor processor(is_valid_mac_address);
  for (struct ifaddrs* ifa = ifaddrs; ifa; ifa = ifa->ifa_next) {
    bool keep_going = processor.ProcessInterface(
        ifa, kNetDeviceNamePrefixes, arraysize(kNetDeviceNamePrefixes));
    if (!keep_going)
      break;
  }
  freeifaddrs(ifaddrs);
  return processor.mac_address();
}

void GetRawDeviceIdImpl(const IsValidMacAddressCallback& is_valid_mac_address,
                        const DeviceId::IdCallback& callback) {
  base::ThreadRestrictions::AssertIOAllowed();

  std::string disk_id = GetDiskUuid();
  std::string mac_address = GetMacAddress(is_valid_mac_address);

  std::string raw_device_id;
  if (!mac_address.empty() && !disk_id.empty()) {
    raw_device_id = mac_address + disk_id;
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(callback, raw_device_id));
}

}  // namespace

namespace extensions {
namespace api {

// static
void DeviceId::GetRawDeviceId(const IdCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::BrowserThread::PostTask(
      content::BrowserThread::FILE,
      FROM_HERE,
      base::Bind(GetRawDeviceIdImpl,
          base::Bind(DeviceId::IsValidMacAddress),
          callback));
}

}  // namespace api
}  // namespace extensions
