/* Copyright 2010 - 2011 Broadcom Corporation
 **
 ** This program is the proprietary software of Broadcom Corporation and/or its
 ** licensors, and may only be used, duplicated, modified or distributed 
 ** pursuant to the terms and conditions of a separate, written license 
 ** agreement executed between you and Broadcom (an "Authorized License").  
 ** Except as set forth in an Authorized License, Broadcom grants no license 
 ** (express or implied), right to use, or waiver of any kind with respect to 
 ** the Software, and Broadcom expressly reserves all rights in and to the 
 ** Software and all intellectual property rights therein.  
 ** IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS 
 ** SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE 
 ** ALL USE OF THE SOFTWARE.  
 **
 ** Except as expressly set forth in the Authorized License,
 ** 
 ** 1.     This program, including its structure, sequence and organization, 
 **        constitutes the valuable trade secrets of Broadcom, and you shall 
 **        use all reasonable efforts to protect the confidentiality thereof, 
 **        and to use this information only in connection with your use of 
 **        Broadcom integrated circuit products.
 ** 
 ** 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED 
 **        "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, 
 **        REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, 
 **        OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY 
 **        DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, 
 **        NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, 
 **        ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR 
 **        CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 **        OF USE OR PERFORMANCE OF THE SOFTWARE.
 **
 ** 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 **        ITS LICENSORS BE LIABLE FOR 
 **        (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY 
 **              DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO 
 **              YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM 
 **              HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR 
 **        (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
 **              SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE 
 **              LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF 
 **              ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 */

package com.broadcom.bt.service.fm;

import java.util.Iterator;
import java.util.LinkedHashMap;

import android.app.Application;
import android.content.Context;
import android.util.Log;

import com.broadcom.bt.service.framework.IBtService;
import com.broadcom.bt.service.PowerManager;

/**
 * @hide
 */
public class FmServiceManager extends Application {
    public static final String TAG = "FmServiceManager";
    private static boolean _DBG = FmServiceConfig.D;

    private static boolean mInitialized = false;
    private static int mLastFmState = -1;

    // ------------Implements Application API---------------
    private static FmServiceManager instance;

    public FmServiceManager() {
        Log.d(TAG, "****Constructor called*************");
        instance = this;
    }

    public static Context getContext() {
        return instance;
    }

    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "Loading FmRadio package and enabling FmService....");
        FmServiceManager.init(this);
    }

    public void onTerminate() {
        Log.d(TAG, "Disabling FmService and unloading FmRadio package....");
    }

    // ------------Ends implements Application API---------------

    
    
    //Service manager api---------------------------------------
    public synchronized static boolean isInitialized() {
        return mInitialized;
    }

    /**
     * Register Fm service
     * 
     */
    private static void registerServices() {
        _registerService(FmReceiver.SERVICE_NAME, new FmReceiverService(mSystemContext));
    }

    private static LinkedHashMap<String, FmServiceWrapper> mFmServiceWrapper = new LinkedHashMap<String, FmServiceWrapper>();
    private static Context mSystemContext;
    private static boolean mIsFactoryTest;

    private static class FmServiceWrapper {
        private IBtService mSvc;
        private boolean mIsStarted;

        public FmServiceWrapper(IBtService eventLoop, boolean isStarted) {
            mSvc = eventLoop;
            mIsStarted = isStarted;
        }
    }

    /**
     * Initialize the Fm Service Manager
     * 
     * @param systemContext
     * @param isEmulator
     * @param isFactoryTest
     */
    private synchronized static void init(Context systemContext) {
        Log.d(TAG, "init() called ");

        if (mInitialized) {
            Log.w(TAG, "Fm Service Manager already initialized.....Skipping init()...");
            return;
        }
        mSystemContext = systemContext;
        // Register Services
        registerServices();
        mInitialized = true;
    }

    private static void _registerService(String svcName, IBtService svc) {
        boolean isSupported = FmServiceConfig.isServiceSupported(svcName);
        if (_DBG) {
            if (!isSupported) {
                Log.d(TAG, "****Fm Service not supported " + svcName + ": "
                        + (svc == null ? "null" : svc.getClass().getName()) + "...Skipping. ****");
            }

            Log.d(TAG, "****Registering Fm Service " + svcName + ": "
                    + (svc == null ? "null" : svc.getClass().getName()) + "****");
        }

        if (isSupported) {
            synchronized (mFmServiceWrapper) {
                mFmServiceWrapper.put(svcName, new FmServiceWrapper(svc, false));
            }
        }
    }

    private static void _startService(String svcName) {
        if (svcName == null || !FmServiceConfig.isServiceEnabled(mSystemContext, svcName)) {
            if (_DBG) {
                Log.d(TAG, "***startService(): Fm service " + (svcName == null ? "null" : svcName)
                        + " not enabled. Skipping start...***");
            }
            return;
        }

        FmServiceWrapper svcWrapper = null;
        synchronized (mFmServiceWrapper) {
            svcWrapper = mFmServiceWrapper.get(svcName);
        }

        if (svcWrapper == null) {
            if (_DBG) {
                Log.d(TAG, "***startService(): Unable to find service record for: " + svcName + ". Skipping...***");
            }
            return;
        }
        synchronized (svcWrapper) {
            if (svcWrapper.mIsStarted) {
                Log.d(TAG, "***" + svcName + " already started....skipping start...");
                return;
            }
            if (svcWrapper.mSvc == null) {
                if (_DBG) {
                    Log.d(TAG, "***startService(): Service " + svcName
                            + "is not managed by FmServiceManager. Skipping...***");
                    // Set state of service to started
                    svcWrapper.mIsStarted = true;
                }
                return;
            }
            if (_DBG) {
                Log.d(TAG, "***startService(): Starting service: " + svcName + "***");
            }
            svcWrapper.mSvc.init();
            svcWrapper.mSvc.start();
        }
    }

    private static void _stopService(String svcName) {
        if (svcName == null) {
            return;
        }
        if (_DBG) {
            Log.d(TAG, "***stopService(): Stopping service: " + svcName + "***");
        }

        FmServiceWrapper svcWrapper = null;
        synchronized (mFmServiceWrapper) {
            svcWrapper = mFmServiceWrapper.get(svcName);
        }

        if (svcWrapper == null) {
            if (_DBG) {
                Log.d(TAG, "***stopService(): Unable to find service record for: " + svcName + ". Skipping...***");
            }
            return;
        }

        synchronized (svcWrapper) {
            if (!svcWrapper.mIsStarted) {
                Log.d(TAG, "***" + svcName + " already stopped....skipping stop...");
                return;
            }

            if (svcWrapper.mSvc == null) {
                if (_DBG) {
                    Log.d(TAG, "***stopService(): Service " + svcName
                            + "is not managed by Service Manager. Skipping...***");
                    svcWrapper.mIsStarted = false;
                }
                return;

            }
            if (_DBG) {
                Log.d(TAG, "***stopService(): Stop service: " + svcName + " ***");
            }

            synchronized (svcWrapper) {
                svcWrapper.mSvc.stop();
            }
        }

    }

    public static synchronized void onEnabled() {
        Log.d(TAG, "onEnabled()");

        if (mIsFactoryTest) {
            return;
        }

        if (mLastFmState == FmServiceConfig.STATE_STARTED) {
            Log.d(TAG, "Already STARTED..Skipping");
            return;
        }

        synchronized (mFmServiceWrapper) {
            Iterator<String> svcIterator = mFmServiceWrapper.keySet().iterator();
            while (svcIterator.hasNext()) {
                String svcName = svcIterator.next();
                _startService(svcName);
            }
        }
        mLastFmState = FmServiceConfig.STATE_STARTED;
    }

    private static void onDisabled() {
        Log.d(TAG, "onDisabled()");

        if (mLastFmState == FmServiceConfig.STATE_STOPPED) {
            Log.d(TAG, "Already STOPPED..Skipping");
            return;
        }

        synchronized (mFmServiceWrapper) {
            Iterator<String> svcIterator = mFmServiceWrapper.keySet().iterator();
            while (svcIterator.hasNext()) {
                String svcName = svcIterator.next();
                _stopService(svcName);
            }
        }

        mLastFmState = FmServiceConfig.STATE_STOPPED;

    }


    public static IBtService getService(String svcName) {
        synchronized (mFmServiceWrapper) {
            FmServiceWrapper svcWrapper = mFmServiceWrapper.get(svcName);
            return svcWrapper == null ? null : svcWrapper.mSvc;
        }
    }

    /**
     * returns the state of the service: NOT_AVAILABLE,STOPPED, or STARTED
     * 
     * @param svcName
     * @return
     */
    public static int getServiceState(String svcName) {
        FmServiceWrapper svcWrapper = mFmServiceWrapper.get(svcName);
        if (svcWrapper == null) {
            return FmServiceConfig.STATE_NOT_AVAILABLE;
        } else {
            return svcWrapper.mIsStarted ? FmServiceConfig.STATE_STARTED : FmServiceConfig.STATE_STOPPED;
        }
    }

}
