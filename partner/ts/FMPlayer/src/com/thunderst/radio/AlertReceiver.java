package com.thunderst.radio;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class AlertReceiver extends BroadcastReceiver{
    
	private static final String TAG = "AlertReceiver";

	@Override
	public void onReceive(Context context, Intent intent) {
		Log.d(TAG, "aaa: intent's actino = " + intent.getAction());
		String action = intent.getAction();
		if(action.equals("android.intent.action.EVENT_REMINDER")){
			Intent intent1 = new Intent("android.intent.action.AlertReceiver");
			context.sendBroadcast(intent1);
		}
	}

}
