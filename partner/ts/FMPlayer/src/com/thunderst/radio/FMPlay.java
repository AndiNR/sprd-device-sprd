/*

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.os.SystemProperties;

import android.R.bool;
import android.R.string;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.res.Resources;
import android.database.Cursor;
import android.media.AudioManager;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.provider.MediaStore;
import android.provider.Settings.System;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.text.Editable;
import android.text.InputFilter;
import android.text.Selection;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;


import com.thunderst.radio.CheckableImageButton.OnCheckedChangedListener;
import com.thunderst.radio.Recorder.OnStateChangedListener;
import com.thunderst.radio.TuneWheel.OnTuneWheelValueChangedListener;

public class FMPlay extends Activity
                   implements ServiceConnection
                            , OnCheckedChangedListener
                            , OnClickListener
                            , OnTuneWheelValueChangedListener {

    private static final String LOGTAG = "FMPlay";
    private static final String KEY_CLOSE_REASON = "reason";
    private static final String KEY_FORCE_CLOSE = "force";
    private static final String KEY_MANUAL_SEEK_RESULT = "result";
    private static final String KEY_MANUAL_SEEK_FREQ = "freq";
    private static final String HEADSET_UEVENT_MATCH = "DEVPATH=/devices/virtual/switch/h2w";
    private static final String HEADSET_STATE_PATH = "/sys/class/switch/h2w/state";
    private static final String HEADSET_NAME_PATH = "/sys/class/switch/h2w/name";

    private static final int NO_DIALOG = -1;
    private static final int AIRPLANE_DIALOG = 0;
    private static final int PHONE_DIALOG = 1;
    private static final int HEADSET_DIALOG = 2;
    private static final int BLUETOOTH_DIALOG = 3;
    private static final int STARTUP_DIALOG = 4;
    private static final int OPENERROR_DIALOG = 5;
    private static final int ADD_DIALOG = 6;
    private static final int SEARCH_DIALOG = 7;
    private static final int DIALOG_COUNT = 8;
    private static final int FREQ_POINTER_SCALE = 10;

    private static final int MSG_ROUTE = 0;
    private static final int MSG_CLOSE = 1;
    private static final int MSG_LISTEN = 2;
    private static final int MSG_UPDATE = 3;
    private static final int MSG_REFER = 4;
    private static final int MSG_TUNE = 5;
    private static final int MSG_OPEN = 6;
    private static final int MSG_PROGRESS = 7;
    private static final int MSG_MANUAL_SEEK_STARTED = 8;
    private static final int MSG_MANUAL_SEEK_FINISHED = 9;
    private static final int MSG_ABLED = 10;
    private static final int EVENT_START_RECODER = 11;

    private static final int MANUAL_SEEK_NEXT = 1;
    private static final int MANUAL_SEEK_PREV = -1;

    private static final float DEFAULT_FREQUENCY_SPAN = 2.0f;
    private static final float DEFAULT_FREQUENCY_INTERVAL = 0.1f;

    private static final int OPEN_DELAY = 200;
    private static final int CHECK_DELAY = 100;
    private static final int PROGRESS_DELAY = 100;
    private static final int MANUAL_SEEK_FAILED_DELAY = 1000;
    private static final int ANIMATION_DERATION = 2000;
    private static final int ANIMATION_TIME = 2000;
    private static final int TEXT_MAX_LENGTH = 128;
    private static final int MANUAL_SEEK_RSSI_LEVEL = 100;
    private boolean FROM_CURRENT_CHANNEL = false;

    private boolean ismPhoneStateListener = false;
    private int mInitialCallState;
    private CheckableImageButton mPowerToggle;
    private CheckableImageButton mHeadsetToggle;
    private DigitalTextView mDigitalClock;
    private TextView mClockType;
    private DigitalTextView mFreqView;
    private TuneWheel mTuneWheel;
    private ImageView mHeadsetIndicator;
    private View mFmIndicator;
    private View mSearchingIndicator;
    private View mLedBackground;
    private SeekBar mFreqPointer;
    private ImageButton mAddButton;
    private ImageButton mSearchNextButton;
    private ImageButton mSearchPrevButton;
    private ImageButton mNextStationButton;
    private ImageButton mPrevStationButton;
    private ImageButton mRadioListButton;

    private DigitalClockUpdater mDigitalClockUpdater;
    private RadioServiceStub mService;
    private Recorder m_recorder;
    private BroadcastReceiver mSDCardMountEventReceiver = null;
    private FMPlaySharedPreferences mStationList;

    public static int routeDevices; // add for bug 16904
    private int mColor;
    private ImageView mImage;

    private static final String ACTION_CAMERA_PAUSE = "com.android.camera.action.RECORD_VIDEO";
    private TranslateAnimation mAnimation;
    private  static Dialog[] mDialogs;
    private static  int mCurrentDialog;

    private Thread mManualSeekThread = null;

    private boolean addfrq = false;
    private boolean mPhone = true;

    public static boolean  audioChannel;
    private static boolean searchfreq = false;
    public static boolean sendMessage = false;
    private static boolean refresh = false;
    private boolean mIsSpeaker;
    private boolean mIsSpeaker2 = false;
    //mm04 fix bug
    private boolean listBoolean = false;
    //mm04 fix bug 3576
    private boolean onBack = false;
    private boolean mResult;
    private boolean mOver;
    private boolean mIsFromScratch;
    private boolean mPrevEnable;
    private boolean mNextEnable;
    private boolean mReady;

    private TelephonyManager tmgr1;
    private TelephonyManager tmgr2;

    private Thread mOpenThread = null;

    private String mClockTypeString;
    private static final String ACTION_AUTOPREVIEW_START = "com.android.audiopreview.start";
    private static final String ACTION_SMS_START = "com.android.mms.SMS_PLAY_START";
    private static final String ACTION_FM_VIDEO_PAUSE = "com.android.video.stopfmservice";
    private static final String ACTION_FM_MUSIC_PAUSE = "com.android.fm.shutdown";
    private static final String ACTION_RADIO_PAUSE = "com.android.music.musicservicecommand";

    private PhoneStateListener mPhoneStateListener = new PhoneStateListener() {
        @Override
        public void onCallStateChanged(int state, String incomingNumber) {
            Log.d(LOGTAG,"mPhoneStateListener");
            if(state == TelephonyManager.CALL_STATE_RINGING && mIsSpeaker){
                mIsSpeaker2 = true;
                }
            if ((state == TelephonyManager.CALL_STATE_RINGING || state == TelephonyManager.CALL_STATE_OFFHOOK ) && ismPhoneStateListener) {
                Log.d(LOGTAG,"onCallStateChanged");
                mPhone = false;
                onBack = false;
                if(routeDevices == 1){
                    audioChannel = true;
                    }
                setUiPower(false, false);
                //ismPhoneStateListener = false; For Bug 89303
                mIsSpeaker = false;
            }
        }
    };

    private BroadcastReceiver mAudioBroadcastReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent tempIntent) {
            if ((ACTION_AUTOPREVIEW_START.equals(tempIntent)||ACTION_SMS_START.equals(tempIntent)
                    ||ACTION_FM_MUSIC_PAUSE.equals(tempIntent)||ACTION_FM_VIDEO_PAUSE.equals(tempIntent)) && mIsSpeaker) {
                mIsSpeaker2 = true;
                mIsSpeaker = false;
                }
            }
    };
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(LOGTAG, "mReceiver: action="+action);
            if (action.equals(Intent.ACTION_FM)) {
                if (mService != null) {
                    if (intent.getIntExtra("state", 0) == 0 && mService.isFmOn()) {
                        setUiPower(false, false);
                        }
                }
            } else if (action.equals(FMplayService.ACTION_SHUTDOWN)) {
                finish();
            } else if (action.equals(Intent.ACTION_HEADSET_PLUG)) {
                if (!SystemProperties.getBoolean("ro.device.support.antenna", false)) {
                    if (intent.getIntExtra("state", 0) == 0) {
                        Log.d(LOGTAG, "mReady" + mReady + "listBoolean" + listBoolean);
                        if (mReady) { // mm04 fix bug 8001
                            setUiPower(false, false);
                            showAlertDialog(HEADSET_DIALOG);
                        } else if (listBoolean) {
                            setUiPower(false, false);
                        }
                    } else {
                        if (mCurrentDialog == HEADSET_DIALOG) {
                            closeAlertDialog(HEADSET_DIALOG);
                        }
                        mHandler.sendMessage(mHandler.obtainMessage(MSG_OPEN));
                        mHandler.sendMessage(mHandler.obtainMessage(MSG_REFER));
                    }
                }
            } else if ("android.intent.action.AlertReceiver".equals(action)){
                onBack = true;
            } else if (ACTION_CAMERA_PAUSE.equals(action)){ //mm04 fix bug 4723

            }else if (action.equals(Intent.ACTION_AIRPLANE_MODE_CHANGED)) {//mm04 fix bug 5040
                if (intent.hasExtra("state")) {
                    boolean enable = intent.getBooleanExtra("state", false);
                    if (enable) {
                        closeAlertDialog(SEARCH_DIALOG);
                        finish();
                    } else{
                        mPhone = true;
                        mHandler.sendEmptyMessage(MSG_OPEN);
                        closeAlertDialog(AIRPLANE_DIALOG);
                    }
                }
            }else if(action.equals("android.intent.action.UMS_CONNECTED")){
                sendMessage = true;
            }else if(action.equals("android.intent.action.TIME_SET")){
                refresh = true;
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
             file.close();
         } catch (FileNotFoundException e) {
             Log.e(LOGTAG, "This kernel does not have wired headset support");
         } catch (Exception e) {
             Log.e(LOGTAG, "" , e);
         }
         return newState != 0;
    }

    private Thread createOpenThread() {
        return new Thread(new Runnable() {
            public void run() {
                Log.d(LOGTAG, "tring open fm");
                mResult = mService.fmOn();
                Log.d(LOGTAG, "tring open fm result = " + mResult);
                mOver = true;
            }
       });
    }

    private Thread createManualSeekThread(int direction) {
        final int dir = direction;
        return new Thread(new Runnable(){
            public void run() {
                mHandler.sendMessage(mHandler.obtainMessage(MSG_MANUAL_SEEK_STARTED));
                Log.d(LOGTAG, "Start seek at " + mService.getFreq());
                boolean result = true;
                float freq = 0.0f;

                   Log.d(LOGTAG,"starting seek from" + freq);
                   result = mService.startSeek(dir == MANUAL_SEEK_NEXT);
                   try {
                       Thread.sleep(300);
                   } catch (InterruptedException e) {
                       // TODO Auto-generated catch block
                       e.printStackTrace();
                   }
                   freq = mService.getFreq();
                   result = (freq <= WheelConfig.RADIO_MAX_FREQUENCY
                           && freq >= WheelConfig.RADIO_MIN_FREQUENCY
                           && (result == true));

                Log.d(LOGTAG," seek finshed " + result + " freq found=" + freq);
                Message msg = mHandler.obtainMessage(MSG_MANUAL_SEEK_FINISHED);
                Bundle data = new Bundle();
                data.putBoolean(KEY_MANUAL_SEEK_RESULT, result);
                data.putFloat(KEY_MANUAL_SEEK_FREQ, WheelConfig.format(mService.getFreq()));
                msg.setData(data);
                mHandler.sendMessage(msg);
            }
        });
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
    }

    private boolean isFmOn(){
        boolean value = false;
        try {
            value = mService.isFmOn();
            } catch (NullPointerException e){
                Log.d(LOGTAG,"mService NullPointerException");
                mService = new RadioServiceStub(this, this);
                if (!mService.bindToService()) {
                    Log.d(LOGTAG, "resetMService fail to bindToService");
                    mService = null;
                    return true;
                    }
                isFmOn();
                Log.d(LOGTAG,"mService ok");
                }  catch (Exception e) {
                    // TODO Auto-generated catch block
                    Log.d(LOGTAG,"mService Exception");
                    e.printStackTrace();
                    return true;
                    }
                return value;
    }

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            Log.d(LOGTAG, "msg.what:"+msg.what+"mOver:"+mOver);
            switch(msg.what) {
            case MSG_ROUTE:
                removeMessages(MSG_ROUTE);
                if (mReady) {
                    showAudioDevice();
                    if(mIsSpeaker2){
                        if (mIsSpeaker) {
                            routeDevices = FMplayService.RADIO_AUDIO_DEVICE_WIRED_HEADSET;
                        }else {
                            routeDevices = FMplayService.RADIO_AUDIO_DEVICE_SPEAKER;
                        }
                        mService.routeAudio(mIsSpeaker || audioChannel ? FMplayService.RADIO_AUDIO_DEVICE_WIRED_HEADSET : FMplayService.RADIO_AUDIO_DEVICE_SPEAKER);
                    }else{
                        if(mIsSpeaker || audioChannel){
                            routeDevices = FMplayService.RADIO_AUDIO_DEVICE_SPEAKER;
                        }else{
                            routeDevices = FMplayService.RADIO_AUDIO_DEVICE_WIRED_HEADSET;
                        }
                        mService.routeAudio(mIsSpeaker || audioChannel ? FMplayService.RADIO_AUDIO_DEVICE_SPEAKER : FMplayService.RADIO_AUDIO_DEVICE_WIRED_HEADSET);
                        audioChannel = false;
                    }
                } else {
                    Message message = mHandler.obtainMessage(MSG_ROUTE);
                    sendMessageDelayed(message, CHECK_DELAY);
                }
                break;
            case MSG_CLOSE:
                removeMessages(MSG_CLOSE);
                if (mReady) {
                    if(StationSearch.isSearching()){
                        StationSearch.mSearcher.stopSearching();
                        try {
                            StationSearch.mSearcher.join();
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }

                    if (mService.isFmOn()) {
                        enableRecorder(false);
                        mService.fmOff();
                    }

                    if(msg.getData().containsKey(KEY_CLOSE_REASON)){
                        Toast.makeText(FMPlay.this, msg.getData()
                                .getString(KEY_CLOSE_REASON), Toast.LENGTH_LONG).show();
                    }
                    if(routeDevices == 1){
                        audioChannel = true;
                    }
                    setUiPower(false, false);
                } else {
                    Message message = mHandler.obtainMessage(MSG_CLOSE);
                    sendMessageDelayed(message, CHECK_DELAY);
                }
                break;
            case MSG_LISTEN:
                {
                    Bundle data = msg.getData();
                    removeMessages(MSG_LISTEN);
                    if (mReady) {
                        float freq = data.getFloat("freq");
                        setFreqForUi(freq);
                    } else {
                        Message message = mHandler.obtainMessage(MSG_LISTEN);
                        message.setData(data);
                        sendMessageDelayed(message, CHECK_DELAY);
                        break;
                    }
                }
            case MSG_TUNE:
                {
                    Bundle data = msg.getData();
                    removeMessages(MSG_TUNE);
                    if (mReady) {
                        float freq = data.getFloat("freq");
                        FMPlaySharedPreferences.setTunedFreq(freq);
                        Log.d(LOGTAG,"MSG_TUNE: entering showRadioInfo");
                        setFreqForUi(freq);
                        mService.setFreq(freq);
                    } else {
                        Message message = mHandler.obtainMessage(MSG_TUNE);
                        message.setData(data);
                        sendMessageDelayed(message, CHECK_DELAY);
                    }
                }
                break;
            case MSG_UPDATE:
                removeMessages(MSG_UPDATE);
                if (mReady) {
                    if (!isFmOn()) {
                        mIsFromScratch = (FMPlaySharedPreferences.getStationCount(0) == 0);
                    } else {
                        setUiEnabled(true);

                        if (mService.getAudioDevice() == FMplayService.RADIO_AUDIO_DEVICE_SPEAKER || mIsSpeaker2) {
                            mIsSpeaker = true;
                            mIsSpeaker2 = false;
                        } else {
                            mIsSpeaker = false;
                        }

                        if (mIsFromScratch) {
                            mIsFromScratch = false;
                            if (FMPlaySharedPreferences.getStationCount(0) > 0) {
                                FMPlaySharedPreferences.setTunedFreq(FMPlaySharedPreferences.getStation(0, 0).getFreq());
                            }
                        }
                    }

                    float freq;
                    if(searchfreq){
                        freq = FMplayService.getInstance().getFreq();
                        searchfreq = false;
                    }else{
                        freq = FMPlaySharedPreferences.getTunedFreq();
                    }
                    String name = FMPlaySharedPreferences.getStationName(0, freq);
                    if (name == null) {
                        name = "";
                    }
                    Log.d(LOGTAG,"MSG_UPDATE: entering showRadioInfo");
                    setFreqForUi(freq);
                    showAudioDevice();
                    if (mService.isFmOn()) {
                        mService.setFreq(freq);
                    }
                } else {
                    Message message = mHandler.obtainMessage(MSG_UPDATE);
                    sendMessageDelayed(message, CHECK_DELAY);
                }
                break;
            case MSG_REFER:
                removeMessages(MSG_REFER);
                if (mReady) {
                    Intent intent = new Intent(FMplayService.ACTION_COUNT);
                    intent.putExtra("count", 1);
                    sendBroadcast(intent);
                } else {
                    sendMessageDelayed(obtainMessage(MSG_REFER), CHECK_DELAY);
                }
                break;
            case MSG_OPEN:
                removeMessages(MSG_OPEN);
                if (!mReady) {
                    sendMessageDelayed(obtainMessage(MSG_OPEN), OPEN_DELAY);
                } else {

                    if (!isFmOn()) {//fix bug 11354,11420
                        if (System.getInt(getContentResolver(), System.AIRPLANE_MODE_ON, 0) != 0) {
                            showAlertDialog(AIRPLANE_DIALOG);
                            return;
                        }

                        TelephonyManager pm = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
                        if (pm.getCallState() == TelephonyManager.CALL_STATE_RINGING || pm.getCallState() == TelephonyManager.CALL_STATE_OFFHOOK) {
                            showAlertDialog(PHONE_DIALOG);
                                return;
                        }

                            if (!SystemProperties.getBoolean("ro.device.support.antenna", false)) {
                                if (isHeadsetExists()) {
                                    if (mCurrentDialog == HEADSET_DIALOG) {
                                        closeAlertDialog(HEADSET_DIALOG);
                                    }

                                    // mm04 fix bug 2704
                                    if (mPhone) {
                                        mOver = false;
                                        Log.d(LOGTAG, "showAlertDialog:STARTUP_DIALOG");
                                        showAlertDialog(STARTUP_DIALOG);
                                        sendMessage(obtainMessage(MSG_PROGRESS));
                                        mOpenThread = createOpenThread();
                                        Log.d(LOGTAG, " create thread when use ");
                                        mOpenThread.start();
                                    } else { // mm04 fix bug 5847
                                        mPhone = true;
                                    }
                                } else {
                                    if (mCurrentDialog != HEADSET_DIALOG) {
                                        if (mCurrentDialog != NO_DIALOG) {
                                            closeAlertDialog(mCurrentDialog);
                                        }
                                        showAlertDialog(HEADSET_DIALOG);
                                    }
                                    return;
                                }
                            }
                    } else {
                        audioChannel = false;
                        setUiPower(true, false);
                    }
                }
                break;
            case MSG_PROGRESS:
                removeMessages(MSG_PROGRESS);
                if (!mOver) {
                    sendMessageDelayed(obtainMessage(MSG_PROGRESS), PROGRESS_DELAY);
                } else {
                    closeAlertDialog(STARTUP_DIALOG);
                        if (!SystemProperties.getBoolean("ro.device.support.antenna", false)) {
                            if (isHeadsetExists()) {
                                if (!mResult) {
                                    showAlertDialog(OPENERROR_DIALOG);
                                } else {
                                    mIsSpeaker = false;
                                    Message message = mHandler.obtainMessage(MSG_ROUTE);
                                    mHandler.sendMessage(message);

                                    if (FMPlaySharedPreferences.getStationCount(0) <= 0) {
                                        Intent req = new Intent(FMPlay.this, StationSearch.class);
                                        startActivity(req);
                                    } else {
                                        message = mHandler.obtainMessage(MSG_UPDATE);
                                        mHandler.sendMessage(message);
                                    }
                                    setUiPower(true, false);
                                    setUiEnabled(true); // Users can do whatever
                                                        // they want once we are
                                                        // prepared.
                                }
                            } else {
                                Intent intent = new Intent(FMplayService.ACTION_SHUTDOWN);
                                sendBroadcast(intent);
                            }
                        }
                }
                break;
            case MSG_MANUAL_SEEK_STARTED:
                setUiEnabled(false);
                switchUiToSearching(true);
                break;
            case MSG_MANUAL_SEEK_FINISHED:
                if (mManualSeekThread != null && mManualSeekThread.isAlive()) {
                    try {
                        mManualSeekThread.join();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                mManualSeekThread = null;

                boolean seekOK = msg.getData().getBoolean(KEY_MANUAL_SEEK_RESULT, false);
                setUiEnabled(true);
                switchUiToSearching(false);
                if (seekOK) {
                    float freq = msg.getData().getFloat(KEY_MANUAL_SEEK_FREQ, WheelConfig.RADIO_MIN_FREQUENCY);
                    FMPlaySharedPreferences.setTunedFreq(freq);
                    mService.setFreq(freq);
                    setFreqForUi(freq);
                    if(null ==FMPlaySharedPreferences.getStationName(0, freq)){  //add 2012.2.4
                        addfrq = true;
                        showAlertDialog(ADD_DIALOG);
                    }
                } else {
                    mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_UPDATE), MANUAL_SEEK_FAILED_DELAY);
                }
                break;
            case MSG_ABLED:
            	mHeadsetToggle.setEnabled(true);
            	break;
            case EVENT_START_RECODER:
                enableRecorder(m_recorder.state()==0);
                break;
            default:
            }
        }
    };

    public void onCreate(Bundle savedInstanceState) {
        Log.d(LOGTAG,"onCreate");
        super.onCreate(savedInstanceState);
        setVolumeControlStream(AudioManager.STREAM_FM);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                , WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.radio);

        mStationList = FMPlaySharedPreferences.getInstance(this);

        if (!getComponents()) {
            return;
        }

        setUiPower(false, true);

        mService = new RadioServiceStub(this, this);

        m_recorder = new Recorder();
        m_recorder.setOnStateChangedListener(mOnStateChangedListener);
        registerExternalStorageListener();

        mIsSpeaker = false;
        mReady = false;
        mOver = true;
        mResult = false;
        mDialogs = new Dialog[DIALOG_COUNT];
        mCurrentDialog = NO_DIALOG;

        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        filter.addAction(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
        filter.addAction(Intent.ACTION_FM);
        filter.addAction(Intent.ACTION_HEADSET_PLUG);
        filter.addAction("android.intent.action.AlertReceiver");
        filter.addAction(ACTION_CAMERA_PAUSE);
        filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        filter.addAction("android.intent.action.UMS_CONNECTED");
        filter.addAction("android.intent.action.TIME_SET");
        registerReceiver(mReceiver, filter);

        IntentFilter audioFilter = new IntentFilter();
        audioFilter.addAction(ACTION_AUTOPREVIEW_START);
        audioFilter.addAction(ACTION_SMS_START);
        audioFilter.addAction(ACTION_FM_MUSIC_PAUSE);
        audioFilter.addAction(ACTION_FM_VIDEO_PAUSE);
        audioFilter.addAction(ACTION_RADIO_PAUSE);
        registerReceiver(mAudioBroadcastReceiver, audioFilter);
        //mm04 fix bug 2706
        tmgr1 = (TelephonyManager) getSystemService("phone");
        tmgr1.listen(mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
        if (TelephonyManager.getPhoneCount()>1) {
            tmgr2 = (TelephonyManager) getSystemService("phone1");
            tmgr2.listen(mPhoneStateListener,PhoneStateListener.LISTEN_CALL_STATE);
            }
    }

    protected void onStart() {
        //mm04 fix bug 3576
        if(onBack){
            Log.d(LOGTAG,"onStart,onBack");
            super.onStart();
            finish();
            return;
            }
        Log.d(LOGTAG,"onStart");
        super.onStart();

        if (!mService.bindToService()) {
            Log.d(LOGTAG, "fail to bindToService");
            mService = null;
            return;
        }
        if(!sendMessage){
            mHandler.sendMessage(mHandler.obtainMessage(MSG_OPEN));
            mHandler.sendMessage(mHandler.obtainMessage(MSG_REFER));
        }else{
            sendMessage = false;
        }

        //mm04 fix bug 3183
        Intent i = new Intent("com.android.fm.stopmusicservice");
        i.putExtra("playingfm", true);
        this.sendBroadcast(i);
    }

    protected void onResume() {
        Log.d(LOGTAG,"onResume");
        super.onResume();
//        if(!isHeadsetExists()){
//            showAlertDialog(HEADSET_DIALOG);
//        }
        mStationList.load();
        mOver = true;
        if(refresh){
            getComponents();
            refresh = false;
        }
        if(StationSearch.isSearching()){
            Log.d(LOGTAG,"isSearching = true!");
            Intent req = new Intent(getBaseContext(), StationSearch.class);
            startActivity(req);
        } else {
            Log.d(LOGTAG,"isSearching = false!");
            Message msg = mHandler.obtainMessage(MSG_UPDATE);
            mHandler.sendMessage(msg);
        }
    }

    protected void onPause() {
        Log.d(LOGTAG,"onPause");
        mStationList.save();
        listBoolean = true;
        super.onPause();
    }

    public void onBackPressed() {
        if(mService!=null && isFmOn()){
            moveTaskToBack(true);
            return;
            }
        super.onBackPressed();
    }

    protected void onDestroy() {
        Log.d(LOGTAG,"onDestroyRadio");
        enableRecorder(false);
        setUiPower(false, false);
        unregisterReceiver(mReceiver);
        unregisterReceiver(mAudioBroadcastReceiver);
        //unregister Receiver
        unregisterReceiver(mSDCardMountEventReceiver);

        mHandler.removeMessages(MSG_ROUTE);
        mHandler.removeMessages(MSG_CLOSE);
        mHandler.removeMessages(MSG_LISTEN);
        mHandler.removeMessages(MSG_UPDATE);
        mHandler.removeMessages(MSG_REFER);
        mHandler.removeMessages(MSG_TUNE);
        mHandler.removeMessages(MSG_OPEN);
        mHandler.removeMessages(MSG_PROGRESS);
        mHandler.removeMessages(MSG_UPDATE);
        mHandler.removeMessages(MSG_ABLED);
        mHandler.removeMessages(MSG_MANUAL_SEEK_FINISHED);
        mHandler.removeMessages(MSG_MANUAL_SEEK_STARTED);

        //mm04 fix bug 3183
        Intent i = new Intent("com.android.fm.stopmusicservice");
        i.putExtra("playingfm", false);
        this.sendBroadcast(i);
        tmgr1.listen(mPhoneStateListener, 0);
        if (TelephonyManager.getPhoneCount()>1) {
            if (tmgr2 != null) {
                tmgr2.listen(mPhoneStateListener,
                        PhoneStateListener.LISTEN_NONE);
                }
        }
        tmgr1 = null;
        tmgr2 = null;
        mPhoneStateListener = null;
        mDigitalClockUpdater.stop();
        super.onDestroy();
    }

    private class DigitalClockUpdater {
        private static final int MSG_UPDATE_TIME = 0;

        private static final int UPDATE_INTERNAL = 1000;

        DigitalTextView mView = null;

        boolean mRunning = false;

        Handler mUpdateHandler = new Handler () {
            @Override
            public void handleMessage(Message msg) {
                if (msg.what == MSG_UPDATE_TIME) {
                    Date curDate = new Date(java.lang.System.currentTimeMillis());
                    int hours = curDate.getHours();
                    int minutes = curDate.getMinutes();

                    //mm04 fix bug 5109
//                    ContentResolver cv = FMPlay.this.getContentResolver();
//                    String strTimeFormat = android.provider.Settings.System.getString(cv, android.provider.Settings.System.TIME_12_24);
					if (DateFormat.is24HourFormat(FMPlay.this)) {
					        //for bug 81638
						mClockTypeString = "";
					} else {
					        mClockTypeString = (hours >= 12 ? getString(R.string.pm) : getString(R.string.am));
			                        hours = hours > 12 ? hours-12 : (hours == 0 ? 12 : hours);
					}
					mView.setDigitalText(String.format("%d:%02d", hours, minutes));
					mClockType.setText(mClockTypeString);
					//for bug 81638

                    if (mRunning)
                        this.sendEmptyMessageDelayed(MSG_UPDATE_TIME, UPDATE_INTERNAL);
                }
            }
        };

        public DigitalClockUpdater(DigitalTextView view) {
            mView = view;
        }

        public void run() {
            mRunning = true;
            mUpdateHandler.removeMessages(MSG_UPDATE_TIME);
            mUpdateHandler.sendEmptyMessage(MSG_UPDATE_TIME);
        }

        public void stop() {
            mRunning = false;
            mView = null;
            mUpdateHandler.removeMessages(MSG_UPDATE_TIME);
        }
    }


    protected void onStop() {
        Log.d(LOGTAG,"onStop");
        //mm04 fix bug 3576
        if(!onBack){
            if (mCurrentDialog != NO_DIALOG) {
                closeAlertDialog(mCurrentDialog);
                }
            Intent intent = new Intent(FMplayService.ACTION_COUNT);
            intent.putExtra("count", -1);
            sendBroadcast(intent);
            if (mService != null && (mOver || isFinishing())) {
                mService.unbindFromService();
                mReady = false;
                }
        }
        if(mService!=null){
            mService.getFreq2(true);//mm04 fix bug 5871
        }
        //mm04 fix bug 3183
        Intent i = new Intent("com.android.fm.stopmusicservice");
        i.putExtra("playingfm", false);
        this.sendBroadcast(i);
        super.onStop();
    }

    public boolean onCreateOptionsMenu(Menu menu) {
        Resources res = getResources();
        menu.add(Menu.NONE, Menu.FIRST + 1, 1, res.getText(R.string.auto_search)).setIcon(R.drawable.menu_auto_search);
        menu.add(Menu.NONE+2, Menu.FIRST + 3, 3, res.getText(R.string.from_current_search)).setIcon(R.drawable.menu_auto_search);
        return true;
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()) {
        case Menu.FIRST + 1:
            FROM_CURRENT_CHANNEL = false;
            if(mService!=null && mService.isFmOn()){
                showAlertDialog(SEARCH_DIALOG);
                }
            break;
        case Menu.FIRST + 2:
            Message message = Message.obtain(mHandler,
                    EVENT_START_RECODER, 0, 0);
            mHandler.sendMessage(message);
            break;
        case Menu.FIRST + 3:
            FROM_CURRENT_CHANNEL = true;
            if(mService!=null && mService.isFmOn()){
                showAlertDialog(SEARCH_DIALOG);
                }
            break;
        default:
        }
        return false;
    }

    void enableRecorder(boolean bEnable) {
        if (m_recorder != null) {
            boolean isRecordStart = false;
            if (bEnable && m_recorder.state()==m_recorder.IDLE_STATE && mService!=null && mService.isFmOn()) {
                isRecordStart = m_recorder.startRecording(MediaRecorder.OutputFormat.AMR_NB, ".amr", this);
            } else if (!bEnable && m_recorder.state()!=m_recorder.IDLE_STATE) {
            	m_recorder.stop();
                saveSample();
                //Usb Storage modify start
                m_recorder.finishSaveSample();
                //Usb Storage modify end
            }
        }
    }

    private void saveSample() {
        if (m_recorder.sampleLength() == 0)
            return;
        Uri uri = null;
        try {
            uri = this.addToMediaDB(m_recorder.sampleFile());
        } catch(UnsupportedOperationException ex) {  // Database manipulation failure
            Log.d(LOGTAG, "save record file failed.");
            return;
        }
    }

    protected void onPrepareDialog(int id, Dialog dialog) {
        switch (id) {
        case ADD_DIALOG:
            EditText freq = (EditText) dialog.findViewById(R.id.edit_freq);
            float fFreq = FMPlaySharedPreferences.getTunedFreq();
            freq.setText(Float.toString(fFreq));
            freq.setEnabled(false);
            freq.setFocusable(false);
            String strName = FMPlaySharedPreferences.getStationName(0, fFreq);
            if (strName == null) {//changmod 2012.2.4
                if(addfrq){
                    addfrq = false;
                    strName = getResources().getString(R.string.station_info)+fFreq;
                    }else{
                        strName = "";
                        }
            }
            EditText name = (EditText) dialog.findViewById(R.id.edit_name);
            name.setText(strName);
            Editable edit = name.getText();
            Selection.setSelection(edit, edit.length());
            break;
        default:
        }
    }

    protected Dialog onCreateDialog(int id) {
        Log.d(LOGTAG,"onCreateDialog" + id);
        final Toast nameInputInfo = Toast.makeText(this, getResources().getString(R.string.name_input_info),Toast.LENGTH_SHORT);
        final Toast nameError = Toast.makeText(this, getResources().getString(R.string.name_error),Toast.LENGTH_SHORT);
        final Toast freqError = Toast.makeText(this, getResources().getString(R.string.current_freq_error),Toast.LENGTH_SHORT);
        final Toast sameInfo = Toast.makeText(this, getResources().getString(R.string.same_info),Toast.LENGTH_SHORT);
        final Toast have_been_completed = Toast.makeText(this, getResources().getString(R.string.have_been_completed), Toast.LENGTH_SHORT);// mm04 fix bug 7032
        switch (id) {
            case AIRPLANE_DIALOG:
            {
                LayoutInflater inflater = LayoutInflater.from(this);
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                View view = inflater.inflate(R.layout.airplane_alert_dialog, null);
                Button btOK = (Button) view.findViewById(R.id.bt_ok);
                btOK.setOnClickListener(new Button.OnClickListener() {
                    public void onClick(View v) {
                        finish();
                        closeAlertDialog(AIRPLANE_DIALOG);
                    }
                });

                mDialogs[AIRPLANE_DIALOG] = builder.setView(view).create();
                mDialogs[AIRPLANE_DIALOG].setOnKeyListener(new DialogInterface.OnKeyListener() {
                    public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                        if (mCurrentDialog == AIRPLANE_DIALOG) {
                            if (keyCode == KeyEvent.KEYCODE_BACK) {
                                finish();
                                closeAlertDialog(AIRPLANE_DIALOG);
                                return true;
                            }
                        }

                        return false;
                    }
                });
                return mDialogs[AIRPLANE_DIALOG];
            }
            case PHONE_DIALOG:
            {
                LayoutInflater inflater = LayoutInflater.from(this);
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                View view = inflater.inflate(R.layout.phone_alert_dialog, null);
                Button btOK = (Button) view.findViewById(R.id.bt_ok);
                btOK.setOnClickListener(new Button.OnClickListener() {
                    public void onClick(View v) {
                        if(TelephonyManager.getPhoneCount()>1)
                        {
                            TelephonyManager pm = (TelephonyManager) FMPlay.this.getSystemService(Context.TELEPHONY_SERVICE);
                             TelephonyManager pm1 = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE+"1");

                             if (pm.getCallState() == TelephonyManager.CALL_STATE_OFFHOOK || (pm1 == null ? false : (pm1.getCallState() == TelephonyManager.CALL_STATE_OFFHOOK))
                                     || pm.getCallState() == TelephonyManager.CALL_STATE_RINGING || (pm1 == null ? false : (pm1.getCallState() == TelephonyManager.CALL_STATE_RINGING))) {
                                 finish();
                             } else {
                                 mHandler.sendMessage(mHandler.obtainMessage(MSG_OPEN));
                             }
                             }else{
                                 TelephonyManager pm = (TelephonyManager) FMPlay.this.getSystemService(Context.TELEPHONY_SERVICE);
                             if (pm.getCallState() == TelephonyManager.CALL_STATE_OFFHOOK || pm.getCallState() == TelephonyManager.CALL_STATE_RINGING) {
                                 finish();
                             } else {
                                 mHandler.sendMessage(mHandler.obtainMessage(MSG_OPEN));
                             }
                             }
                        closeAlertDialog(PHONE_DIALOG);
                    }
                });

                mDialogs[PHONE_DIALOG] = builder.setView(view).create();
                mDialogs[PHONE_DIALOG].setOnKeyListener(new DialogInterface.OnKeyListener() {
                    public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                        if (mCurrentDialog == PHONE_DIALOG) {
                            if (keyCode == KeyEvent.KEYCODE_BACK) {
                                finish();
                                closeAlertDialog(PHONE_DIALOG);
                                return true;
                            } else if (keyCode == KeyEvent.KEYCODE_SEARCH) {
                                return true;
                            }
                        }

                        return false;
                    }
                });
                return mDialogs[PHONE_DIALOG];
            }
             case HEADSET_DIALOG:
             {
                 LayoutInflater inflater = LayoutInflater.from(this);
                 AlertDialog.Builder builder = new AlertDialog.Builder(this);
                 View view = inflater.inflate(R.layout.headset_alert_dialog, null);

                 mDialogs[HEADSET_DIALOG] = builder.setView(view).create();
                 mDialogs[HEADSET_DIALOG].setOnKeyListener(new DialogInterface.OnKeyListener() {
                     public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                         if (mCurrentDialog == HEADSET_DIALOG) {
                             if (keyCode == KeyEvent.KEYCODE_BACK) {
                                 finish();
                                 closeAlertDialog(HEADSET_DIALOG);
                                 return true;
                             } else if (keyCode == KeyEvent.KEYCODE_SEARCH) {
                                 return true;
                             }
                         }

                         return false;
                     }
                 });
                 return mDialogs[HEADSET_DIALOG];
             }
             case BLUETOOTH_DIALOG:
             {
                 LayoutInflater inflater = LayoutInflater.from(this);
                 AlertDialog.Builder builder = new AlertDialog.Builder(this);
                 View view = inflater.inflate(R.layout.bluetooth_alert_dialog, null);
                 Button btOK = (Button) view.findViewById(R.id.bt_ok);
                 btOK.setOnClickListener(new Button.OnClickListener() {
                     public void onClick(View v) {
                         Intent intent = new Intent();
                         intent.setAction("android.settings.WIRELESS_SETTINGS");
                         startActivity(intent);
                         closeAlertDialog(BLUETOOTH_DIALOG);
                     }
                 });

                 mDialogs[BLUETOOTH_DIALOG] = builder.setView(view).create();
                 mDialogs[BLUETOOTH_DIALOG].setOnKeyListener(new DialogInterface.OnKeyListener() {
                     public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                         if (mCurrentDialog == BLUETOOTH_DIALOG) {
                             if (keyCode == KeyEvent.KEYCODE_BACK) {
                                 finish();
                                 closeAlertDialog(BLUETOOTH_DIALOG);
                                 return true;
                             } else if (keyCode == KeyEvent.KEYCODE_SEARCH) {
                                 return true;
                             }
                         }

                         return false;
                     }
                 });
                 return mDialogs[BLUETOOTH_DIALOG];
             }
             case STARTUP_DIALOG:
             {
            	 Log.d(LOGTAG, "STARTUP_DIALOG:start_alert_dialog");
                 LayoutInflater inflater = LayoutInflater.from(this);
                 AlertDialog.Builder builder = new AlertDialog.Builder(this);
                 View view = inflater.inflate(R.layout.start_alert_dialog, null);
                 mImage = (ImageView) view.findViewById(R.id.image);
                 mDialogs[STARTUP_DIALOG] = builder.setView(view).create();
                 mDialogs[STARTUP_DIALOG].setOnKeyListener(new DialogInterface.OnKeyListener() {
                     public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                         if (mCurrentDialog == STARTUP_DIALOG) {
                             if (keyCode == KeyEvent.KEYCODE_BACK || keyCode == KeyEvent.KEYCODE_SEARCH) {
                                 return true;
                             }
                         }

                         return false;
                     }
                 });
                 mDialogs[STARTUP_DIALOG].setOnShowListener(new DialogInterface.OnShowListener() {
                     public void onShow(DialogInterface dialog) {
                         mImage.startAnimation(mAnimation);
                     }
                 });
                 mDialogs[STARTUP_DIALOG].setOnDismissListener(new DialogInterface.OnDismissListener() {
                     public void  onDismiss(DialogInterface dialog) {
                         mImage.clearAnimation();
                     }
                 });
                 return mDialogs[STARTUP_DIALOG];
             }
             case OPENERROR_DIALOG:
             {
                 LayoutInflater inflater = LayoutInflater.from(this);
                 AlertDialog.Builder builder = new AlertDialog.Builder(this);
                 View view = inflater.inflate(R.layout.open_error_alert_dialog, null);
                 Button btOK = (Button) view.findViewById(R.id.bt_ok);
                 btOK.setOnClickListener(new Button.OnClickListener() {
                     public void onClick(View v) {
                         finish();
                         closeAlertDialog(OPENERROR_DIALOG);
                     }
                 });

                 mDialogs[OPENERROR_DIALOG] = builder.setView(view).create();
                 mDialogs[OPENERROR_DIALOG].setOnKeyListener(new DialogInterface.OnKeyListener() {
                     public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                         if (mCurrentDialog == OPENERROR_DIALOG) {
                             if (keyCode == KeyEvent.KEYCODE_BACK) {
                                 finish();
                                 closeAlertDialog(OPENERROR_DIALOG);
                                 return true;
                             } else if (keyCode == KeyEvent.KEYCODE_SEARCH) {
                                 return true;
                             }
                         }

                         return false;
                     }
                 });
                 return mDialogs[OPENERROR_DIALOG];
             }
             case ADD_DIALOG:
             {
                 LayoutInflater inflater = LayoutInflater.from(this);
                 AlertDialog.Builder builder = new AlertDialog.Builder(this);
                 View view = inflater.inflate(R.layout.name_freq_alert_dialog, null);
                 final TextView title = (TextView) view.findViewById(R.id.title);
                 title.setText(R.string.add_title);
                 final EditText edit = (EditText) view.findViewById(R.id.edit_name);
                 setupLengthFilter(edit, TEXT_MAX_LENGTH);
                 Button btOK = (Button) view.findViewById(R.id.bt_ok);
                 Button btCancel = (Button) view.findViewById(R.id.bt_cancel);
                 btOK.setOnClickListener(new Button.OnClickListener() {
                     public void onClick(View v) {
                         String name = edit.getText().toString().trim();
                         if (name.length() == 0) {
                               nameInputInfo.show();
                         } else {
                             if (FMPlaySharedPreferences.exists(0, name)) {
                            	 Log.d(LOGTAG, "shit! ");
                                 String stationName = FMPlaySharedPreferences.getStationName(0, FMPlaySharedPreferences.getTunedFreq());
                                 Log.d(LOGTAG, "shit! stationName is " + stationName);
                                 if(stationName != null && stationName.equals(name)){
                                     sameInfo.show();
                                     closeAlertDialog(ADD_DIALOG);
                                 } else {
                                     nameError.show();
                                 }
                             } else {
                                 if (FMPlaySharedPreferences.exists(0, FMPlaySharedPreferences.getTunedFreq())) {
                                     //mm04 fix bug 5882 start
                                     FMPlaySharedPreferences.setStationName(0, FMPlaySharedPreferences.getTunedFreq(), name);
                                     FMPlaySharedPreferences.setStationEditStatus(0, FMPlaySharedPreferences.getTunedFreq(), true);
                                   //mm04 fix bug 5882 end
                                 } else {
                                     FMPlaySharedPreferences.addStation(0, name, FMPlaySharedPreferences.getTunedFreq(), true);
                                 }

                                 edit.setText("");
                                 mStationList.save();
                                 closeAlertDialog(ADD_DIALOG);
                                 have_been_completed.show();
                                 Message msg = mHandler.obtainMessage(MSG_UPDATE);
                                 mHandler.sendMessage(msg);
                             }
                         }
                     }
                 });
                 btCancel.setOnClickListener(new Button.OnClickListener() {
                     public void onClick(View v) {
                         edit.setText("");
                         closeAlertDialog(ADD_DIALOG);
                     }
                 });

                 mDialogs[ADD_DIALOG] = builder.setView(view).create();

                 mDialogs[ADD_DIALOG].setOnDismissListener(new DialogInterface.OnDismissListener() {
                     public void onDismiss(DialogInterface dialog) {
                         Log.d(LOGTAG,"key captured! mCurrentDialog = " + mCurrentDialog);
                         if (mCurrentDialog == ADD_DIALOG) {
                             closeAlertDialog(ADD_DIALOG);
                         }
                     }
                 });

                 return mDialogs[ADD_DIALOG];
             }
         case SEARCH_DIALOG:
             {
                 LayoutInflater inflater = LayoutInflater.from(this);
                 AlertDialog.Builder builder = new AlertDialog.Builder(this);
                 View view = inflater.inflate(R.layout.search_alert_dialog, null);
                 Button btOK = (Button) view.findViewById(R.id.bt_ok);
                 Button btCancel = (Button) view.findViewById(R.id.bt_cancel);
                 btOK.setOnClickListener(new Button.OnClickListener() {
                     public void onClick(View v) {
                    	 Log.d(LOGTAG, "SEARCH_DIALOG-0");
                    	 closeAlertDialog(SEARCH_DIALOG);
                    	 Log.d(LOGTAG, "SEARCH_DIALOG-1");
//                         FMPlaySharedPreferences.clearList(0);
                         Intent req = new Intent(getBaseContext(), StationSearch.class);
                         if(FROM_CURRENT_CHANNEL){
                             req.putExtra("freq",mService.getFreq());
                         }
                         startActivity(req);
                    	 Log.d(LOGTAG, "SEARCH_DIALOG-2");
                     }
                 });
                 btCancel.setOnClickListener(new Button.OnClickListener() {
                     public void onClick(View v) {
                         closeAlertDialog(SEARCH_DIALOG);
                     }
                 });

                 mDialogs[SEARCH_DIALOG] = builder.setView(view).create();

                 mDialogs[SEARCH_DIALOG].setOnKeyListener(new DialogInterface.OnKeyListener() {
                     public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                         if (mCurrentDialog == SEARCH_DIALOG) {
                             if (keyCode == KeyEvent.KEYCODE_BACK || keyCode == KeyEvent.KEYCODE_SEARCH) {
                                 closeAlertDialog(SEARCH_DIALOG);
                                 return true;
                             }
                         }
                         return false;
                     }
                 });

                 return mDialogs[SEARCH_DIALOG];
             }
            default:
        }

        return null;
    }

    //mm04 fix bug 3183
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(LOGTAG, "onKeyDown"+"keyCode ="+keyCode);
        if (keyCode == event.KEYCODE_VOLUME_UP || keyCode == event.KEYCODE_VOLUME_DOWN) {
            if(keyCode == event.KEYCODE_VOLUME_UP){

            }if(keyCode == event.KEYCODE_VOLUME_DOWN){

            }
        }else if(keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE){
            if (mService.isFmOn()){
                //mm04 fix bug 4992
                mHandler.removeMessages(MSG_ABLED);
                 mHandler.sendEmptyMessage(MSG_CLOSE);
             }else{
                 mPhone = true;
                 mHandler.sendEmptyMessage(MSG_OPEN);
             }
        }else if(keyCode == KeyEvent.KEYCODE_MEDIA_NEXT){
            PresetStation station = FMPlaySharedPreferences.getNextStation(0, FMPlaySharedPreferences.getTunedFreq());
            if (station != null) {
                Message msg = mHandler.obtainMessage(MSG_LISTEN);
                Bundle data = new Bundle();
                data.putFloat("freq", station.getStationFreq());
                data.putString("name", station.getStationName());
                msg.setData(data);
                mHandler.sendMessage(msg);
            }
        }else if(keyCode == KeyEvent.KEYCODE_MEDIA_PREVIOUS){
            PresetStation station = FMPlaySharedPreferences.getPrevStation(0, FMPlaySharedPreferences.getTunedFreq());
            if (station != null) {
                Message msg = mHandler.obtainMessage(MSG_LISTEN);
                Bundle data = new Bundle();
                data.putFloat("freq", station.getStationFreq());
                data.putString("name", station.getStationName());
                msg.setData(data);
                mHandler.sendMessage(msg);
            }
        }
        // TODO Auto-generated method stub
        return super.onKeyDown(keyCode, event);
        }

    protected void setupLengthFilter(EditText editText, int length){
        InputFilter[] filters = new InputFilter[1];
        final int maxLength = length;
        filters[0] = new InputFilter.LengthFilter(maxLength){
            private final int TOAST_INTERVAL = 2000;
            private long mToastTime = 0;
            @Override
            public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend){
                if(source.length() > 0 && dest.length() == maxLength && java.lang.System.currentTimeMillis() - mToastTime > TOAST_INTERVAL){
                    mToastTime = java.lang.System.currentTimeMillis();
                    Toast.makeText(FMPlay.this, getResources().getText(R.string.input_too_many_chars), Toast.LENGTH_SHORT).show();
                }
                return super.filter(source, start, end, dest, dstart, dend);
            }
        };
        editText.setFilters(filters);
    }

    protected boolean getComponents() {

        mPowerToggle = (CheckableImageButton)findViewById(R.id.power_toggle);
        mPowerToggle.setDrawable(R.drawable.power_button_checked
                                , R.drawable.power_button_unchecked
                                , R.drawable.power_button_disabled);
        mPowerToggle.setCheckedChangedListener(this);

        mHeadsetToggle =  (CheckableImageButton)findViewById(R.id.headset_toggle);
        mHeadsetToggle.setDrawable(R.drawable.headset_button_checked
                                , R.drawable.headset_button_unchecked
                                , R.drawable.headset_button_disabled);

        mHeadsetToggle.setCheckedChangedListener(this);

        mAddButton = (ImageButton)findViewById(R.id.add_button);
        mAddButton.setOnClickListener(this);

        mRadioListButton = (ImageButton)findViewById(R.id.radio_list_button);
        mRadioListButton.setOnClickListener(this);

        mNextStationButton = (ImageButton)findViewById(R.id.next_station_button);
        mNextStationButton.setOnClickListener(this);

        mPrevStationButton = (ImageButton)findViewById(R.id.prev_station_button);
        mPrevStationButton.setOnClickListener(this);

        mSearchNextButton = (ImageButton)findViewById(R.id.search_next_button);
        mSearchNextButton.setOnClickListener(this);

        mSearchPrevButton = (ImageButton)findViewById(R.id.search_prev_button);
        mSearchPrevButton.setOnClickListener(this);
        mClockType = (TextView)findViewById(R.id.text_hour_type);

        Date curDate = new Date(java.lang.System.currentTimeMillis());
        int hours = curDate.getHours();

        //mm04 fix bug 5109
        ContentResolver cv = FMPlay.this.getContentResolver();
//        String strTimeFormat = android.provider.Settings.System.getString(cv, android.provider.Settings.System.TIME_12_24);
        if(!DateFormat.is24HourFormat(FMPlay.this)){
            if(hours > 12){
                mClockTypeString = getString(R.string.pm);
            }else{
                mClockTypeString = getString(R.string.am);
            }
            mClockType.setText(mClockTypeString);
        }else{
            mClockType.setText("");
        }

        mDigitalClock = (DigitalTextView)findViewById(R.id.digital_clock);
        mDigitalClock.setResourcePrefix("time");
        mDigitalClockUpdater = new DigitalClockUpdater(mDigitalClock);
        mDigitalClockUpdater.run();

        mFreqView = (DigitalTextView)findViewById(R.id.digital_freq);
        mFreqView.setResourcePrefix("freq");
        mFreqView.setDigitalText("107.5");

        mTuneWheel = (TuneWheel)findViewById(R.id.tune_wheel);
        mTuneWheel.setOnValueChangedListener(this);

        mHeadsetIndicator = (ImageView)findViewById(R.id.headset_indicator);

        mLedBackground = findViewById(R.id.led_background);

        mFmIndicator = findViewById(R.id.fm_indicator);

        mSearchingIndicator = findViewById(R.id.searching_indicator);
        mSearchingIndicator.setVisibility(View.INVISIBLE);

        mFreqPointer = (SeekBar)findViewById(R.id.freq_indicator);
        mFreqPointer.setMax((int)((WheelConfig.RADIO_MAX_FREQUENCY - WheelConfig.RADIO_MIN_FREQUENCY) * FREQ_POINTER_SCALE));
        mFreqPointer.setEnabled(false);

        mAnimation = new TranslateAnimation(Animation.RELATIVE_TO_PARENT, 0.0f, Animation.RELATIVE_TO_PARENT, 1.0f,
                                            Animation.RELATIVE_TO_PARENT, 0.0f, Animation.RELATIVE_TO_PARENT, 0.0f);
        mAnimation.setDuration(ANIMATION_DERATION);
        mAnimation.setRepeatCount(Animation.INFINITE);
        mAnimation.setFillAfter(true);

        return true;
    }

    public void onServiceConnected(ComponentName name, IBinder service) {
        mReady = true;
        Log.d(LOGTAG, "mReady = true");
    }

    public void onServiceDisconnected(ComponentName name) {
        mReady = false;
    }

    protected void showAlertDialog(int id) {
        if(!FMPlay.this.isFinishing()){
            showDialog(id);
            mCurrentDialog = id;
        }
    }

    protected void closeAlertDialog(int id) {
        //mm04 fix bug 3360
        if(mDialogs[id]==null){
            try {
                Thread.sleep(500);
                } catch (InterruptedException e) {

                }
                if(mDialogs[id]!=null){
                    mDialogs[id].cancel();
                    }
                }else{
                    mDialogs[id].cancel();
                    }
        mCurrentDialog = NO_DIALOG;
    }

    public boolean onCheckedChanged(View view, boolean checked) {
        switch(view.getId()) {
        case R.id.power_toggle:
            if (checked) {
                ismPhoneStateListener = false;
                mPhone = true;
                mHandler.sendEmptyMessage(MSG_OPEN);
            } else if (mService.isFmOn()){
                ismPhoneStateListener = true;
                //mm04 fix bug 4992
                mHandler.removeMessages(MSG_ABLED);
                mHandler.sendEmptyMessage(MSG_CLOSE);
            }
            break;
        case R.id.headset_toggle:

            mIsSpeaker = !checked;
            Message msg = mHandler.obtainMessage(MSG_ROUTE);
            mHandler.sendMessage(msg);
            if(mService.getAudioDevice()==0){
                mHeadsetToggle.setDrawable(R.drawable.headset_button_checked
                        , R.drawable.headset_button_unchecked
                        , R.drawable.headset_button_unchecked_disable);
            }else{
                mHeadsetToggle.setDrawable(R.drawable.headset_button_checked
                        , R.drawable.headset_button_unchecked
                        , R.drawable.headset_button_disabled);
            }
            mHeadsetToggle.setEnabled(false);
            Message msgabled = mHandler.obtainMessage(MSG_ABLED);
            mHandler.sendMessageDelayed(msgabled, 0);
            break;
        }
        return false;
    }

    private void setUiPower(boolean powerOn, boolean immediate) {
        AlphaAnimation am = new AlphaAnimation(
                powerOn ? 0 : 1,
                        powerOn ? 1 : 0
                );

        am.setFillEnabled(true);
        am.setFillAfter(true);
        am.setFillBefore(false);
        am.setStartOffset(powerOn ? 0 : 100);
        mLedBackground.startAnimation(am);
        AlphaAnimation am2 = new AlphaAnimation(
                powerOn ? 0 : 1,
                        powerOn ? 1 : 0
                );

        am2.setFillEnabled(true);
        am2.setFillAfter(true);
        am2.setFillBefore(false);
        mFreqView.startAnimation(am2);
        mDigitalClock.startAnimation(am2);
        setUiEnabled(powerOn);
        // setUiEnable may disable power toggle, so enable it here
        mPowerToggle.setEnabled(true);
        mPowerToggle.setChecked(powerOn, false);
    }

    private void setUiEnabled(boolean enabled) {
    	Log.d(LOGTAG,"setUiEnabled"+enabled);
        mAddButton.setEnabled(enabled);
        mRadioListButton.setEnabled(enabled);
        mPrevStationButton.setEnabled(enabled);
        mNextStationButton.setEnabled(enabled);
        mSearchPrevButton.setEnabled(enabled);
        mSearchNextButton.setEnabled(enabled);
        mHeadsetToggle.setEnabled(enabled);
        mTuneWheel.setEnabled(enabled);
        mTuneWheel.setClickable(enabled);
        mPowerToggle.setEnabled(enabled);
    }

    public void onClick(View view) {
        switch (view.getId()) {
        case R.id.next_station_button:
        {
            PresetStation station = FMPlaySharedPreferences.getNextStation(0, FMPlaySharedPreferences.getTunedFreq());
            if (station != null) {
                Message msg = mHandler.obtainMessage(MSG_LISTEN);
                Bundle data = new Bundle();
                data.putFloat("freq", station.getStationFreq());
                data.putString("name", station.getStationName());
                msg.setData(data);
                mHandler.sendMessage(msg);
            }
            break;
        }
        case R.id.prev_station_button:
        {
            PresetStation station = FMPlaySharedPreferences.getPrevStation(0, FMPlaySharedPreferences.getTunedFreq());
            if (station != null) {
                Message msg = mHandler.obtainMessage(MSG_LISTEN);
                Bundle data = new Bundle();
                data.putFloat("freq", station.getStationFreq());
                data.putString("name", station.getStationName());
                msg.setData(data);
                mHandler.sendMessage(msg);
            }
            break;
        }
        case R.id.search_next_button:
            if (mService != null && (mManualSeekThread == null || !mManualSeekThread.isAlive())) {
                searchfreq = true;
                mManualSeekThread = createManualSeekThread(MANUAL_SEEK_NEXT);
                mManualSeekThread.start();
            }
            break;
        case R.id.search_prev_button:
            if (mService != null && (mManualSeekThread == null || !mManualSeekThread.isAlive())) {
                searchfreq = true;
                mManualSeekThread = createManualSeekThread(MANUAL_SEEK_PREV);
                mManualSeekThread.start();
            }
            break;
        case R.id.add_button:
            showAlertDialog(ADD_DIALOG);
            break;
        case R.id.radio_list_button:
            Intent req = new Intent(this, FMPlayList.class);
            startActivity(req);
            break;
        }
    }

    public void onTuneWheelValueChanged(View v, float changedBy) {
        Message msg = mHandler.obtainMessage(MSG_TUNE);
        Bundle data = new Bundle();
        float freq = adjustFreq(WheelConfig.format(mService.getFreq() + changedBy));

        data.putFloat("freq", freq);
        msg.setData(data);
        mHandler.sendMessage(msg);
    }

    @Override
    public void onOptionsMenuClosed(Menu menu) {
        super.onOptionsMenuClosed(menu);
        }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
//		if(m_recorder.state()==0){
//			menu.findItem(Menu.FIRST + 2).setTitle(R.string.start_recorder).setIcon(R.drawable.idle_led);
//		}else{
//			menu.findItem(Menu.FIRST + 2).setTitle(R.string.stop_recorder).setIcon(R.drawable.recording_led);
//		}
        if(mService!=null && isFmOn()){
            menu.getItem(Menu.NONE).setVisible(true);
        }else{
            menu.getItem(Menu.NONE).setVisible(false);
        }
		return true;
	}

	private void setFreqForUi(float freq) {
        if (freq== WheelConfig.RADIO_MIN_FREQUENCY) {
            mTuneWheel.setDragEnable(TuneWheel.DIRECTION_PREV, false);
            mTuneWheel.setDragEnable(TuneWheel.DIRECTION_NEXT, true);
        }
        else if (freq == WheelConfig.RADIO_MAX_FREQUENCY) {
            mTuneWheel.setDragEnable(TuneWheel.DIRECTION_PREV, true);
            mTuneWheel.setDragEnable(TuneWheel.DIRECTION_NEXT, false);
        } else {
            mTuneWheel.setDragEnable(TuneWheel.DIRECTION_PREV, true);
            mTuneWheel.setDragEnable(TuneWheel.DIRECTION_NEXT, true);
        }

        int count = FMPlaySharedPreferences.getStationCount(0);
        if (count == 0) {
            mPrevEnable = false;
            mPrevStationButton.setEnabled(false);
            mNextEnable = false;
            mNextStationButton.setEnabled(false);
        } else {
            PresetStation first = FMPlaySharedPreferences.getStation(0, 0);
                mPrevStationButton.setPressed(false);

            PresetStation last = FMPlaySharedPreferences.getStation(0, count - 1);
                mNextStationButton.setPressed(false);
        }

        mFreqPointer.setProgress((int)((freq - WheelConfig.RADIO_MIN_FREQUENCY)* FREQ_POINTER_SCALE));
        mFreqView.setDigitalText("" + freq);
    }

    private float adjustFreq(float freq) {
        float result = ((int)(freq * 10)) / 10f;
        if (result < WheelConfig.RADIO_MIN_FREQUENCY)
            result = WheelConfig.RADIO_MIN_FREQUENCY;

        if (result > WheelConfig.RADIO_MAX_FREQUENCY)
            result = WheelConfig.RADIO_MAX_FREQUENCY;
        return result;
    }

    private void showAudioDevice() {
        mHeadsetToggle.setChecked(!mIsSpeaker, false);
        if(mIsSpeaker ||audioChannel){
        	mHeadsetIndicator.setImageResource(R.drawable.headset_indicator_spk);
        }else {
        	mHeadsetIndicator.setImageResource(R.drawable.headset_indicator);
        }
    }

    private void switchUiToSearching(boolean searching) {
        int visibility = searching ? View.VISIBLE : View.INVISIBLE;
        AlphaAnimation aa = new AlphaAnimation(
                searching ? 1 : 0.2f,
                searching ? 0.2f : 1
                );
        aa.setDuration(500);
        aa.setFillEnabled(true);
        aa.setFillAfter(true);
        aa.setFillBefore(false);
        mSearchingIndicator.setVisibility(visibility);
        mFreqView.startAnimation(aa);
    }

    /*
     * Adds file and returns content uri.
     */
    private Uri addToMediaDB(File file) {
        Resources res = getResources();
        ContentValues cv = new ContentValues();
        long current = java.lang.System.currentTimeMillis();
        long modDate = file.lastModified();
        Date date = new Date(current);
        SimpleDateFormat formatter = new SimpleDateFormat(
                res.getString(R.string.audio_db_title_format));
        String title = formatter.format(date);

        // Lets label the recorded audio file as NON-MUSIC so that the file
        // won't be displayed automatically, except for in the playlist.
        cv.put(MediaStore.Audio.Media.IS_MUSIC, "0");

        cv.put(MediaStore.Audio.Media.TITLE, title);
        cv.put(MediaStore.Audio.Media.DATA, file.getAbsolutePath());
        cv.put(MediaStore.Audio.Media.DATE_ADDED, (int) (current / 1000));
        cv.put(MediaStore.Audio.Media.DATE_MODIFIED, (int) (modDate / 1000));
        cv.put(MediaStore.Audio.Media.MIME_TYPE, "audio/3gpp");
        cv.put(MediaStore.Audio.Media.ARTIST,
                res.getString(R.string.audio_db_artist_name));
        cv.put(MediaStore.Audio.Media.ALBUM,
                res.getString(R.string.audio_db_album_name));
        ContentResolver resolver = getContentResolver();
        Uri base = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        Uri result = resolver.insert(base, cv);
        if (result == null) {
            Toast.makeText(this, R.string.error_mediadb_new_record, Toast.LENGTH_SHORT).show();
            return null;
        }
        if (getPlaylistId(res) == -1) {
            createPlaylist(res, resolver);
        }
        int audioId = Integer.valueOf(result.getLastPathSegment());
        addToPlaylist(resolver, audioId, getPlaylistId(res));

        // Notify those applications such as Music listening to the
        // scanner events that a recorded audio file just created.
        sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE, result));
        return result;
    }

    /*
     * Add the given audioId to the playlist with the given playlistId; and maintain the
     * play_order in the playlist.
     */
    private void addToPlaylist(ContentResolver resolver, int audioId, long playlistId) {
        String[] cols = new String[] {
                "count(*)"
        };
        Uri uri = MediaStore.Audio.Playlists.Members.getContentUri("external", playlistId);
        Cursor cur = resolver.query(uri, cols, null, null, null);
        cur.moveToFirst();
        final int base = cur.getInt(0);
        cur.close();
        ContentValues values = new ContentValues();
        values.put(MediaStore.Audio.Playlists.Members.PLAY_ORDER, Integer.valueOf(base + audioId));
        values.put(MediaStore.Audio.Playlists.Members.AUDIO_ID, audioId);
        resolver.insert(uri, values);
    }


    /*
     * Create a playlist with the given default playlist name, if no such playlist exists.
     */
    private Uri createPlaylist(Resources res, ContentResolver resolver) {
        ContentValues cv = new ContentValues();
        cv.put(MediaStore.Audio.Playlists.NAME, res.getString(R.string.audio_db_playlist_name));
        Uri uri = resolver.insert(MediaStore.Audio.Playlists.getContentUri("external"), cv);
        if (uri == null) {
            new AlertDialog.Builder(this)
                .setTitle(R.string.app_name)
                .setMessage(R.string.error_mediadb_new_record)
                .setPositiveButton(R.string.button_ok, null)
                .setCancelable(false)
                .show();
        }
        return uri;
    }

    /*
     * A simple utility to do a query into the databases.
     */
    private Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        try {
            ContentResolver resolver = getContentResolver();
            if (resolver == null) {
                return null;
            }
            return resolver.query(uri, projection, selection, selectionArgs, sortOrder);
         } catch (UnsupportedOperationException ex) {
            return null;
        }
    }

    /*
     * Obtain the id for the default play list from the audio_playlists table.
     */
    private int getPlaylistId(Resources res) {
        Uri uri = MediaStore.Audio.Playlists.getContentUri("external");
        final String[] ids = new String[] { MediaStore.Audio.Playlists._ID };
        final String where = MediaStore.Audio.Playlists.NAME + "=?";
        final String[] args = new String[] { res.getString(R.string.audio_db_playlist_name) };
        Cursor cursor = query(uri, ids, where, args, null);
        if (cursor == null) {
            Log.d(LOGTAG,"query returns null");
        }
        int id = -1;
        if (cursor != null) {
            cursor.moveToFirst();
            if (!cursor.isAfterLast()) {
                id = cursor.getInt(0);
            }
        }
        cursor.close();
        return id;
    }

    OnStateChangedListener mOnStateChangedListener = new OnStateChangedListener() {

        @Override
        public void onStateChanged(int state) {
        }

        @Override
        public void onError(int error) {
//            log("onError error:"+error);
            Resources res = getResources();
            String msg = null;
            switch (error) {
                case Recorder.SDCARD_ACCESS_ERROR:
                    msg = res.getString(R.string.error_sdcard_access);
                    break;
                case Recorder.IN_CALL_RECORD_ERROR:
//                    update error message to reflect that the recording could not be performed during a call.
                case Recorder.INTERNAL_ERROR:
                    msg = res.getString(R.string.error_app_internal);
                    break;
                case Recorder.SDCARD_IS_FULL:
                    msg = res.getString(R.string.storage_is_full);
                    break;
                default:
                    msg = res.getString(R.string.error_app_internal);
            }
            if (!TextUtils.isEmpty(msg)) {
                Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_LONG).show();
            }
        }
    };

    /*
     * Registers an intent to listen for ACTION_MEDIA_EJECT/ACTION_MEDIA_MOUNTED
     * notifications.
     */
    private void registerExternalStorageListener() {
        if (mSDCardMountEventReceiver == null) {
            mSDCardMountEventReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    String action = intent.getAction();
                    if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
                        m_recorder.delete();
                    } else if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
                    }
                }
            };
            IntentFilter iFilter = new IntentFilter();
            iFilter.addAction(Intent.ACTION_MEDIA_EJECT);
            iFilter.addDataScheme("file");
            registerReceiver(mSDCardMountEventReceiver, iFilter);
        }
    }
}
