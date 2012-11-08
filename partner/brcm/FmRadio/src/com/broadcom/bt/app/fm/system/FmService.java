package com.broadcom.bt.app.fm.system;

import java.lang.ref.WeakReference;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.os.IBinder;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.util.Log;
import android.os.PowerManager;
import com.broadcom.bt.app.fm.R;
import com.broadcom.bt.app.fm.rx.FmRadio;
import com.broadcom.bt.app.fm.rx.FmRadioSettings;
import com.broadcom.bt.service.fm.FmReceiver;
import com.broadcom.bt.service.fm.FmReceiverService;
import com.broadcom.bt.service.fm.FmServiceManager;
import com.broadcom.bt.service.fm.IFmReceiverCallback;
import com.broadcom.bt.service.fm.IFmReceiverService;
import java.io.FileReader;
import com.android.internal.telephony.ITelephony;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.os.ServiceManager;
import android.os.Handler;
import android.os.Message;

public class FmService extends Service {

    public static String TAG = "FmService";

    public static final String ACTION_RADIO_FORCEQUIT = "android.fm.service.QUIT";
    // fix bug 4718 start
    // recovery from state without audio focus
    public static final String ACTION_RADIO_RESTORE = "android.fm.service.RESTORE";
    // fix bug 4718 end
    public static final String EXTRA_QUIT_REASON = "reason";
    public static final int QUIT_REASON_AIRPLANE = 1;

    private boolean mIsFmOn = false;

    private IBinder mBinder = null;
    private FmReceiverService mSvc = (FmReceiverService) FmServiceManager
            .getService(FmReceiver.SERVICE_NAME);
    private HeadsetPlugUnplugBroadcastReceiver mHeadsetPlugUnplugBroadcastReceiver;
    private AudioManager mAudioManager = null;
    private int mCurrentAudioPath = -1;
    private boolean mRetryAudioFocus = false;
    private final int DELAY_TIME = 600;
    private final int RETRY_COUNT_MAX = 15;
    private int mRetryCount = 0;

    private void retryAudioFocus() {
        if (mHandler != null) {
            ++mRetryCount;
            mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_RETRY_AUDIO_FOCUS), DELAY_TIME);
        }
    }

    private final PhoneStateListener mPhoneStateListener = new PhoneStateListener() {
        public void onCallStateChanged(int state, String incomingNumber) {
            if (!isPhoneInUse() && mRetryAudioFocus) {
                retryAudioFocus();
            }
        }
    };

    private final int MSG_RETRY_AUDIO_FOCUS = 1;

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            removeMessages(msg.what);
            switch(msg.what) {
                case MSG_RETRY_AUDIO_FOCUS: {
                    if (requestAudioFocus()) {
                        updateCurrentMuteAudio();
                        updateCurrentAudioPath();
                        mRetryAudioFocus = false;
                        mRetryCount = 0;
                    } else {
                        if (mRetryCount < RETRY_COUNT_MAX) {
                            ++mRetryCount;
                            sendMessageDelayed(obtainMessage(MSG_RETRY_AUDIO_FOCUS), DELAY_TIME);
                        } else {
                            mRetryAudioFocus = false;
                            mRetryCount = 0;
                            Log.w(TAG, "failed to requet audio focus. It may be error in phone");
                        }
                    }
                }
                break;
                default:
            }
        }
    };
   // PowerManager.WakeLock mFmWakeLock = null;
    private int getPersistedAudioPath() {
        SharedPreferences mSharedPrefs = PreferenceManager
                .getDefaultSharedPreferences(FmService.this);
        int currAudioPath = Integer.parseInt(mSharedPrefs.getString(
                FmRadioSettings.FM_PREF_AUDIO_PATH,
                String.valueOf(FmReceiver.AUDIO_PATH_WIRE_HEADSET)));
        return currAudioPath;
    }

    private void updateCurrentAudioPath() {
        updateAudioPath(getPersistedAudioPath());
    }

    private void updateCurrentMuteAudio() {
        try {
            if (mIsMutedByAudioFocus) {
                muteAudio(mIsMutedPrevious, true);
                mIsMutedByAudioFocus = false;
                mIsMutedPrevious = false;
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    private boolean isPhoneInUse() {
        boolean isPhoneInUse = false;

        try {
            ITelephony phone = ITelephony.Stub.asInterface(ServiceManager.checkService("phone"));
            if (phone != null) {
                isPhoneInUse = !phone.isIdle();
            }
        } catch (RemoteException e) {
            Log.w(TAG, "phone.isIdle() failed", e);
        }

        return isPhoneInUse;
    }

    private boolean isInCallMode() {
        boolean isInCallMode = false;

        if (mAudioManager != null) {
            int mode = mAudioManager.getMode();
            if (mode == AudioManager.MODE_RINGTONE ||
                mode == AudioManager.MODE_IN_CALL ||
                mode == AudioManager.MODE_IN_COMMUNICATION) {
                isInCallMode = true;
            }
        }

        return isInCallMode;
    }

    public class HeadsetPlugUnplugBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equalsIgnoreCase(Intent.ACTION_HEADSET_PLUG)) {
                if (!mIsFmOn)
                    return;

                int state = intent.getIntExtra("state", 0);
                if (state == 0) {
                    // set audio path is none
                    abandonAudioFocus();
                    updateAudioPath(FmReceiver.AUDIO_PATH_NONE);
                } else {
                    // set audio path
                    if (requestAudioFocus()) {
                        updateCurrentMuteAudio();
                        updateCurrentAudioPath();
                    } else {
                        if (isInCallMode()) {
                            mRetryAudioFocus = true;
                            if (!isPhoneInUse()) {
                                retryAudioFocus();
                            }
                        }
                    }
                }
            } else if (intent.getAction().equalsIgnoreCase(
                    AudioManager.ACTION_AUDIO_BECOMING_NOISY)) {
                abandonAudioFocus();
                updateAudioPath(FmReceiver.AUDIO_PATH_NONE);

            } else if (intent.getAction().equals(
                    AudioManager.VOLUME_CHANGED_ACTION)) {
                if (intent.hasExtra(AudioManager.EXTRA_VOLUME_STREAM_TYPE)
                        && intent
                                .hasExtra(AudioManager.EXTRA_VOLUME_STREAM_VALUE)) {
                    int streamType = intent.getIntExtra(
                            AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
                    if (streamType == AudioManager.STREAM_FM) {
                        final int value = intent.getIntExtra(
                                AudioManager.EXTRA_VOLUME_STREAM_VALUE, -1);
                        if (value != -1) {
                            Log.d(TAG, "stream type " + streamType + " value "
                                    + value);
                            try {
                                setFMVolume(FmReceiver
                                        .convertVolumeFromSystemToBsp(
                                                value,
                                                mAudioManager
                                                        .getStreamMaxVolume(AudioManager.STREAM_FM)));
                            } catch (RemoteException e) {
                                Log.e(TAG, "set FM Volume value " + value
                                        + "error! " + e);
                            }
                        }
                    }
                }
            } else if (intent.getAction().equals(Intent.ACTION_AIRPLANE_MODE_CHANGED)) {
                boolean isAirplane = Settings.System.getInt(getContentResolver(),
                        Settings.System.AIRPLANE_MODE_ON, 0) == 1;
                if (isAirplane) {
                    try {
                        abandonAudioFocus();
                        setAudioPath(FmReceiver.AUDIO_PATH_NONE);
                        try {
                            turnOffRadio();
                        } catch (Exception e) {
                            
                        }
                        broadcastQuit(QUIT_REASON_AIRPLANE);
                    } catch (RemoteException e) {
                    }
                }
            } else if (intent.getAction().equals(ACTION_RADIO_RESTORE)) {
                if (!mRetryAudioFocus) {
                    updateCurrentMuteAudio();
                    updateCurrentAudioPath();
                    Log.d(TAG, "recovery");
                }
            } else if (intent.getAction().equals(Intent.ACTION_SHUTDOWN)) {
                try {
                    abandonAudioFocus();
                    setAudioPath(FmReceiver.AUDIO_PATH_NONE);

                    try {
                        turnOffRadio();
                    } catch (Exception e) {
                    }
                } catch (RemoteException e) {
                }
            }
        }
    }

    private void broadcastQuit(int reason) {
        Intent intent = new Intent(ACTION_RADIO_FORCEQUIT);
        intent.putExtra(EXTRA_QUIT_REASON, reason);
        sendBroadcast(intent);
    }

	@Override
	public IBinder onBind(Intent intent) {
		Log.d(TAG, "onBindService mSvc=" + mSvc);
		if(mBinder == null){
			mBinder = new FmServiceStub(this);
		}
		return mBinder;
	}

    public void init() {}

    @Override
	public void onCreate() {
    	Log.d(TAG, "onCreate");
		super.onCreate();
	      if (mHeadsetPlugUnplugBroadcastReceiver == null){
	    	  mHeadsetPlugUnplugBroadcastReceiver = new HeadsetPlugUnplugBroadcastReceiver();
	      }
		  IntentFilter filter = new IntentFilter();
		  filter.addAction(Intent.ACTION_HEADSET_PLUG);
//		  filter.addAction(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
		  filter.addAction(AudioManager.VOLUME_CHANGED_ACTION);
		  filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
          filter.addAction(ACTION_RADIO_RESTORE);
          filter.addAction(Intent.ACTION_SHUTDOWN);
		  registerReceiver(mHeadsetPlugUnplugBroadcastReceiver,filter);
		mAudioManager = ((AudioManager) getSystemService(Context.AUDIO_SERVICE));
        TelephonyManager telephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        telephonyManager.listen(mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		Log.d(TAG, "onDestroy");
		abandonAudioFocus();
		unregisterReceiver(mHeadsetPlugUnplugBroadcastReceiver);
		mHeadsetPlugUnplugBroadcastReceiver = null;
        TelephonyManager telephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        telephonyManager.listen(mPhoneStateListener, PhoneStateListener.LISTEN_NONE);
    	mBinder = null;
    	mSvc.finish();
	}

	private int updateAudioPath(int audioPath){
		int rs = FmReceiver.STATUS_OK;
		try {
			rs = setAudioPath(audioPath);
		} catch (RemoteException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return rs;
	}

	public void registerCallback(IFmReceiverCallback cb) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        mSvc.registerCallback(cb);
    }

    public void unregisterCallback(IFmReceiverCallback cb) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        mSvc.unregisterCallback(cb);
    }

    public int estimateNoiseFloorLevel(int nflLevel) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.estimateNoiseFloorLevel(nflLevel);
    }

    public boolean getRadioIsOn() throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.getRadioIsOn();
    }

    public int getMonoStereoMode() throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.getMonoStereoMode();
    }

    public int getTunedFrequency() throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.getTunedFrequency();
    }

    public boolean getIsMute() throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.getIsMute();
    }

    public int getStatus() throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.getStatus();
    }

    public int muteAudio(boolean mute) throws RemoteException {
        return muteAudio(mute, true);
    }

    public int muteAudio(boolean mute, boolean audioFocus) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();

        if (audioFocus && !mute && !requestAudioFocus()) {
            return FmReceiver.STATUS_AUDIO_FUCOS_FAILED;
        }

        int retryCount = 3;
        int ret = FmReceiver.STATUS_FAIL;
        while((ret = mSvc.muteAudio(mute)) != FmReceiver.STATUS_OK
                && retryCount > 0) {
            try {
                Thread.sleep(ret == FmReceiver.STATUS_STARTING ? 300 : 100);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            retryCount--;
        }

        if (audioFocus && !mute && ret != FmReceiver.STATUS_OK) {
            abandonAudioFocus();
        }

        if (audioFocus && ret == FmReceiver.STATUS_OK && mute) {
            abandonAudioFocus();
        }
        if (ret != FmReceiver.STATUS_OK) {
            Log.e(TAG, "error muteaudio :" + ret);
        }
        return ret;
    }

    public int seekRdsStation(int scanDirection, int minSignalStrength, int rdsCondition, int rdsValue)
            throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.seekRdsStation(scanDirection, minSignalStrength, rdsCondition, rdsValue);
    }

    public int seekStation(int scanDirection, int minSignalStrength) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.seekStation(scanDirection, minSignalStrength);
    }

    public int seekStationCombo(int startFrequency, int endFrequency, int minSignalStrength, int scanDirection, int scanMethod,
		boolean multi_channel, int rdsType, int rdsTypeValue)
		throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        
        int status = mSvc.seekStationCombo (startFrequency, endFrequency, minSignalStrength, scanDirection, scanMethod, multi_channel, rdsType, rdsType);
        
        return status;
    }

    public int seekStationAbort() throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.seekStationAbort();
    }

    public int setAudioMode(int audioMode) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.setAudioMode(audioMode);
    }

    public int setAudioPath(int audioPath) throws RemoteException {
        return setAudioPath(audioPath, false);
    }

    public int setAudioPath(int audioPath, boolean audioFocus) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();

        if (audioFocus && audioPath != FmReceiver.AUDIO_PATH_NONE && !requestAudioFocus()) {
            return FmReceiver.STATUS_AUDIO_FUCOS_FAILED;
        }

/*        if (audioPath == mCurrentAudioPath)
            return FmReceiver.STATUS_OK;*/

        if (audioPath != FmReceiver.AUDIO_PATH_NONE && !isHeadsetPlugged()) {
            Log.e(TAG, "setAudioPath() headset unplugged");
            return FmReceiver.STATUS_FAIL;
        }

        int retryCount = 3;
        int ret = FmReceiver.STATUS_FAIL;
        while((ret = mSvc.setAudioPath(audioPath)) != FmReceiver.STATUS_OK
                && retryCount > 0) {
            try {
                Thread.sleep(150);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            retryCount--;
        }

        if (audioFocus && ret != FmReceiver.STATUS_OK && audioPath != FmReceiver.AUDIO_PATH_NONE) {
            abandonAudioFocus();
        }

        if (audioFocus && ret == FmReceiver.STATUS_OK && audioPath == FmReceiver.AUDIO_PATH_NONE) {
            abandonAudioFocus();
        }

        if (ret == FmReceiver.STATUS_OK) {
            mCurrentAudioPath = audioPath;
            audioPathChangedByAudioFocus = false;
        }

        return ret;
    }

    public int setFMVolume(int volume) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.setFMVolume(volume);
    }

    public int setLiveAudioPolling(boolean liveAudioPolling, int signalPollInterval) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.setLiveAudioPolling(liveAudioPolling, signalPollInterval);
    }

    public int setSNRThreshold( int snrThreshold) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.setSNRThreshold(snrThreshold);
    }

    public int setRdsMode(int rdsMode, int rdsFeatures, int afMode, int afThreshold) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.setRdsMode(rdsMode, rdsFeatures, afMode, afThreshold);
    }

    public int setStepSize(int stepSize) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.setStepSize(stepSize);
    }

    public int setWorldRegion(int worldRegion, int deemphasisTime) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        return mSvc.setWorldRegion(worldRegion, deemphasisTime);
    }

    public int tuneRadio(int freq) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        int ret = mSvc.tuneRadio(freq);
        if (ret == FmReceiver.STATUS_OK)
            showNotification(this,freq);
        return ret;
    }

    public int turnOffRadio() throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();

        int retryCount = 3;
        int ret = FmReceiver.STATUS_FAIL;
        while((ret = mSvc.turnOffRadio()) != FmReceiver.STATUS_OK
                && retryCount > 0) {
            try {
                Thread.sleep(150);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            retryCount--;
        }

        if (ret == FmReceiver.STATUS_OK || isInAirplaneMode()) {
            abandonAudioFocus();
            cancelNotification(this);
            mIsFmOn = false;
        }
       // if (mFmWakeLock != null && mFmWakeLock.isHeld())
       //     mFmWakeLock.release();
        return ret;
    }

    private boolean isInAirplaneMode() {
        // check whether the current for airplane mode
        return Settings.System.getInt(getContentResolver(),Settings.System.AIRPLANE_MODE_ON,0) == 1;
    }
    public int turnOnRadio(int functionalityMask) throws RemoteException {
        if (mSvc == null)
            throw new RemoteException();
        if (isInAirplaneMode())
            return FmReceiver.STATUS_AIREPLANE_MODE;

        if (!requestAudioFocus())
            return FmReceiver.STATUS_AUDIO_FUCOS_FAILED;
       // if(mFmWakeLock == null){
       //     PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
       //     mFmWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "FmService");
       //}
       // if (!mFmWakeLock.isHeld())
       //    mFmWakeLock.acquire();
        int ret = mSvc.turnOnRadio(functionalityMask);

        if (ret == FmReceiver.STATUS_OK) {
            showNotification(this,0);
            mIsFmOn = true;
            setForeground(true);
        } else {
            abandonAudioFocus();
        }

        return ret;
    }

    OnAudioFocusChangeListener audioFocusChangeListener = null;

    private boolean audioPathChangedByAudioFocus = false;

    // fix bug 4718 start
    private boolean mIsMutedByAudioFocus = false;

    private boolean mIsMutedPrevious = false;
    // fix bug 4718 end

    public static boolean isHeadsetPlugged() {
        final String HEADSET_STATE_PATH = "/sys/class/switch/h2w/state";

        int state = 0;

        FileReader file = null;
        try {
            char[] buffer = new char[1024];
            file = new FileReader(HEADSET_STATE_PATH);
            int length = file.read(buffer, 0, 1024);
            state = Integer.valueOf((new String(buffer, 0, length)).trim());
        } catch (Exception e) {
            Log.e(TAG, "failed to get headset state");
        } finally {
            if (file != null) {
                try {
                    file.close();
                } catch (Exception e) {
                    Log.e(TAG, "failed to close file in isHeadsetPlugged()");
                }
                file = null;
            }
        }

        return (state != 0);
    }

    private synchronized boolean requestAudioFocus() {
        if(audioFocusChangeListener == null){
            audioFocusChangeListener = new AudioManager.OnAudioFocusChangeListener() {
                @Override
                public void onAudioFocusChange(int focusChange) {
                    Log.d(TAG, "AudioFocusChanged " + focusChange);
                    try {
                        mIsMutedPrevious = getIsMute();
                        mIsMutedByAudioFocus = (focusChange < 0);
                        muteAudio(focusChange < 0, false);
                        if (focusChange < 0) {
                            updateAudioPath(FmReceiver.AUDIO_PATH_NONE);
                            audioPathChangedByAudioFocus = true;
                            //fixed in searching stations ,fm interrupted by phone. after hung up the phone and fm finishes searching, there is no fm sound.
                            mCurrentAudioPath = -1;
                        } else {
                            updateCurrentAudioPath();
                        }
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                }
            };
        }
        boolean success = mAudioManager.requestAudioFocus(audioFocusChangeListener,
                AudioManager.STREAM_VOICE_CALL | AudioManager.STREAM_SYSTEM
                        | AudioManager.STREAM_RING | AudioManager.STREAM_MUSIC
                        | AudioManager.STREAM_ALARM
                        | AudioManager.STREAM_NOTIFICATION
                        | AudioManager.STREAM_DTMF,
                AudioManager.AUDIOFOCUS_GAIN) == AudioManager.AUDIOFOCUS_GAIN;

        if (success && audioPathChangedByAudioFocus) { // lost of audio focus may changed the audio path, it's my duty to restore it
            int persistedAudioPath = getPersistedAudioPath();
            if (mCurrentAudioPath != persistedAudioPath)
                updateAudioPath(persistedAudioPath);
        }
        return success;
    }

    private synchronized void abandonAudioFocus(){
        if (audioFocusChangeListener != null) {
            mAudioManager.abandonAudioFocus(audioFocusChangeListener);
            audioFocusChangeListener = null;
        }
    }

    public static void showNotification(Context context, int freq) {
        int icon = R.drawable.fm_status;
        String tickerText = String.format(context.getString(R.string.app_name)+" (%.01f MHz)", ((double)freq)/100);
        long when = System.currentTimeMillis();
        Notification notification = new Notification(icon, tickerText, when);

//        Context context = getApplicationContext();
        String contentTitle = context.getString(R.string.app_name);
        String contentText = String.format("%.01f MHz", ((double)freq)/100);
        Intent notificationIntent = new Intent(context, FmRadio.class);
        PendingIntent contentIntent = PendingIntent.getActivity(context, 0, notificationIntent, 0);

        notification.setLatestEventInfo(context, contentTitle, contentText, contentIntent);
        notification.flags |= Notification.FLAG_ONGOING_EVENT | Notification.FLAG_NO_CLEAR;

        final int PLAYING_ID = 1;
        NotificationManager nm = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        nm.notify(PLAYING_ID, notification);
    }

    public static void cancelNotification(Context context) {
        NotificationManager nm = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        nm.cancel(1);
    }

	public static class FmServiceStub extends IFmReceiverService.Stub {

		WeakReference<FmService> mService = null;

		public FmServiceStub(FmService service) {
			mService = new WeakReference<FmService>(service);
		}

		@Override
		public void init() throws RemoteException {
			mService.get().init();

		}

		@Override
		public void registerCallback(IFmReceiverCallback cb)
				throws RemoteException {
			mService.get().registerCallback(cb);
		}

		@Override
		public void unregisterCallback(IFmReceiverCallback cb)
				throws RemoteException {
			mService.get().unregisterCallback(cb);

		}

		@Override
		public boolean getRadioIsOn() throws RemoteException {
			return mService.get().getRadioIsOn();
		}

		@Override
		public int getMonoStereoMode() throws RemoteException {
			return mService.get().getMonoStereoMode();
		}

		@Override
		public int getTunedFrequency() throws RemoteException {
			return mService.get().getTunedFrequency();
		}

		@Override
		public boolean getIsMute() throws RemoteException {
			return mService.get().getIsMute();
		}

		@Override
		public int turnOnRadio(int functionalityMask) throws RemoteException {
			return mService.get().turnOnRadio(functionalityMask);
		}

		@Override
		public int turnOffRadio() throws RemoteException {
			return mService.get().turnOffRadio();
		}

		@Override
		public int tuneRadio(int freq) throws RemoteException {
			return mService.get().tuneRadio(freq);
		}

		@Override
		public int getStatus() throws RemoteException {
			return mService.get().getStatus();
		}

		@Override
		public int muteAudio(boolean mute) throws RemoteException {
			return mService.get().muteAudio(mute);
		}

		@Override
		public int seekStation(int scanDirection, int minSignalStrength)
				throws RemoteException {
			return mService.get().seekStation(scanDirection, minSignalStrength);
		}

		@Override
		public int seekStationCombo(int startFreq, int endFreq,
				int minSignalStrength, int dir, int scanMethod,
				boolean multiChannel, int rdsType, int rdsTypeValue)
				throws RemoteException {
			return mService.get().seekStationCombo(startFreq, endFreq, minSignalStrength, dir, scanMethod, multiChannel, rdsType, rdsTypeValue);
		}

		@Override
		public int seekRdsStation(int scanDirection, int minSignalStrength,
				int rdsCondition, int rdsValue) throws RemoteException {
			return mService.get().seekRdsStation(scanDirection, minSignalStrength, rdsCondition, rdsValue);
		}

		@Override
		public int seekStationAbort() throws RemoteException {
			return mService.get().seekStationAbort();
		}

		@Override
		public int setRdsMode(int rdsMode, int rdsFeatures, int afMode,
				int afThreshold) throws RemoteException {
			return mService.get().setRdsMode(rdsMode, rdsFeatures, afMode, afThreshold);
		}

		@Override
		public int setAudioMode(int audioMode) throws RemoteException {
			return mService.get().setAudioMode(audioMode);
		}

		@Override
		public int setAudioPath(int audioPath) throws RemoteException {
			return mService.get().setAudioPath(audioPath);
		}

		@Override
		public int setStepSize(int stepSize) throws RemoteException {
			return mService.get().setStepSize(stepSize);
		}

		@Override
		public int setWorldRegion(int worldRegion, int deemphasisTime)
				throws RemoteException {
			return mService.get().setWorldRegion(worldRegion, deemphasisTime);
		}

		@Override
		public int estimateNoiseFloorLevel(int nflLevel) throws RemoteException {
			return mService.get().estimateNoiseFloorLevel(nflLevel);
		}

		@Override
		public int setLiveAudioPolling(boolean liveAudioPolling,
				int signalPollInterval) throws RemoteException {
			return mService.get().setLiveAudioPolling(liveAudioPolling, signalPollInterval);
		}

		@Override
		public int setFMVolume(int volume) throws RemoteException {
			return mService.get().setFMVolume(volume);
		}

		@Override
		public int setSNRThreshold(int snrThreshold) throws RemoteException {
			return mService.get().setSNRThreshold(snrThreshold);
		}

	}
}

