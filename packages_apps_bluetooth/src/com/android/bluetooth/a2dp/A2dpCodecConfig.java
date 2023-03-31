/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.bluetooth.a2dp;

import android.bluetooth.BluetoothCodecConfig;
import android.bluetooth.BluetoothCodecStatus;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.util.Log;

import com.android.bluetooth.R;

import java.util.Arrays;
import java.util.Objects;
/*
 * A2DP Codec Configuration setup.
 */
class A2dpCodecConfig {
    private static final boolean DBG = true;
    private static final String TAG = "A2dpCodecConfig";

    private Context mContext;
    private A2dpNativeInterface mA2dpNativeInterface;

    private BluetoothCodecConfig[] mCodecConfigPriorities;
    private int mA2dpSourceCodecPrioritySbc = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
    private int mA2dpSourceCodecPriorityAac = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
    private int mA2dpSourceCodecPriorityAptx = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
    private int mA2dpSourceCodecPriorityAptxHd = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
    private int mA2dpSourceCodecPriorityLdac = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
    // Savitech Patch
    private int mA2dpSourceCodecPriorityLhdcV3 = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
    private int mA2dpSourceCodecPriorityLhdcV2 = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
    private int mA2dpSourceCodecPriorityLhdcV1 = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
    private int mA2dpSourceCodecPriorityLhdcV5 = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;

    A2dpCodecConfig(Context context, A2dpNativeInterface a2dpNativeInterface) {
        mContext = context;
        mA2dpNativeInterface = a2dpNativeInterface;
        mCodecConfigPriorities = assignCodecConfigPriorities();
    }

    BluetoothCodecConfig[] codecConfigPriorities() {
        return mCodecConfigPriorities;
    }

    void setCodecConfigPreference(BluetoothDevice device,
                                  BluetoothCodecStatus codecStatus,
                                  BluetoothCodecConfig codecConfig) {
        Objects.requireNonNull(codecStatus);
        
        // Check whether the codecConfig is selectable for this Bluetooth device.
        BluetoothCodecConfig[] selectableCodecs = codecStatus.getCodecsSelectableCapabilities();
        if (!Arrays.asList(selectableCodecs).stream().anyMatch(codec ->
                codec.isMandatoryCodec())) {
            // Do not set codec preference to native if the selectableCodecs not contain mandatory
            // codec. The reason could be remote codec negotiation is not completed yet.
            Log.w(TAG, "Cannot find mandatory codec in selectableCodecs.");
            return;
        }
        if (!isCodecConfigSelectable(codecConfig, selectableCodecs)) {
            Log.w(TAG, "Codec is not selectable: " + codecConfig);
            return;
        }

        // Check whether the codecConfig would change current codec config.
        int prioritizedCodecType = getPrioitizedCodecType(codecConfig, selectableCodecs);
        BluetoothCodecConfig currentCodecConfig = codecStatus.getCodecConfig();
        if (prioritizedCodecType == currentCodecConfig.getCodecType()
                && (currentCodecConfig.getCodecType() != codecConfig.getCodecType()
                || currentCodecConfig.sameAudioFeedingParameters(codecConfig))) {
            // Same codec with same parameters, no need to send this request to native.
            Log.i(TAG, "setCodecConfigPreference: codec not changed.");
            return;
        }

        BluetoothCodecConfig[] codecConfigArray = new BluetoothCodecConfig[1];
        codecConfigArray[0] = codecConfig;
        mA2dpNativeInterface.setCodecConfigPreference(device, codecConfigArray);
    }
    
    /************************************************
     * Savitech Patch - LHDC Extended API Start
     ***********************************************/
    int getLhdcCodecExtendAPIVer(BluetoothDevice device,
                                byte[] exApiVer) {
        return mA2dpNativeInterface.getLhdcCodecExtendAPIVer(device, exApiVer);
    }
        
    int setLhdcCodecExtendAPIConfigAR(BluetoothDevice device,
                                byte[] codecConfig) {
        return mA2dpNativeInterface.setLhdcCodecExtendAPIConfigAR(device, codecConfig);
    }
    
    int getLhdcCodecExtendAPIConfigAR(BluetoothDevice device,
                                byte[] codecConfig) {
        return mA2dpNativeInterface.getLhdcCodecExtendAPIConfigAR(device, codecConfig);
    }    
    
    int setLhdcCodecExtendAPIConfigMeta(BluetoothDevice device,
                                byte[] codecConfig) {
        return mA2dpNativeInterface.setLhdcCodecExtendAPIConfigMeta(device, codecConfig);
    }
    
    int getLhdcCodecExtendAPIConfigMeta(BluetoothDevice device,
                                byte[] codecConfig) {
        return mA2dpNativeInterface.getLhdcCodecExtendAPIConfigMeta(device, codecConfig);
    }    
    
    int getLhdcCodecExtendAPIConfigA2dpCodecSpecific(BluetoothDevice device,
                                byte[] codecConfig) {
        return mA2dpNativeInterface.getLhdcCodecExtendAPIConfigA2dpCodecSpecific(device, codecConfig);
    }    
    
    void setLhdcCodecExtendAPIDataGyro2D(BluetoothDevice device,
                                byte[] codecData) {
        mA2dpNativeInterface.setLhdcCodecExtendAPIDataGyro2D(device, codecData);
    }
    /************************************************
     * Savitech Patch - LHDC Extended API End
     ***********************************************/

    void enableOptionalCodecs(BluetoothDevice device, BluetoothCodecConfig currentCodecConfig) {
        if (currentCodecConfig != null && !currentCodecConfig.isMandatoryCodec()) {
            Log.i(TAG, "enableOptionalCodecs: already using optional codec: "
                    + currentCodecConfig.getCodecType());
            return;
        }

        BluetoothCodecConfig[] codecConfigArray = assignCodecConfigPriorities();
        if (codecConfigArray == null) {
            return;
        }

        // Set the mandatory codec's priority to default, and remove the rest
        for (int i = 0; i < codecConfigArray.length; i++) {
            BluetoothCodecConfig codecConfig = codecConfigArray[i];
            if (!codecConfig.isMandatoryCodec()) {
                codecConfigArray[i] = null;
            }
        }

        mA2dpNativeInterface.setCodecConfigPreference(device, codecConfigArray);
    }

    void disableOptionalCodecs(BluetoothDevice device, BluetoothCodecConfig currentCodecConfig) {
        if (currentCodecConfig != null && currentCodecConfig.isMandatoryCodec()) {
            Log.i(TAG, "disableOptionalCodecs: already using mandatory codec");
            return;
        }

        BluetoothCodecConfig[] codecConfigArray = assignCodecConfigPriorities();
        if (codecConfigArray == null) {
            return;
        }
        // Set the mandatory codec's priority to highest, and remove the rest
        for (int i = 0; i < codecConfigArray.length; i++) {
            BluetoothCodecConfig codecConfig = codecConfigArray[i];
            if (codecConfig.isMandatoryCodec()) {
                codecConfig.setCodecPriority(BluetoothCodecConfig.CODEC_PRIORITY_HIGHEST);
            } else {
                codecConfigArray[i] = null;
            }
        }
        mA2dpNativeInterface.setCodecConfigPreference(device, codecConfigArray);
    }

    // Get the codec type of the highest priority of selectableCodecs and codecConfig.
    private int getPrioitizedCodecType(BluetoothCodecConfig codecConfig,
            BluetoothCodecConfig[] selectableCodecs) {
        BluetoothCodecConfig prioritizedCodecConfig = codecConfig;
        for (BluetoothCodecConfig config : selectableCodecs) {
            if (prioritizedCodecConfig == null) {
                prioritizedCodecConfig = config;
            }
            if (config.getCodecPriority() > prioritizedCodecConfig.getCodecPriority()) {
                prioritizedCodecConfig = config;
            }
        }
        return prioritizedCodecConfig.getCodecType();
    }

    // Check whether the codecConfig is selectable
    private static boolean isCodecConfigSelectable(BluetoothCodecConfig codecConfig,
            BluetoothCodecConfig[] selectableCodecs) {
        for (BluetoothCodecConfig config : selectableCodecs) {
        	
        	// Savitech Patch
            Log.w(TAG, "Selected=>" + codecConfig);
            Log.w(TAG, "config=>" + config);
            if (codecConfig.getCodecType() == config.getCodecType())
                    //&& (codecConfig.getSampleRate() & config.getSampleRate()) != 0
                    //&& (codecConfig.getBitsPerSample() & config.getBitsPerSample()) != 0
                    //&& (codecConfig.getChannelMode() & config.getChannelMode()) != 0)
                {
                return true;
            }
        }
        return false;
    }

    // Assign the A2DP Source codec config priorities
    private BluetoothCodecConfig[] assignCodecConfigPriorities() {
        Resources resources = mContext.getResources();
        if (resources == null) {
            return null;
        }

        int value;
        try {
            value = resources.getInteger(R.integer.a2dp_source_codec_priority_sbc);
        } catch (NotFoundException e) {
            value = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
        }
        if ((value >= BluetoothCodecConfig.CODEC_PRIORITY_DISABLED) && (value
                < BluetoothCodecConfig.CODEC_PRIORITY_HIGHEST)) {
            mA2dpSourceCodecPrioritySbc = value;
        }

        try {
            value = resources.getInteger(R.integer.a2dp_source_codec_priority_aac);
        } catch (NotFoundException e) {
            value = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
        }
        if ((value >= BluetoothCodecConfig.CODEC_PRIORITY_DISABLED) && (value
                < BluetoothCodecConfig.CODEC_PRIORITY_HIGHEST)) {
            mA2dpSourceCodecPriorityAac = value;
        }

        try {
            value = resources.getInteger(R.integer.a2dp_source_codec_priority_aptx);
        } catch (NotFoundException e) {
            value = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
        }
        if ((value >= BluetoothCodecConfig.CODEC_PRIORITY_DISABLED) && (value
                < BluetoothCodecConfig.CODEC_PRIORITY_HIGHEST)) {
            mA2dpSourceCodecPriorityAptx = value;
        }

        try {
            value = resources.getInteger(R.integer.a2dp_source_codec_priority_aptx_hd);
        } catch (NotFoundException e) {
            value = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
        }
        if ((value >= BluetoothCodecConfig.CODEC_PRIORITY_DISABLED) && (value
                < BluetoothCodecConfig.CODEC_PRIORITY_HIGHEST)) {
            mA2dpSourceCodecPriorityAptxHd = value;
        }

        try {
            value = resources.getInteger(R.integer.a2dp_source_codec_priority_ldac);
        } catch (NotFoundException e) {
            value = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
        }
        if ((value >= BluetoothCodecConfig.CODEC_PRIORITY_DISABLED) && (value
                < BluetoothCodecConfig.CODEC_PRIORITY_HIGHEST)) {
            mA2dpSourceCodecPriorityLdac = value;
        }

        // Savitech Patch - START
        try {
            value = resources.getInteger(R.integer.a2dp_source_codec_priority_lhdcv3);
        } catch (NotFoundException e) {
            value = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
        }
        if ((value >= BluetoothCodecConfig.CODEC_PRIORITY_DISABLED) && (value
                < BluetoothCodecConfig.CODEC_PRIORITY_HIGHEST)) {
            mA2dpSourceCodecPriorityLhdcV3 = value;
        }

        try {
            value = resources.getInteger(R.integer.a2dp_source_codec_priority_lhdcv2);
        } catch (NotFoundException e) {
            value = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
        }
        if ((value >= BluetoothCodecConfig.CODEC_PRIORITY_DISABLED) && (value
                < BluetoothCodecConfig.CODEC_PRIORITY_HIGHEST)) {
            mA2dpSourceCodecPriorityLhdcV2 = value;
        }

        try {
            value = resources.getInteger(R.integer.a2dp_source_codec_priority_lhdcv1);
        } catch (NotFoundException e) {
            value = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
        }
        if ((value >= BluetoothCodecConfig.CODEC_PRIORITY_DISABLED) && (value
                < BluetoothCodecConfig.CODEC_PRIORITY_HIGHEST)) {
            mA2dpSourceCodecPriorityLhdcV1 = value;
        }

        try {
            value = resources.getInteger(R.integer.a2dp_source_codec_priority_lhdcv5);
        } catch (NotFoundException e) {
            value = BluetoothCodecConfig.CODEC_PRIORITY_DEFAULT;
        }
        if ((value >= BluetoothCodecConfig.CODEC_PRIORITY_DISABLED) && (value
                < BluetoothCodecConfig.CODEC_PRIORITY_HIGHEST)) {
            mA2dpSourceCodecPriorityLhdcV5 = value;
        }
        // Savitech Patch - END

        BluetoothCodecConfig codecConfig;
        BluetoothCodecConfig[] codecConfigArray =
                new BluetoothCodecConfig[BluetoothCodecConfig.SOURCE_CODEC_TYPE_MAX];
        codecConfig = new BluetoothCodecConfig(BluetoothCodecConfig.SOURCE_CODEC_TYPE_SBC,
                mA2dpSourceCodecPrioritySbc, BluetoothCodecConfig.SAMPLE_RATE_NONE,
                BluetoothCodecConfig.BITS_PER_SAMPLE_NONE, BluetoothCodecConfig
                .CHANNEL_MODE_NONE, 0 /* codecSpecific1 */,
                0 /* codecSpecific2 */, 0 /* codecSpecific3 */, 0 /* codecSpecific4 */);
        codecConfigArray[0] = codecConfig;
        codecConfig = new BluetoothCodecConfig(BluetoothCodecConfig.SOURCE_CODEC_TYPE_AAC,
                mA2dpSourceCodecPriorityAac, BluetoothCodecConfig.SAMPLE_RATE_NONE,
                BluetoothCodecConfig.BITS_PER_SAMPLE_NONE, BluetoothCodecConfig
                .CHANNEL_MODE_NONE, 0 /* codecSpecific1 */,
                0 /* codecSpecific2 */, 0 /* codecSpecific3 */, 0 /* codecSpecific4 */);
        codecConfigArray[1] = codecConfig;
        codecConfig = new BluetoothCodecConfig(BluetoothCodecConfig.SOURCE_CODEC_TYPE_APTX,
                mA2dpSourceCodecPriorityAptx, BluetoothCodecConfig.SAMPLE_RATE_NONE,
                BluetoothCodecConfig.BITS_PER_SAMPLE_NONE, BluetoothCodecConfig
                .CHANNEL_MODE_NONE, 0 /* codecSpecific1 */,
                0 /* codecSpecific2 */, 0 /* codecSpecific3 */, 0 /* codecSpecific4 */);
        codecConfigArray[2] = codecConfig;
        codecConfig = new BluetoothCodecConfig(BluetoothCodecConfig.SOURCE_CODEC_TYPE_APTX_HD,
                mA2dpSourceCodecPriorityAptxHd, BluetoothCodecConfig.SAMPLE_RATE_NONE,
                BluetoothCodecConfig.BITS_PER_SAMPLE_NONE, BluetoothCodecConfig
                .CHANNEL_MODE_NONE, 0 /* codecSpecific1 */,
                0 /* codecSpecific2 */, 0 /* codecSpecific3 */, 0 /* codecSpecific4 */);
        codecConfigArray[3] = codecConfig;
        codecConfig = new BluetoothCodecConfig(BluetoothCodecConfig.SOURCE_CODEC_TYPE_LDAC,
                mA2dpSourceCodecPriorityLdac, BluetoothCodecConfig.SAMPLE_RATE_NONE,
                BluetoothCodecConfig.BITS_PER_SAMPLE_NONE, BluetoothCodecConfig
                .CHANNEL_MODE_NONE, 0 /* codecSpecific1 */,
                0 /* codecSpecific2 */, 0 /* codecSpecific3 */, 0 /* codecSpecific4 */);
        codecConfigArray[4] = codecConfig;
        // Savitech Patch - START
        codecConfig = new BluetoothCodecConfig(BluetoothCodecConfig.SOURCE_CODEC_TYPE_LHDCV3,
                mA2dpSourceCodecPriorityLhdcV3, BluetoothCodecConfig.SAMPLE_RATE_NONE,
                BluetoothCodecConfig.BITS_PER_SAMPLE_NONE, BluetoothCodecConfig.CHANNEL_MODE_NONE,
                0 /* codecSpecific1 */, 0 /* codecSpecific2 */, 0 /* codecSpecific3 */,
                0 /* codecSpecific4 */);
        codecConfigArray[5] = codecConfig;
        codecConfig = new BluetoothCodecConfig(BluetoothCodecConfig.SOURCE_CODEC_TYPE_LHDCV2,
                mA2dpSourceCodecPriorityLhdcV2, BluetoothCodecConfig.SAMPLE_RATE_NONE,
                BluetoothCodecConfig.BITS_PER_SAMPLE_NONE, BluetoothCodecConfig.CHANNEL_MODE_NONE,
                0 /* codecSpecific1 */, 0 /* codecSpecific2 */, 0 /* codecSpecific3 */,
                0 /* codecSpecific4 */);
        codecConfigArray[6] = codecConfig;
        codecConfig = new BluetoothCodecConfig(BluetoothCodecConfig.SOURCE_CODEC_TYPE_LHDCV1,
                mA2dpSourceCodecPriorityLhdcV1, BluetoothCodecConfig.SAMPLE_RATE_NONE,
                BluetoothCodecConfig.BITS_PER_SAMPLE_NONE, BluetoothCodecConfig.CHANNEL_MODE_NONE,
                0 /* codecSpecific1 */, 0 /* codecSpecific2 */, 0 /* codecSpecific3 */,
                0 /* codecSpecific4 */);
        codecConfigArray[7] = codecConfig;
        codecConfig = new BluetoothCodecConfig(BluetoothCodecConfig.SOURCE_CODEC_TYPE_LHDCV5,
                mA2dpSourceCodecPriorityLhdcV5, BluetoothCodecConfig.SAMPLE_RATE_NONE,
                BluetoothCodecConfig.BITS_PER_SAMPLE_NONE, BluetoothCodecConfig.CHANNEL_MODE_NONE,
                0 /* codecSpecific1 */, 0 /* codecSpecific2 */, 0 /* codecSpecific3 */,
                0 /* codecSpecific4 */);
        codecConfigArray[8] = codecConfig;
        // Savitech Patch - END

        return codecConfigArray;
    }
}

