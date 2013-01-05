/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import android.content.Context;
import android.content.ServiceConnection;
import android.content.Intent;
import android.content.ComponentName;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

public class RadioServiceStub extends Object {
    public static final String LOGTAG = "FMplayServiceStub";

    private IRadioService mService;
    private Context mContext;
    private ServiceConnection mCallback;
    private ServiceConnection mBinder;

    public RadioServiceStub(Context context, ServiceConnection callback) {
        mContext = context;
        mCallback = callback;
        mService = null;
        mBinder = null;
    }

    public RadioServiceStub(Context context) {
        mContext = context;
        mService = null;
        mCallback = null;
        mBinder = null;
    }

    public boolean bindToService() {
        mContext.startService(new Intent(mContext, FMplayService.class));
        mBinder = new BinderCallback(mCallback);
        return mContext.bindService((new Intent()).setClass(mContext, FMplayService.class), mBinder,
                Context.BIND_AUTO_CREATE);
    }

    public void unbindFromService() {
        mContext.unbindService(mBinder);
    }

    public boolean fmOn() {
        boolean value = false;
        if (mService != null) {
            try {
                value = mService.fmOn();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }

        return value;
    }

    public float getFreq() {
        float freq = -1.0f;
        if (mService != null) {
            try {
                freq = mService.getFreq();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }

        return freq;
    }

    public boolean setFreq(float freq) {
        boolean value = false;
        if (mService != null) {
            try {
                value = mService.setFreq(freq);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }

        return value;
    }

    private class BinderCallback implements ServiceConnection {
        private ServiceConnection mCallback;

        public void onServiceDisconnected(ComponentName name) {
            if (mCallback != null) {
                mCallback.onServiceDisconnected(name);
            }
            mService = null;
        }
        public void onServiceConnected(ComponentName name, IBinder service) {
            mService = IRadioService.Stub.asInterface(service);
            if (mCallback != null) {
                mCallback.onServiceConnected(name, service);
            }
        }
        public BinderCallback(ServiceConnection callback) {
            mCallback = callback;
        }
    }
    
    public boolean fmOff() {
        boolean value = false;
        if (mService != null) {
            try {
                value = mService.fmOff();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return value;
    }

    public boolean isFmOn() {
        boolean value = false;
        if (mService != null) {
            try {
                value = mService.isFmOn();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return value;
    }

    public boolean mute() {
        boolean value = false;
        if (mService != null) {
            try {
                value = mService.mute();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return value;
    }

    public boolean unMute() {
        boolean value = false;
        if (mService != null) {
            try {
                value = mService.unMute();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return value;
    }

    public boolean isMuted() {
        boolean value = false;
        if (mService != null) {
            try {
                value = mService.isMuted();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return value;
    }

    public boolean startSeek(boolean up) {
        boolean value = false;
        if (mService != null) {
            try {
                value = mService.startSeek(up);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return value;
    }

    public boolean stopSeek() {
    	Log.d(LOGTAG, "stopSearching, Thread: " + Thread.currentThread().getName());
        boolean value = false;
        if (mService != null) {
            try {
                value = mService.stopSeek();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return value;
    }

    public boolean setVolume(int volume) {
        boolean value = false;
        if (mService != null) {
            try {
                value = mService.setVolume(volume);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return value;
    }

    public synchronized float getFreq2(boolean issech) {
        float freq = -1.0f;
        if (mService != null) {
            try {
                freq = mService.getFreq2(issech);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return freq;
    }

    public boolean routeAudio(int device) {
        boolean value = false;
        try {
            value = mService.routeAudio(device);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return value;
    }

    public int getAudioDevice() {
        int device = FMplayService.RADIO_AUDIO_DEVICE_SPEAKER;
        if (mService != null) {
            try {
                device = mService.getAudioDevice();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        return device;
    }
}


