package com.spreadtrum.android.eng;

import com.spreadtrum.android.eng.R;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.widget.Toast;

public class SlogUIServerReceiver extends BroadcastReceiver {

	@Override
	public void onReceive(Context receiverContext, Intent receivedIntent) {
		if (receivedIntent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) {
			SlogAction.contextMainActivity = receiverContext;
			if (SlogAction.isAlwaysRun(SlogAction.SERVICESLOG)
					&& SlogAction.GetState(SlogAction.GENERALKEY)) {
				// start service
				if (receiverContext.startService(new Intent(receiverContext,
						SlogService.class)) == null) {
					Toast.makeText(
							receiverContext,
							receiverContext
									.getText(R.string.toast_receiver_failed),
							Toast.LENGTH_LONG).show();
					Log.e("Slog->ServerReceiver",
							"Start service when BOOT_COMPLETE failed");
				}
			}
			// start snap service
			if (SlogAction.isAlwaysRun(SlogAction.SERVICESNAP)) {
				if (receiverContext.startService(new Intent(receiverContext,
						SlogUISnapService.class)) == null) {
					Toast.makeText(
							receiverContext,
							receiverContext
									.getText(R.string.toast_receiver_failed),
							Toast.LENGTH_LONG).show();
					Log.e("Slog->ServerReceiver",
							"Start service when BOOT_COMPLETE failed");
				}
			}

		}
		SlogAction.SlogStart();

	}
}
