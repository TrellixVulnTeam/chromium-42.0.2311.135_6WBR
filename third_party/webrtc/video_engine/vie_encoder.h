/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_ENGINE_VIE_ENCODER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_ENCODER_H_

#include <list>
#include <map>
#include <vector>

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/thread_annotations.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/bitrate_controller/include/bitrate_controller.h"
#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"
#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/typedefs.h"
#include "webrtc/frame_callback.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"
#include "webrtc/video_engine/vie_defines.h"
#include "webrtc/video_engine/vie_frame_provider_base.h"

namespace webrtc {

class Config;
class CriticalSectionWrapper;
class EncodedImageCallback;
class PacedSender;
class PayloadRouter;
class ProcessThread;
class QMVideoSettingsCallback;
class RtpRtcp;
class SendStatisticsProxy;
class ViEBitrateObserver;
class ViEEffectFilter;
class ViEEncoderObserver;
class VideoCodingModule;
class ViEPacedSenderCallback;

class ViEEncoder
    : public RtcpIntraFrameObserver,
      public VCMPacketizationCallback,
      public VCMProtectionCallback,
      public VCMSendStatisticsCallback,
      public ViEFrameCallback {
 public:
  friend class ViEBitrateObserver;
  friend class ViEPacedSenderCallback;

  ViEEncoder(int32_t engine_id,
             int32_t channel_id,
             uint32_t number_of_cores,
             const Config& config,
             ProcessThread& module_process_thread,
             BitrateController* bitrate_controller,
             bool disable_default_encoder);
  ~ViEEncoder();

  bool Init();

  // This function is assumed to be called before any frames are delivered and
  // only once.
  // Ideally this would be done in Init, but the dependencies between ViEEncoder
  // and ViEChannel makes it really hard to do in a good way.
  void StartThreadsAndSetSendPayloadRouter(
      scoped_refptr<PayloadRouter> send_payload_router);

  // This function must be called before the corresponding ViEChannel is
  // deleted.
  void StopThreadsAndRemovePayloadRouter();

  void SetNetworkTransmissionState(bool is_transmitting);

  // Returns the id of the owning channel.
  int Owner() const;

  // Drops incoming packets before they get to the encoder.
  void Pause();
  void Restart();

  // Codec settings.
  uint8_t NumberOfCodecs();
  int32_t GetCodec(uint8_t list_index, VideoCodec* video_codec);
  int32_t RegisterExternalEncoder(VideoEncoder* encoder,
                                  uint8_t pl_type,
                                  bool internal_source);
  int32_t DeRegisterExternalEncoder(uint8_t pl_type);
  int32_t SetEncoder(const VideoCodec& video_codec);
  int32_t GetEncoder(VideoCodec* video_codec);

  int32_t GetCodecConfigParameters(
    unsigned char config_parameters[kConfigParameterSize],
    unsigned char& config_parameters_size);

  PacedSender* GetPacedSender();

  // Scale or crop/pad image.
  int32_t ScaleInputImage(bool enable);

  // RTP settings.
  RtpRtcp* SendRtpRtcpModule();

  // Implementing ViEFrameCallback.
  virtual void DeliverFrame(int id,
                            I420VideoFrame* video_frame,
                            const std::vector<uint32_t>& csrcs) OVERRIDE;
  virtual void DelayChanged(int id, int frame_delay) OVERRIDE;
  virtual int GetPreferedFrameSettings(int* width,
                                       int* height,
                                       int* frame_rate) OVERRIDE;

  virtual void ProviderDestroyed(int id) OVERRIDE {
    return;
  }

  int32_t SendKeyFrame();
  int32_t SendCodecStatistics(uint32_t* num_key_frames,
                              uint32_t* num_delta_frames);

  int64_t PacerQueuingDelayMs() const;

  int CodecTargetBitrate(uint32_t* bitrate) const;
  // Loss protection.
  int32_t UpdateProtectionMethod(bool enable_nack);
  bool nack_enabled() const { return nack_enabled_; }

  // Buffering mode.
  void SetSenderBufferingMode(int target_delay_ms);

  // Implements VCMPacketizationCallback.
  virtual int32_t SendData(uint8_t payload_type,
                           const EncodedImage& encoded_image,
                           const RTPFragmentationHeader& fragmentation_header,
                           const RTPVideoHeader* rtp_video_hdr) OVERRIDE;

  // Implements VideoProtectionCallback.
  virtual int ProtectionRequest(
      const FecProtectionParams* delta_fec_params,
      const FecProtectionParams* key_fec_params,
      uint32_t* sent_video_rate_bps,
      uint32_t* sent_nack_rate_bps,
      uint32_t* sent_fec_rate_bps) OVERRIDE;

  // Implements VideoSendStatisticsCallback.
  virtual int32_t SendStatistics(const uint32_t bit_rate,
                                 const uint32_t frame_rate) OVERRIDE;

  int32_t RegisterCodecObserver(ViEEncoderObserver* observer);

  // Implements RtcpIntraFrameObserver.
  virtual void OnReceivedIntraFrameRequest(uint32_t ssrc) OVERRIDE;
  virtual void OnReceivedSLI(uint32_t ssrc, uint8_t picture_id) OVERRIDE;
  virtual void OnReceivedRPSI(uint32_t ssrc, uint64_t picture_id) OVERRIDE;
  virtual void OnLocalSsrcChanged(uint32_t old_ssrc,
                                  uint32_t new_ssrc) OVERRIDE;

  // Sets SSRCs for all streams.
  bool SetSsrcs(const std::list<unsigned int>& ssrcs);

  void SetMinTransmitBitrate(int min_transmit_bitrate_kbps);

  // Effect filter.
  int32_t RegisterEffectFilter(ViEEffectFilter* effect_filter);

  // Enables recording of debugging information.
  int StartDebugRecording(const char* fileNameUTF8);

  // Disables recording of debugging information.
  int StopDebugRecording();

  // Lets the sender suspend video when the rate drops below
  // |threshold_bps|, and turns back on when the rate goes back up above
  // |threshold_bps| + |window_bps|.
  void SuspendBelowMinBitrate();

  // New-style callbacks, used by VideoSendStream.
  void RegisterPreEncodeCallback(I420FrameCallback* pre_encode_callback);
  void DeRegisterPreEncodeCallback();
  void RegisterPostEncodeImageCallback(
        EncodedImageCallback* post_encode_callback);
  void DeRegisterPostEncodeImageCallback();

  void RegisterSendStatisticsProxy(SendStatisticsProxy* send_statistics_proxy);

  int channel_id() const { return channel_id_; }

 protected:
  // Called by BitrateObserver.
  void OnNetworkChanged(uint32_t bitrate_bps,
                        uint8_t fraction_lost,
                        int64_t round_trip_time_ms);

  // Called by PacedSender.
  bool TimeToSendPacket(uint32_t ssrc, uint16_t sequence_number,
                        int64_t capture_time_ms, bool retransmission);
  size_t TimeToSendPadding(size_t bytes);
 private:
  bool EncoderPaused() const EXCLUSIVE_LOCKS_REQUIRED(data_cs_);
  void TraceFrameDropStart() EXCLUSIVE_LOCKS_REQUIRED(data_cs_);
  void TraceFrameDropEnd() EXCLUSIVE_LOCKS_REQUIRED(data_cs_);

  void UpdateHistograms();

  int32_t engine_id_;
  const int channel_id_;
  const uint32_t number_of_cores_;
  const bool disable_default_encoder_;

  VideoCodingModule& vcm_;
  VideoProcessingModule& vpm_;
  rtc::scoped_ptr<RtpRtcp> default_rtp_rtcp_;
  scoped_refptr<PayloadRouter> send_payload_router_;
  rtc::scoped_ptr<CriticalSectionWrapper> callback_cs_;
  rtc::scoped_ptr<CriticalSectionWrapper> data_cs_;
  rtc::scoped_ptr<BitrateObserver> bitrate_observer_;
  rtc::scoped_ptr<PacedSender> paced_sender_;
  rtc::scoped_ptr<ViEPacedSenderCallback> pacing_callback_;

  BitrateController* bitrate_controller_;

  int64_t time_of_last_incoming_frame_ms_ GUARDED_BY(data_cs_);
  bool send_padding_ GUARDED_BY(data_cs_);
  int min_transmit_bitrate_kbps_ GUARDED_BY(data_cs_);
  int target_delay_ms_ GUARDED_BY(data_cs_);
  bool network_is_transmitting_ GUARDED_BY(data_cs_);
  bool encoder_paused_ GUARDED_BY(data_cs_);
  bool encoder_paused_and_dropped_frame_ GUARDED_BY(data_cs_);
  std::map<unsigned int, int64_t> time_last_intra_request_ms_
      GUARDED_BY(data_cs_);

  bool fec_enabled_;
  bool nack_enabled_;

  ViEEncoderObserver* codec_observer_ GUARDED_BY(callback_cs_);
  ViEEffectFilter* effect_filter_ GUARDED_BY(callback_cs_);
  ProcessThread& module_process_thread_;
  rtc::scoped_ptr<ProcessThread> pacer_thread_;

  bool has_received_sli_ GUARDED_BY(data_cs_);
  uint8_t picture_id_sli_ GUARDED_BY(data_cs_);
  bool has_received_rpsi_ GUARDED_BY(data_cs_);
  uint64_t picture_id_rpsi_ GUARDED_BY(data_cs_);
  std::map<unsigned int, int> ssrc_streams_ GUARDED_BY(data_cs_);

  // Quality modes callback
  QMVideoSettingsCallback* qm_callback_;
  bool video_suspended_ GUARDED_BY(data_cs_);
  I420FrameCallback* pre_encode_callback_ GUARDED_BY(callback_cs_);
  const int64_t start_ms_;

  SendStatisticsProxy* send_statistics_proxy_;
};

}  // namespace webrtc

#endif  // WEBRTC_VIDEO_ENGINE_VIE_ENCODER_H_
