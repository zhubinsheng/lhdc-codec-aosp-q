/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.settings.development;

import android.bluetooth.BluetoothCodecConfig;
import android.content.Context;

import com.android.settings.R;
import com.android.settingslib.core.lifecycle.Lifecycle;

public class BluetoothLHDCAudioLatencyPreferenceController extends
        AbstractBluetoothA2dpPreferenceController {

    private static final int DEFAULT_INDEX = 0;
    private static final String BLUETOOTH_SELECT_A2DP_LHDC_LATENCY_KEY =
            "bluetooth_select_a2dp_codec_lhdc_latency";

    public BluetoothLHDCAudioLatencyPreferenceController(Context context, Lifecycle lifecycle,
            BluetoothA2dpConfigStore store) {
        super(context, lifecycle, store);
    }

    @Override
    public String getPreferenceKey() {
        return BLUETOOTH_SELECT_A2DP_LHDC_LATENCY_KEY;
    }

    @Override
    protected String[] getListValues() {
        return mContext.getResources().getStringArray(
                R.array.bluetooth_a2dp_codec_lhdc_latency_values);
    }

    @Override
    protected String[] getListSummaries() {
        return mContext.getResources().getStringArray(
                R.array.bluetooth_a2dp_codec_lhdc_latency_summaries);
    }

    @Override
    protected int getDefaultIndex() {
        return DEFAULT_INDEX;
    }

    @Override
    protected void writeConfigurationValues(Object newValue) {
    	//Log.e("LHDC_LAT", "Object = " + newValue);
        final int index = mPreference.findIndexOfValue(newValue.toString());
        int codecSpecific2Value = 0; // default
        if (index <= 1) {
            codecSpecific2Value = 0xC000 | index;
        }else{
	        codecSpecific2Value = 0xC000 | DEFAULT_INDEX;
        }
        mBluetoothA2dpConfigStore.setCodecSpecific2Value(codecSpecific2Value);
    }

    @Override
    protected int getCurrentA2dpSettingIndex(BluetoothCodecConfig config) {
        // The actual values are 0, 1, 2 - those are extracted
        // as mod-10 remainders of a larger value.
        // The reason is because within BluetoothCodecConfig we cannot use
        // a codec-specific value of zero.
        int index = (int)config.getCodecSpecific2();
        int tmp = index & 0xC000;
        if (tmp == 0xC000) {
            index &= 0x1;
        } else {
            index = DEFAULT_INDEX;
        }
        return index;
    }
}
