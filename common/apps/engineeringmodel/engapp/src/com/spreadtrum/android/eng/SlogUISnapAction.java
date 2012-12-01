package com.spreadtrum.android.eng;

import com.spreadtrum.android.eng.R;

import android.app.Activity;
import android.os.Bundle;
import android.widget.Toast;

/* SlogUI Added by Yuntao.xiao*/

public class SlogUISnapAction extends Activity {
	@Override
	protected void onCreate(Bundle snap) {
		super.onCreate(snap);
		finish();
	}

	@Override
	protected void onDestroy() {
		// need to correct
		try {
			Thread.sleep(500);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		String prompt;
		if (SlogAction.Snap()) {
			prompt = (String) getText(R.string.toast_snap_success);
		} else {
			prompt = (String) getText(R.string.toast_snap_failed);
		}
		Toast.makeText(this, prompt, Toast.LENGTH_SHORT).show();
		super.onDestroy();

	}
}
