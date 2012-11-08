/* Copyright 2009-2011 Broadcom Corporation
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
package com.broadcom.bt.app.fm.rx;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.text.Editable;
import android.text.Selection;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.app.ProgressDialog;

import com.broadcom.bt.app.fm.FmConstants;
import com.broadcom.bt.app.fm.R;
import com.broadcom.bt.app.fm.system.FmService;
import com.broadcom.bt.service.fm.FmReceiver;
import com.broadcom.bt.service.fm.IFmReceiverEventHandler;
import com.broadcom.bt.service.framework.IBluetoothProxyCallback;
import java.util.List;
import android.os.SystemProperties;
import android.view.WindowManager;

/**
 * An example FM Receiver application that supports the following features: <li>
 * Access to the FM Receiver Service. <li>Preset station management. <li>RDS
 * capability
 */
public class FmRadio extends Activity
		implements IRadioViewRxTouchEventHandler, OnSharedPreferenceChangeListener, IBluetoothProxyCallback {

    private static final String TAG = "FmRxRadio";
    private static final boolean V = true;
    public static final String PLAYSTATE_CHANGED = "com.android.music.playstatechanged";

    /* CONSTANT BLOCK */

    /* GUI message codes. */
    private static final int GUI_UPDATE_MSG_SIGNAL_STATUS = 1;
    private static final int GUI_UPDATE_MSG_MUTE_STATUS = 2;
    private static final int GUI_UPDATE_MSG_FREQ_STATUS = 3;
    private static final int GUI_UPDATE_MSG_WORLD_STATUS = 4;
    private static final int GUI_UPDATE_MSG_RDS_STATUS = 5;
    private static final int GUI_UPDATE_MSG_AF_STATUS = 6;
    private static final int GUI_UPDATE_MSG_RDS_DATA = 7;
    private static final int UPDATE_NOTIFICATION = 8;
    private static final int AUTO_SEARCH = 9;
    private static final int POWER_UP_DONE = 10;
    private static final int SIGNAL_CHECK_PENDING_EVENTS = 20;
    private static final int NFL_TIMER_EVENT = 21;

    private static final int MENU_CH_SET = 1;
    private static final int MENU_CH_CLEAR = 2;
    private static final int MENU_CH_CANCEL = 3;

    private static final int ADD_DIALOG = 16;
    private static final int SEARCH_DIALOG = 17;
    private static final int SEARCH_QUERY_DIALOG = 18;

    private static final int MESSAGE_DELAYED_TIME = 600;
    private static final int WAIT_TIME = 1000;

    /* Default frequency. */
    private static final int DEFAULT_FREQUENCY = 8750;

    private static final int STATE_UNINITIAL = 0;
    private static final int STATE_INITIALIZING = 1;
    private static final int STATE_INITIALIZED = 2;

    /* VARIABLE BLOCK */

    /* Object instant references. */
    private FmReceiveView mView;
    private FmReceiverEventHandler mFmReceiverEventHandler;
    private FmReceiver mFmReceiver;
    private SharedPreferences mSharedPrefs;
    private final static int NUM_OF_CHANNELS = 10;
    private int mChannels[] = new int[NUM_OF_CHANNELS];
    private final static String freqPreferenceKey = "channel";
    private final static String lastFreqPreferenceKey = "last";

    /* Local GUI status variables. */
    private int mWorldRegion = FmReceiver.FUNC_REGION_DEFAULT;

    private int mFrequency = DEFAULT_FREQUENCY;
    private int mFrequencyStep = 10; // Step in increments of 100 Hz
    private int mMinFreq, mMaxFreq; // updated with mPendingRegion
    private int mNfl;

    private boolean mSeekInProgress = false;
    boolean mPowerOffRadio =false;

    /* Pending values. (To be requested) (Startup values specified) */
    private int mPendingRegion = FmReceiver.FUNC_REGION_DEFAULT;
    private int mPendingDeemphasis = FmReceiver.DEEMPHASIS_75U;
    private int mPendingAudioMode = FmReceiver.AUDIO_MODE_AUTO;
    private int mPendingAudioPath = FmReceiver.AUDIO_PATH_WIRE_HEADSET;
    private int mPendingFrequency = DEFAULT_FREQUENCY;
    private boolean mPendingMute = false;
    private int mPendingScanStep = FmReceiver.FREQ_STEP_100KHZ; // Step in increments of 100 Hz
    private int mPendingScanMethod = FmReceiver.SCAN_MODE_FAST;
    private int mPendingRdsMode = FmReceiver.RDS_MODE_OFF;
    private int mPendingRdsType = FmReceiver.RDS_COND_NONE;
    private int mPendingAfMode = FmReceiver.AF_MODE_OFF;
    private int mPendingNflEstimate = FmReceiver.NFL_MED;
    private int mPendingSearchDirection = FmReceiver.SCAN_MODE_NORMAL;
    private boolean mPendingLivePoll = false;
    private int mPendingSnrThreshold = FmReceiver.FM_MIN_SNR_THRESHOLD;
    private int mPendingLivePollinterval = 2000; // 2 second polling of rssi

    /* Pending updates. */
    // sprivate boolean enabledUpdatePending = false;
    private boolean shutdownPending = false;
    private boolean worldRegionUpdatePending = false;
    private boolean audioModeUpdatePending = false;
    private boolean audioPathUpdatePending = false;
    private boolean frequencyUpdatePending = false;
    private boolean muteUpdatePending = false;
    private boolean scanStepUpdatePending = false;
    private boolean rdsModeUpdatePending = false;
    private boolean nflEstimateUpdatePending = false;
    private boolean stationSearchUpdatePending = false;
    private boolean livePollingUpdatePending = false;
    private boolean fmVolumeUpdatepending=false;
    private boolean fmSetSNRThresholdPending=false;
    private boolean mFinish = false;
    private boolean bFinishCalled = false;
    private boolean mRadioIsOn=false;

    private static final Object mSeekSearchLock = new Object();
    // fix bug 4960 start
    // add tag to record notification
    private boolean mIsSeekSearchNotified = false;
    // fix bug 4960 end
    private static StationManage mStationManage;
    private boolean isSeekSearch =false;
    private Dialog mAddDialog;
    private Dialog mSearchDialog;
    private Dialog mSearchQueryDialog;
    private boolean isHeadsetUnplug = true;
    private boolean isAirplane = true;
    private Handler mMainThreadHandler = new Handler();
    private ProgressDialog mRadioOnProgressDialog;

    private int mOperationState = STATE_UNINITIAL;

    // to fix bug 1492, we shell avoid reacting with the change of shared preference temporarily
    private static boolean sSuppressReactingWithPreferenceChanged = false;
    public static void setSuppressReactingWithPreferenceChanged(boolean enable) {
        sSuppressReactingWithPreferenceChanged = enable;
    }

    /* Array of RDS program type names (English). */
	String mRdsProgramTypes[];

	TelephonyManager mTelephonyManager;
	AudioManager mAudioManager;
	private AudioManager.OnAudioFocusChangeListener audioFocusChangeListener;

	private HeadsetPlugUnplugBroadcastReceiver mHeadsetPlugUnplugBroadcastReceiver;

	private boolean mInCall = false;

    /** Called when the activity is first created. */
	@Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate");

        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        mRadioOnProgressDialog = new ProgressDialog(this);
        mRadioOnProgressDialog.setTitle("");
        mRadioOnProgressDialog.setIndeterminate(false);
        mRadioOnProgressDialog.setCancelable(false);
        mRadioOnProgressDialog.setMessage(getResources().getString(R.string.open_info));

        mTelephonyManager    = (TelephonyManager)    getSystemService(Context.TELEPHONY_SERVICE);
        mAudioManager        = (AudioManager)        getSystemService(Context.AUDIO_SERVICE);

        mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        mSharedPrefs.registerOnSharedPreferenceChangeListener(this);

        mStationManage = new StationManage(this,mSharedPrefs);

        for (int i=0; i < NUM_OF_CHANNELS; i++)
        	mChannels[i] = mSharedPrefs.getInt(freqPreferenceKey + i, 0);

        /* Retrieve array of program types from arrays.xml. */
		mRdsProgramTypes = getResources().getStringArray(R.array.fm_rds_pty_names);
        mView = (FmReceiveView) (View.inflate(this,R.layout.radio_rx, null));
		mView.init(this);
		setContentView(mView);

		updateMinMaxFrequencies();
        mView.setFrequencyStep(mFrequencyStep);

//        for (int i = FmConstants.BUTTON_CH_1; i <= FmConstants.BUTTON_CH_10; i++) {
//			View v = mView.findViewById(i);
//			registerForContextMenu(v);
//		}

		IntentFilter filter = new IntentFilter();
		filter.addAction(FmService.ACTION_RADIO_FORCEQUIT);
		registerReceiver(mMediaStateReceiver,filter);

    }

    protected void onResume() {
       	if (V) {
           Log.d(TAG, "onResume");
        }

        if (mFmReceiver == null
         && !bFinishCalled
         && mOperationState == STATE_UNINITIAL) { // Avoid calling getProxy
                                                  // is finish has been
                                                  // called already
            if (V) {
                Log.d(TAG, "Getting FmReceiver proxy...");
            }
            FmReceiver.getProxy(this, this);
            mOperationState = STATE_INITIALIZING;
        } else if (mFmReceiver != null) {
            Intent intent = new Intent(FmService.ACTION_RADIO_RESTORE);
            sendBroadcast(intent);
            Log.d(TAG, "onResume() send FmService.ACTION_RADIO_RESTORE");
        }

        mAudioManager.forceVolumeControlStream(AudioManager.STREAM_FM);
        if (mHeadsetPlugUnplugBroadcastReceiver == null)
            mHeadsetPlugUnplugBroadcastReceiver = new HeadsetPlugUnplugBroadcastReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_HEADSET_PLUG);
//        filter.addAction(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
        Intent intent = registerReceiver(mHeadsetPlugUnplugBroadcastReceiver,
                filter);

        super.onResume();
    }

    protected void onPause() {
        if (V) {
            Log.d(TAG, "onPause");
        }
        mAudioManager.forceVolumeControlStream(-1);

	    this.unregisterReceiver(mHeadsetPlugUnplugBroadcastReceiver);

    	if (mPendingFrequency != 0) { // save last frequency
	    	Editor e = mSharedPrefs.edit();
	    	e.putInt(lastFreqPreferenceKey, mPendingFrequency);
	    	e.apply();
    	}
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        Log.d(TAG, "Calling onDestroy()");
		unregisterReceiver(mMediaStateReceiver);
        if (mFmReceiver != null) {
            if (V) {
                Log.d(TAG, "Finishing FmReceiver proxy...");
            }
            mFmReceiver.unregisterEventHandler();
            mFmReceiver.finish();
            mFmReceiver = null;
            mOperationState = STATE_UNINITIAL;
        }
    }

    protected void onPrepareDialog(int id, Dialog dialog) {
        switch (id) {
        case ADD_DIALOG:
            EditText freq = (EditText) dialog.findViewById(R.id.edit_freq);
            int fFreq = mPendingFrequency;
            freq.setText((fFreq/100.0)+"");
            freq.setEnabled(false);
            freq.setFocusable(false);
            String strName = mStationManage.getStationName(fFreq);
            if (strName == null) {
                strName = "";
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
    	final Toast nameInputInfo = Toast.makeText(this, getResources().getString(R.string.name_input_info),Toast.LENGTH_SHORT);
    	final Toast sameInfo = Toast.makeText(this, getResources().getString(R.string.same_info),Toast.LENGTH_SHORT);
    	final Toast have_been_completed = Toast.makeText(this, getResources().getString(R.string.have_been_completed), Toast.LENGTH_SHORT);
    	final Toast nameError = Toast.makeText(this, getResources().getString(R.string.name_error),Toast.LENGTH_SHORT);
    	switch (id) {
    	case ADD_DIALOG:
        {
            LayoutInflater inflater = LayoutInflater.from(this);
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            View view = inflater.inflate(R.layout.name_freq_alert_dialog, null);
            final TextView title = (TextView) view.findViewById(R.id.title);
            title.setText(R.string.add_title);
            final EditText edit = (EditText) view.findViewById(R.id.edit_name);
            final EditText freq = (EditText) view.findViewById(R.id.edit_freq);
            Button btOK = (Button) view.findViewById(R.id.bt_ok);
            Button btCancel = (Button) view.findViewById(R.id.bt_cancel);
            btOK.setOnClickListener(new Button.OnClickListener() {
                public void onClick(View v) {
                    String name = edit.getText().toString().trim();
                    if (name.length() == 0) {
                          nameInputInfo.show();
                    } else {
                        String strFreq = freq.getText().toString().trim();
                        int nFreq = mPendingFrequency;
                        if (strFreq != null && !strFreq.isEmpty()) {
                            try {
                                nFreq = (int)(Float.parseFloat(strFreq) * 100);
                            } catch(Exception e) {
                                Log.d(TAG, "failed to parse float");
                            }
                        }

                        if (mStationManage.isExistStation(name)) {
                            String stationName = mStationManage.getStationName(nFreq);
                            Log.d(TAG, " stationName is " + stationName);
                            if(stationName != null && stationName.equals(name)){
                                sameInfo.show();
                                mAddDialog.cancel();
                            } else {
                                nameError.show();
                            }
                        } else {
                            if (mStationManage.isExistStation(nFreq)) {
                                mStationManage.setStationName(nFreq, name);
                            } else {
                                mStationManage.addStation(name, nFreq);
                            }

                            edit.setText("");
                            mAddDialog.cancel();
                            mStationManage.save();
                            have_been_completed.show();
                        }
                    }
                }
            });
            btCancel.setOnClickListener(new Button.OnClickListener() {
                public void onClick(View v) {
                    edit.setText("");
                    mAddDialog.cancel();
                }
            });

            mAddDialog = builder.setView(view).create();

            mAddDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                public void onDismiss(DialogInterface dialog) {
                	if(mAddDialog.isShowing()){
                		mAddDialog.cancel();
                	}
                }
            });

            return mAddDialog;
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
                        mStationManage.clearStation();
                        mStationManage.save();
						Intent req = new Intent(FmRadio.this,
								SeekStation.class);
						startActivity(req);
	                    mSearchDialog.cancel();
	                }
	            });
	            btCancel.setOnClickListener(new Button.OnClickListener() {
	                public void onClick(View v) {
	                    mSearchDialog.cancel();
	                }
	            });

	            mSearchDialog = builder.setView(view).create();
	            return mSearchDialog;
	        }

        case SEARCH_QUERY_DIALOG:
	        {
	            LayoutInflater inflater = LayoutInflater.from(this);
	            AlertDialog.Builder builder = new AlertDialog.Builder(this);
	            View view = inflater.inflate(R.layout.search_query_dialog, null);
	            Button btOK = (Button) view.findViewById(R.id.bt_ok);
	            Button btCancel = (Button) view.findViewById(R.id.bt_cancel);
	            btOK.setOnClickListener(new Button.OnClickListener() {
	                public void onClick(View v) {
//	                	mStationManage.clearStation();
						Intent req = new Intent(FmRadio.this,
								SeekStation.class);
						startActivity(req);
	                    mSearchQueryDialog.cancel();
	                }
	            });
	            btCancel.setOnClickListener(new Button.OnClickListener() {
	                public void onClick(View v) {
	                    mSearchQueryDialog.cancel();
	                }
	            });

	            mSearchQueryDialog = builder.setView(view).create();
	            return mSearchQueryDialog;
	        }

        default:
    	}
    	return null;
    }


    @Override
    public void onBackPressed() {
        moveTaskToBack(true);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            moveTaskToBack(true);
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    private BroadcastReceiver mMediaStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "mMediaStateReceiver onReceive action=" + action);
            if (action.equals(FmService.ACTION_RADIO_FORCEQUIT)) {
                int reason = intent.getIntExtra(FmService.EXTRA_QUIT_REASON, -1);
                if (reason == FmService.QUIT_REASON_AIRPLANE) {
                    String airplaneInfo = getString(R.string.airplane_info);
                    displayErrorMessageAndExit(airplaneInfo, true);
                } else {
                    Log.e(TAG, "WARNING: unhandled exit reason:" + reason);
                }
            }
        }
    };

     /**
     * This function is called to initialize pending operations and activate the
     * FM Radio HW on startup.
     */
    private void powerUpSequence() {
        if (V) {
            Log.d(TAG, "powerUpSequence()");
        }

        /* Set pending updates to trigger on response from startup. */
        // enabledUpdatePending = false;
        shutdownPending = false;
        audioModeUpdatePending = true;
        audioPathUpdatePending = true;
        nflEstimateUpdatePending = false;
        frequencyUpdatePending = true;
        scanStepUpdatePending = true;
        rdsModeUpdatePending = true;
        fmVolumeUpdatepending = true;
        livePollingUpdatePending = false;
        stationSearchUpdatePending = false;
        mSeekInProgress = false;
        fmSetSNRThresholdPending = false;

        /* Initialize the radio. This can give us GUI initialization info. */
        Log.d(TAG, "Turning on radio... mFmReceiver = " + mFmReceiver+" ; Softmute state:"+FmConstants.FM_SOFTMUTE_FEATURE_ENABLED);
        if (mFmReceiver == null) {
            Log.e(TAG, "Invalid FM Receiver Proxy!!!!");
            return;
        }

        mView.enableSkipUpdatingViewForMuted();

        Message msg = Message.obtain();
        msg.what = GUI_UPDATE_MSG_FREQ_STATUS;
        msg.arg1 = mPendingFrequency;
        msg.arg2 = 1;
        viewUpdateHandler.sendMessage(msg);

        mTurnOnRadioThread = new Thread() {
            public void run() {
                int status;

                if(FmConstants.FM_SOFTMUTE_FEATURE_ENABLED) {
                    status = mFmReceiver.turnOnRadio(FmReceiver.FUNC_REGION_NA | FmReceiver.FUNC_RBDS | FmReceiver.FUNC_AF | FmReceiver.FUNC_SOFTMUTE);
                } else {
                    status = mFmReceiver.turnOnRadio(FmReceiver.FUNC_REGION_NA | FmReceiver.FUNC_RBDS | FmReceiver.FUNC_AF);
                }

                Log.d(TAG, "Turn on radio status = " + status);

                Message msg = Message.obtain();
                msg.what = POWER_UP_DONE;
                msg.arg1 = status;
                viewUpdateHandler.sendMessage(msg);
            }    
        };

        mRadioOnProgressDialog.show();
        mTurnOnRadioThread.start();
    }

    private Thread mTurnOnRadioThread = null;
    private boolean mAutoSearchQueryDone = true;

    private void onPowerUpDone(int status) {
        mRadioOnProgressDialog.dismiss();
        if (status == FmReceiver.STATUS_OK) {
            //powerupComplete();
            // As turnOnRadio an asynchronous call, the callbacks will initialize the frequencies accordingly.
            // Update only the GUI

            if (mStationManage != null) {
                List<PresetStation> stations = mStationManage.getStations();
                if (stations.isEmpty()) {
                    mAutoSearchQueryDone = false;
                    viewUpdateHandler.sendMessage(viewUpdateHandler.obtainMessage(AUTO_SEARCH));
                }
            }
        } else if (status == FmReceiver.STATUS_AUDIO_FUCOS_FAILED) {
            String error = getString(R.string.error_request_audio_focus_faild);
            Log.e(TAG, error);
            displayErrorMessageAndExit(error, false);
            return;
        } else if (status == FmReceiver.STATUS_AIREPLANE_MODE) {
            String error = getString(R.string.airplane_message);
            Log.e(TAG, error);
            displayErrorMessageAndExit(error, false);
            return;
        } else {
            /* Add recovery code here if startup fails. */
            String error = getString(R.string.error_failed_powerup) + "\nStatus = " + status;
            Log.e(TAG, error);
            displayErrorMessageAndExit(error, false);
            return;
        }

        if(/*isHeadsetUnplug*/!FmService.isHeadsetPlugged()){
       		if(mFmReceiver != null){
                Log.d(TAG, "set audio path to none because of no headset");
       			audioPathUpdatePending = false;
       			mFmReceiver.setAudioPath(FmReceiver.AUDIO_PATH_NONE);
       		}
       	}

        // update mute state, we should in un-mute state on start up
        updateMuted(false);
    }

    /**
     * This function is called to initialize pending operations and activate the
     * FM Radio HW on startup.
     */
    private boolean powerDownSequence(boolean alreadyShutdown) {
        if (V) {
            Log.d(TAG, "powerDownSequence()");
        }

        moveTaskToBack(true);

        if (mTurnOnRadioThread != null) {
            try {
                mTurnOnRadioThread.join(WAIT_TIME);
            } catch (Exception e) {
            }

            mTurnOnRadioThread = null;
        }

        /* Set pending updates to trigger on response from startup. */
        // enabledUpdatePending = false;
        shutdownPending = true;
        audioModeUpdatePending = false;
        nflEstimateUpdatePending = false;
        frequencyUpdatePending = false;
        scanStepUpdatePending = false;
        rdsModeUpdatePending = false;
        livePollingUpdatePending = false;
        stationSearchUpdatePending = false;
        fmVolumeUpdatepending= false;
        fmSetSNRThresholdPending = false;

        if (!alreadyShutdown) {
            if (mFmReceiver != null)
                mFmReceiver.setAudioPath(mFmReceiver.AUDIO_PATH_NONE);
            int status = mFmReceiver.turnOffRadio();
            if (mFmReceiver != null && (status != FmReceiver.STATUS_OK)) {
                Log.w(TAG, "WARNING shutdown status=" + status +" but we will try later.");
            }
        }
        if (mPendingFrequency != 0) { // save last frequency
            Editor e = mSharedPrefs.edit();
            e.putInt(lastFreqPreferenceKey, mPendingFrequency);
            e.apply();
        }
        return true; // success
    }

    private void powerupComplete() {
    	Log.d(TAG,"powerupcomplete");
    	mPendingFrequency = mSharedPrefs.getInt(lastFreqPreferenceKey, DEFAULT_FREQUENCY);
        updateFrequency(mPendingFrequency);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
    	int action = event.getAction();
    	int keyCode = event.getKeyCode();
//    	if (action == KeyEvent.ACTION_UP) {
//    		if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
//    			int newVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
//    	        mFmReceiver.setFMVolume(FmReceiver.FM_VOLUME_MAX
//    	        		* newVolume
//    	        		/ mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC));
//    		} else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
//    			int newVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
//    	        mFmReceiver.setFMVolume(FmReceiver.FM_VOLUME_MAX
//    	        		* newVolume
//    	        		/ mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC));
//    		}
//    	}

		return super.dispatchKeyEvent(event);
    }

    /**
     * Request new region operation and pend if necessary.
     *
     * @param sp
     *            the shared preferences reference to get world region from.
     */
    private void updateWorldRegion(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateWorldRegion()");
        }
        /* Extract preferences and request these settings. */
        mPendingRegion = Integer.parseInt(sp.getString(FmRadioSettings.FM_PREF_WORLD_REGION, "0"));
        updateMinMaxFrequencies();
        mPendingDeemphasis = Integer.parseInt(sp.getString(FmRadioSettings.FM_PREF_DEEMPHASIS, "0"));
        if (mPendingRegion == FmReceiver.FUNC_REGION_DEFAULT)
            mPendingDeemphasis = FmReceiver.DEEMPHASIS_75U;
        else if (mPendingRegion == FmReceiver.FUNC_REGION_EUR || mPendingRegion == FmReceiver.FUNC_REGION_JP
             || mPendingRegion == FmReceiver.FUNC_REGION_JP_II)
            mPendingDeemphasis = FmReceiver.DEEMPHASIS_50U;
        if (V) {
            Log.d(TAG, "Sending region (" + Integer.toString(mPendingRegion) + ")");
            Log.d(TAG, "Sending deemphasis (" + Integer.toString(mPendingDeemphasis) + ")");
        }
        if (null != mFmReceiver) {
            worldRegionUpdatePending = (FmReceiver.STATUS_OK != mFmReceiver.setWorldRegion(mPendingRegion,
                    mPendingDeemphasis));
        }
    }

    /**
     * Request new scan step operation and pend if necessary.
     *
     * @param sp
     *            the shared preferences reference to get scan step from.
     */
    private void updateScanStep(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateScanStep()");
        }
        /* Extract preferences and request these settings. */
        mPendingScanStep = Integer.parseInt(mSharedPrefs.getString(FmRadioSettings.FM_PREF_SCAN_STEP, "0"));
        if (V) {
            Log.d(TAG, "Sending scan step (" + Integer.toString(mPendingScanStep) + ")");
        }

        if (null != mFmReceiver) {
            scanStepUpdatePending = (FmReceiver.STATUS_OK != mFmReceiver.setStepSize(mPendingScanStep));
            /*
             * If this succeeds, start using that scan step in manual step
             * updates.
             */
            if (!scanStepUpdatePending) {
                mFrequencyStep = (0 == mPendingScanStep) ? 10 : 5;
    			mView.setFrequencyStep(mFrequencyStep);
            }
        }
    }

    /**
     * Request new FM volume operation and pend if necessary.
     *
     * @param sp the shared preferences reference to FM volume text editor from.
     */
    private void updateFMVolume(int volume) {
		// CK - Send the command only of the Service is not busy
		    	Log.d(TAG, "updateFMVolume()");
    	if (null != mFmReceiver) {
			fmVolumeUpdatepending =
				(FmReceiver.STATUS_OK != mFmReceiver.setFMVolume(volume));
		}
    }

    /**
     * Request new RDS mode if necessary.
     *
     * @param sp the shared preferences reference to get rds mode from.
     */
    private void updateRdsMode(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateRdsMode()");
        }
        /* Extract preferences and request these settings. */
        mPendingRdsMode = Integer.parseInt(mSharedPrefs.getString(FmRadioSettings.FM_PREF_RDS_ENABLED, "0"));
        if (mPendingRdsMode != 0) {
            mPendingAfMode = (mSharedPrefs.getBoolean(FmRadioSettings.FM_PREF_AF_ENABLED, false)) ? 1 : 0;
        } else {
            mPendingAfMode = 0;
        }

        if (V) {
            Log.d(TAG, "Sending RDS mode (" + Integer.toString(mPendingRdsMode) + ")");
            Log.d(TAG, "Sending AF mode (" + Integer.toString(mPendingAfMode) + ")");
        }
        if (null != mFmReceiver) {
            rdsModeUpdatePending = (FmReceiver.STATUS_OK != mFmReceiver.setRdsMode(mPendingRdsMode,
                    FmReceiver.RDS_FEATURE_PS | FmReceiver.RDS_FEATURE_RT | FmReceiver.RDS_FEATURE_TP
                            | FmReceiver.RDS_FEATURE_PTY | FmReceiver.RDS_FEATURE_PTYN, mPendingAfMode, mNfl));
        }
    }

    /**
     * Request new audio mode operation and pend if necessary.
     *
     * @param sp
     *            the shared preferences reference to get audio mode from.
     */
    private void updateAudioMode(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateAudioMode()");
        }
        /* Extract preferences and request these settings. */
        mPendingAudioMode = Integer.parseInt(mSharedPrefs.getString(FmRadioSettings.FM_PREF_AUDIO_MODE, "0"));
        if (V) {
            Log.d(TAG, "Sending audio mode (" + Integer.toString(mPendingAudioMode) + ")");
        }
        if (null != mFmReceiver) {
            audioModeUpdatePending = (FmReceiver.STATUS_OK != mFmReceiver.setAudioMode(mPendingAudioMode));
        }
    }

    /**
     * Request new audio path operation and pend if necessary.
     *
     * @param sp
     *            the shared preferences reference to get audio path from.
     */
    private void updateAudioPath(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateAudioPath()");
        }
        /* Extract preferences and request these settings. */
        mPendingAudioPath = Integer.parseInt(mSharedPrefs.getString(FmRadioSettings.FM_PREF_AUDIO_PATH, String.valueOf(FmReceiver.AUDIO_PATH_WIRE_HEADSET)));
        if (V) {
            Log.d(TAG, "Sending audio path (" + Integer.toString(mPendingAudioPath) + ")");
        }

        mAudioPathUpdateHandler.removeMessages(MSG_UPDATE_AUDIO_PATH);
        mAudioPathUpdateHandler.sendMessageDelayed(mAudioPathUpdateHandler.obtainMessage(MSG_UPDATE_AUDIO_PATH), 500);
    }

    private final static int MSG_UPDATE_AUDIO_PATH = 1;

    private Handler mAudioPathUpdateHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (null != mFmReceiver) {
                audioPathUpdatePending = (FmReceiver.STATUS_OK != mFmReceiver.setAudioPath(mPendingAudioPath));
                Log.v(TAG, "audioPathUpdatePending == " + audioPathUpdatePending);
            }
        }
    };

    /**
     * Request frequency tuning operation and pend if necessary.
     *
     * @param freq
     *            the frequency to tune to.
     */
    private void updateFrequency(int freq) {
        if (V) {
            Log.d(TAG, "updateFrequency() :" + freq);
        }
        /* Extract pending data and request these settings. */
        mPendingFrequency = freq;
		mView.setFrequencyGraphics(freq);
		mView.setSeekStatus(mSeekInProgress, (mPendingFrequency != mFrequency));
		mView.resetRdsText();

        if (V) {
            Log.d(TAG, "Sending frequency (" + Integer.toString(mPendingFrequency) + ")");
        }
        if (null != mFmReceiver) {
            frequencyUpdatePending = (FmReceiver.STATUS_OK != mFmReceiver.tuneRadio(mPendingFrequency));
        }
    }

    /**
     * Request mute operation and pend if necessary.
     *
     * @param muted
     *            true if muting requested, false otherwise.
     */
    private int updateMuted(boolean muted) {
        if (V) {
            Log.d(TAG, "updateMuted() :" + muted);
        }
        /* Extract pending data and request these settings. */
        mPendingMute = muted;
        if (V) {
            Log.d(TAG, "Sending muted (" + (mPendingMute ? "TRUE" : "FALSE") + ")");
        }
        return mFmReceiver.muteAudio(mPendingMute);
    }

    /**
     * Request NFL estimate operation and pend if necessary.
     *
     * @param sp
     *            the shared preferences reference to get NFL mode from.
     */
    private void updateNflEstimate(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateNflEstimate()");
        }

        /* Extract preferences and request these settings. */
        mPendingNflEstimate = Integer.parseInt(mSharedPrefs.getString(FmRadioSettings.FM_PREF_NFL_MODE, "1"));
        if (V) {
            Log.d(TAG, "Sending NFL mode (" + Integer.toString(mPendingNflEstimate) + ")");
        }
        if (null != mFmReceiver) {
            nflEstimateUpdatePending = (FmReceiver.STATUS_OK != mFmReceiver
                    .estimateNoiseFloorLevel(mPendingNflEstimate));
	    if(nflEstimateUpdatePending== false) {
	             frequencyUpdatePending = true;
	    }
        }
    }

    public static StationManage getStationManage(){
    	if(mStationManage!=null){
    		return mStationManage;
    	}
    	return null;
    }

    public boolean seekSearch(int direction){
    	isSeekSearch = true;
    	Log.d(TAG, "tick");
    	boolean success = updateStationSearch(direction);
    	if (success) {
	    	synchronized (mSeekSearchLock) {
                if (!mIsSeekSearchNotified) {
                    Log.d(TAG, "seekSearch() wait for notification");
                    try{
                        mSeekSearchLock.wait();
                    }catch(Exception e){}
                    Log.d(TAG, "seekSearch() end waiting");
                }
                mIsSeekSearchNotified = false;
			}
    	}
    	isSeekSearch = false;
    	return success;
    }

    public int getFrequency(){
    	return mPendingFrequency;
    }

    // save last frequency
    public void setTunedFreq(int freq){
    	if (mPendingFrequency != 0 || mPendingFrequency==freq) {
    		Editor e = mSharedPrefs.edit();
    		e.putInt(lastFreqPreferenceKey, mPendingFrequency);
    		e.apply();
    	}
    }

    public boolean tuneRadio(int freq){
		if (null != mFmReceiver) {
			mPendingFrequency = freq;
			 return (FmReceiver.STATUS_OK == mFmReceiver.tuneRadio(mPendingFrequency));
		}
		return false;
	}

    /**
     * Request broadcast next/previous station
     *
     * @param direction the operation direction can be up or down
     */
    private int updateStation(int direction) {
        if (mStationManage == null) {
            return mPendingFrequency;
        }

        List<PresetStation> stationList = mStationManage.getStations();
        if (stationList.isEmpty()) {
            return mPendingFrequency;
        }

        int freq = mPendingFrequency;
        PresetStation[] dataroom = new PresetStation[1];
        PresetStation[] stations = stationList.toArray(dataroom);
        if ((direction & FmReceiver.SCAN_MODE_UP) == FmReceiver.SCAN_MODE_UP ) {
            if (freq >= stations[stations.length - 1].getFreq()) {
                freq = stations[0].getFreq();
            } else {
                for (int i=0; i<stations.length; ++i) {
                    if (freq < stations[i].getFreq()) {
                        freq = stations[i].getFreq();
                        break;
                    }
                }
            }
        } else {
            if (freq <= stations[0].getFreq()) {
                freq = stations[stations.length - 1].getFreq();
            } else {
                for (int i=stations.length - 1; i>=0; --i) {
                    if (freq > stations[i].getFreq()) {
                        freq = stations[i].getFreq();
                        break;
                    }
                }
            }
        }

        return freq;
    }


    /**
     * Request station search operation and pend if necessary.
     *
     * @param direction
     *            the search direction can be up or down.
     */
    private boolean updateStationSearch(int direction) {
    	boolean failed = true;
    	int status = -1;
        int endFrequency = mPendingFrequency;
        // will restore the pending frequency if failed
        int oldPendingFrequency = mPendingFrequency;
        if (V) {
            Log.d(TAG, "updateStationSearch()");
        }
        /* Extract pending data and request these settings. */
        mPendingSearchDirection = direction;
        if (V) {
            Log.d(TAG, "Sending search direction (" + Integer.toString(mPendingSearchDirection) + ")");
        }

        if (mFmReceiver != null) {
            int rssi = SystemProperties.getInt("ro.fm.rssi", FmReceiver.MIN_SIGNAL_STRENGTH_DEFAULT);
            if(FmConstants.FM_COMBO_SEARCH_ENABLED) {
                if ((mPendingSearchDirection & FmReceiver.SCAN_MODE_UP) == FmReceiver.SCAN_MODE_UP)
                {
                    /* Increase the current listening frequency by one step. */
                    if ((mMinFreq <= mPendingFrequency) && (mPendingFrequency <= mMaxFreq)) {
                            mPendingFrequency = (int)(mPendingFrequency + mFrequencyStep);
                            if (mPendingFrequency > mMaxFreq) mPendingFrequency = mMinFreq;
                    } else {
                        mPendingFrequency = mMinFreq;
                        endFrequency = mMaxFreq;
                    }
                } else {
                    /* Decrease the current listening frequency by one step. */
                    if ((mMinFreq <= mPendingFrequency) && (mPendingFrequency <= mMaxFreq)) {
                            mPendingFrequency = (int)(mPendingFrequency - mFrequencyStep);
                            if (mPendingFrequency < mMinFreq) mPendingFrequency = mMaxFreq;
                    } else {
                        mPendingFrequency = mMaxFreq;
                        endFrequency = mMinFreq;
                    }
                }

                failed = stationSearchUpdatePending = mSeekInProgress =
                    (FmReceiver.STATUS_OK != (status = mFmReceiver.seekStationCombo (
                mPendingFrequency,
                         endFrequency,
                rssi,
                mPendingSearchDirection,
                         mPendingScanMethod,
                FmConstants.COMBO_SEARCH_MULTI_CHANNEL_DEFAULT,
                mPendingRdsType,
                         FmReceiver.RDS_COND_PTY_VAL)));

            } else {

            	failed  = stationSearchUpdatePending  = mSeekInProgress =
                    (FmReceiver.STATUS_OK != (status = mFmReceiver.seekStation(mPendingSearchDirection, rssi)));
            }

            if (failed) {
            	Log.d("TAG","seekStationCombo sataus="+status);
            	if(status == FmReceiver.STATUS_ILLEGAL_COMMAND){
            		mPendingFrequency = oldPendingFrequency;
            	}

            	// disable trying if is in auto search mode. SeekStation will handle it
            	if (isSeekSearch)
            		stationSearchUpdatePending = false;
            }

            if (mSeekInProgress&&!isSeekSearch)
                mView.setSeekStatus(true, false); // no new frequency, just turn on the indicator
        }
        return !failed;
    }

    /**
     * Update the live polling settings from preferences and pend if necessary.
     *
     * @param sp
     *            the shared preferences reference to get settings from.
     */
    private void updateLivePolling(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateLivePolling()");
        }

        /* Extract preferences and request these settings. */
        mPendingLivePoll = sp.getBoolean(FmRadioSettings.FM_PREF_LIVE_POLLING, false);
        mPendingLivePollinterval = Integer.parseInt(sp.getString(FmRadioSettings.FM_PREF_LIVE_POLL_INT, "2000"));
        if (V) {
            Log.d(TAG, "Sending live poll (" + (mPendingLivePoll ? "TRUE" : "FALSE") + ")");
            Log.d(TAG, "Sending live poll interval (" + Integer.toString(mPendingLivePollinterval) + ")");
        }

        if (null != mFmReceiver) {
            livePollingUpdatePending = (FmReceiver.STATUS_OK != mFmReceiver.setLiveAudioPolling(mPendingLivePoll,
                    mPendingLivePollinterval));
        }
    }

    /**
     * Update the SNR Threshold  from preferences and pend if necessary.
     *
     * @param sp
     *            the shared preferences reference to get settings from.
     */
    private void updateSetSNRThreshold(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateSetSNRThreshold()");
        }

        /* Extract preferences and request these settings. */
        mPendingSnrThreshold =Integer.parseInt(sp.getString(FmRadioSettings.FM_PREF_SNR_THRESHOLD, String.valueOf(FmReceiver.FM_MIN_SNR_THRESHOLD)));
        if (V) {
            Log.d(TAG, "Setting SNR Threshold(" + mPendingSnrThreshold+ ")");
        }

        if (null != mFmReceiver) {
            fmSetSNRThresholdPending = (FmReceiver.STATUS_OK != mFmReceiver.setSNRThreshold(mPendingSnrThreshold));
        }
    }

    private void updateMinMaxFrequencies() {
        if ((mPendingRegion == FmReceiver.FUNC_REGION_EUR) || (mPendingRegion == FmReceiver.FUNC_REGION_NA)) {
        	mMinFreq = FmConstants.MIN_FREQUENCY_US_EUROPE;
        	mMaxFreq = FmConstants.MAX_FREQUENCY_US_EUROPE;
        } else if (mPendingRegion == FmReceiver.FUNC_REGION_JP) {
        	mMinFreq = FmConstants.MIN_FREQUENCY_JAPAN;
        	mMaxFreq = FmConstants.MAX_FREQUENCY_JAPAN;
        } else if (mPendingRegion == FmReceiver.FUNC_REGION_JP_II) {
            mMinFreq = FmConstants.MIN_FREQUENCY_JAPAN_II;
            mMaxFreq = FmConstants.MAX_FREQUENCY_JAPAN_II;
        } else {
        	// where are we?
        	return;
        }
        mView.setMinMaxFrequencies(mMinFreq, mMaxFreq);
    }

    /**
     * Execute any pending commands. Only the latest command will be stored.
     */
    private void retryPendingCommands() {
        if (V) {
            Log.d(TAG, "retryPendingCommands()");
        }
        /* Update event chain. */
        if (nflEstimateUpdatePending) {
            updateNflEstimate(mSharedPrefs);
        } else if (audioModeUpdatePending) {
            updateAudioMode(mSharedPrefs);
        } else if (audioPathUpdatePending) {
            updateAudioPath(mSharedPrefs);
        } else if (fmVolumeUpdatepending) {
            updateFMVolume(
                    FmReceiver.convertVolumeFromSystemToBsp(
                          mAudioManager.getStreamVolume(AudioManager.STREAM_FM)
                          , mAudioManager.getStreamMaxVolume(AudioManager.STREAM_FM)
            ));
        } else if (frequencyUpdatePending) {
            updateFrequency(mPendingFrequency);
        } else if (muteUpdatePending) {
            updateMuted(mPendingMute);
        } else if (scanStepUpdatePending) {
            updateScanStep(mSharedPrefs);
        } else if (stationSearchUpdatePending) {
            updateStationSearch(mPendingSearchDirection);
        } else if (rdsModeUpdatePending) {
            updateRdsMode(mSharedPrefs);
        } else if(fmSetSNRThresholdPending) {
            updateSetSNRThreshold(mSharedPrefs);
        } else {
            if (livePollingUpdatePending) {
                updateLivePolling(mSharedPrefs);
            }
            if (worldRegionUpdatePending) {
                updateWorldRegion(mSharedPrefs);
            }
        }
    }

    public void onProxyAvailable(Object ProxyObject) {
         Log.d(TAG, "onProxyAvailable bFinishCalled:"+bFinishCalled);
         if(mFmReceiver==null)
            mFmReceiver = (FmReceiver)ProxyObject;
        if (mFmReceiver == null) {
        	String error = getString(R.string.error_unable_to_get_proxy);
            Log.e(TAG, error);
            displayErrorMessageAndExit(error, true);
            mOperationState = STATE_UNINITIAL;
            return;
        }

        /* Initiate audio startup procedure. */
        if (null != mFmReceiver && mFmReceiverEventHandler==null) {
          mFmReceiverEventHandler = new FmReceiverEventHandler();
          mFmReceiver.registerEventHandler(mFmReceiverEventHandler);
        }
        /* make sure we update frequency display and volume etc upon resume */
        if (!mFmReceiver.getRadioIsOn()) {
            if(mFinish || bFinishCalled) {
                Log.d(TAG, "Finish already initiated here. Hence exiting");
                mOperationState = STATE_UNINITIAL;
                return;
            }
            mPendingFrequency = mSharedPrefs.getInt(lastFreqPreferenceKey, DEFAULT_FREQUENCY);
            powerUpSequence();
        } else {
        	powerupComplete();
            mFmReceiver.getStatus(); // is this even needed?
        }
        mOperationState = STATE_INITIALIZED;
    }


    private void displayErrorMessageAndExit(final String errorMessage, final boolean alreadyShutDown) {
        mMainThreadHandler.post(new Runnable() {
            public void run() {
                if (!mPowerOffRadio) {
                    mPowerOffRadio=true;
                    powerDownSequence(alreadyShutDown);
                    Toast.makeText(FmRadio.this, errorMessage, Toast.LENGTH_SHORT).show();
                    if (!alreadyShutDown) {
                        finish();
                    }
/*                    new AlertDialog.Builder(FmRadio.this)
                                   .setTitle(R.string.app_name)
                                   .setMessage(errorMessage)
                                   .setIcon(android.R.drawable.ic_dialog_alert)
                                   .setCancelable(false)
                                   .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                                       public void onClick(DialogInterface dialog, int which) {
                                           finish();
                                       }
                                   })
                                   .create()
                                   .show();*/
                }
            }
        });
    }

//    @Override
//    public boolean onCreateOptionsMenu(Menu menu) {
//        MenuInflater inflater = getMenuInflater();
//        inflater.inflate(R.menu.fm_rx_menu, menu);
//        return true;
//    }
//
//    @Override
//    public boolean onOptionsItemSelected(MenuItem item) {
//        // Handle item selection
//        switch (item.getItemId()) {
//        case R.id.menu_item_settings:
//        	Intent intent = new Intent();
//        	intent.setClass(FmRadio.this, FmRadioSettings.class);
//        	startActivity(intent);
//            return true;
//        case R.id.menu_item_exit:
//			/* Shutdown system. */
//			if (null != mFmReceiver) {
//				powerDownSequence();
//			}
//			finish();
//			return true;
//        default:
//            return super.onOptionsItemSelected(item);
//        }
//    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
    	if(!isHeadsetUnplug){
    		Intent intent = new Intent();
    		intent.setClass(FmRadio.this, FmRadioSettings.class);
    		startActivity(intent);
    	}

     	return false;
    }

/*    @Override
    public void onCreateContextMenu(ContextMenu menu, View v,
    		ContextMenuInfo menuInfo) {
    	super.onCreateContextMenu(menu, v, menuInfo);
    	mLongPressChannel = v.getId() - FmConstants.BUTTON_CH_1; // 0 - 9
    	if (mLongPressChannel < 0 || mLongPressChannel > 9)
    		return;
    	menu.clear();
    	menu.add(Menu.NONE, MENU_CH_SET, Menu.NONE,
    			getResources().getString(R.string.menu_ch_set, (mLongPressChannel+1), ((double) mPendingFrequency)/100));
    	if (mChannels[mLongPressChannel] != 0)
        	menu.add(Menu.NONE, MENU_CH_CLEAR, Menu.NONE,
        			getResources().getString(R.string.menu_ch_clear, (mLongPressChannel+1)));
    	menu.add(Menu.NONE, MENU_CH_CANCEL, Menu.NONE,
    			getResources().getString(R.string.menu_ch_cancel));

    }*/

    /*private static int mLongPressChannel;
    @Override
    public boolean onContextItemSelected(MenuItem item) {
    	switch (item.getItemId()) {
    	case MENU_CH_SET:
    		setChannel(mLongPressChannel);
    		mView.updateChannelButtons();
    		return true;
    	case MENU_CH_CLEAR:
    		clearChannel(mLongPressChannel);
    		mView.updateChannelButtons();
    		return true;
    	case MENU_CH_CANCEL:
    		return true;
    	default:
    		return super.onContextItemSelected(item);
    	}
    }*/
    /*
     * private void clearFmObjects() {
     * mSharedPrefs.unregisterOnSharedPreferenceChangeListener(this); if ((null
     * != mFmReceiver) && (null != mFmReceiverEventHandler)) { Log.d(TAG,
     * "Call mFmReceiver.unregisterEventHandler(...)");
     * mFmReceiver.unregisterEventHandler(); }
     *
     * if (mFmReceiver != null) { mFmReceiver.finish(); mFmReceiver = null; }
     *
     * }
     */

    /**
     * Keep listening for settings changes that require actions from FM.
     *
     * @param sharedPreferences
     *            the shared preferences.
     * @param key
     *            the key name of the preference that was changed.
     */
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (V) {
            Log.d(TAG, "onSharedPreferenceChanged key " + key +" avoid update ? " + sSuppressReactingWithPreferenceChanged);
        }
        if (sSuppressReactingWithPreferenceChanged)
            return;
        /*
        if (key.equals(FmRadioSettings.FM_PREF_ENABLE)) {
            boolean isRadioOn = mFmReceiver == null? false: mFmReceiver.getRadioIsOn();
            boolean isTurnOnRequest =sharedPreferences.getBoolean(FmRadioSettings.FM_PREF_ENABLE, true);
            if (V) {
                Log.d(TAG,"Turn Radio On/Off: turnOnRequested: " + isTurnOnRequest + ", radioIsOn" + isRadioOn);
            }
            if (isTurnOnRequest && !isRadioOn)  {
                powerUpSequence();
            } else if (!isTurnOnRequest && isRadioOn){
                powerDownSequence();
            }
        } else
        */
        if (key.equals(FmRadioSettings.FM_PREF_AUDIO_MODE)) {
            updateAudioMode(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_AUDIO_PATH)) {
            updateAudioPath(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_SCAN_STEP)) {
            updateScanStep(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_RDS_ENABLED)) {
            updateRdsMode(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_AF_ENABLED)) {
            updateRdsMode(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_WORLD_REGION)) {
            updateWorldRegion(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_DEEMPHASIS)) {
            updateWorldRegion(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_LIVE_POLLING)) {
            updateLivePolling(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_LIVE_POLL_INT)) {
            updateLivePolling(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_NFL_MODE)) {
            updateNflEstimate(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_SNR_THRESHOLD)) {
            updateSetSNRThreshold(sharedPreferences);
        }
    }

    /**
     * This class ensures that only the UI thread access view core.
     */
    protected Handler viewUpdateHandler = new Handler() {

        /**
         * Internal helper function to update the GUI frequency display.
         *
         * @param freq the frequency (multiplied by 100) to cache and display.
         */
        private void updateFrequency(int freq, int isCompletedSeekInt) {
        	Log.d(TAG, "updateFrequency  : " + freq);
        	if (freq < 0)
        		return;

        	boolean isCompletedSeek = (isCompletedSeekInt != 0);
        	/* only update GUI, setting of actual frequency is done outside of this class */
            // fixme -- all viewUpdateHandler functions should ONLY update GUI
            mFrequency = freq;
            // fix bug 5009 start
            // update UI every time
            mView.setFrequencyGraphics(freq);
//            if (isCompletedSeek) {
            // mView.setFrequencyGraphics(freq);
            // fix bug 5009 end
//				mPendingFrequency = mFrequency;
//            }
    		mView.setSeekStatus(mSeekInProgress, (mPendingFrequency != mFrequency));
			mView.resetRdsText();
        }

        public void handleMessage(Message msg) {

        	Log.d(TAG, "handleMessage  : " + msg);
            /*
             * Here it is safe to perform UI view access since this
             * class is running in UI context.
             */

            switch (msg.what) {
                case GUI_UPDATE_MSG_SIGNAL_STATUS: mView.setSignalStrength(msg.arg1); break;
                case GUI_UPDATE_MSG_FREQ_STATUS: updateFrequency(msg.arg1, msg.arg2); break;
                case GUI_UPDATE_MSG_MUTE_STATUS: mView.setMutedState(msg.arg1 == FmConstants.MUTE_STATE_MUTED, mInCall); break;
                case GUI_UPDATE_MSG_RDS_STATUS: mView.setRdsState(msg.arg1); break;
                case GUI_UPDATE_MSG_AF_STATUS: mView.setAfState(msg.arg1); break;
    			case GUI_UPDATE_MSG_RDS_DATA: mView.setRdsText(msg.arg1, msg.arg2, (String)msg.obj); break;
//    			case GUI_UPDATE_MSG_DEBUG:
//    				((TextView)mView.findViewById(msg.arg1)).setText((String)msg.obj);
//    				break;
                case SIGNAL_CHECK_PENDING_EVENTS: retryPendingCommands(); break;
                case UPDATE_NOTIFICATION: FmService.showNotification(FmRadio.this,msg.arg1); break;
                case AUTO_SEARCH:
                {
                    removeMessages(AUTO_SEARCH);
                    if (FmService.isHeadsetPlugged() && !mAutoSearchQueryDone) {
                        mAutoSearchQueryDone = true;
                        try {
                            showDialog(SEARCH_QUERY_DIALOG);
                        } catch (Exception e) {
                            Log.e(TAG, "exception has happened when dialog was shown");
                        }
                    }
                    break;
                }
                case POWER_UP_DONE: onPowerUpDone(msg.arg1); break;
                default: break;
            }
        }
    };

    /**
     * Internal class used to specify FM callback/callout events from
     * the FM Receiver Service subsystem.
     */
    protected class FmReceiverEventHandler implements IFmReceiverEventHandler {

        /**
         * Transfers execution of GUI function to the UI thread.
         *
         * @param rssi the signal strength to display.
         */
        private void displayNewSignalStrength(int rssi) {
        	Log.d(TAG, "displayNewSignalStrength  : " + rssi);
            /* Update signal strength icon. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_SIGNAL_STATUS;
            msg.arg1 = rssi;
            viewUpdateHandler.sendMessage(msg);
        }

        private void displayNewRdsData(int rdsDataType, int rdsIndex,
                String rdsText) {
        	Log.d(TAG, "displayNewRdsData  : " );
            /* Update RDS texts. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_RDS_DATA;
            msg.arg1 = rdsDataType;
            if (rdsDataType == FmConstants.RDS_ID_PTY_EVT) {
            	if (rdsIndex < 0 || rdsIndex >= mRdsProgramTypes.length)
            		return; // invalid index
            	msg.arg2 = rdsIndex;
            	msg.obj =  mRdsProgramTypes[rdsIndex];
            } else {
            	msg.obj = (Object)rdsText;
            }
            viewUpdateHandler.sendMessage(msg);
        }

        /**
         * Transfers execution of GUI function to the UI thread.
         *
         * @param rdsMode the RDS mode to display.
         */
        private void displayNewRdsState(int rdsMode) {
        	Log.d(TAG, "displayNewRdsState  : " + rdsMode);
            /* Update RDS state icon. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_RDS_STATUS;
            msg.arg1 = rdsMode;
            viewUpdateHandler.sendMessage(msg);
        }

        /**
         * Transfers execution of GUI function to the UI thread.
         *
         * @param afMode the AF mode to display.
         */
        private void displayNewAfState(int afMode) {
        	Log.d(TAG, "displayNewAfState  : " );
            /* Update AF state icon. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_AF_STATUS;
            msg.arg1 = afMode;
            viewUpdateHandler.sendMessage(msg);
        }

        /**
         * Transfers execution of GUI function to the UI thread.
         *
         * @param freq the frequency (multiplied by 100) to cache and display.
         */
        private void displayNewFrequency(int freq, int isCompletedSeekInt) {
        	Log.d(TAG, "displayNewFrequency  : " + freq );
            /* Update frequency data. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_FREQ_STATUS;
            msg.arg1 = freq;
            msg.arg2 = isCompletedSeekInt; // nonzero if completed seek
            viewUpdateHandler.sendMessage(msg);
        }

        /**
         * Transfers execution of GUI function to the UI thread.
         *
         * @param isMute TRUE if muted, FALSE if not.
         */
        private void displayNewMutedState(boolean isMute) {

        	Log.d(TAG, "displayNewMutedState  : " + isMute);
            /* Update frequency data. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_MUTE_STATUS;
			msg.arg1 = isMute ? FmConstants.MUTE_STATE_MUTED : FmConstants.MUTE_STATE_UNMUTED;
			viewUpdateHandler.sendMessage(msg);
        }


        public void onAudioModeEvent(int audioMode) {
            if (V) {
                Log.v(TAG,"onAudioModeEvent("+audioMode+")" );
            }
            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }


        public void onAudioPathEvent(int audioPath) {
            if (V) {
                Log.v(TAG,"onAudioPathEvent("+audioPath+")" );
            }
        	/* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onEstimateNoiseFloorLevelEvent(int nfl) {
            if (V) {
                Log.v(TAG,"onEstimateNoiseFloorLevelEvent("+nfl+")" );
            }
        	/* Local cache only! Not currently used directly. */
            mNfl = nfl;
            /* Update GUI display variables. */
            mView.LOW_SIGNAL_STRENGTH = nfl;
            mView.MEDIUM_SIGNAL_STRENGTH = nfl - 15;
            mView.HIGH_SIGNAL_STRENGTH = nfl - 25;
            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }


        public void onLiveAudioQualityEvent(int rssi, int snr) {
            if (V) {
                Log.v(TAG,"onLiveAudioQualityEvent("+rssi+", "+snr+" )" );
            }
            /* Update signal strength icon. */
            displayNewSignalStrength(rssi);

            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onRdsDataEvent(int rdsDataType, int rdsIndex,
                String rdsText) {
            if (V) {
                Log.v(TAG,"onRdsDataEvent("+rdsDataType + ", " + rdsIndex+")" );
            }

            /* Update GUI text. */
            displayNewRdsData(rdsDataType, rdsIndex, rdsText);

        	/* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onRdsModeEvent(int rdsMode, int alternateFreqHopEnabled) {
            if (V) {
                Log.v(TAG,"onRdsModeEvent("+rdsMode + ", " + alternateFreqHopEnabled+")" );
            }

        	/* Update signal strength icon. */
            displayNewRdsState(rdsMode);

            /* Update mute status. */
            displayNewAfState(alternateFreqHopEnabled);

        	/* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onSeekCompleteEvent(int freq, int rssi,
                int snr, boolean seeksuccess) {
        	Log.d(TAG, "Tock" + freq);
            if (V) {
                Log.v(TAG,"onSeekCompleteEvent("+freq + ", " + rssi +", "+ snr +", "+ seeksuccess +")" );
            }
            mSeekInProgress = false;
            mFrequency = freq;

            mPendingFrequency = freq;
            /* Update frequency display. */
            displayNewFrequency(freq, 1);
            /* Update signal strength icon. */
            displayNewSignalStrength(rssi);

            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
            msg = viewUpdateHandler.obtainMessage(UPDATE_NOTIFICATION);
            msg.arg1 = freq;
            viewUpdateHandler.sendMessage(msg);

            synchronized (mSeekSearchLock) {
        		try{
        			mSeekSearchLock.notify();
        		}catch(Exception e){}
                mIsSeekSearchNotified = true;
                Log.d(TAG, "onSeekCompleteEvent() notify");
    		}

//			Message msg0 = Message.obtain();
//			msg0.what = GUI_UPDATE_MSG_DEBUG;
//			msg0.arg1 = R.id.debugview1;
//			msg0.obj = "Seek: f="+freq+" rssi="+rssi+" s="+seeksuccess + " update="+updateCount++;
//			viewUpdateHandler.sendMessage(msg0);
        }

        public void onStatusEvent(int freq, int rssi, int snr, boolean radioIsOn,
                int rdsProgramType, String rdsProgramService,
                String rdsRadioText, String rdsProgramTypeName, boolean isMute) {
            if (V) {
                Log.v(TAG,"onStatusEvent("+freq + ", " + rssi +", "+snr+", "
                    + radioIsOn +", " + rdsProgramType + ", " +rdsProgramService
                    +", "+ rdsRadioText +", "+rdsProgramTypeName + ", " + isMute +")" );
            }

           if(mPowerOffRadio && !radioIsOn){
             finish();
           }

            if (freq < mMinFreq || freq > mMaxFreq) {
                freq = mPendingFrequency;
            }

//			String debug =
//				"freq: " + freq + "\n "+
//				"rssi: " + rssi + "\n "+
//				"radioIsOn: " + radioIsOn + "\n "+
//				"rdsProgramType/name: " + rdsProgramType + " " + rdsProgramTypeName + "\n "+
//				"rdsProgramService: " + rdsProgramService + "\n "+
//				"rdsRadioText: " + rdsRadioText + "\n "+
//				"isMute: " + isMute + "\n" +
//				"update: " +updateCount++;
//
//			Message msg0 = Message.obtain();
//			msg0.what = GUI_UPDATE_MSG_DEBUG;
//			msg0.arg1 = R.id.debugview2;
//			msg0.obj = debug;
//			viewUpdateHandler.sendMessage(msg0);

            if (mFrequency != freq) {
                /* Update frequency display. */
                displayNewFrequency(freq, 0);

                /* Update signal strength icon. */
                displayNewSignalStrength(rssi);
            }

            /* Update mute status. */
            displayNewMutedState(isMute);

            /* Update GUI with RDS material if available. */
            //mRadioView.setRdsData(rdsProgramService, rdsRadioText, rdsProgramTypeName);

            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onWorldRegionEvent(int worldRegion) {
            if (V) {
                Log.v(TAG,"onWorldRegionEvent("+worldRegion + ")");
            }

        	/* Local cache. */
        	mWorldRegion = worldRegion;
        	/* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onVolumeEvent(int status, int volume) {
            if (V) {
                Log.v(TAG,"onVolumeEvent("+status + ", " + volume + ")");
            }

            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }
    }


    /* manages frequency wrap around and scan step */
    private void buttonSeekFrequencyUp() {
        Log.v(TAG, "buttonSeekFrequencyUp: region " + mPendingRegion);

        mSeekInProgress = false;
        mPendingFrequency = updateStation(FmReceiver.SCAN_MODE_UP);
        /* Increase the current listening frequency by one step. */
        if (mPendingFrequency == 0) {
            updateFrequency(DEFAULT_FREQUENCY);
        } else {
/*            if (mPendingFrequency < mMaxFreq) {
                if(mPendingFrequency%FmConstants.SCAN_STEP_100KHZ != 0)
                	mPendingFrequency = (int)(mPendingFrequency + FmConstants.SCAN_STEP_50KHZ);
                else
                	mPendingFrequency = (int)(mPendingFrequency + mFrequencyStep);
            } else {
            	mPendingFrequency = mMinFreq;
            }*/
			updateFrequency(mPendingFrequency);
        }
    }


    /* manages frequency wrap around and scan step */
    private void buttonSeekFrequencyDown() {
    	Log.v(TAG, "buttonSeekFrequencyDown region: " + mPendingRegion);

        mSeekInProgress = false;
        mPendingFrequency = updateStation(FmReceiver.SCAN_MODE_DOWN);
    	/* Decrease the current listening frequency by one step. */
    	if (mPendingFrequency == 0) {
    		updateFrequency(DEFAULT_FREQUENCY);
    	} else {
/*			if (mPendingFrequency > mMinFreq) {
				if(mPendingFrequency%FmConstants.SCAN_STEP_100KHZ !=0 )
					mPendingFrequency = (int)(mPendingFrequency - FmConstants.SCAN_STEP_50KHZ);
				else
					mPendingFrequency = (int)(mPendingFrequency - mFrequencyStep);
			} else {
				mPendingFrequency = mMaxFreq;
			}*/
			updateFrequency(mPendingFrequency);
    	}
    }

	public void handleButtonEvent(int buttonId, int event) {
		/*Perform the functionality linked to the activated GUI release. */
		/* For each button, perform the requested action. */
		switch (buttonId) {
		case FmConstants.BUTTON_POWER_OFF:
			/* Shutdown system. */
			if (mFmReceiver != null && mFmReceiver.getRadioIsOn()==true) {
				mPowerOffRadio=true;
				powerDownSequence(false);
			}
			break;

		case FmConstants.BUTTON_MUTE_ON:
			updateMuted(true);
			break;

		case FmConstants.BUTTON_MUTE_OFF:
			if (updateMuted(false) == FmReceiver.STATUS_AUDIO_FUCOS_FAILED) {
	            new AlertDialog.Builder(this)
	            .setTitle(R.string.app_name)
	            .setIcon(android.R.drawable.ic_dialog_alert)
	            .setMessage(R.string.error_request_audio_focus_faild)
	            .setCancelable(false)
	            .setPositiveButton(android.R.string.ok, null)
	            .create()
	            .show();
			}
			break;

		case FmConstants.BUTTON_TUNE_DOWN:
			/* Scan downwards to next station. Let the FM server determine NFL for us. */
			updateStationSearch(FmReceiver.SCAN_MODE_DOWN);
			break;

		case FmConstants.BUTTON_TUNE_UP:
			/* Scan upwards to next station. Let the FM server determine NFL for us.*/
			updateStationSearch(FmReceiver.SCAN_MODE_UP);
			break;

		case FmConstants.BUTTON_SEEK_DOWN:
			/* Decrease the current listening frequency by one step. */
			buttonSeekFrequencyDown();
			break;

		case FmConstants.BUTTON_SEEK_UP:
			/* Increase the current listening frequency by one step. */
			buttonSeekFrequencyUp();
			break;

		case FmConstants.BUTTON_SETTINGS:
			//openOptionsMenu();
        	Intent intent = new Intent();
        	intent.setClass(FmRadio.this, FmRadioSettings.class);
        	startActivity(intent);
			break;
		case FmConstants.BUTTON_RADIO_LIST:
			Intent intentList = new Intent();
			intentList.setClass(FmRadio.this, StationList.class);
			startActivity(intentList);
			break;
		case FmConstants.BUTTON_ADD_RADIO:
			showDialog(ADD_DIALOG);
			break;
		case FmConstants.BUTTON_AUTO_SEARCH:
            {
                if (mStationManage != null) {
                    List<PresetStation> stations = mStationManage.getStations();
                    if (!stations.isEmpty()) {
                        showDialog(SEARCH_DIALOG);
                        break;
                    }
                }

                Intent req = new Intent(FmRadio.this, SeekStation.class);
                startActivity(req);
            }
			break;
		default:
			break;
		}
	}

	public int[] getChannels() {
		return mChannels;
	}

	public void setChannel(int position) {
		mChannels[position] = mPendingFrequency;
		Editor e = mSharedPrefs.edit();
		e.putInt(freqPreferenceKey + position, mPendingFrequency);
		e.apply();
	}

	public void clearChannel(int position) {
		mChannels[position] = 0;
		Editor e = mSharedPrefs.edit();
		e.putInt(freqPreferenceKey + position, 0);
		e.apply();
	}

	public void selectChannel(int position) {
		mSeekInProgress = false;
		updateFrequency(mChannels[position]);
	}

	// callback from frequency slider
	public void setFrequency(int freq) {
		mSeekInProgress = false;
		updateFrequency(freq);
	}

	private void wiredHeadsetIsOn(boolean isOn) {
		// TODO: fancy warning dialog
		View v = mView.findViewById(R.id.plug_headset_warning);
//		v.setVisibility(isOn ? View.GONE : View.VISIBLE);
		if(isOn){
			v.setVisibility(View.GONE);
			mView.setEnabled(true);
			isHeadsetUnplug = false;
		}else{
			v.setVisibility(View.VISIBLE);
			mView.setEnabled(false);
			isHeadsetUnplug = true;
		}
	}

    public class HeadsetPlugUnplugBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equalsIgnoreCase(Intent.ACTION_HEADSET_PLUG)) {
                int state = intent.getIntExtra("state", 0);
                isHeadsetUnplug = state == 0;
                if (!isHeadsetUnplug) {
                    if (!mAutoSearchQueryDone) {
                        viewUpdateHandler.sendMessage(viewUpdateHandler.obtainMessage(AUTO_SEARCH));
                    }
                }
                FmRadio.this.wiredHeadsetIsOn(state != 0);
            } else if (intent.getAction().equalsIgnoreCase(
                    AudioManager.ACTION_AUDIO_BECOMING_NOISY)) {
                FmRadio.this.wiredHeadsetIsOn(false);
                isHeadsetUnplug = true;
            }

            if (isHeadsetUnplug) {
                try {
                    if (mAutoSearchQueryDone) {
                        dismissDialog(SEARCH_QUERY_DIALOG);
                        if (mStationManage != null) {
                            List<PresetStation> stations = mStationManage.getStations();
                            if (stations.isEmpty()) {
                                mAutoSearchQueryDone = false;
                            }
                        }
                    }
                } catch (Exception e) {
                }
            }
        }
    }
}
