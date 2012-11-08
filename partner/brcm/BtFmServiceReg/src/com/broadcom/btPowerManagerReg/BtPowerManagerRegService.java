package com.broadcom.btPowerManagerReg;

import android.app.Notification;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import android.os.ServiceManager;
import com.broadcom.bt.service.PowerManagementService;
import com.broadcom.bt.service.PowerManager;

public class BtPowerManagerRegService extends Service {
    private static final String TAG = "BtPowerManagerRegService";
    private PowerManagementService mPms;
    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // TODO Auto-generated method stub
        startForeground(1, new Notification());
        mPms = new PowerManagementService(this);
        try {
            Log.i(TAG, "BT FM Power Management Service");
            ServiceManager.addService(PowerManagementService.SERVICE_NAME, mPms);
        } catch (Throwable e) {
            Log.e(TAG, "Failure starting BT FM Power Management Service", e);
        }
        PowerManager.getProxy().updateService();
        return super.onStartCommand(intent, flags, startId);
    }

}
