/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * A2DP Codecs Configuration
 */

#define LOG_TAG "a2dp_codec"

#include "a2dp_codec_api.h"

#include <base/logging.h>
#include <inttypes.h>

#include "a2dp_aac.h"
#include "a2dp_sbc.h"
#include "a2dp_vendor.h"
#include "a2dp_vendor_aptx.h"
#include "a2dp_vendor_aptx_hd.h"
#include "a2dp_vendor_ldac.h"
#include "a2dp_vendor_lhdcv1.h"
#include "a2dp_vendor_lhdcv2.h"
#include "a2dp_vendor_lhdcv3.h"
#include "a2dp_vendor_lhdcv3_dec.h"
#include "a2dp_vendor_lhdcv5.h"
#include "bta/av/bta_av_int.h"
#include "osi/include/log.h"
#include "osi/include/properties.h"

/* The Media Type offset within the codec info byte array */
#define A2DP_MEDIA_TYPE_OFFSET 1

/* A2DP Offload enabled in stack */
static bool a2dp_offload_status;

// Initializes the codec config.
// |codec_config| is the codec config to initialize.
// |codec_index| and |codec_priority| are the codec type and priority to use
// for the initialization.

static void init_btav_a2dp_codec_config(
    btav_a2dp_codec_config_t* codec_config, btav_a2dp_codec_index_t codec_index,
    btav_a2dp_codec_priority_t codec_priority) {
  memset(codec_config, 0, sizeof(btav_a2dp_codec_config_t));
  codec_config->codec_type = codec_index;
  codec_config->codec_priority = codec_priority;
}

A2dpCodecConfig::A2dpCodecConfig(btav_a2dp_codec_index_t codec_index,
                                 const std::string& name,
                                 btav_a2dp_codec_priority_t codec_priority)
    : codec_index_(codec_index),
      name_(name),
      default_codec_priority_(codec_priority) {
  setCodecPriority(codec_priority);

  init_btav_a2dp_codec_config(&codec_config_, codec_index_, codecPriority());
  init_btav_a2dp_codec_config(&codec_capability_, codec_index_,
                              codecPriority());
  init_btav_a2dp_codec_config(&codec_local_capability_, codec_index_,
                              codecPriority());
  init_btav_a2dp_codec_config(&codec_selectable_capability_, codec_index_,
                              codecPriority());
  init_btav_a2dp_codec_config(&codec_user_config_, codec_index_,
                              BTAV_A2DP_CODEC_PRIORITY_DEFAULT);
  init_btav_a2dp_codec_config(&codec_audio_config_, codec_index_,
                              BTAV_A2DP_CODEC_PRIORITY_DEFAULT);

  memset(ota_codec_config_, 0, sizeof(ota_codec_config_));
  memset(ota_codec_peer_capability_, 0, sizeof(ota_codec_peer_capability_));
  memset(ota_codec_peer_config_, 0, sizeof(ota_codec_peer_config_));
}

A2dpCodecConfig::~A2dpCodecConfig() {}

void A2dpCodecConfig::setCodecPriority(
    btav_a2dp_codec_priority_t codec_priority) {
  if (codec_priority == BTAV_A2DP_CODEC_PRIORITY_DEFAULT) {
    // Compute the default codec priority
    setDefaultCodecPriority();
  } else {
    codec_priority_ = codec_priority;
  }
  codec_config_.codec_priority = codec_priority_;
}

void A2dpCodecConfig::setDefaultCodecPriority() {
  if (default_codec_priority_ != BTAV_A2DP_CODEC_PRIORITY_DEFAULT) {
    codec_priority_ = default_codec_priority_;
  } else {
    // Compute the default codec priority
    uint32_t priority = 1000 * (codec_index_ + 1) + 1;
    codec_priority_ = static_cast<btav_a2dp_codec_priority_t>(priority);
  }
  codec_config_.codec_priority = codec_priority_;
}

A2dpCodecConfig* A2dpCodecConfig::createCodec(
    btav_a2dp_codec_index_t codec_index,
    btav_a2dp_codec_priority_t codec_priority) {
  LOG_DEBUG(LOG_TAG, "%s: codec %s", __func__, A2DP_CodecIndexStr(codec_index));

  A2dpCodecConfig* codec_config = nullptr;
  switch (codec_index) {
    case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
      codec_config = new A2dpCodecConfigSbcSource(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SINK_SBC:
      codec_config = new A2dpCodecConfigSbcSink(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
      codec_config = new A2dpCodecConfigAacSource(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SINK_AAC:
      codec_config = new A2dpCodecConfigAacSink(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX:
      codec_config = new A2dpCodecConfigAptx(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_HD:
      codec_config = new A2dpCodecConfigAptxHd(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_LDAC:
      codec_config = new A2dpCodecConfigLdacSource(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SINK_LDAC:
      codec_config = new A2dpCodecConfigLdacSink(codec_priority);
      break;
    // Savitech Patch
    case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV3:
      codec_config = new A2dpCodecConfigLhdcV3(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV2:
      codec_config = new A2dpCodecConfigLhdcV2(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV1:
      codec_config = new A2dpCodecConfigLhdcV1(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV5:
      codec_config = new A2dpCodecConfigLhdcV5Source(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SINK_LHDCV3:
      codec_config = new A2dpCodecConfigLhdcV3Sink(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SINK_LHDCV5:
      codec_config = new A2dpCodecConfigLhdcV5Sink(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_MAX:
      break;
  }

  if (codec_config != nullptr) {
    if (!codec_config->init()) {
      delete codec_config;
      codec_config = nullptr;
    }
  }

  return codec_config;
}

int A2dpCodecConfig::getTrackBitRate() const {
  uint8_t p_codec_info[AVDT_CODEC_SIZE];
  memcpy(p_codec_info, ota_codec_config_, sizeof(ota_codec_config_));
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetBitrateSbc();
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetBitRateAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetBitRate(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

bool A2dpCodecConfig::getCodecSpecificConfig(tBT_A2DP_OFFLOAD* p_a2dp_offload) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  uint8_t codec_config[AVDT_CODEC_SIZE];
  uint32_t vendor_id;
  uint16_t codec_id;

  memset(p_a2dp_offload->codec_info, 0, sizeof(p_a2dp_offload->codec_info));

  if (!A2DP_IsSourceCodecValid(ota_codec_config_)) {
    return false;
  }

  memcpy(codec_config, ota_codec_config_, sizeof(ota_codec_config_));
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(codec_config);
  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      p_a2dp_offload->codec_info[0] =
          codec_config[4];  // blk_len | subbands | Alloc Method
      p_a2dp_offload->codec_info[1] = codec_config[5];  // Min bit pool
      p_a2dp_offload->codec_info[2] = codec_config[6];  // Max bit pool
      p_a2dp_offload->codec_info[3] =
          codec_config[3];  // Sample freq | channel mode
      break;
    case A2DP_MEDIA_CT_AAC:
      p_a2dp_offload->codec_info[0] = codec_config[3];  // object type
      p_a2dp_offload->codec_info[1] = codec_config[6];  // VBR | BR
      break;
    case A2DP_MEDIA_CT_NON_A2DP:
      vendor_id = A2DP_VendorCodecGetVendorId(codec_config);
      codec_id = A2DP_VendorCodecGetCodecId(codec_config);
      p_a2dp_offload->codec_info[0] = (vendor_id & 0x000000FF);
      p_a2dp_offload->codec_info[1] = (vendor_id & 0x0000FF00) >> 8;
      p_a2dp_offload->codec_info[2] = (vendor_id & 0x00FF0000) >> 16;
      p_a2dp_offload->codec_info[3] = (vendor_id & 0xFF000000) >> 24;
      p_a2dp_offload->codec_info[4] = (codec_id & 0x000000FF);
      p_a2dp_offload->codec_info[5] = (codec_id & 0x0000FF00) >> 8;
      if (vendor_id == A2DP_LDAC_VENDOR_ID && codec_id == A2DP_LDAC_CODEC_ID) {
        if (codec_config_.codec_specific_1 == 0) {  // default is 0, ABR
          p_a2dp_offload->codec_info[6] =
              A2DP_LDAC_QUALITY_ABR_OFFLOAD;  // ABR in offload
        } else {
          switch (codec_config_.codec_specific_1 % 10) {
            case 0:
              p_a2dp_offload->codec_info[6] =
                  A2DP_LDAC_QUALITY_HIGH;  // High bitrate
              break;
            case 1:
              p_a2dp_offload->codec_info[6] =
                  A2DP_LDAC_QUALITY_MID;  // Mid birate
              break;
            case 2:
              p_a2dp_offload->codec_info[6] =
                  A2DP_LDAC_QUALITY_LOW;  // Low birate
              break;
            case 3:
              FALLTHROUGH_INTENDED; /* FALLTHROUGH */
            default:
              p_a2dp_offload->codec_info[6] =
                  A2DP_LDAC_QUALITY_ABR_OFFLOAD;  // ABR in offload
              break;
          }
        }
        p_a2dp_offload->codec_info[7] =
            codec_config[10];  // LDAC specific channel mode
        LOG_VERBOSE(LOG_TAG, "%s: Ldac specific channelmode =%d", __func__,
                    p_a2dp_offload->codec_info[7]);
      }
      break;
    default:
      break;
  }
  return true;
}

bool A2dpCodecConfig::isValid() const { return true; }

bool A2dpCodecConfig::copyOutOtaCodecConfig(uint8_t* p_codec_info) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  // TODO: We should use a mechanism to verify codec config,
  // not codec capability.
  if (!A2DP_IsSourceCodecValid(ota_codec_config_)) {
    return false;
  }
  memcpy(p_codec_info, ota_codec_config_, sizeof(ota_codec_config_));
  return true;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecConfig() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  // TODO: We should check whether the codec config is valid
  return codec_config_;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecCapability() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  // TODO: We should check whether the codec capability is valid
  return codec_capability_;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecLocalCapability() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  // TODO: We should check whether the codec capability is valid
  return codec_local_capability_;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecSelectableCapability() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  // TODO: We should check whether the codec capability is valid
  return codec_selectable_capability_;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecUserConfig() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  return codec_user_config_;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecAudioConfig() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  return codec_audio_config_;
}

uint8_t A2dpCodecConfig::getAudioBitsPerSample() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  switch (codec_config_.bits_per_sample) {
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16:
      return 16;
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24:
      return 24;
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32:
      return 32;
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE:
      break;
  }
  return 0;
}

bool A2dpCodecConfig::isCodecConfigEmpty(
    const btav_a2dp_codec_config_t& codec_config) {
  return (
      (codec_config.codec_priority == BTAV_A2DP_CODEC_PRIORITY_DEFAULT) &&
      (codec_config.sample_rate == BTAV_A2DP_CODEC_SAMPLE_RATE_NONE) &&
      (codec_config.bits_per_sample == BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE) &&
      (codec_config.channel_mode == BTAV_A2DP_CODEC_CHANNEL_MODE_NONE) &&
      (codec_config.codec_specific_1 == 0) &&
      (codec_config.codec_specific_2 == 0) &&
      (codec_config.codec_specific_3 == 0) &&
      (codec_config.codec_specific_4 == 0));
}

bool A2dpCodecConfig::setCodecUserConfig(
    const btav_a2dp_codec_config_t& codec_user_config,
    const btav_a2dp_codec_config_t& codec_audio_config,
    const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
    const uint8_t* p_peer_codec_info, bool is_capability,
    uint8_t* p_result_codec_config, bool* p_restart_input,
    bool* p_restart_output, bool* p_config_updated) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  *p_restart_input = false;
  *p_restart_output = false;
  *p_config_updated = false;

  // Save copies of the current codec config, and the OTA codec config, so they
  // can be compared for changes.
  btav_a2dp_codec_config_t saved_codec_config = getCodecConfig();
  uint8_t saved_ota_codec_config[AVDT_CODEC_SIZE];
  memcpy(saved_ota_codec_config, ota_codec_config_, sizeof(ota_codec_config_));

  btav_a2dp_codec_config_t saved_codec_user_config = codec_user_config_;
  codec_user_config_ = codec_user_config;
  btav_a2dp_codec_config_t saved_codec_audio_config = codec_audio_config_;
  codec_audio_config_ = codec_audio_config;
  bool success =
      setCodecConfig(p_peer_codec_info, is_capability, p_result_codec_config);
  if (!success) {
    // Restore the local copy of the user and audio config
    codec_user_config_ = saved_codec_user_config;
    codec_audio_config_ = saved_codec_audio_config;
    return false;
  }

  //
  // The input (audio data) should be restarted if the audio format has changed
  //
  btav_a2dp_codec_config_t new_codec_config = getCodecConfig();
  if ((saved_codec_config.sample_rate != new_codec_config.sample_rate) ||
      (saved_codec_config.bits_per_sample != new_codec_config.bits_per_sample) ||
      (saved_codec_config.codec_specific_3 != new_codec_config.codec_specific_3) || // Savitech Patch
      (saved_codec_config.codec_specific_1 != new_codec_config.codec_specific_1) ||
      (saved_codec_config.channel_mode != new_codec_config.channel_mode)) {
    *p_restart_input = true;
  }

  //
  // The output (the connection) should be restarted if OTA codec config
  // has changed.
  //
  if (!A2DP_CodecEquals(saved_ota_codec_config, p_result_codec_config)) {
    *p_restart_output = true;
  }

  bool encoder_restart_input = *p_restart_input;
  bool encoder_restart_output = *p_restart_output;
  bool encoder_config_updated = *p_config_updated;

  if (!a2dp_offload_status) {
    if (updateEncoderUserConfig(p_peer_params, &encoder_restart_input,
                                &encoder_restart_output,
                                &encoder_config_updated)) {
      if (encoder_restart_input) *p_restart_input = true;
      if (encoder_restart_output) *p_restart_output = true;
      if (encoder_config_updated) *p_config_updated = true;
    }
  }
  if (*p_restart_input || *p_restart_output) *p_config_updated = true;

  return true;
}

bool A2dpCodecConfig::codecConfigIsValid(
    const btav_a2dp_codec_config_t& codec_config) {
  return (codec_config.codec_type < BTAV_A2DP_CODEC_INDEX_MAX) &&
         (codec_config.sample_rate != BTAV_A2DP_CODEC_SAMPLE_RATE_NONE) &&
         (codec_config.bits_per_sample !=
          BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE) &&
         (codec_config.channel_mode != BTAV_A2DP_CODEC_CHANNEL_MODE_NONE);
}

std::string A2dpCodecConfig::codecConfig2Str(
    const btav_a2dp_codec_config_t& codec_config) {
  std::string result;

  if (!codecConfigIsValid(codec_config)) return "Invalid";

  result.append("Rate=");
  result.append(codecSampleRate2Str(codec_config.sample_rate));
  result.append(" Bits=");
  result.append(codecBitsPerSample2Str(codec_config.bits_per_sample));
  result.append(" Mode=");
  result.append(codecChannelMode2Str(codec_config.channel_mode));

  return result;
}

std::string A2dpCodecConfig::codecSampleRate2Str(
    btav_a2dp_codec_sample_rate_t codec_sample_rate) {
  std::string result;

  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_44100) {
    if (!result.empty()) result += "|";
    result += "44100";
  }
  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_48000) {
    if (!result.empty()) result += "|";
    result += "48000";
  }
  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_88200) {
    if (!result.empty()) result += "|";
    result += "88200";
  }
  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_96000) {
    if (!result.empty()) result += "|";
    result += "96000";
  }
  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_176400) {
    if (!result.empty()) result += "|";
    result += "176400";
  }
  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_192000) {
    if (!result.empty()) result += "|";
    result += "192000";
  }
  if (result.empty()) {
    std::stringstream ss;
    ss << "UnknownSampleRate(0x" << std::hex << codec_sample_rate << ")";
    ss >> result;
  }

  return result;
}

std::string A2dpCodecConfig::codecBitsPerSample2Str(
    btav_a2dp_codec_bits_per_sample_t codec_bits_per_sample) {
  std::string result;

  if (codec_bits_per_sample & BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16) {
    if (!result.empty()) result += "|";
    result += "16";
  }
  if (codec_bits_per_sample & BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24) {
    if (!result.empty()) result += "|";
    result += "24";
  }
  if (codec_bits_per_sample & BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32) {
    if (!result.empty()) result += "|";
    result += "32";
  }
  if (result.empty()) {
    std::stringstream ss;
    ss << "UnknownBitsPerSample(0x" << std::hex << codec_bits_per_sample << ")";
    ss >> result;
  }

  return result;
}

std::string A2dpCodecConfig::codecChannelMode2Str(
    btav_a2dp_codec_channel_mode_t codec_channel_mode) {
  std::string result;

  if (codec_channel_mode & BTAV_A2DP_CODEC_CHANNEL_MODE_MONO) {
    if (!result.empty()) result += "|";
    result += "MONO";
  }
  if (codec_channel_mode & BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO) {
    if (!result.empty()) result += "|";
    result += "STEREO";
  }
  if (result.empty()) {
    std::stringstream ss;
    ss << "UnknownChannelMode(0x" << std::hex << codec_channel_mode << ")";
    ss >> result;
  }

  return result;
}

void A2dpCodecConfig::debug_codec_dump(int fd) {
  std::string result;
  dprintf(fd, "\nA2DP %s State:\n", name().c_str());
  dprintf(fd, "  Priority: %d\n", codecPriority());
  dprintf(fd, "  Encoder interval (ms): %" PRIu64 "\n", encoderIntervalMs());
  dprintf(fd, "  Effective MTU: %d\n", getEffectiveMtu());

  result = codecConfig2Str(getCodecConfig());
  dprintf(fd, "  Config: %s\n", result.c_str());

  result = codecConfig2Str(getCodecSelectableCapability());
  dprintf(fd, "  Selectable: %s\n", result.c_str());

  result = codecConfig2Str(getCodecLocalCapability());
  dprintf(fd, "  Local capability: %s\n", result.c_str());
}

//
// Compares two codecs |lhs| and |rhs| based on their priority.
// Returns true if |lhs| has higher priority (larger priority value).
// If |lhs| and |rhs| have same priority, the unique codec index is used
// as a tie-breaker: larger codec index value means higher priority.
//
static bool compare_codec_priority(const A2dpCodecConfig* lhs,
                                   const A2dpCodecConfig* rhs) {
  if (lhs->codecPriority() > rhs->codecPriority()) return true;
  if (lhs->codecPriority() < rhs->codecPriority()) return false;
  return (lhs->codecIndex() > rhs->codecIndex());
}

A2dpCodecs::A2dpCodecs(
    const std::vector<btav_a2dp_codec_config_t>& codec_priorities)
    : current_codec_config_(nullptr) {
  for (auto config : codec_priorities) {
    codec_priorities_.insert(
        std::make_pair(config.codec_type, config.codec_priority));
  }
}

A2dpCodecs::~A2dpCodecs() {
  std::unique_lock<std::recursive_mutex> lock(codec_mutex_);
  for (const auto& iter : indexed_codecs_) {
    delete iter.second;
  }
  for (const auto& iter : disabled_codecs_) {
    delete iter.second;
  }
  lock.unlock();
}

bool A2dpCodecs::init() {
  LOG_DEBUG(LOG_TAG, "%s", __func__);
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  char* tok = NULL;
  char* tmp_token = NULL;
  bool offload_codec_support[BTAV_A2DP_CODEC_INDEX_MAX] = {false};
  char value_sup[PROPERTY_VALUE_MAX], value_dis[PROPERTY_VALUE_MAX];

  osi_property_get("ro.bluetooth.a2dp_offload.supported", value_sup, "false");
  osi_property_get("persist.bluetooth.a2dp_offload.disabled", value_dis,
                   "false");
  a2dp_offload_status =
      (strcmp(value_sup, "true") == 0) && (strcmp(value_dis, "false") == 0);

  if (a2dp_offload_status) {
    char value_cap[PROPERTY_VALUE_MAX];
    osi_property_get("persist.bluetooth.a2dp_offload.cap", value_cap, "");
    tok = strtok_r((char*)value_cap, "-", &tmp_token);
    while (tok != NULL) {
      if (strcmp(tok, "sbc") == 0) {
        LOG_INFO(LOG_TAG, "%s: SBC offload supported", __func__);
        offload_codec_support[BTAV_A2DP_CODEC_INDEX_SOURCE_SBC] = true;
      } else if (strcmp(tok, "aac") == 0) {
        LOG_INFO(LOG_TAG, "%s: AAC offload supported", __func__);
        offload_codec_support[BTAV_A2DP_CODEC_INDEX_SOURCE_AAC] = true;
      } else if (strcmp(tok, "aptx") == 0) {
        LOG_INFO(LOG_TAG, "%s: APTX offload supported", __func__);
        offload_codec_support[BTAV_A2DP_CODEC_INDEX_SOURCE_APTX] = true;
      } else if (strcmp(tok, "aptxhd") == 0) {
        LOG_INFO(LOG_TAG, "%s: APTXHD offload supported", __func__);
        offload_codec_support[BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_HD] = true;
      } else if (strcmp(tok, "ldac") == 0) {
        LOG_INFO(LOG_TAG, "%s: LDAC offload supported", __func__);
        offload_codec_support[BTAV_A2DP_CODEC_INDEX_SOURCE_LDAC] = true;
      } else if (strcmp(tok, "lhdcv3") == 0) {  // Savitech Patch
          LOG_INFO(LOG_TAG, "%s: LHDCV3 offload supported", __func__);
          offload_codec_support[BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV3] = false;
      } else if (strcmp(tok, "lhdcv2") == 0) {
          LOG_INFO(LOG_TAG, "%s: LHDCV2 offload supported", __func__);
          offload_codec_support[BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV2] = false;
      } else if (strcmp(tok, "lhdcv1") == 0) {
          LOG_INFO(LOG_TAG, "%s: LHDCV1 offload supported", __func__);
          offload_codec_support[BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV1] = false;
      } else if (strcmp(tok, "lhdcv5") == 0) {
        LOG_INFO(LOG_TAG, "%s: LHDCV5 offload supported", __func__);
        offload_codec_support[BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV5] = false;
    }
      tok = strtok_r(NULL, "-", &tmp_token);
    };
  }

  for (int i = BTAV_A2DP_CODEC_INDEX_MIN; i < BTAV_A2DP_CODEC_INDEX_MAX; i++) {
    btav_a2dp_codec_index_t codec_index =
        static_cast<btav_a2dp_codec_index_t>(i);

    // Select the codec priority if explicitly configured
    btav_a2dp_codec_priority_t codec_priority =
        BTAV_A2DP_CODEC_PRIORITY_DEFAULT;
    auto cp_iter = codec_priorities_.find(codec_index);
    if (cp_iter != codec_priorities_.end()) {
      codec_priority = cp_iter->second;
    }

    // In offload mode, disable the codecs based on the property
    if ((codec_index < BTAV_A2DP_CODEC_INDEX_SOURCE_MAX) &&
        a2dp_offload_status && (offload_codec_support[i] != true)) {
      codec_priority = BTAV_A2DP_CODEC_PRIORITY_DISABLED;
    }

    A2dpCodecConfig* codec_config =
        A2dpCodecConfig::createCodec(codec_index, codec_priority);
    if (codec_config == nullptr) continue;

    if (codec_priority != BTAV_A2DP_CODEC_PRIORITY_DEFAULT) {
      LOG_INFO(LOG_TAG, "%s: updated %s codec priority to %d", __func__,
               codec_config->name().c_str(), codec_priority);
    }

    // Test if the codec is disabled
    if (codec_config->codecPriority() == BTAV_A2DP_CODEC_PRIORITY_DISABLED) {
      disabled_codecs_.insert(std::make_pair(codec_index, codec_config));
      continue;
    }

    indexed_codecs_.insert(std::make_pair(codec_index, codec_config));

    if (codec_index < BTAV_A2DP_CODEC_INDEX_SOURCE_MAX) {
      ordered_source_codecs_.push_back(codec_config);
      ordered_source_codecs_.sort(compare_codec_priority);
    } else {
      ordered_sink_codecs_.push_back(codec_config);
      ordered_sink_codecs_.sort(compare_codec_priority);
    }
  }

  if (ordered_source_codecs_.empty()) {
    LOG_ERROR(LOG_TAG, "%s: no Source codecs were initialized", __func__);
  } else {
    for (auto iter : ordered_source_codecs_) {
      LOG_INFO(LOG_TAG, "%s: initialized Source codec %s", __func__,
               iter->name().c_str());
    }
  }
  if (ordered_sink_codecs_.empty()) {
    LOG_ERROR(LOG_TAG, "%s: no Sink codecs were initialized", __func__);
  } else {
    for (auto iter : ordered_sink_codecs_) {
      LOG_INFO(LOG_TAG, "%s: initialized Sink codec %s", __func__,
               iter->name().c_str());
    }
  }

  return (!ordered_source_codecs_.empty() && !ordered_sink_codecs_.empty());
}

A2dpCodecConfig* A2dpCodecs::findSourceCodecConfig(
    const uint8_t* p_codec_info) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  btav_a2dp_codec_index_t codec_index = A2DP_SourceCodecIndex(p_codec_info);
  if (codec_index == BTAV_A2DP_CODEC_INDEX_MAX) return nullptr;

  auto iter = indexed_codecs_.find(codec_index);
  if (iter == indexed_codecs_.end()) return nullptr;
  return iter->second;
}

A2dpCodecConfig* A2dpCodecs::findSinkCodecConfig(const uint8_t* p_codec_info) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  btav_a2dp_codec_index_t codec_index = A2DP_SinkCodecIndex(p_codec_info);
  if (codec_index == BTAV_A2DP_CODEC_INDEX_MAX) return nullptr;

  auto iter = indexed_codecs_.find(codec_index);
  if (iter == indexed_codecs_.end()) return nullptr;
  return iter->second;
}

bool A2dpCodecs::isSupportedCodec(btav_a2dp_codec_index_t codec_index) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  return indexed_codecs_.find(codec_index) != indexed_codecs_.end();
}

bool A2dpCodecs::setCodecConfig(const uint8_t* p_peer_codec_info,
                                bool is_capability,
                                uint8_t* p_result_codec_config,
                                bool select_current_codec) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  A2dpCodecConfig* a2dp_codec_config = findSourceCodecConfig(p_peer_codec_info);
  if (a2dp_codec_config == nullptr) return false;
  if (!a2dp_codec_config->setCodecConfig(p_peer_codec_info, is_capability,
                                         p_result_codec_config)) {
    return false;
  }
  if (select_current_codec) {
    current_codec_config_ = a2dp_codec_config;
  }
  return true;
}

bool A2dpCodecs::setSinkCodecConfig(const uint8_t* p_peer_codec_info,
                                    bool is_capability,
                                    uint8_t* p_result_codec_config,
                                    bool select_current_codec) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  A2dpCodecConfig* a2dp_codec_config = findSinkCodecConfig(p_peer_codec_info);
  if (a2dp_codec_config == nullptr) return false;
  if (!a2dp_codec_config->setCodecConfig(p_peer_codec_info, is_capability,
                                         p_result_codec_config)) {
    return false;
  }
  if (select_current_codec) {
    current_codec_config_ = a2dp_codec_config;
  }
  return true;
}

/***********************************************
 * Savitech Patch - LHDC Extended API Start
 ***********************************************/
static bool swapInt64toByteArray(unsigned char *byteArray, int64_t integer64)
{
	bool ret = false;
	if (!byteArray) {
	  APPL_TRACE_ERROR("%s: null ptr", __func__);
	  return ret;
	}
	
	byteArray[7] = ((integer64 & 0x00000000000000FF) >> 0);
	byteArray[6] = ((integer64 & 0x000000000000FF00) >> 8);
	byteArray[5] = ((integer64 & 0x0000000000FF0000) >> 16);
	byteArray[4] = ((integer64 & 0x00000000FF000000) >> 24);
	byteArray[3] = ((integer64 & 0x000000FF00000000) >> 32);
	byteArray[2] = ((integer64 & 0x0000FF0000000000) >> 40);
	byteArray[1] = ((integer64 & 0x00FF000000000000) >> 48);
	byteArray[0] = ((integer64 & 0xFF00000000000000) >> 56);

	ret = true;
	return ret;
}

static bool getLHDCA2DPSpecficV2(btav_a2dp_codec_config_t *a2dpCfg, unsigned char *pucConfig, const int clen)
{
  if (clen < (int)LHDC_EXTEND_FUNC_CONFIG_TOTAL_FIXED_SIZE_V2 ) {
    APPL_TRACE_ERROR("%s: payload size too small! clen=%d ",__func__, clen);
    return false;
  }

  /* copy specifics into buffer */
  if ( !(
      swapInt64toByteArray(&pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2], a2dpCfg->codec_specific_1) &&
      swapInt64toByteArray(&pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2], a2dpCfg->codec_specific_2) &&
      swapInt64toByteArray(&pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2], a2dpCfg->codec_specific_3) &&
      swapInt64toByteArray(&pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2], a2dpCfg->codec_specific_4)
      )) {
    APPL_TRACE_ERROR("%s: fail to copy specifics to buffer!",  __func__);
    return false;
  }

  /* fill capability metadata fields */
  APPL_TRACE_WARNING("%s: total %d metadata of capabilities",  __func__, (LHDC_EXTEND_FUNC_CONFIG_CAPMETA_SIZE_V2>>1) );

  if( A2DP_VendorGetSrcCapVectorLhdcv3(&pucConfig[LHDC_EXTEND_FUNC_A2DP_CAPMETA_HEAD_V2]) ) {
    APPL_TRACE_WARNING("%s: Get metadata of capabilities success!", __func__);
  } else {
    APPL_TRACE_ERROR("%s: fail to get capability fields!",  __func__);
    return false;
  }

#if 0   //debug print
  APPL_TRACE_WARNING("%s:(spec):: SP1[%02X %02X %02X %02X %02X %02X %02X %02X]; SP2[%02X %02X %02X %02X %02X %02X %02X %02X]",
      __func__,
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2+1],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2+2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2+3],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2+4], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2+5],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2+6], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V2+7],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2+1],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2+2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2+3],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2+4], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2+5],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2+6], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V2+7]);

  APPL_TRACE_WARNING("%s:(spec):: SP3[%02X %02X %02X %02X %02X %02X %02X %02X]; SP4[%02X %02X %02X %02X %02X %02X %02X %02X]",
      __func__,
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2+1],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2+2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2+3],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2+4], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2+5],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2+6], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V2+7],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2+1],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2+2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2+3],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2+4], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2+5],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2+6], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V2+7]);

  for(int i=0, j=0; i<(LHDC_EXTEND_FUNC_CONFIG_CAPMETA_SIZE_V2>>1); i++)
  {
    APPL_TRACE_WARNING("%s:(capMeta):: Cap%d[0x%02X 0x%02X]", __func__,
        i,
        pucConfig[LHDC_EXTEND_FUNC_A2DP_CAPMETA_HEAD_V2 + j],
        pucConfig[LHDC_EXTEND_FUNC_A2DP_CAPMETA_HEAD_V2 + (j+1)]);
    j+=2;
  }
#endif

  return true;
}

static bool getLHDCA2DPSpecficV1(btav_a2dp_codec_config_t *a2dpCfg, unsigned char *pucConfig, const int clen)
{
  if (clen < (int)LHDC_EXTEND_FUNC_CONFIG_TOTAL_FIXED_SIZE_V1 )
  {
    APPL_TRACE_ERROR("%s: payload size too small! clen=%d ",__func__, clen);
    return false;
  }

  /* copy specifics into buffer */
  if( !(
      swapInt64toByteArray(&pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1], a2dpCfg->codec_specific_1) &&
      swapInt64toByteArray(&pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1], a2dpCfg->codec_specific_2) &&
      swapInt64toByteArray(&pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1], a2dpCfg->codec_specific_3) &&
      swapInt64toByteArray(&pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V1], a2dpCfg->codec_specific_4)
      ))
  {
    APPL_TRACE_ERROR("%s: fail to copy specifics to buffer!",  __func__);
    return false;
  }

#if 0   //debug print
  APPL_TRACE_WARNING("%s:(spec):: SP1[%02X %02X %02X %02X %02X %02X %02X %02X]; SP2[%02X %02X %02X %02X %02X %02X %02X %02X]",
      __func__,
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1+1],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1+2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1+3],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1+4], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1+5],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1+6], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS1_HEAD_V1+7],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1+1],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1+2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1+3],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1+4], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1+5],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1+6], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS2_HEAD_V1+7]);

  APPL_TRACE_WARNING("%s:(spec):: SP3[%02X %02X %02X %02X %02X %02X %02X %02X]; SP4[%02X %02X %02X %02X %02X %02X %02X %02X]",
      __func__,
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1+1],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1+2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1+3],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1+4], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1+5],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1+6], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS3_HEAD_V1+7],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V1], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V1+1],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V1+2], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V1+3],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V1+4], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V1+5],
      pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V1+6], pucConfig[LHDC_EXTEND_FUNC_A2DP_SPECIFICS4_HEAD_V1+7]);
#endif

  return true;
}

int A2dpCodecs::getLHDCCodecUserConfig(
    A2dpCodecConfig* peerCodec,
    const char* codecConfig, const int clen) {

  int result = BT_STATUS_FAIL;

  if (peerCodec == nullptr || codecConfig == nullptr) {
    APPL_TRACE_ERROR("A2dpCodecs::%s: null input(peerCodec:%p codecConfig:%p)", __func__, peerCodec, codecConfig);
    return BT_STATUS_FAIL;
  }
  btav_a2dp_codec_index_t peerCodecIndex = peerCodec->codecIndex();
  APPL_TRACE_WARNING("A2dpCodecs::%s: CodecIndex=%d, clen=%d", __func__, peerCodecIndex , clen);

  switch(peerCodecIndex)
  {
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV5:
    result = peerCodec->getLhdcExtendAPIConfig(peerCodec, codecConfig, clen);
    break;

  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV3:
    if( codecConfig[LHDC_EXTEND_FUNC_CONFIG_API_CODE_HEAD] == LHDC_EXTEND_FUNC_CODE_A2DP_TYPE_MASK ){
        /* **************************************
         * LHDC A2DP related APIs:
         * **************************************/
        unsigned char *pucConfig = (unsigned char *) codecConfig;
        unsigned int exFuncVer = 0;
        unsigned int exFuncCode = 0;

        if (pucConfig == NULL)
        {
            APPL_TRACE_ERROR("%s: User Config error!(%p)",  __func__, codecConfig);
            goto Fail;
        }

        /* check required buffer size for generic header */
        if (clen < (int)(LHDC_EXTEND_FUNC_CONFIG_API_VERSION_SIZE + LHDC_EXTEND_FUNC_CONFIG_API_CODE_SIZE))
        {
             // Buffer is too small for generic header size
            APPL_TRACE_ERROR("%s: buffer is too small for command clen=%d",  __func__, clen);
            goto Fail;
        }

        if(current_codec_config_ == NULL)
        {
            APPL_TRACE_ERROR("%s: Can not get current a2dp codec config!",  __func__);
            goto Fail;
        }

        A2dpCodecConfig *a2dp_codec_config = current_codec_config_;
        btav_a2dp_codec_config_t codec_config_tmp;

        exFuncVer = (((unsigned int) pucConfig[3]) & ((unsigned int)0xff)) |
                   ((((unsigned int) pucConfig[2]) & ((unsigned int)0xff)) << 8)  |
                   ((((unsigned int) pucConfig[1]) & ((unsigned int)0xff)) << 16) |
                   ((((unsigned int) pucConfig[0]) & ((unsigned int)0xff)) << 24);
        exFuncCode = (((unsigned int) pucConfig[7]) & ((unsigned int)0xff)) |
                    ((((unsigned int) pucConfig[6]) & ((unsigned int)0xff)) << 8)  |
                    ((((unsigned int) pucConfig[5]) & ((unsigned int)0xff)) << 16) |
                    ((((unsigned int) pucConfig[4]) & ((unsigned int)0xff)) << 24);

        switch (exFuncCode)
        {
          case EXTEND_FUNC_CODE_GET_SPECIFIC:
            /* **************************************
             * API::Get A2DP Specifics
             * **************************************/
            APPL_TRACE_WARNING("%s: target cfg = 0x%02X",__func__, pucConfig[LHDC_EXTEND_FUNC_CONFIG_A2DPCFG_CODE_HEAD]);
            switch(pucConfig[LHDC_EXTEND_FUNC_CONFIG_A2DPCFG_CODE_HEAD])
            {
              case LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_FINAL_CFG:
                codec_config_tmp = a2dp_codec_config->getCodecConfig();
                break;
              case LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_FINAL_CAP:
                codec_config_tmp = a2dp_codec_config->getCodecCapability();
                break;
              case LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_LOCAL_CAP:
                codec_config_tmp = a2dp_codec_config->getCodecLocalCapability();
                break;
              case LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_SELECTABLE_CAP:
                codec_config_tmp = a2dp_codec_config->getCodecSelectableCapability();
                break;
              case LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_USER_CFG:
                codec_config_tmp = a2dp_codec_config->getCodecUserConfig();
                break;
              case LHDC_EXTEND_FUNC_A2DP_TYPE_SPECIFICS_AUDIO_CFG:
                codec_config_tmp = a2dp_codec_config->getCodecAudioConfig();
                break;
              default:
                APPL_TRACE_ERROR("%s: target a2dp config not found!",  __func__);
                goto Fail;
            }
            APPL_TRACE_WARNING("%s: Cfg(int64):: SP1=%lld(0x%016llX); SP2=%lld(0x%016llX); SP3=%lld(0x%016llX); SP4=%lld(0x%016llX)",__func__,
                  codec_config_tmp.codec_specific_1,codec_config_tmp.codec_specific_1,
                  codec_config_tmp.codec_specific_2, codec_config_tmp.codec_specific_2,
                  codec_config_tmp.codec_specific_3, codec_config_tmp.codec_specific_3,
                  codec_config_tmp.codec_specific_4, codec_config_tmp.codec_specific_4);

            switch (exFuncVer)
            {
              case EXTEND_FUNC_VER_GET_SPECIFIC_V1:
                if( !getLHDCA2DPSpecficV1(&codec_config_tmp, pucConfig, clen) )
                  goto Fail;
                break;
              case EXTEND_FUNC_VER_GET_SPECIFIC_V2:
                if( !getLHDCA2DPSpecficV2(&codec_config_tmp, pucConfig, clen) )
                  goto Fail;
                break;
              default:
                APPL_TRACE_WARNING("%s: Invalid Ex. Function Version!(0x%X)",  __func__, exFuncVer);
                goto Fail;
            }
            result = BT_STATUS_SUCCESS;
            break;

        default:
          APPL_TRACE_WARNING("%s: Invalid Ex. Function Code!(0x%X)",  __func__, exFuncCode);
          goto Fail;
        } // switch (exFuncCode)
    }
    else if( codecConfig[LHDC_EXTEND_FUNC_CONFIG_API_CODE_HEAD] == LHDC_EXTEND_FUNC_CODE_LIB_TYPE_MASK ){
    	result = A2dpCodecConfigLhdcV3::getEncoderExtendFuncUserConfig(codecConfig, clen);
    }
    break;
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV1:
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV2:
  default:
    APPL_TRACE_WARNING("%s: feature not support!", __func__);
    goto Fail;
  }

Fail:
  return result;
}

int A2dpCodecs::setLHDCCodecUserConfig(
    A2dpCodecConfig* peerCodec,
    const char* codecConfig, const int clen) {

  int result = BT_STATUS_FAIL;

  if (peerCodec == nullptr || codecConfig == nullptr) {
    APPL_TRACE_ERROR("A2dpCodecs::%s: null input(peerCodec:%p version:%p)", __func__, peerCodec, codecConfig);
    return BT_STATUS_FAIL;
  }
  btav_a2dp_codec_index_t peerCodecIndex = peerCodec->codecIndex();
  APPL_TRACE_WARNING("A2dpCodecs::%s: CodecIndex=%d, clen=%d", __func__, peerCodecIndex , clen);

  switch(peerCodecIndex)
  {
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV5:
    result = peerCodec->setLhdcExtendAPIConfig(peerCodec, codecConfig, clen);
    break;
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV3:
    result = A2dpCodecConfigLhdcV3::setEncoderExtendFuncUserConfig(codecConfig, clen);
    break;
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV1:
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV2:
  default:
    APPL_TRACE_WARNING("%s: peer codecIndex(%d) not support the feature!", __func__, peerCodecIndex);
    return result;
  }

  return result;
}

bool A2dpCodecs::setLHDCCodecUserData(
    A2dpCodecConfig* peerCodec,
    const char* codecData, const int clen) {

  if (peerCodec == nullptr || codecData == nullptr) {
    APPL_TRACE_ERROR("A2dpCodecs::%s: null input(peerCodec:%p version:%p)", __func__, peerCodec, codecData);
    return false;
  }
  btav_a2dp_codec_index_t peerCodecIndex = peerCodec->codecIndex();
  APPL_TRACE_API("A2dpCodecs::%s: CodecIndex=%d, clen=%d", __func__, peerCodecIndex , clen);

  switch(peerCodecIndex)
  {
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV5:
    peerCodec->setLhdcExtendAPIData(peerCodec, codecData, clen);
    break;
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV3:
    A2dpCodecConfigLhdcV3::setEncoderExtendFuncUserData(codecData, clen);
    break;
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV1:
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV2:
  default:
    APPL_TRACE_WARNING("%s: peer codecIndex(%d) not support the feature!", __func__, peerCodecIndex);
    return false;
  }

  return true;
}

int A2dpCodecs::getLHDCCodecUserApiVer(
    A2dpCodecConfig* peerCodec,
    const char* version, const int clen) {
  int result = BT_STATUS_FAIL;

  if (peerCodec == nullptr || version == nullptr) {
    APPL_TRACE_ERROR("A2dpCodecs::%s: null input(peerCodec:%p version:%p)", __func__, peerCodec, version);
    return BT_STATUS_FAIL;
  }
  btav_a2dp_codec_index_t peerCodecIndex = peerCodec->codecIndex();
  APPL_TRACE_API("A2dpCodecs::%s: CodecIndex=%d, clen=%d", __func__, peerCodecIndex , clen);

  switch(peerCodecIndex) {
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV5:
    result = peerCodec->getLhdcExtendAPIVersion(peerCodec, version, clen);
    break;
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV3:
    result = A2dpCodecConfigLhdcV3::getEncoderExtendFuncUserApiVer(version, clen);
    break;
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV1:
  case BTAV_A2DP_CODEC_INDEX_SOURCE_LHDCV2:
  default:
    APPL_TRACE_WARNING("%s: peer codecIndex(%d) not support the feature!", __func__, peerCodecIndex);
    return result;
  }

  return result;
}
/***********************************************
 * Savitech Patch - LHDC Extended API End
 ***********************************************/


bool A2dpCodecs::setCodecUserConfig(
    const btav_a2dp_codec_config_t& codec_user_config,
    const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
    const uint8_t* p_peer_sink_capabilities, uint8_t* p_result_codec_config,
    bool* p_restart_input, bool* p_restart_output, bool* p_config_updated) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  btav_a2dp_codec_config_t codec_audio_config;
  A2dpCodecConfig* a2dp_codec_config = nullptr;
  A2dpCodecConfig* last_codec_config = current_codec_config_;
  *p_restart_input = false;
  *p_restart_output = false;
  *p_config_updated = false;

  LOG_DEBUG(LOG_TAG, "%s: Configuring: %s", __func__,
            codec_user_config.ToString().c_str());

  if (codec_user_config.codec_type < BTAV_A2DP_CODEC_INDEX_MAX) {
    auto iter = indexed_codecs_.find(codec_user_config.codec_type);
    if (iter == indexed_codecs_.end()) goto fail;
    a2dp_codec_config = iter->second;
  } else {
    // Update the default codec
    a2dp_codec_config = current_codec_config_;
  }
  if (a2dp_codec_config == nullptr) goto fail;

  // Reuse the existing codec audio config
  codec_audio_config = a2dp_codec_config->getCodecAudioConfig();
  if (!a2dp_codec_config->setCodecUserConfig(
          codec_user_config, codec_audio_config, p_peer_params,
          p_peer_sink_capabilities, true, p_result_codec_config,
          p_restart_input, p_restart_output, p_config_updated)) {
    goto fail;
  }

  // Update the codec priorities, and eventually restart the connection
  // if a new codec needs to be selected.
  do {
    // Update the codec priority
    btav_a2dp_codec_priority_t old_priority =
        a2dp_codec_config->codecPriority();
    btav_a2dp_codec_priority_t new_priority = codec_user_config.codec_priority;
    a2dp_codec_config->setCodecPriority(new_priority);
    // Get the actual (recomputed) priority
    new_priority = a2dp_codec_config->codecPriority();

    // Check if there was no previous codec
    if (last_codec_config == nullptr) {
      current_codec_config_ = a2dp_codec_config;
      *p_restart_input = true;
      *p_restart_output = true;
      break;
    }

    // Check if the priority of the current codec was updated
    if (a2dp_codec_config == last_codec_config) {
      if (old_priority == new_priority) break;  // No change in priority

      *p_config_updated = true;
      if (new_priority < old_priority) {
        // The priority has become lower - restart the connection to
        // select a new codec.
        *p_restart_output = true;
      }
      break;
    }

    if (new_priority <= old_priority) {
      // No change in priority, or the priority has become lower.
      // This wasn't the current codec, so we shouldn't select a new codec.
      if (*p_restart_input || *p_restart_output ||
          (old_priority != new_priority)) {
        *p_config_updated = true;
      }
      *p_restart_input = false;
      *p_restart_output = false;
      break;
    }

    *p_config_updated = true;
    if (new_priority >= last_codec_config->codecPriority()) {
      // The new priority is higher than the current codec. Restart the
      // connection to select a new codec.
      current_codec_config_ = a2dp_codec_config;
      last_codec_config->setDefaultCodecPriority();
      *p_restart_input = true;
      *p_restart_output = true;
    }
  } while (false);
  ordered_source_codecs_.sort(compare_codec_priority);

  if (*p_restart_input || *p_restart_output) *p_config_updated = true;

  LOG_DEBUG(LOG_TAG,
            "%s: Configured: restart_input = %d restart_output = %d "
            "config_updated = %d",
            __func__, *p_restart_input, *p_restart_output, *p_config_updated);

  return true;

fail:
  current_codec_config_ = last_codec_config;
  return false;
}

bool A2dpCodecs::setCodecAudioConfig(
    const btav_a2dp_codec_config_t& codec_audio_config,
    const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
    const uint8_t* p_peer_sink_capabilities, uint8_t* p_result_codec_config,
    bool* p_restart_output, bool* p_config_updated) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  btav_a2dp_codec_config_t codec_user_config;
  A2dpCodecConfig* a2dp_codec_config = current_codec_config_;
  *p_restart_output = false;
  *p_config_updated = false;

  if (a2dp_codec_config == nullptr) return false;

  // Reuse the existing codec user config
  codec_user_config = a2dp_codec_config->getCodecUserConfig();
  bool restart_input = false;  // Flag ignored - input was just restarted
  if (!a2dp_codec_config->setCodecUserConfig(
          codec_user_config, codec_audio_config, p_peer_params,
          p_peer_sink_capabilities, true, p_result_codec_config, &restart_input,
          p_restart_output, p_config_updated)) {
    return false;
  }

  return true;
}

bool A2dpCodecs::setCodecOtaConfig(
    const uint8_t* p_ota_codec_config,
    const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
    uint8_t* p_result_codec_config, bool* p_restart_input,
    bool* p_restart_output, bool* p_config_updated) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  btav_a2dp_codec_index_t codec_type;
  btav_a2dp_codec_config_t codec_user_config;
  btav_a2dp_codec_config_t codec_audio_config;
  A2dpCodecConfig* a2dp_codec_config = nullptr;
  A2dpCodecConfig* last_codec_config = current_codec_config_;
  *p_restart_input = false;
  *p_restart_output = false;
  *p_config_updated = false;

  // Check whether the current codec config is explicitly configured by
  // user configuration. If yes, then the OTA codec configuration is ignored.
  if (current_codec_config_ != nullptr) {
    codec_user_config = current_codec_config_->getCodecUserConfig();
    if (!A2dpCodecConfig::isCodecConfigEmpty(codec_user_config)) {
      LOG_WARN(LOG_TAG,
               "%s: ignoring peer OTA configuration for codec %s: "
               "existing user configuration for current codec %s",
               __func__, A2DP_CodecName(p_ota_codec_config),
               current_codec_config_->name().c_str());
      goto fail;
    }
  }

  // Check whether the codec config for the same codec is explicitly configured
  // by user configuration. If yes, then the OTA codec configuration is
  // ignored.
  codec_type = A2DP_SourceCodecIndex(p_ota_codec_config);
  if (codec_type == BTAV_A2DP_CODEC_INDEX_MAX) {
    LOG_WARN(LOG_TAG,
             "%s: ignoring peer OTA codec configuration: "
             "invalid codec",
             __func__);
    goto fail;  // Invalid codec
  } else {
    auto iter = indexed_codecs_.find(codec_type);
    if (iter == indexed_codecs_.end()) {
      LOG_WARN(LOG_TAG,
               "%s: cannot find codec configuration for peer OTA codec %s",
               __func__, A2DP_CodecName(p_ota_codec_config));
      goto fail;
    }
    a2dp_codec_config = iter->second;
  }
  if (a2dp_codec_config == nullptr) goto fail;
  codec_user_config = a2dp_codec_config->getCodecUserConfig();
  if (!A2dpCodecConfig::isCodecConfigEmpty(codec_user_config)) {
    LOG_WARN(LOG_TAG,
             "%s: ignoring peer OTA configuration for codec %s: "
             "existing user configuration for same codec",
             __func__, A2DP_CodecName(p_ota_codec_config));
    goto fail;
  }
  current_codec_config_ = a2dp_codec_config;

  // Reuse the existing codec user config and codec audio config
  codec_audio_config = a2dp_codec_config->getCodecAudioConfig();
  if (!a2dp_codec_config->setCodecUserConfig(
          codec_user_config, codec_audio_config, p_peer_params,
          p_ota_codec_config, false, p_result_codec_config, p_restart_input,
          p_restart_output, p_config_updated)) {
    LOG_WARN(LOG_TAG,
             "%s: cannot set codec configuration for peer OTA codec %s",
             __func__, A2DP_CodecName(p_ota_codec_config));
    goto fail;
  }
  CHECK(current_codec_config_ != nullptr);

  if (*p_restart_input || *p_restart_output) *p_config_updated = true;

  return true;

fail:
  current_codec_config_ = last_codec_config;
  return false;
}

bool A2dpCodecs::setPeerSinkCodecCapabilities(
    const uint8_t* p_peer_codec_capabilities) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  if (!A2DP_IsPeerSinkCodecValid(p_peer_codec_capabilities)) return false;
  A2dpCodecConfig* a2dp_codec_config =
      findSourceCodecConfig(p_peer_codec_capabilities);
  if (a2dp_codec_config == nullptr) return false;
  return a2dp_codec_config->setPeerCodecCapabilities(p_peer_codec_capabilities);
}

bool A2dpCodecs::setPeerSourceCodecCapabilities(
    const uint8_t* p_peer_codec_capabilities) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  if (!A2DP_IsPeerSourceCodecValid(p_peer_codec_capabilities)) return false;
  A2dpCodecConfig* a2dp_codec_config =
      findSinkCodecConfig(p_peer_codec_capabilities);
  if (a2dp_codec_config == nullptr) return false;
  return a2dp_codec_config->setPeerCodecCapabilities(p_peer_codec_capabilities);
}

bool A2dpCodecs::getCodecConfigAndCapabilities(
    btav_a2dp_codec_config_t* p_codec_config,
    std::vector<btav_a2dp_codec_config_t>* p_codecs_local_capabilities,
    std::vector<btav_a2dp_codec_config_t>* p_codecs_selectable_capabilities) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  if (current_codec_config_ != nullptr) {
    *p_codec_config = current_codec_config_->getCodecConfig();
  } else {
    btav_a2dp_codec_config_t codec_config;
    memset(&codec_config, 0, sizeof(codec_config));
    *p_codec_config = codec_config;
  }

  std::vector<btav_a2dp_codec_config_t> codecs_capabilities;
  for (auto codec : orderedSourceCodecs()) {
    codecs_capabilities.push_back(codec->getCodecLocalCapability());
  }
  *p_codecs_local_capabilities = codecs_capabilities;

  codecs_capabilities.clear();
  for (auto codec : orderedSourceCodecs()) {
    btav_a2dp_codec_config_t codec_capability =
        codec->getCodecSelectableCapability();
    // Don't add entries that cannot be used
    if ((codec_capability.sample_rate == BTAV_A2DP_CODEC_SAMPLE_RATE_NONE) ||
        (codec_capability.bits_per_sample ==
         BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE) ||
        (codec_capability.channel_mode == BTAV_A2DP_CODEC_CHANNEL_MODE_NONE)) {
      continue;
    }
    codecs_capabilities.push_back(codec_capability);
  }
  *p_codecs_selectable_capabilities = codecs_capabilities;

  return true;
}

void A2dpCodecs::debug_codec_dump(int fd) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  dprintf(fd, "\nA2DP Codecs State:\n");

  // Print the current codec name
  if (current_codec_config_ != nullptr) {
    dprintf(fd, "  Current Codec: %s\n", current_codec_config_->name().c_str());
  } else {
    dprintf(fd, "  Current Codec: None\n");
  }

  // Print the codec-specific state
  for (auto codec_config : ordered_source_codecs_) {
    codec_config->debug_codec_dump(fd);
  }
}

tA2DP_CODEC_TYPE A2DP_GetCodecType(const uint8_t* p_codec_info) {
  return (tA2DP_CODEC_TYPE)(p_codec_info[AVDT_CODEC_TYPE_INDEX]);
}

bool A2DP_IsSourceCodecValid(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsSourceCodecValidSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsSourceCodecValidAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorSourceCodecValid(p_codec_info);
    default:
      break;
  }

  return false;
}

bool A2DP_IsSinkCodecValid(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsSinkCodecValidSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsSinkCodecValidAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorSinkCodecValid(p_codec_info);
    default:
      break;
  }

  return false;
}

bool A2DP_IsPeerSourceCodecValid(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsPeerSourceCodecValidSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsPeerSourceCodecValidAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorPeerSourceCodecValid(p_codec_info);
    default:
      break;
  }

  return false;
}

bool A2DP_IsPeerSinkCodecValid(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsPeerSinkCodecValidSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsPeerSinkCodecValidAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorPeerSinkCodecValid(p_codec_info);
    default:
      break;
  }

  return false;
}

bool A2DP_IsSinkCodecSupported(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsSinkCodecSupportedSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsSinkCodecSupportedAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorSinkCodecSupported(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}

bool A2DP_IsPeerSourceCodecSupported(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsPeerSourceCodecSupportedSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsPeerSourceCodecSupportedAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorPeerSourceCodecSupported(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}

void A2DP_InitDefaultCodec(uint8_t* p_codec_info) {
  A2DP_InitDefaultCodecSbc(p_codec_info);
}

bool A2DP_UsesRtpHeader(bool content_protection_enabled,
                        const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  if (codec_type != A2DP_MEDIA_CT_NON_A2DP) return true;

  return A2DP_VendorUsesRtpHeader(content_protection_enabled, p_codec_info);
}

uint8_t A2DP_GetMediaType(const uint8_t* p_codec_info) {
  uint8_t media_type = (p_codec_info[A2DP_MEDIA_TYPE_OFFSET] >> 4) & 0x0f;
  return media_type;
}

const char* A2DP_CodecName(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_CodecNameSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_CodecNameAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorCodecName(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return "UNKNOWN CODEC";
}

bool A2DP_CodecTypeEquals(const uint8_t* p_codec_info_a,
                          const uint8_t* p_codec_info_b) {
  tA2DP_CODEC_TYPE codec_type_a = A2DP_GetCodecType(p_codec_info_a);
  tA2DP_CODEC_TYPE codec_type_b = A2DP_GetCodecType(p_codec_info_b);

  if (codec_type_a != codec_type_b) return false;

  switch (codec_type_a) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_CodecTypeEqualsSbc(p_codec_info_a, p_codec_info_b);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_CodecTypeEqualsAac(p_codec_info_a, p_codec_info_b);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorCodecTypeEquals(p_codec_info_a, p_codec_info_b);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type_a);
  return false;
}

bool A2DP_CodecEquals(const uint8_t* p_codec_info_a,
                      const uint8_t* p_codec_info_b) {
  tA2DP_CODEC_TYPE codec_type_a = A2DP_GetCodecType(p_codec_info_a);
  tA2DP_CODEC_TYPE codec_type_b = A2DP_GetCodecType(p_codec_info_b);

  if (codec_type_a != codec_type_b) return false;

  switch (codec_type_a) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_CodecEqualsSbc(p_codec_info_a, p_codec_info_b);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_CodecEqualsAac(p_codec_info_a, p_codec_info_b);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorCodecEquals(p_codec_info_a, p_codec_info_b);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type_a);
  return false;
}

int A2DP_GetTrackSampleRate(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetTrackSampleRateSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetTrackSampleRateAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetTrackSampleRate(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

int A2DP_GetTrackBitsPerSample(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetTrackBitsPerSampleSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetTrackBitsPerSampleAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetTrackBitsPerSample(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

int A2DP_GetTrackChannelCount(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetTrackChannelCountSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetTrackChannelCountAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetTrackChannelCount(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

int A2DP_GetSinkTrackChannelType(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetSinkTrackChannelTypeSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetSinkTrackChannelTypeAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetSinkTrackChannelType(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

bool A2DP_GetPacketTimestamp(const uint8_t* p_codec_info, const uint8_t* p_data,
                             uint32_t* p_timestamp) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetPacketTimestampSbc(p_codec_info, p_data, p_timestamp);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetPacketTimestampAac(p_codec_info, p_data, p_timestamp);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetPacketTimestamp(p_codec_info, p_data, p_timestamp);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}

bool A2DP_BuildCodecHeader(const uint8_t* p_codec_info, BT_HDR* p_buf,
                           uint16_t frames_per_packet) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_BuildCodecHeaderSbc(p_codec_info, p_buf, frames_per_packet);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_BuildCodecHeaderAac(p_codec_info, p_buf, frames_per_packet);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorBuildCodecHeader(p_codec_info, p_buf,
                                         frames_per_packet);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}

const tA2DP_ENCODER_INTERFACE* A2DP_GetEncoderInterface(
    const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetEncoderInterfaceSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetEncoderInterfaceAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetEncoderInterface(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return NULL;
}

const tA2DP_DECODER_INTERFACE* A2DP_GetDecoderInterface(
    const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetDecoderInterfaceSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetDecoderInterfaceAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetDecoderInterface(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return NULL;
}

bool A2DP_AdjustCodec(uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_AdjustCodecSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_AdjustCodecAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorAdjustCodec(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}

btav_a2dp_codec_index_t A2DP_SourceCodecIndex(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_SourceCodecIndexSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_SourceCodecIndexAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorSourceCodecIndex(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return BTAV_A2DP_CODEC_INDEX_MAX;
}

btav_a2dp_codec_index_t A2DP_SinkCodecIndex(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_SinkCodecIndexSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_SinkCodecIndexAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorSinkCodecIndex(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return BTAV_A2DP_CODEC_INDEX_MAX;
}

const char* A2DP_CodecIndexStr(btav_a2dp_codec_index_t codec_index) {
  switch (codec_index) {
    case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
      return A2DP_CodecIndexStrSbc();
    case BTAV_A2DP_CODEC_INDEX_SINK_SBC:
      return A2DP_CodecIndexStrSbcSink();
    case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
      return A2DP_CodecIndexStrAac();
    case BTAV_A2DP_CODEC_INDEX_SINK_AAC:
      return A2DP_CodecIndexStrAacSink();
    default:
      break;
  }

  if (codec_index < BTAV_A2DP_CODEC_INDEX_MAX)
    return A2DP_VendorCodecIndexStr(codec_index);

  return "UNKNOWN CODEC INDEX";
}

bool A2DP_InitCodecConfig(btav_a2dp_codec_index_t codec_index,
                          AvdtpSepConfig* p_cfg) {
  LOG_VERBOSE(LOG_TAG, "%s: codec %s", __func__,
              A2DP_CodecIndexStr(codec_index));

  /* Default: no content protection info */
  p_cfg->num_protect = 0;
  p_cfg->protect_info[0] = 0;

  switch (codec_index) {
    case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
      return A2DP_InitCodecConfigSbc(p_cfg);
    case BTAV_A2DP_CODEC_INDEX_SINK_SBC:
      return A2DP_InitCodecConfigSbcSink(p_cfg);
    case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
      return A2DP_InitCodecConfigAac(p_cfg);
    case BTAV_A2DP_CODEC_INDEX_SINK_AAC:
      return A2DP_InitCodecConfigAacSink(p_cfg);
    default:
      break;
  }

  if (codec_index < BTAV_A2DP_CODEC_INDEX_MAX)
    return A2DP_VendorInitCodecConfig(codec_index, p_cfg);

  return false;
}

std::string A2DP_CodecInfoString(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_CodecInfoStringSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_CodecInfoStringAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorCodecInfoString(p_codec_info);
    default:
      break;
  }

  return "Unsupported codec type: " + loghex(codec_type);
}
