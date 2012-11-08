package com.broadcom.bt.service;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.util.Log;
import com.broadcom.bt.service.IPowerManager;

public class PowerManagementService extends IPowerManager.Stub {

	private static final String TAG = "PowerManagmentService";

	private static final boolean DBG = true;
	private final Context mContext;
	private static final int MESSAGE_BT_BOOTUP = 5;
	public static final String SERVICE_NAME = "bt_fm_radio";

        private static final int BTOFF_FMOFF = 0;
        private static final int BTON_FMOFF = 1;
        private static final int BTOFF_FMON = 2;
        private static final int BTON_FMON = 3;

	static {
        //System.loadLibrary("btld");
                System.loadLibrary("fmpmservice");
		classInitNative();
	}

	public PowerManagementService(Context context) {
		mContext = context;
		initializeNativeDataNative();
	}

	public boolean enableFm() {
		try {
			Log.e(TAG, "enableFm called");
			return (enableFmNative() == -1 ? false : true);
		} catch (Throwable t) {
			Log.e(TAG, "Unable to enable FM", t);
			return false;
		}
	}

	public boolean disableFm() {
		try {
			return (disableFmNative() == -1 ? false : true);
		} catch (Throwable t) {
			Log.e(TAG, "Unable to disable FM");
		}
		return false;
	}

	public boolean enableBt() {
		try {
			return (enableBtNative() == -1 ? false : true);
		} catch (Throwable t) {
			Log.e(TAG, "Unable to enbale BT");
		}
		return false;
	}

	public boolean disableBt() {
		try {
			return (disableBtNative() == -1 ? false : true);
		} catch (Throwable t) {
			Log.e(TAG, "Unable to disable BT");
		}
		return false;
	}

	public boolean isfmEnabled() {
		try {
			int mCurrentState = getCurrentStateNative();
			if (mCurrentState == BTOFF_FMON || mCurrentState == BTON_FMON)
				return true;
		} catch (Throwable t) {
			Log.e(TAG, "unable to get current FM state", t);
		}
		return false;
	}

	public boolean isBtEnabled() {
		try {
			int mCurrentState = getCurrentStateNative();
			if (mCurrentState == BTON_FMOFF || mCurrentState == BTON_FMON)
				return true;
		} catch (Throwable t) {
			Log.e(TAG, "Unable to get current BT state", t);
		}
		return false;
	}


	private static void log(String msg) {
		Log.d(TAG, msg);
	}

	private native static void classInitNative();

	private native void initializeNativeDataNative();

	private native void cleanupNativeDataNative();

	private native int getCurrentStateNative();

	private native int enableFmNative();

	private native int disableFmNative();

	private native int enableBtNative();

	private native int disableBtNative();

}
