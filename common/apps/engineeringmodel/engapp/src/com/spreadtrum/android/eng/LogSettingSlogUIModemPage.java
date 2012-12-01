package com.spreadtrum.android.eng;

import com.spreadtrum.android.eng.R;
import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.CheckBox;

/* SlogUI Added by Yuntao.xiao*/

public class LogSettingSlogUIModemPage extends Activity {

	private CheckBox chkModem, chkBuleTooth, chkWifi, chkMisc;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_modem);

		chkModem = (CheckBox) findViewById(R.id.chk_modem_branch);
		chkBuleTooth = (CheckBox) findViewById(R.id.chk_modem_bluetooth);
		chkWifi = (CheckBox) findViewById(R.id.chk_modem_wifi);
		chkMisc = (CheckBox) findViewById(R.id.chk_modem_misc);

		SyncState();

		ClkListenner clickListen = new ClkListenner();
		chkModem.setOnClickListener(clickListen);
		chkBuleTooth.setOnClickListener(clickListen);
		chkWifi.setOnClickListener(clickListen);
		chkMisc.setOnClickListener(clickListen);

	}

	@Override
	protected void onResume() {
		super.onResume();
		SyncState();
	}

	protected class ClkListenner implements OnClickListener {
		public void onClick(View onClickView) {
			switch (onClickView.getId()) {
			case R.id.chk_modem_branch:
				SlogAction.SetState(SlogAction.MODEMKEY, chkModem.isChecked(),
						false);
				break;
			case R.id.chk_modem_bluetooth:
				SlogAction.SetState(SlogAction.BULETOOTHKEY,
						chkBuleTooth.isChecked(), false);
				break;
			case R.id.chk_modem_wifi:
				SlogAction.SetState(SlogAction.WIFIKEY, chkWifi.isChecked(),
						false);
				break;
			case R.id.chk_modem_misc:
				SlogAction.SetState(SlogAction.MISCKEY, chkMisc.isChecked(),
						false);

			}
		}
	}

	private void SyncState() {
		boolean tempHost = SlogAction.GetState(SlogAction.GENERALKEY);
		SlogAction.SetCheckBoxBranchState(chkModem, tempHost,
				SlogAction.GetState(SlogAction.MODEMKEY));
		SlogAction.SetCheckBoxBranchState(chkBuleTooth, tempHost,
				SlogAction.GetState(SlogAction.BULETOOTHKEY));
		SlogAction.SetCheckBoxBranchState(chkWifi, tempHost,
				SlogAction.GetState(SlogAction.WIFIKEY));
		SlogAction.SetCheckBoxBranchState(chkMisc, tempHost,
				SlogAction.GetState(SlogAction.MISCKEY));

	}
}
