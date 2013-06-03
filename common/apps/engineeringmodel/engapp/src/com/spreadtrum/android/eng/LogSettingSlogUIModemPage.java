package com.spreadtrum.android.eng;

/* Actually engconstents and engfetch are em... incorrect or bad coding style */
import com.spreadtrum.android.eng.engconstents;
import com.spreadtrum.android.eng.engfetch;
import com.spreadtrum.android.eng.R;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.CheckBox;

/* SlogUI Added by Yuntao.xiao*/

public class LogSettingSlogUIModemPage extends Activity implements SlogUISyncState {

    private CheckBox chkModem, chkBuleTooth, chkTcp, chkMisc;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_modem);

        chkModem = (CheckBox) findViewById(R.id.chk_modem_branch);
        chkBuleTooth = (CheckBox) findViewById(R.id.chk_modem_bluetooth);
        chkTcp = (CheckBox) findViewById(R.id.chk_modem_tcp);
        chkMisc = (CheckBox) findViewById(R.id.chk_modem_misc);

        syncState();

        // TODO: Should better using setOnClickListener(this) instead of newing a class.
        ClkListenner clickListen = new ClkListenner();
        chkModem.setOnClickListener(clickListen);
        chkBuleTooth.setOnClickListener(clickListen);
        chkTcp.setOnClickListener(clickListen);
        chkMisc.setOnClickListener(clickListen);

    }

    @Override
    protected void onResume() {
        super.onResume();
        syncState();
    }

    protected class ClkListenner implements OnClickListener {
        public void onClick(View onClickView) {
            switch (onClickView.getId()) {
            case R.id.chk_modem_branch:
                SlogAction.SetState(SlogAction.MODEMKEY, chkModem.isChecked(),
                        false);
                SlogAction.sendATCommand(engconstents.ENG_AT_SETARMLOG, chkModem.isChecked());
                break;
            case R.id.chk_modem_bluetooth:
                SlogAction.SetState(SlogAction.BULETOOTHKEY,
                        chkBuleTooth.isChecked(), false);
                break;
            case R.id.chk_modem_tcp:
                SlogAction.SetState(SlogAction.TCPKEY, chkTcp.isChecked(),
                        false);
                SlogAction.sendATCommand(engconstents.ENG_AT_SETCAPLOG, chkTcp.isChecked());
                break;
            case R.id.chk_modem_misc:
                SlogAction.SetState(SlogAction.MISCKEY, chkMisc.isChecked(),
                        false);
                break;

            }
        }
    }

    @Override
    public void syncState() {
        boolean tempHostOn = SlogAction.GetState(SlogAction.GENERALKEY, true)
                                        .equals(SlogAction.GENERALON);
        boolean tempHostLowPower = SlogAction.GetState(SlogAction.GENERALKEY, true)
                                        .equals(SlogAction.GENERALLOWPOWER);
        boolean tempHost = tempHostOn || tempHostLowPower;
        SlogAction.SetCheckBoxBranchState(chkModem, tempHost,
                SlogAction.GetState(SlogAction.MODEMKEY));
        SlogAction.SetCheckBoxBranchState(chkBuleTooth, tempHost && SlogAction.GetState(SlogAction.STORAGEKEY),
                SlogAction.GetState(SlogAction.BULETOOTHKEY));
        SlogAction.SetCheckBoxBranchState(chkTcp, tempHost,
                SlogAction.GetState(SlogAction.TCPKEY));
        SlogAction.SetCheckBoxBranchState(chkMisc, tempHost,
                SlogAction.GetState(SlogAction.MISCKEY));

    }

    // TODO: Underconstruction
    @Override
    public void onSlogConfigChanged() {
        syncState();
    }

}
