/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file contains the WebRTC suppressions for LeakSanitizer.
// You can also pass additional suppressions via LSAN_OPTIONS:
// LSAN_OPTIONS=suppressions=/path/to/suppressions. Please refer to
// http://dev.chromium.org/developers/testing/leaksanitizer for more info.

#if defined(LEAK_SANITIZER)

// Please make sure the code below declares a single string variable
// kLSanDefaultSuppressions which contains LSan suppressions delimited by
// newlines. See http://dev.chromium.org/developers/testing/leaksanitizer
// for the instructions on writing suppressions.
char kLSanDefaultSuppressions[] =

// ============ Leaks in third-party code shared with Chromium =============
// These entries are copied from build/sanitizers/lsan_suppressions.cc in
// Chromium. Please don't add new entries here unless they're present in there.

// False positives in libfontconfig. http://crbug.com/39050
"leak:libfontconfig\n"

// Leaks in Nvidia's libGL.
"leak:libGL.so\n"

// TODO(earthdok): revisit NSS suppressions after the switch to BoringSSL
// NSS leaks in CertDatabaseNSSTest tests. http://crbug.com/51988
"leak:net::NSSCertDatabase::ImportFromPKCS12\n"
"leak:net::NSSCertDatabase::ListCerts\n"
"leak:net::NSSCertDatabase::DeleteCertAndKey\n"
"leak:crypto::ScopedTestNSSDB::ScopedTestNSSDB\n"
// Another leak due to not shutting down NSS properly. http://crbug.com/124445
"leak:error_get_my_stack\n"
// The NSS suppressions above will not fire when the fast stack unwinder is
// used, because it can't unwind through NSS libraries. Apply blanket
// suppressions for now.
"leak:libnssutil3\n"
"leak:libnspr4\n"
"leak:libnss3\n"
"leak:libplds4\n"
"leak:libnssckbi\n"

// XRandR has several one time leaks.
"leak:libxrandr\n"

// xrandr leak. http://crbug.com/119677
"leak:XRRFindDisplay\n"

// ========== Leaks in third-party code not shared with Chromium ===========

// None known so far.

// ================ Leaks in WebRTC code ================
// PLEASE DO NOT ADD SUPPRESSIONS FOR NEW LEAKS.
// Instead, commits that introduce memory leaks should be reverted. Suppressing
// the leak is acceptable in some cases when reverting is impossible, i.e. when
// enabling leak detection for the first time for a test target with
// pre-existing leaks.

// rtc_unittest
// https://code.google.com/p/webrtc/issues/detail?id=3827 for details.
"leak:rtc::unstarted_task_test_DoNotDeleteTask2_Test::TestBody\n"
"leak:rtc::HttpServer::HandleConnection\n"
"leak:rtc::HttpServer::Connection::onHttpHeaderComplete\n"
"leak:rtc::HttpResponseData::set_success\n"
"leak:rtc::HttpData::changeHeader\n"
// https://code.google.com/p/webrtc/issues/detail?id=4149 for details.
"leak:StartDNSLookup\n"

// libjingle_media_unittest
"leak:cricket::FakeNetworkInterface::SetOption\n"
"leak:CodecTest_TestCodecOperators_Test::TestBody\n"
"leak:VideoEngineTest*::ConstrainNewCodecBody\n"
"leak:VideoMediaChannelTest*::AddRemoveRecvStreams\n"
"leak:WebRtcVideoCapturerTest_TestCapture_Test::TestBody\n"
"leak:WebRtcVideoEngineTestFake_MultipleSendStreamsWithOneCapturer_Test::TestBody\n"
"leak:WebRtcVideoEngineTestFake_SetBandwidthInConference_Test::TestBody\n"
"leak:WebRtcVideoEngineTestFake_SetSendCodecsRejectBadFormat_Test::TestBody\n"

// libjingle_peerconnection_unittest
// https://code.google.com/p/webrtc/issues/detail?id=2528
"leak:cricket::FakeVideoMediaChannel::~FakeVideoMediaChannel\n"
"leak:cricket::MediaSessionDescriptionFactory::CreateAnswer\n"
"leak:cricket::MediaSessionDescriptionFactory::CreateOffer\n"
"leak:DtmfSenderTest_InsertEmptyTonesToCancelPreviousTask_Test::TestBody\n"
"leak:sigslot::_signal_base2*::~_signal_base2\n"
"leak:testing::internal::CmpHelperEQ\n"
"leak:webrtc::AudioDeviceLinuxALSA::InitMicrophone\n"
"leak:webrtc::AudioDeviceLinuxALSA::InitSpeaker\n"
"leak:webrtc::CreateIceCandidate\n"
"leak:webrtc::FakeConstraints::AddOptional\n"
"leak:webrtc::WebRtcIdentityRequestObserver::OnSuccess\n"
"leak:webrtc::WebRtcSessionDescriptionFactory::InternalCreateAnswer\n"
"leak:webrtc::WebRtcSessionDescriptionFactory::InternalCreateOffer\n"
"leak:PeerConnectionInterfaceTest_SsrcInOfferAnswer_Test::TestBody\n"
"leak:PeerConnectionInterfaceTest_CloseAndTestMethods_Test::TestBody\n"
"leak:WebRtcSdpTest::TestDeserializeRtcpFb\n"
"leak:WebRtcSdpTest::TestSerialize\n"
"leak:WebRtcSdpTest_SerializeSessionDescriptionWithDataChannelAndBandwidth_Test::TestBody\n"
"leak:WebRtcSdpTest_SerializeSessionDescriptionWithBandwidth_Test::TestBody\n"
"leak:WebRtcSessionTest::SetLocalDescriptionExpectError\n"
"leak:WebRtcSessionTest_TestAVOfferWithAudioOnlyAnswer_Test::TestBody\n"

// libjingle_unittest
// https://code.google.com/p/webrtc/issues/detail?id=2527
"leak:buzz::FakeXmppClient::SendStanza\n"
"leak:buzz::XmppTask::XmppTask\n"
"leak:HangoutPubSubClientTest::HangoutPubSubClientTest\n"
"leak:MucRoomConfigTaskTest::SetUp\n"
"leak:MucRoomDiscoveryTaskTest::SetUp\n"
"leak:MucRoomLookupTaskTest::SetUp\n"
"leak:MucRoomUniqueHangoutIdTaskTest::SetUp\n"
"leak:PingTaskTest::SetUp\n"
"leak:PingXmppClient::SendStanza\n"
"leak:PubSubClientTest::PubSubClientTest\n"
"leak:PubSubTasksTest::PubSubTasksTest\n"

// PLEASE READ ABOVE BEFORE ADDING NEW SUPPRESSIONS.

// End of suppressions.
;  // Please keep this semicolon.

#endif  // LEAK_SANITIZER
