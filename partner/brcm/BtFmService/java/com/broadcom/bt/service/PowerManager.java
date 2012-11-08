package com.broadcom.bt.service;

import android.os.IBinder;
import android.os.ServiceManager;
import android.util.Log;

public class PowerManager {
    private static final String TAG = "BTFMPowerManager";

    /**
     * Lazyily initialized singleton. Guaranteed final after first object
     * constructed.
     */
    private static PowerManager mProxy;

    private IPowerManager mService;

    /**
     * Get a handle to the PowerManagementService
     * <p>
     *      * 
     * @return the proxy object for PowerManagementService
     *         
     */
    public static synchronized PowerManager getProxy() {
        if (mProxy == null) {
            IBinder b = ServiceManager.getService(PowerManagementService.SERVICE_NAME);
            if (b != null) {
                IPowerManager service = IPowerManager.Stub.asInterface(b);
                mProxy = new PowerManager(service);
            }
        }
        return mProxy;
    }

    public PowerManager(IPowerManager service) {
        if (service == null) {
            throw new IllegalArgumentException("service is null");
        }
        mService = service;
    }

    public void updateService() {
        IBinder b = ServiceManager.getService(PowerManagementService.SERVICE_NAME);
        if (b != null) {
            IPowerManager service = IPowerManager.Stub.asInterface(b);
            mService = service;
        }
    }

    public boolean enableFm() {
        try {
        	return mService.enableFm();         
        } catch (Throwable t) {
            Log.e(TAG, "Unable to enable FM", t);
        }
        return false;
    }

    public boolean disableFm() {
        try {
        	return mService.disableFm();             
        } catch (Throwable t) {
            Log.e(TAG, "Unable to enable FM", t);
        }
        return false;
    }

    public boolean enableBt() {
        try {
            return mService.enableBt();            
        } catch (Throwable t) {
            Log.e(TAG, "Unable to enable FM", t);
        }
        return false;
    }

    public boolean disableBt() {
        try {
        	return mService.disableFm();
        } catch (Throwable t) {
            Log.e(TAG, "Unable to disable FM", t);
        }
        return false;

    }

    public boolean isBtEnabled() {
    	   try {
               return mService.isBtEnabled();               
           } catch (Throwable t) {
               Log.e(TAG, "Unable to disable FM", t);
           }
        return false;
    }

    public boolean isfmEnabled() {
    	try {
            return mService.isfmEnabled();               
        } catch (Throwable t) {
            Log.e(TAG, "Unable to disable FM", t);
        }
        return false;
    }
}
