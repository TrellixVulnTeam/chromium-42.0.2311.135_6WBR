// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/usb_error.h"

#if defined(__FreeBSD__)
#include "libusb.h"
#else
#include "third_party/libusb/src/libusb/libusb.h"
#endif

namespace device {

std::string ConvertPlatformUsbErrorToString(int errcode) {
  return libusb_strerror(static_cast<libusb_error>(errcode));
}

}  // namespace device
