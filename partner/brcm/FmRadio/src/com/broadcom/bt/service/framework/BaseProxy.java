/* Copyright 2010 Broadcom Corporation
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
package com.broadcom.bt.service.framework;



import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Process;
import android.util.Log;

/**
 * Base class all Bluetooth service proxies must implement. This class should
 * only be used by Proxy implementations and not be exposed to Android
 * application writers.
 * 
 * @hide
 * 
 */
public abstract class BaseProxy implements ServiceConnection {
	private static final String TAG = "BaseProxy";
	
	/**
	 * Permission required to interact with Bluetooth profiles
	 */
	public static final String BLUETOOTH_PERM = android.Manifest.permission.BLUETOOTH;

	/**
	 * The default priority for a Broadcast Receiver created by the
	 * proxy object, if Broadcast Intents are used to for events and
	 * a receiver priority is not specified.
	 */
	public static final int DEFAULT_BROADCAST_RECEIVER_PRIORITY = 200;



	/**
	 * Fast comparison function to determine if two Bluetooth action strings
	 * are equal. The first 'offset' characters are ignored.
	 * @hide
	 * @param a1
	 * @param a2
	 * @param offset
	 * @return
	 * @hide
	 */
	protected static boolean actionsEqual(String a1, String a2, int offset) {
		if (a1 == a2)
			return true;
		int a1length = a1.length();
		if (a1length != a2.length())
			return false;

		return (a1.regionMatches(offset, a2, offset, a1length - offset));
	}
	
	
	
	
	/**
	 * @hide
	 */
	protected Context mContext;
	
	/**
	 * @hide
	 */
	protected EventCallbackHandler mEventCallbackHandler;
	
	/**
	 * @hide
	 */
	protected BroadcastReceiver mBroadcastReceiver;
	
	/**
	 * @hide
	 */
	protected int mReceiverPriority=DEFAULT_BROADCAST_RECEIVER_PRIORITY;
	
	
    /**
     * @hide
     */
	protected boolean mIsAvailable;

    protected synchronized Handler initEventCallbackHandler() {
        return initEventCallbackHandler(null);
    }
    
	/**
	 * @hide
	 */
	protected synchronized Handler initEventCallbackHandler(IHandlerCreator creator) {
		if (mEventCallbackHandler == null) {
			mEventCallbackHandler = new EventCallbackHandler(creator);
			mEventCallbackHandler.start();
			while (mEventCallbackHandler.mHandler ==null) {
				try {
					Thread.sleep(100);
				} catch (Exception e) {}
				
			}
		}
		return mEventCallbackHandler.mHandler;
	}
	
	/**
	 * @hide
	 */ 
	protected synchronized void finishEventCallbackHandler() {
		if (mEventCallbackHandler != null) {
			mEventCallbackHandler.finish();
			mEventCallbackHandler=null;
		}
	}
	
	/**
	 * @hide
	 */
	protected BaseProxy() {
		
	}
	

	/**
	 * @hide
	 * @param ctx
	 * @param cb
	 * @param pkgName
	 * @param className
	 * @return
	 */
	public boolean create(Context ctx, IBluetoothProxyCallback cb, 
			String pkgName, String className) {
		mContext=ctx;
		mProxyAvailCb =cb;
		Intent i = new Intent();
		i.setClassName(pkgName, className);
		return mContext.bindService(i, this, Context.BIND_AUTO_CREATE);
	}
	
	/**
	 * @hide
	 * @param service
	 * @return
	 */
	protected abstract boolean init(IBinder service);
	
	/**
	 * Closes connection to the Bluetooth service and releases resources.
	 * Applications must call this method when it is done interacting with
	 * the Bluetooth service.
	 */
	public synchronized void finish() {
		if (mEventCallbackHandler != null) {
			mEventCallbackHandler.finish();
			mEventCallbackHandler=null;
		}
		
		if (mContext != null) {
			mContext.unbindService(this);
			mContext = null;
		}
	}

	/**
	 * @hide
	 * @return
	 */
	public boolean requiresAccessProcessing() {
		return false;
	}

	/**
	 * @hide
	 * @param opCode
	 * @param allow
	 * @param data
	 */
	public void setAccess(int opCode, boolean allow, Object data) {

	}

	protected void finalize() {
		finish();
	}
	
	protected interface IHandlerCreator {
	    Handler create();
	}

	
	/**
	 * @hide
	 */
	protected class EventCallbackHandler extends Thread {
	    
		public Handler mHandler;
		public IHandlerCreator mCreator;
		
		public EventCallbackHandler(IHandlerCreator creator) {
		    mCreator = creator;
		}
		public EventCallbackHandler() {
			super();
			setPriority(Process.THREAD_PRIORITY_BACKGROUND);
		}
		public void run() {
			Looper.prepare();
			mHandler = (mCreator == null? new Handler(): mCreator.create());
			Looper.loop();
		}
		
		
		public void finish() {
			if (mHandler != null) {
				Looper l = mHandler.getLooper();
				if (l != null) {
					l.quit();
				}
			}
		}
	}
	
	/**
	 * @hide
	 */
	protected IBluetoothProxyCallback mProxyAvailCb;
	/**
	 * @hide
	 */
	public synchronized void onServiceConnected(ComponentName name, IBinder service) {
		if (service == null || !init(service) && mProxyAvailCb!=null) {
			Log.e(TAG, "Unable to create proxy");
		}
		mProxyAvailCb.onProxyAvailable(this);
		mProxyAvailCb = null;		
	}

	/**
	 * @hide
	 */
	public synchronized void onServiceDisconnected(ComponentName name) {
	}
	
}
