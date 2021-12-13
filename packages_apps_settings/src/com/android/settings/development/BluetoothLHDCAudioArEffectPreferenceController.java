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
import android.util.Log;

public class BluetoothLHDCAudioArEffectPreferenceController extends
        AbstractBluetoothA2dpPreferenceController {

    private static final int DEFAULT_INDEX = 0;
    private static final String BLUETOOTH_SELECT_A2DP_LHDC_AR_EFFECT_KEY =
            "bluetooth_enable_a2dp_codec_lhdc_ar_effect";
/*
//specific3
#define A2DP_LHDC_JAS_ENABLED		0x1ULL
#define A2DP_LHDC_AR_ENABLED		0x2ULL
#define A2DP_LHDC_META_ENABLED		0x4ULL
#define A2DP_LHDC_LLAC_ENABLED		0x8ULL
#define A2DP_LHDC_MBR_ENABLED		0x10ULL
#define A2DP_LHDC_LARC_ENABLED		0x20ULL
#define A2DP_LHDC_V4_ENABLED		0x40ULL
*/
    private static final int LHDC_FEATURE_MASK = 0xFF000000;
    private static final int LHDC_FEATURE_TAG = 0x4C000000;
	private static final int LHDC_AR_FEATURE = 0x02;

    public BluetoothLHDCAudioArEffectPreferenceController(Context context, Lifecycle lifecycle,
            BluetoothA2dpConfigStore store) {
        super(context, lifecycle, store);
    }

    @Override
    public String getPreferenceKey() {
        return BLUETOOTH_SELECT_A2DP_LHDC_AR_EFFECT_KEY;
    }

    @Override
    protected String[] getListValues() {
        return mContext.getResources().getStringArray(
                R.array.bluetooth_enable_a2dp_codec_lhdc_ar_effect_values);
    }

    @Override
    protected String[] getListSummaries() {
        return mContext.getResources().getStringArray(
                R.array.bluetooth_enable_a2dp_codec_lhdc_ar_effect_summaries);
    }

    @Override
    protected int getDefaultIndex() {
        return DEFAULT_INDEX;
    }

    @Override
    protected void writeConfigurationValues(Object newValue) {
    	//Log.e("LHDC_LAT", "Object = " + newValue);
        final int index = mPreference.findIndexOfValue(newValue.toString());
        
        int codecSpecific3Value = 0; // default
        codecSpecific3Value |= LHDC_FEATURE_TAG;
        
        if (index != 0) {
            codecSpecific3Value |= LHDC_AR_FEATURE;
        }else{
	        codecSpecific3Value &= ~LHDC_AR_FEATURE;
        }
        Log.i("LHDC_UI_AR", "write codecSpecific3Value = " + codecSpecific3Value);
        mBluetoothA2dpConfigStore.setCodecSpecific3Value(codecSpecific3Value);
    }

    @Override
    protected int getCurrentA2dpSettingIndex(BluetoothCodecConfig config) {
        // The actual values are 0, 1, 2 - those are extracted
        // as mod-10 remainders of a larger value.
        // The reason is because within BluetoothCodecConfig we cannot use
        // a codec-specific value of zero.
    	int ret = 0;
        int index = (int)config.getCodecSpecific3();
        Log.i("LHDC_UI_AR", "get index = " + index);
        
        int tmp = index & LHDC_FEATURE_MASK;
        if (tmp == LHDC_FEATURE_TAG)
        {
	        if ((index & LHDC_AR_FEATURE) != 0) {
		    	Log.i("LHDC_UI_AR", "return AR ON");
	            ret = 1;
	        } else {
				Log.i("LHDC_UI_AR", "return AR OFF");
	            ret = 0;
	        }
        }
        else
        {
			Log.i("LHDC_UI_AR", "tag not matched, return AR OFF");
            ret = 0;
        }
        return ret;
    }
}
