/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.os.RemoteException;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.os.SystemProperties;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.lang.ref.WeakReference;
import android.content.Context;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.util.Log;
import android.widget.Toast;
import android.view.Gravity;
import android.provider.Telephony;
import android.provider.Settings.System;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.widget.RemoteViews;
import android.content.ComponentName;
import android.bluetooth.BluetoothAdapter;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.widget.Toast;
import android.os.HandlerThread;
import android.os.PowerManager.WakeLock;
import java.lang.InterruptedException;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.hardware.fm.FmManager;
import android.hardware.fm.FmConsts.*;


public class FMplayService extends Service {
    private static final String LOGTAG = "FMplayService";
    
    public static final int RADIO_AUDIO_DEVICE_WIRED_HEADSET = 0;
    public static final int RADIO_AUDIO_DEVICE_SPEAKER = 1;
    private static final int IDLE_DELAY = 500;
    private static final int MSG_DELAYED_CLOSE = 0;
    private static final int MSG_SET_VOLUME = 1;

    public static final String ACTION_SHUTDOWN = "com.thunderst.radio.RadioService.SHUTDOWN";
    public static final String ACTION_COUNT = "com.thunderst.radio.RadioService.COUNT";
    private static final String FM_ALARM_ALERT = "com.android.deskclock.ALARM_ALERT";
    private static final String FM_ALARM_DONE = "com.android.deskclock.ALARM_DONE";
    private static final String FM_ALARM_SNOOZE = "com.android.deskclock.ALARM_SNOOZE";
    private static final String ACTION_MUSIC_PAUSE = "com.android.music.musicservicecommand.pause";
    private static final String ACTION_SOUNDRECORDER_PAUSE = "com.android.soundercorder.soundercorder.pause";
    private static final String ACTION_SOUNDRECORDER_START = "com.android.soundercorder.soundercorder.start";
    private static final String ACTION_AUTOPREVIEW_PAUSE = "com.android.audiopreview.pause";
    private static final String ACTION_AUTOPREVIEW_START = "com.android.audiopreview.start";
    private static final String ACTION_FM_PAUSE = "com.thundersoft.radio.RadioService.PAUSE";
    private static final String ACTION_FM_START = "com.thundersoft.radio.radioService.START";
    private static final String ACTION_SMS_START = "com.android.mms.SMS_PLAY_START";
    private static final String ACTION_SMS_STOP = "com.android.mms.SMS_PLAY_STOP";
    private static final String ACTION_FM_MUSIC_PAUSE = "com.android.fm.shutdown";
    private static final String ACTION_FM_MUSIC_START = "com.android.fm.start";
    private static final String ACTION_FM_RING_PAUSE = "com.android.fm.ring.shutdown";
    private static final String ACTION_FM_RING_RESET = "com.android.fm.ring.reset";
    private static final String ACTION_FM_VIDEO_PAUSE = "com.android.video.stopfmservice";
    private static final String ACTION_RADIO_PAUSE = "com.android.music.musicservicecommand";//mm 04 fix bug 6008
	private static final String  ACTION_ATV_START = "com.nmi.test.START";
    private static final String ACTION_ATV_STOP = "com.nmi.test.STOP";
    private static final String ACTION_MBBM_START = "com.cmcc.mbbms.START";
    private static final String ACTION_MBBM_STOP = "com.cmcc.mbbms.STOP";
    //mm04 fix bug 3434
    private static final String ACTION_CAMERA_PAUSE = "com.android.camera.action.RECORD_VIDEO";
    private static final String ACTION_CAMERA_START ="com.android.camera.action.RECORD_VIDEO.PAUSE";

    private static final String RADIO_DEVICE = "/dev/radio0";
    private static int mAudioDevice = RADIO_AUDIO_DEVICE_WIRED_HEADSET;

    private static final int TRANS_MULT = 1000;
    private static final int PHONE_POST_DELAY = 800; //millisecond, to give the system enough time to do phone-post deal
    private static final int FM_STATUS = 1;
    private static final int FM_SEARCH_TIMEOUT = 3000;

    private volatile boolean mFmOn = false;
    private volatile boolean mIsMuted = false;
    private boolean mServiceInUse = false;
    private boolean mRecovery = false;
    //used to track what type of audio focus loss caused the playback to pause
    private boolean mPausedByTransientLossOfFocus = false;
    private boolean sounderRecordStop = false;
    private boolean cameraStop = false;
    private boolean musicStop = false;
    private boolean autopreviewStop = false;

    private final IBinder mBinder = new ServiceStub(this);

    private int mServiceId = -1;

    private static final String HEADSET_STATE_PATH = "/sys/class/switch/h2w/state";


    private int mCount = 0;
    private int mMuteCount = 0;
    private boolean mIsTuned = false;

    private HandlerThread mHandlerThread = null;
    private Handler mVolumeHandler = null;

    private volatile boolean mNeedShutdown = false;

    public boolean ismute = true;

    public static boolean isstopNotification = true;

    public static float freq = 87.5f;//fix bug 11784

    //mm04 fix bug 4935
    private int headset_plug_count = 0;

    //mm04 fix bug 5871
    private RemoteViews views = null;

    private boolean ismPhoneStateListener = false;
    private Notification notice = null;

    private boolean mIsSpeaker = false;

    private TelephonyManager tmgr1;
    private TelephonyManager tmgr2;

    private WakeLock mWakeLock;

    private FmManager mFmManager = null;

    public void onCreate() {
        Log.d(LOGTAG,"onCreate");
        super.onCreate();
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, this.getClass().getName());

        mFmManager = new FmManager(this);

        Message msg = mDelayedStopHandler.obtainMessage(MSG_DELAYED_CLOSE);
        mDelayedStopHandler.sendMessageDelayed(msg, IDLE_DELAY);

        NotificationManager nm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        nm.cancel(FM_STATUS);

        mHandlerThread = new HandlerThread("handler_thread");
        if (mHandlerThread != null) {
            mHandlerThread.start();
            mVolumeHandler = new Handler(mHandlerThread.getLooper()) {
                public void handleMessage(Message msg) {
                    if (msg != null) {
                        removeMessages(msg.what);
                        setVolume(msg.arg1);
                    }
                }
            };
        }

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_HEADSET_PLUG);
        filter.addAction(AudioManager.VOLUME_CHANGED_ACTION);
        filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        filter.addAction(ACTION_SHUTDOWN);
        filter.addAction(Intent.ACTION_SHUTDOWN);
        filter.addAction(ACTION_COUNT);
        registerReceiver(mMonitor, filter);

        //mm04 fix bug 2580
        IntentFilter filterMedia = new IntentFilter();
        filterMedia.addAction(FM_ALARM_ALERT);
        filterMedia.addAction(FM_ALARM_DONE);
        filterMedia.addAction(ACTION_SOUNDRECORDER_PAUSE);
        filterMedia.addAction(ACTION_FM_MUSIC_PAUSE);
        filterMedia.addAction(ACTION_FM_RING_PAUSE);
        filterMedia.addAction(ACTION_FM_RING_RESET);
        filterMedia.addAction(ACTION_FM_VIDEO_PAUSE);
        filterMedia.addAction(ACTION_CAMERA_PAUSE);
        filterMedia.addAction(ACTION_RADIO_PAUSE);
        filterMedia.addAction(ACTION_AUTOPREVIEW_PAUSE);
        filterMedia.addAction(ACTION_AUTOPREVIEW_START);
        filterMedia.addAction(ACTION_SMS_START);
        filterMedia.addAction(ACTION_SMS_STOP);
        filterMedia.addAction(ACTION_MBBM_START);
        filterMedia.addAction(ACTION_MBBM_STOP);
        filterMedia.addAction(ACTION_ATV_START);
        filterMedia.addAction(ACTION_ATV_STOP);
        filterMedia.addAction(ACTION_CAMERA_START);
        filterMedia.addAction(ACTION_SOUNDRECORDER_START);
        filterMedia.addAction(ACTION_FM_MUSIC_START);
        registerReceiver(mAudioInteropReceiver,filterMedia);

        //mm04 fix bug 2706
        tmgr1 = (TelephonyManager) getSystemService("phone");
        tmgr1.listen(mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
        if (TelephonyManager.getPhoneCount()>1){
			tmgr2 = (TelephonyManager) getSystemService("phone1");
			if (tmgr2 != null) {
				tmgr2.listen(mPhoneStateListener,
						PhoneStateListener.LISTEN_CALL_STATE);
			}
        }

	///mm04 fix bug 3183
        Intent i = new Intent("com.android.fmservice.stopmusicservice");
        i.putExtra("playingfmservice", true);


        this.sendBroadcast(i);
        mRadioService = this;
    }

    //for bug 13287
    public static FMplayService mRadioService;

    public static FMplayService getInstance(){
    	Log.v(LOGTAG,"----------get Instance");
    	if(mRadioService != null){
    	    return mRadioService;
    	}else{
    	    return new FMplayService();
    	}
    }
    //end
	private PhoneStateListener mPhoneStateListener = new PhoneStateListener() {
        @Override
        public void onCallStateChanged(int state, String incomingNumber) {
            Log.d(LOGTAG,"mPhoneStateListener"+"state1111:"+state);
            if (state == TelephonyManager.CALL_STATE_RINGING || state == TelephonyManager.CALL_STATE_OFFHOOK) {
                if(isFmOn()){
                    mPausedByTransientLossOfFocus = true;
                    FMPlay.sendMessage = false;
                    isstopNotification=false;
                    freq = getFreq();
                    fmOff();
                    if(FMPlay.routeDevices == 1){
                        FMPlay.audioChannel = true;
                        }
                    ismPhoneStateListener = true;
                    }else{
                        FMPlay.sendMessage = true;
                        ismPhoneStateListener = true;
                        }
                } else if (state == TelephonyManager.CALL_STATE_IDLE) {
                    if(!isFmOn() && isHeadsetExists() && (System.getInt(getContentResolver(), System.AIRPLANE_MODE_ON, 0) == 0) && ismPhoneStateListener){
                        if(mPausedByTransientLossOfFocus){
                            Log.d(LOGTAG,"onCallStateChanged,fmOn()");
                            fmOn();
                            Log.d(LOGTAG, "mPhoneStateListener-------setFreq "+freq);
                            setFreq(freq);
                            mPausedByTransientLossOfFocus = false;
                            }
                        ismPhoneStateListener = false;
                        }
            }
        }
    };

    private boolean isHeadsetExists(){
        char[] buffer = new char[1024];

        int newState = 0;

        try {
            FileReader file = new FileReader(HEADSET_STATE_PATH);
            int len = file.read(buffer, 0, 1024);
            newState = Integer.valueOf((new String(buffer, 0, len)).trim());
        } catch (FileNotFoundException e) {
            Log.e(LOGTAG, "This kernel does not have wired headset support");
        } catch (Exception e) {
            Log.e(LOGTAG, "" , e);
        }
        return newState != 0;
   }

    private OnAudioFocusChangeListener mAudioFocusListener = new OnAudioFocusChangeListener() {
        public void  onAudioFocusChange(int focusChange) {
        }
    };

    private RadioServiceReceiver mMonitor = new RadioServiceReceiver();

    private class RadioServiceReceiver extends BroadcastReceiver {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_HEADSET_PLUG)) {
                if (!SystemProperties.getBoolean("ro.device.support.antenna", false)) {
                    Log.d(LOGTAG, "intent.getIntExtra" + intent.getIntExtra("state", 0)
                            + ",headset_plug_count:" + headset_plug_count);
                    if (isFmOn()) {
                        // mm04 fix bug 4935
                        if (intent.getIntExtra("state", 0) == 0) {
                            Log.d(LOGTAG, "ACTION_HEADSET_PLUG");
                            fmOff();
                        }
                    } else if (headset_plug_count > 0 && intent.getIntExtra("state", 0) == 0) {
                        Log.d(LOGTAG, "ACTION_HEADSET_PLUG , headset_plug_count:"
                                + headset_plug_count);
                        stopNotification();
                    }
                    headset_plug_count++;
                }
            } else if (action.equals(AudioManager.VOLUME_CHANGED_ACTION)) {
                if (intent.hasExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE)
                 && intent.hasExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE)) {
                    int streamType = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
                    if (streamType == AudioManager.STREAM_FM) {
                        int value = intent.getIntExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE, -1);
                        if (value != -1) {
                            Log.d(LOGTAG, "stream type " + streamType + "value " + value);
                            if (mVolumeHandler != null) {
                                Message msg = mVolumeHandler.obtainMessage(MSG_SET_VOLUME);
                                msg.arg1 = value;
                                mVolumeHandler.sendMessage(msg);
                            }
                        }
                    }
                }
            } else if (action.equals(Intent.ACTION_AIRPLANE_MODE_CHANGED)) {
                if (intent.hasExtra("state")) {
                    boolean enable = intent.getBooleanExtra("state", false);
                    Log.d(LOGTAG, "airplane mode enable " + enable);
                    if (enable) {
                            mNeedShutdown = true;
                            Toast airplaneInfo = Toast.makeText(context, getResources().getString(R.string.airplane_info),Toast.LENGTH_LONG);
                            airplaneInfo.show();
                            fmOff();
                    } else {//mm04 fix bug 5040
                    	Toast airplaneInfo = Toast.makeText(context, getResources().getString(R.string.airplane_info_close),Toast.LENGTH_LONG);
                    	airplaneInfo.show();
                    }
                }
            } else if (action.equals(ACTION_SHUTDOWN) || action.equals(Intent.ACTION_SHUTDOWN)) {
                mNeedShutdown = true;
                if (isFmOn()) {
                    fmOff();
                }
            } else if (action.equals(ACTION_COUNT)) {
                if (intent.hasExtra("count")) {
                    int count = intent.getIntExtra("count", 0);
                    mCount += count;
                    Log.d(LOGTAG, "count: " + count);
                    if (mCount == 0) {
                        mServiceInUse = false;
                    } else {
                        mServiceInUse = true;
                    }
                }
            } else if (action.equals(BluetoothAdapter.ACTION_STATE_CHANGED)){
                int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
                if (state == BluetoothAdapter.STATE_TURNING_OFF || state == BluetoothAdapter.ERROR){
                    String info = "";
                    if(state == BluetoothAdapter.STATE_TURNING_OFF){
                        info = getResources().getText(R.string.bluetooth_disabled_message).toString();
                    } else {
                        info = getResources().getText(R.string.bluetooth_error_message).toString();
                    }

                    Toast.makeText(FMplayService.this, info, Toast.LENGTH_LONG).show();

                    mNeedShutdown = true;

                    if(isFmOn()){
                        fmOff();
                    }
                }
            }
        }
    }

    BroadcastReceiver mAudioInteropReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            Log.d(LOGTAG, "intent received:" + intent);
            String tempIntent = intent.getAction();
            if ((FM_ALARM_ALERT.equals(tempIntent) || ACTION_FM_RING_PAUSE.equals(tempIntent))
                    && ismute) {
                ismute = false;
                mute();
            } else if (ACTION_FM_MUSIC_PAUSE.equals(tempIntent)) {
                if (isFmOn()) {// fix bug 15854 start
                    freq = getFreq();
                    isstopNotification = true;// For bug 91848
                    fmOff();
                    mPausedByTransientLossOfFocus = false;
                    if (FMPlay.routeDevices == 1) {
                        FMPlay.audioChannel = true;
                    }

                    FMPlay.sendMessage = false;
                }
            } else if ((FM_ALARM_DONE.equals(tempIntent) || ACTION_FM_RING_RESET.equals(tempIntent) || (FM_ALARM_SNOOZE
                    .equals(tempIntent)))
                    && !ismute) {
                ismute = true;
                unMute();
            } else if(ACTION_SOUNDRECORDER_PAUSE.equals(tempIntent)){
                if (isFmOn()) {// fix bug 15854 start
                    freq = getFreq();
                    isstopNotification = false;
                    fmOff();
                    mPausedByTransientLossOfFocus = false;
                    sounderRecordStop = true;
                }
            } else if(ACTION_CAMERA_PAUSE.equals(tempIntent)){
                if (isFmOn()) {// fix bug 15854 start
                    freq = getFreq();
                    isstopNotification = false;
                    fmOff();
                    mPausedByTransientLossOfFocus = false;
                    cameraStop = true;
                }
            } else if(ACTION_AUTOPREVIEW_START.equals(tempIntent)){
                if (isFmOn()) {// fix bug 15854 start
                    freq = getFreq();
                    isstopNotification = false;
                    fmOff();
                    mPausedByTransientLossOfFocus = false;
                    autopreviewStop = true;
                }// fix bug 1
            }
            else if ( ACTION_FM_VIDEO_PAUSE.equals(tempIntent)
                    || ACTION_SMS_START.equals(tempIntent) || ACTION_ATV_START.equals(tempIntent)
                    || ACTION_MBBM_START.equals(tempIntent)) {
                if (isFmOn()) {// fix bug 15854 start
                    freq = getFreq();
                    isstopNotification = false;
                    fmOff();
                    mPausedByTransientLossOfFocus = false;
                }// fix bug 15854 end
            } else if (ACTION_RADIO_PAUSE.equals(tempIntent)) {// mm 04 fix bug 6911
                if ("stop".equals(intent.getStringExtra("fm_stop"))) {
                    isstopNotification = false;// fix bug 15854
                    freq = getFreq();
                    fmOff();
                    mPausedByTransientLossOfFocus = false;
                    if (FMPlay.routeDevices == 1) {
                        FMPlay.audioChannel = true;
                    }
                }
            } else if(ACTION_SOUNDRECORDER_START.equals(tempIntent) && sounderRecordStop){
                if (!isFmOn() && isHeadsetExists()
                        && (System.getInt(getContentResolver(), System.AIRPLANE_MODE_ON, 0) == 0)) {
                    fmOn();
                    setFreq(freq);
                    sounderRecordStop = false;
                }
            } else if(ACTION_CAMERA_START.equals(tempIntent) && cameraStop){
                if (!isFmOn() && isHeadsetExists()
                        && (System.getInt(getContentResolver(), System.AIRPLANE_MODE_ON, 0) == 0)) {
                    fmOn();
                    setFreq(freq);
                    cameraStop = false;
                }
            }else if(ACTION_AUTOPREVIEW_PAUSE.equals(tempIntent) && autopreviewStop){
                if (!isFmOn() && isHeadsetExists()
                        && (System.getInt(getContentResolver(), System.AIRPLANE_MODE_ON, 0) == 0)) {
                    fmOn();
                    setFreq(freq);
                    autopreviewStop = false;
                }
            }
            else if (ACTION_SMS_STOP.equals(tempIntent)
                    || ACTION_FM_MUSIC_START.equals(tempIntent)) {
                if (!isFmOn() && isHeadsetExists()
                        && (System.getInt(getContentResolver(), System.AIRPLANE_MODE_ON, 0) == 0)) {
                    fmOn();
                    setFreq(freq);
                }
            } else if (ACTION_ATV_STOP.equals(tempIntent) || ACTION_MBBM_STOP.equals(tempIntent)) {
                if (!isFmOn() && isHeadsetExists()
                        && (System.getInt(getContentResolver(), System.AIRPLANE_MODE_ON, 0) == 0)) {
                    fmOn();
                    AudioManager mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
                    Log.d(LOGTAG, "mAudioManager.isFmActive() " + mAudioManager.isFmActive());
                    if (!mAudioManager.isFmActive()) {
                        Intent intentaudio = new Intent(Intent.ACTION_FM);
                        intentaudio.putExtra("FM", 0);
                        intentaudio.putExtra("state", 1);
                        intentaudio.putExtra("speaker", FMPlay.routeDevices);
                        Log.d(LOGTAG, "mAudioManager.isFmActive() " + intentaudio);
                        sendBroadcast(intentaudio);
                    }
                    setFreq(freq);
                    Log.d(LOGTAG, "mAudioManager.isFmActive() " + mAudioManager.isFmActive());
                }
            }
        }
    };

    private synchronized void refMute() {
        if (mMuteCount == 0) {
            mute();
        }
        ++mMuteCount;
        Log.d(LOGTAG, "(refMute)mMuteCount = " + mMuteCount);
    }

    private synchronized void refUnmute() {
        --mMuteCount;
        if (mMuteCount < 0) {
            mMuteCount = 0;
        }

        Log.d(LOGTAG, "(refUnmute)mMuteCount = " + mMuteCount);
        if (mMuteCount == 0) {
            unMute();
            setFreq(getFreq(), true);
        }
    }

    public void onDestroy() {
        Log.d(LOGTAG,"onDestroyFMplayService");
        if (isFmOn()) {
            mFmOn = false;
        }

        mFmManager.setAudioPath(FmAudioPath.FM_AUDIO_PATH_NONE);

        mFmManager.powerDown();
        if (mWakeLock.isHeld()) {
            Log.d(LOGTAG, "onDestroy release wakelock");
            mWakeLock.release();
        }

        if (mDelayedStopHandler != null) {
            mDelayedStopHandler.removeMessages(MSG_DELAYED_CLOSE);
            mDelayedStopHandler.removeCallbacksAndMessages(null);
            mDelayedStopHandler = null;
        }

        if (mVolumeHandler != null) {
            mVolumeHandler.removeMessages(MSG_SET_VOLUME);
            mVolumeHandler.removeCallbacksAndMessages(null);
            mVolumeHandler = null;
            mHandlerThread.quit();
            mHandlerThread =null;
        }

        unregisterReceiver(mMonitor);
        unregisterReceiver(mAudioInteropReceiver);

        //mm04 fix bug 3183
        Intent i = new Intent("com.android.fmservice.stopmusicservice");
        i.putExtra("playingfmservice", false);
        this.sendBroadcast(i);

        tmgr1.listen(mPhoneStateListener, 0);
        if (TelephonyManager.getPhoneCount()>1){
			if (tmgr2 != null) {
				tmgr2.listen(mPhoneStateListener,
						PhoneStateListener.LISTEN_NONE);
			}
        }
        super.onDestroy();
    }

    public void onStart(Intent intent, int startId) {
        Log.d(LOGTAG,"onStart");
        super.onStart(intent, startId);
        mServiceId = startId;
        mDelayedStopHandler.removeCallbacksAndMessages(null);
        Message msg = mDelayedStopHandler.obtainMessage(MSG_DELAYED_CLOSE);
        mDelayedStopHandler.sendMessageDelayed(msg, IDLE_DELAY);
    }

    public IBinder onBind(Intent intent) {
    	Log.d(LOGTAG,"onBind");
        mDelayedStopHandler.removeCallbacksAndMessages(null);
        return mBinder;
    }

    public void onRebind(Intent intent) {
    	Log.d(LOGTAG, "onRebind");
        mDelayedStopHandler.removeCallbacksAndMessages(null);
    }

    public boolean onUnbind(Intent intent) {
    	Log.d(LOGTAG, "onUnbind");
        if (isFmOn()) {
            return true;
        }

        stopSelf(mServiceId);
        return true;
    }

    private Handler mDelayedStopHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (isFmOn() || mServiceInUse) {
                return;
            }

            stopSelf(mServiceId);
        }
    };

    public boolean fmOn() {
        mNeedShutdown = false;

        isstopNotification = true;

        AudioManager am = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
        Log.d(LOGTAG,"mAudioManager.isFmActive() "+am.isFmActive());

        if(!am.isFmActive()){
            mFmManager.setAudioPath((FMPlay.routeDevices == 1) ? FmAudioPath.FM_AUDIO_PATH_SPEAKER : FmAudioPath.FM_AUDIO_PATH_HEADSET);
        }

	    while (am.requestAudioFocus(mAudioFocusListener, AudioManager.STREAM_FM, AudioManager.AUDIOFOCUS_GAIN) == AudioManager.AUDIOFOCUS_REQUEST_FAILED) {
	        Log.d(LOGTAG, "fail to get AudioFocus");
	        try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
	    }
        boolean value = false;
        if (!isFmOn()) {
            value = mFmManager.powerUp();
            if (!mWakeLock.isHeld()) {
                Log.d(LOGTAG, "fmOn acquire wakelock");
                mWakeLock.acquire();
            }

            if (value) {
                startNotification();
                Log.d(LOGTAG, "turn on radio " + value);
            }
        } else {
            value = true;
        }

        if (!value) {
            am.abandonAudioFocus(mAudioFocusListener);
        } else {
            if ((am.requestAudioFocus(mAudioFocusListener, AudioManager.STREAM_FM, AudioManager.AUDIOFOCUS_GAIN) == AudioManager.AUDIOFOCUS_REQUEST_FAILED) || mNeedShutdown) {
                Log.d(LOGTAG, "close fm because fail to get AudioFocus or mNeedShutdown = " + mNeedShutdown);
                fmOff();
                value = false;
            }
        }

        return value;
    }

    protected void startNotification() {
    	Log.d(LOGTAG, "startNotification");
        views = new RemoteViews(getPackageName(), R.layout.status_bar);
        views.setImageViewResource(R.id.icon, R.drawable.fm_status);
        views.setTextViewText(R.id.appname, getResources().getString(R.string.app_name));
        views.setTextViewText(R.id.tunedname, FMPlaySharedPreferences.getStationName(0, freq)==null?
                freq +"MHZ":freq +"MHZ("+FMPlaySharedPreferences.getStationName(0, freq)+")");// For Bug 91906
        views.setTextViewText(R.id.comment, getResources().getString(R.string.comment));
        notice = new Notification();
        notice.contentView = views;
        notice.flags |= Notification.FLAG_ONGOING_EVENT;
        notice.icon = R.drawable.fm_status;
        notice.contentIntent = PendingIntent.getActivity(this,0,new Intent("com.thunderst.radio.RADIO").addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP), 0);
       //add for monkey test
        try {
        	if(notice.contentIntent != null && notice.contentView != null) { //add for bug 19553
        		startForeground(FM_STATUS, notice);
        	}
        	else {
        		Log.i("FMplayService", "=== log for bug 19553, Notification argument error! ===");
        		//return;
        	}
		} catch (IllegalArgumentException e) {
			// TODO: handle exception
			Log.v(LOGTAG,"catch the IllegalArgumentException "+e);
		}
        mFmOn = true;
        Intent intent2 = new Intent();
        intent2.setAction(ACTION_MUSIC_PAUSE);
        sendBroadcast(intent2);
        // mm04 2670
        setVolume();

    }

    private void setVolume(){
    	AudioManager am = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
        int volume = am.getStreamVolume(AudioManager.STREAM_FM);
        setVolume(volume);
    }

    protected void stopNotification() {
    	Log.d(LOGTAG, "stopNotification"+isstopNotification);
    	//mm04 fix bug 3778
    	if(isstopNotification && mDelayedStopHandler!=null){
    		NotificationManager nm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
    		nm.cancel(FM_STATUS);
    		stopForeground(true);
    		mDelayedStopHandler.removeCallbacksAndMessages(null);
    		Message msg = mDelayedStopHandler.obtainMessage(MSG_DELAYED_CLOSE);
    		mDelayedStopHandler.sendMessageDelayed(msg, IDLE_DELAY);
    		isstopNotification = false;
    	}else{   //fix bug 15854
    		isstopNotification = true;
    	}
        mFmOn = false;
    }

    public boolean fmOff() {
    	Log.d(LOGTAG, "fmOff");
        mNeedShutdown = false;

        AudioManager am = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
        Log.d(LOGTAG,"mAudioManager.isFmActive() "+am.isFmActive());

        if(am.isFmActive()){
            mFmManager.setAudioPath(FmAudioPath.FM_AUDIO_PATH_NONE);
        }
        //mm04 fix bug 3252
        mFmManager.powerDown();
        if (mWakeLock.isHeld()) {
            Log.d(LOGTAG, "fmOff release wakelock");
            mWakeLock.release();
        }
        stopNotification();
        am.abandonAudioFocus(mAudioFocusListener);

        return true;
    }

    public boolean isFmOn() {
        return mFmManager.isFmOn();
    }

    public boolean mute() {
        boolean value = false;
        if (isFmOn()) {
            value = mFmManager.mute();
            Log.d(LOGTAG, "mute" + value);
        }
        mIsMuted = value;

        return value;
    }

    public boolean unMute() {
        boolean value = false;
        if (isFmOn()) {
            value = mFmManager.unmute();
            Log.d(LOGTAG, "unmute" + value);
        }
        mIsMuted = !value;

        return value;
    }

    public boolean isMuted() {
        return mIsMuted;
    }

    public boolean setFreq(float freq) {
    	Log.d(LOGTAG, "setFreq " + freq);
        boolean value = false;
        if (mIsTuned) {
            value = setFreq(freq, false);
        } else {
            value = setFreq(freq, true);
            if (value) {
                mIsTuned = true;
            }
        }

        return value;
    }

    protected boolean setFreq(float freq, boolean force) {
        boolean value = false;

        if (isFmOn()) {
            if (force || freq != getFreq()) {
                value = mFmManager.setFreq((int)(freq * TRANS_MULT));
                Log.d(LOGTAG, "tune frequency " + freq);
            }
        }

        return value;
    }

    public boolean startSeek(boolean up) {
        boolean value = false;
        if (isFmOn()) {
            if (up) {
                value = mFmManager.startSearch((int)(this.getFreq()*TRANS_MULT),
                        FmSearchDirection.FM_SEARCH_UP, FM_SEARCH_TIMEOUT);
                Log.d(LOGTAG, "up search " + value);
            } else {
                value = mFmManager.startSearch((int)(this.getFreq()*TRANS_MULT),
                        FmSearchDirection.FM_SEARCH_DOWN, FM_SEARCH_TIMEOUT);
                Log.d(LOGTAG, "down search " + value);
            }
        }
        return value;
    }

    public boolean stopSeek() {
        boolean value = false;
        if (isFmOn()) {
            value = mFmManager.cancelSearch();
        }

        return value;
    }
    
    public float getFreq() {
        float freq = -1.0f;
        if (isFmOn()) {
            int iFreq = mFmManager.getFreq();
            Log.d(LOGTAG, "get frequency " + iFreq);
            if (iFreq > 0) {
                freq = (float) iFreq / TRANS_MULT;
            }
        }
        return freq;
    }

    public float getFreq2(boolean issech) {//mm04 fix bug 6402
        float freq = -1.0f;
        if (isFmOn()) {
            int iFreq = mFmManager.getFreq();
            Log.d(LOGTAG, "getFreq2 frequency " + iFreq);
            if (iFreq > 0) {
                freq = (float) iFreq / TRANS_MULT;
                if(views!=null && notice!=null){    //mm04 fix bug 5871
                	views.setTextViewText(R.id.tunedname, FMPlaySharedPreferences.getStationName(0, freq)==null?
                			+freq+"MHZ":freq+"MHZ("+FMPlaySharedPreferences.getStationName(0, freq)+")");
                	startForeground(FM_STATUS, notice);
                }else{
                	if(views!=null){Log.d(LOGTAG, "Notification views is null");}
                	if(notice!=null){Log.d(LOGTAG, "Notification views is null");}
                }
            }
        }
        return freq;
    }

    public boolean routeAudio(int device) {
        Log.d(LOGTAG, "begin to route audio:" + device);

        boolean value = false;
        if (isFmOn()) {
            mAudioDevice = device;
            if (mAudioDevice == RADIO_AUDIO_DEVICE_WIRED_HEADSET) {
                value = mFmManager.setAudioPath(FmAudioPath.FM_AUDIO_PATH_HEADSET);
            } else {
                value = mFmManager.setAudioPath(FmAudioPath.FM_AUDIO_PATH_SPEAKER);
            }
         // mm04 2670
            setVolume();
        }

        Log.d(LOGTAG, "end to route audio " + device);

        return value;
    }

    public boolean setVolume(int volume) {
        boolean value = false;
        if (isFmOn()) {
            value = mFmManager.setVolume(volume);
            Log.d(LOGTAG, "set volume" + volume);
        }

        return value;
    }

    public int getAudioDevice() {
        return mAudioDevice;
    }

    static class ServiceStub extends IRadioService.Stub {
        WeakReference<FMplayService> mService;

        public ServiceStub(FMplayService service) {
            mService = new WeakReference<FMplayService>(service);
        }

        public boolean fmOn() throws RemoteException {
            if(mService != null){
                if(mService.get() != null){
                    return mService.get().fmOn();
                }
            }
            return false;
        }

        public boolean fmOff() throws RemoteException {
            if(mService != null){
                if(mService.get() != null){
                    return mService.get().fmOff();
                }
            }
            return false;
        }

        public boolean isFmOn() throws RemoteException {
            if(mService != null){
                if(mService.get() != null){
                    return mService.get().isFmOn();
                }
            }
            return false;
        }

        public boolean mute() throws RemoteException {
            return mService.get().mute();
        }

        public boolean unMute() throws RemoteException {
            return mService.get().unMute();
        }

        public boolean isMuted() throws RemoteException {
            return mService.get().isMuted();
        }

        public boolean setFreq(float freq) throws RemoteException {
            if(mService != null){
                if (mService.get() != null) {
                    return mService.get().setFreq(freq);
                }
            }
            return false;
        }

        public boolean startSeek(boolean up) throws RemoteException {
            return mService.get().startSeek(up);
        }

        public boolean stopSeek() throws RemoteException {
            return mService.get().stopSeek();
        }

        public float getFreq() throws RemoteException {
            if(mService != null){
                if (mService.get() != null) {
                    return mService.get().getFreq();
                }
            }
            return FMPlaySharedPreferences.getTunedFreq();
        }

        public float getFreq2(boolean issech) throws RemoteException {
            if(mService != null){
                if(mService.get() != null){
                    return mService.get().getFreq2(issech);
                }
            }
            return FMPlaySharedPreferences.getTunedFreq();
        }

        public boolean routeAudio(int device) {
            return mService.get().routeAudio(device);
        }

        public boolean setVolume(int volume) {
            return mService.get().setVolume(volume);
        }

        public int getAudioDevice() {
            return mService.get().getAudioDevice();
        }
    }
}
