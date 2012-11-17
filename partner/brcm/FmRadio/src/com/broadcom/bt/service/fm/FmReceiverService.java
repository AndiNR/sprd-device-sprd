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

import com.broadcom.bt.service.fm.IFmReceiverCallback;
import com.broadcom.bt.service.framework.BaseService;
import com.broadcom.bt.service.framework.LocalCallbackList;
import com.broadcom.bt.service.PowerManager;

import android.os.Handler;
import android.os.Message;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.util.Log;
import android.content.Context;
import android.content.Intent;
import android.media.AudioSystem;
import android.media.AudioManager;

/**
 * /** Provides a service for accessing and altering FM receiver hardware
 * settings. Requests should be made via proxy to this server and once the
 * requests have been processed, event(s) will be issued with the result of the
 * request execution. Also provides the callback event loop.
 *
 * @hide
 */
public final class FmReceiverService extends BaseService {
    private static final boolean V = FmReceiverServiceConfig.V;


    private static final String TAG = "FmReceiverService";


    /* The list of all registered client callbacks. */
    private final RemoteCallbackList<IFmReceiverCallback> m_callbacks;

    // some gets and sets
    public boolean getRadioIsOn() {
        return FmReceiverServiceState.mRadioIsOn;
    }

	/**
     * Returns the FM Audio Mode state.
     * @param none
     * @return {@link FmReceiver#AUDIO_MODE_AUTO},
     *            {@link FmReceiver#AUDIO_MODE_STEREO},
     *            {@link FmReceiver#AUDIO_MODE_MONO} or
     *            {@link FmReceiver#AUDIO_MODE_BLEND}.
     */
    public int getMonoStereoMode() {
        return FmReceiverServiceState.mAudioMode;
    }

	/**
     * Returns the present tuned FM Frequency
     * @param none
     * @return Tuned frequency
     */
    public int getTunedFrequency() {
        return FmReceiverServiceState.mFreq;
    }

	/**
     * Returns whether MUTE is turned ON or OFF
     * @param none
     * @return false if MUTE is OFF ; true otherwise
     */
    public boolean getIsMute() {
        return FmReceiverServiceState.mIsMute;
    }

    public void registerCallback(IFmReceiverCallback cb) throws RemoteException {
        if (cb != null) {
            m_callbacks.register(cb);
        }
    }

    public synchronized void unregisterCallback(IFmReceiverCallback cb) throws RemoteException {
        if (cb != null) {
            m_callbacks.unregister(cb);
        }
    }

    static {
        //System.loadLibrary("btld");
        System.loadLibrary("fmservice");
        classFmInitNative();
    }

    private native static void classFmInitNative();

    public FmReceiverService(Context context) {
        super(context);
        if (FmReceiverServiceConfig.IS_LOCAL_SVC) {
            m_callbacks = new LocalCallbackList<IFmReceiverCallback>();
        } else {
            m_callbacks = new RemoteCallbackList<IFmReceiverCallback>();
        }
    }

    protected void finalize() throws Throwable {
        stopFM_Loop();
        super.finalize();
    }

    public synchronized void start() {
        if (V) Log.d(TAG, "start");
        if (mIsStarted) {
            Log.d(TAG,"Service already started...Skipping");
            return;
        }
        startFM_Loop();
        //
        // turn the radio on in FM service if it
        // is not on already
        //
        //turnOnRadio(FmReceiverServiceState.mFuncMask);
        onStateChangeEvent(true);
    }

    public synchronized void stop() {
        if (V) Log.d(TAG, "stop");
        if (!mIsStarted) {
            Log.d(TAG,"Service already stopped...Skipping");
            return;
        }
        stopFM_Loop();
        //turnOffRadio();
        onStateChangeEvent(false);
    }

    public void startFM_Loop() {
        Log.d(TAG, "startFM_Loop()");
        initLoopNative();
        initNativeDataNative();
    }

    private native void initLoopNative();

    public void stopFM_Loop() {
        Log.d(TAG, "stopFM_Loop()");

        cleanupLoopNative();
    }

    private native void cleanupLoopNative();

    public String getName() {
        if (V) Log.d(TAG, "getName");
        return FmReceiver.SERVICE_NAME;
    }

    private void initializeStateMachine() {
        FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_OFF;
        FmReceiverServiceState.mFreq = 0;
        FmReceiverServiceState.mRssi = 127;
        FmReceiverServiceState.mRadioIsOn = false;
        FmReceiverServiceState.mRdsProgramType = 0;
        FmReceiverServiceState.mRdsProgramService = "";
        FmReceiverServiceState.mRdsRadioText = "";
        FmReceiverServiceState.mRdsProgramTypeName = "";
        FmReceiverServiceState.mIsMute = false;
        FmReceiverServiceState.mSeekSuccess = false;
        FmReceiverServiceState.mRdsOn = false;
        FmReceiverServiceState.mAfOn = false;
        FmReceiverServiceState.mRdsType = 0; // RDS
        FmReceiverServiceState.mAlternateFreqHopThreshold = 0;
        FmReceiverServiceState.mAudioMode = FmReceiver.AUDIO_MODE_AUTO;
        FmReceiverServiceState.mAudioPath = FmReceiver.AUDIO_PATH_SPEAKER;
        FmReceiverServiceState.mWorldRegion = FmReceiver.FUNC_REGION_DEFAULT;
        FmReceiverServiceState.mStepSize = FmReceiver.FREQ_STEP_DEFAULT;
        FmReceiverServiceState.mLiveAudioQuality = FmReceiver.LIVE_AUDIO_QUALITY_DEFAULT;
        FmReceiverServiceState.mEstimatedNoiseFloorLevel = FmReceiver.NFL_DEFAULT;
        FmReceiverServiceState.mSignalPollInterval = FmReceiver.SIGNAL_POLL_INTERVAL_DEFAULT;
        FmReceiverServiceState.mDeemphasisTime = FmReceiver.DEEMPHASIS_TIME_DEFAULT;
    }

    /**
     * Sends the contents typically supplied by a Status Event to all registered
     * callbacks.
     *
     * @param freq
     *            the current listening frequency.
     * @param rssi
     *            the RSSI of the current frequency.
     * @param radioIsOn
     *            TRUE if the radio is on and FALSE if off.
     * @param rdsProgramType
     *            integer representation of program type.
     * @param rdsProgramService
     *            name of the service.
     * @param rdsRadioText
     *            text of the current program/service.
     * @param rdsProgramTypeName
     *            string version of the rdsProgramType parameter.
     * @param isMute
     *            TRUE if muted by hardware and FALSE if not.
     */
    private void sendStatusEventCallback(int freq, int rssi, int snr, boolean radioIsOn, int rdsProgramType,
            String rdsProgramService, String rdsRadioText, String rdsProgramTypeName, boolean isMute) {
        if (V) {
            if (V) Log.d(TAG, "sendStatusEventCallback");
        }

        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            Intent i = new Intent(FmReceiver.ACTION_ON_STATUS);
            i.putExtra(FmReceiver.EXTRA_FREQ, freq);
            i.putExtra(FmReceiver.EXTRA_RSSI, rssi);
            i.putExtra(FmReceiver.EXTRA_SNR, snr);
            i.putExtra(FmReceiver.EXTRA_RADIO_ON, radioIsOn);
            i.putExtra(FmReceiver.EXTRA_MUTED, isMute);
            i.putExtra(FmReceiver.EXTRA_RDS_PRGM_TYPE, rdsProgramType);
            i.putExtra(FmReceiver.EXTRA_RDS_PRGM_TYPE_NAME, rdsProgramTypeName);
            i.putExtra(FmReceiver.EXTRA_RDS_PRGM_SVC, rdsProgramService);
            i.putExtra(FmReceiver.EXTRA_RDS_TXT, rdsRadioText);
            mContext.sendOrderedBroadcast(i, FmReceiver.FM_RECEIVER_PERM);
        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {
            final int callbacks = m_callbacks.beginBroadcast();
            for (int i = 0; i < callbacks; i++) {
                try {
                    /* Send the callback to each registered receiver. */
                    m_callbacks.getBroadcastItem(i)
                            .onStatusEvent(freq, rssi, snr, radioIsOn, rdsProgramType,
                                rdsProgramService, rdsRadioText, rdsProgramTypeName, isMute);
                } catch (Throwable t) {
                    Log.e(TAG, "sendStatusEventCallback", t);
                }
            }
            m_callbacks.finishBroadcast();
        }
    }


    /**
     * This class handles operation lock timeouts.
     */
    public Handler operationHandler = new Handler() {

        public void handleMessage(Message msg) {
            /* Check if the current operation can be rescued. */
            switch (msg.what) {
                case FmReceiverServiceState.OPERATION_TIMEOUT:
                    /* Currently assume that any timeout is catastrophic. */
                    Log.w(TAG, "BTAPP TIMEOUT (" + msg.what + "," + msg.arg1 + ")");
                    /* Could not start radio. Reset state machine. */
                    if (msg.arg1 == FmReceiverServiceState.OPERATION_DISABLE) {
                        initializeStateMachine();
                        try {
                            Log.w(TAG, "retry shut down!");
                            disableNative(true);
                        } catch (Exception e) {
                        }
                        FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_OFF;
                    } else
                        /* Send status update to client. */
                        FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
                    sendStatusEventCallbackFromLocalStore();
                    break;
                /* Add command queue here if necessary. */
                default:
                    break;
            }
        }
    };

    /*
     * Callback generator section.
     */

    /**
     * Sends event data from the local cache to all registered callbacks.
     */
    public void sendStatusEventCallbackFromLocalStore() {
        if (V) Log.d(TAG, "sendStatusEventCallbackFromLocalStore");

        sendStatusEventCallback(FmReceiverServiceState.mFreq,
            FmReceiverServiceState.mRssi, FmReceiverServiceState.mSnr,
            FmReceiverServiceState.mRadioIsOn, FmReceiverServiceState.mRdsProgramType,
            FmReceiverServiceState.mRdsProgramService, FmReceiverServiceState.mRdsRadioText,
            FmReceiverServiceState.mRdsProgramTypeName, FmReceiverServiceState.mIsMute);
    }

    /**
     * Sends event data from the local cache to all registered callbacks.
     */
    private void sendSeekCompleteEventCallbackFromLocalStore() {
        if (V) Log.d(TAG, "sendSeekCompleteEventCallbackFromLocalStore");
        sendSeekCompleteEventCallback(FmReceiverServiceState.mFreq, FmReceiverServiceState.mRssi,
            FmReceiverServiceState.mSnr, FmReceiverServiceState.mSeekSuccess);
    }

    /**
     * Sends the contents of a seek complete event to all registered callbacks.
     *
     * @param freq
     *            The station frequency if located or the last seek frequency if
     *            not successful.
     * @param rssi
     *            the RSSI at the current frequency.
     * @param snr
     *            the SNR value at the current frequency.
     * @param seekSuccess
     *            TRUE if search was successful, false otherwise.
     */
    private void sendSeekCompleteEventCallback(int freq, int rssi, int snr, boolean seekSuccess) {
        if (V) {
            if (V) Log.d(TAG, "sendSeekCompleteEventCallback");
        }

        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            Intent i = new Intent(FmReceiver.ACTION_ON_SEEK_CMPL);
            i.putExtra(FmReceiver.EXTRA_FREQ, freq);
            i.putExtra(FmReceiver.EXTRA_RSSI, rssi);
            i.putExtra(FmReceiver.EXTRA_SNR, snr);
            i.putExtra(FmReceiver.EXTRA_SUCCESS, seekSuccess);
            mContext.sendOrderedBroadcast(i, FmReceiver.FM_RECEIVER_PERM);
        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {

            final int callbacks = m_callbacks.beginBroadcast();
            for (int i = 0; i < callbacks; i++) {
                try {
                    /* Send the callback to each registered receiver. */
                    m_callbacks.getBroadcastItem(i).onSeekCompleteEvent(freq, rssi, snr, seekSuccess);
                } catch (Throwable t) {
                    Log.e(TAG, "sendSeekCompleteEventCallback", t);
                }
            }
            m_callbacks.finishBroadcast();
        }
    }

    /**
     * Sends event data from the local cache to all registered callbacks.
     */
    private void sendRdsModeEventCallbackFromLocalStore() {
        if (V) Log.d(TAG, "sendRdsModeEventCallbackFromLocalStore");
        int rds = 0;
        int af = FmReceiverServiceState.mAfOn ? 1 : 0;
        /* Adapt for RDS vs RBDS. */
        if (FmReceiverServiceState.mRdsOn) {
            rds = (FmReceiverServiceState.mRdsType == 0) ? 1 : 2;
        }
        sendRdsModeEventCallback(rds, af);
    }

    /**
     * Sends the contents of an RDS mode event to all registered callbacks.
     *
     * @param rdsMode
     *            the current RDS mode
     * @param alternateFreqHopEnabled
     *            TRUE if AF is enabled, false otherwise.
     */
    private void sendRdsModeEventCallback(int rdsMode, int alternateFreqMode) {
        if (V) {
            if (V) Log.d(TAG, "sendRdsModeEventCallback");
        }

        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            Intent i = new Intent(FmReceiver.ACTION_ON_RDS_MODE);
            i.putExtra(FmReceiver.EXTRA_RDS_MODE, rdsMode);
            i.putExtra(FmReceiver.EXTRA_ALT_FREQ_MODE, alternateFreqMode);
            mContext.sendOrderedBroadcast(i, FmReceiver.FM_RECEIVER_PERM);

        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {

            final int callbacks = m_callbacks.beginBroadcast();
            for (int i = 0; i < callbacks; i++) {
                try {
                    /* Send the callback to each registered receiver. */
                    m_callbacks.getBroadcastItem(i).onRdsModeEvent(rdsMode, alternateFreqMode);
                } catch (Throwable t) {
                    Log.e(TAG, "sendRdsModeEventCallback", t);
                }
            }
            m_callbacks.finishBroadcast();
        }
    }

    /**
     * Sends event data from the local cache to all registered callbacks.
     */
    private void sendRdsDataEventCallbackFromLocalStore() {
        sendRdsDataEventCallback(1, 2, "TEST RDS MESSAGE");
    }

    /**
     * Sends the contents of an RDS mode event to all registered callbacks.
     *
     * @param rdsMode
     *            the current RDS mode
     * @param alternateFreqHopEnabled
     *            TRUE if AF is enabled, false otherwise.
     */
    private void sendRdsDataEventCallback(int rdsDataType, int rdsIndex, String rdsText) {
        if (V) {
            if (V) Log.d(TAG, "sendRdsDataEventCallback");
        }
        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            Intent i = new Intent(FmReceiver.ACTION_ON_RDS_DATA);
            i.putExtra(FmReceiver.EXTRA_RDS_DATA_TYPE, rdsDataType);
            i.putExtra(FmReceiver.EXTRA_RDS_IDX, rdsIndex);
            i.putExtra(FmReceiver.EXTRA_RDS_TXT, rdsIndex);
            mContext.sendOrderedBroadcast(i, FmReceiver.FM_RECEIVER_PERM);
        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {
            final int callbacks = m_callbacks.beginBroadcast();
            for (int i = 0; i < callbacks; i++) {
                try {
                    /* Send the callback to each registered receiver. */
                    m_callbacks.getBroadcastItem(i).onRdsDataEvent(rdsDataType, rdsIndex, rdsText);
                } catch (Throwable t) {
                    Log.e(TAG, "sendRdsModeEventCallback", t);
                }
            }
            m_callbacks.finishBroadcast();
        }
    }

    /**
     * Sends event data from the local cache to all registered callbacks.
     */
    private void sendAudioModeEventCallbackFromLocalStore() {
        if (V) Log.d(TAG, "sendAudioModeEventCallbackFromLocalStore");
        sendAudioModeEventCallback(FmReceiverServiceState.mAudioMode);
    }

    /**
     * Sends the contents of an audio mode event to all registered callbacks.
     *
     * @param audioMode
     *            the current audio mode in use.
     */
    private void sendAudioModeEventCallback(int audioMode) {
        if (V) {
            if (V) Log.d(TAG, "sendAudioModeEventCallback");
        }
        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            Intent i = new Intent(FmReceiver.ACTION_ON_AUDIO_MODE);
            i.putExtra(FmReceiver.EXTRA_AUDIO_MODE, audioMode);
            mContext.sendOrderedBroadcast(i, FmReceiver.FM_RECEIVER_PERM);
        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {
            final int callbacks = m_callbacks.beginBroadcast();
            for (int i = 0; i < callbacks; i++) {
                try {
                    /* Send the callback to each registered receiver. */
                    m_callbacks.getBroadcastItem(i).onAudioModeEvent(audioMode);
                } catch (Throwable t) {
                    Log.e(TAG, "sendAudioModeEventCallback", t);
                }
            }
            m_callbacks.finishBroadcast();
        }
    }

    /**
     * Sends event data from the local cache to all registered callbacks.
     */
    private void sendAudioPathEventCallbackFromLocalStore() {
        if (V) Log.d(TAG, "sendAudioPathEventCallbackFromLocalStore");
        sendAudioPathEventCallback(FmReceiverServiceState.mAudioPath);
    }

    /**
     * Sends the contents of an audio path event to all registered callbacks.
     *
     * @param audioPath
     *            the current audio mode in use.
     */
    private void sendAudioPathEventCallback(int audioPath) {
        if (V) {
            if (V) Log.d(TAG, "sendAudioPathEventCallback");
        }

        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            Intent i = new Intent(FmReceiver.ACTION_ON_AUDIO_PATH);
            i.putExtra(FmReceiver.EXTRA_AUDIO_PATH, audioPath);
            mContext.sendOrderedBroadcast(i, FmReceiver.FM_RECEIVER_PERM);
        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {
            final int callbacks = m_callbacks.beginBroadcast();
            for (int i = 0; i < callbacks; i++) {
                try {
                    /* Send the callback to each registered receiver. */
                    m_callbacks.getBroadcastItem(i).onAudioPathEvent(audioPath);
                } catch (Throwable t) {
                    Log.e(TAG, "sendAudioPathEventCallback", t);
                }
            }
            m_callbacks.finishBroadcast();
        }
    }

    /**
     * Sends event data from the local cache to all registered callbacks.
     */
    private void sendWorldRegionEventCallbackFromLocalStore() {
        if (V) Log.d(TAG, "sendWorldRegionEventCallbackFromLocalStore");
        sendWorldRegionEventCallback(FmReceiverServiceState.mWorldRegion);
    }

    /**
     * Sends the contents of a world region event to all registered callbacks.
     *
     * @param worldRegion
     *            the current frequency band region in use.
     */
    private void sendWorldRegionEventCallback(int worldRegion) {
        if (V) Log.d(TAG, "sendWorldRegionEventCallback");
        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            Intent i = new Intent(FmReceiver.ACTION_ON_WRLD_RGN);
            i.putExtra(FmReceiver.EXTRA_WRLD_RGN, worldRegion);
            mContext.sendOrderedBroadcast(i, FmReceiver.FM_RECEIVER_PERM);

        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {
            final int callbacks = m_callbacks.beginBroadcast();
            for (int i = 0; i < callbacks; i++) {
                try {
                    /* Send the callback to each registered receiver. */
                    m_callbacks.getBroadcastItem(i).onWorldRegionEvent(worldRegion);
                } catch (Throwable t) {
                    Log.e(TAG, "sendWorldRegionEventCallback", t);
                }
            }
            m_callbacks.finishBroadcast();
        }
    }

    /**
     * Sends event data from the local cache to all registered callbacks.
     */
    private void sendEstimateNflEventCallbackFromLocalStore() {
        if (V) Log.d(TAG, "sendEstimateNflEventCallbackFromLocalStore");
        sendEstimateNflEventCallback(FmReceiverServiceState.mEstimatedNoiseFloorLevel);
    }

    /**
     * Sends the contents of a world region event to all registered callbacks.
     *
     * @param worldRegion
     *            the current frequency band region in use.
     */
    private void sendEstimateNflEventCallback(int nfl) {
        if (V) Log.d(TAG, "sendEstimateNflEventCallback");
        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            Intent i = new Intent(FmReceiver.ACTION_ON_EST_NFL);
            i.putExtra(FmReceiver.EXTRA_NFL, nfl);
            mContext.sendOrderedBroadcast(i, FmReceiver.FM_RECEIVER_PERM);

        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {
            final int callbacks = m_callbacks.beginBroadcast();
            for (int i = 0; i < callbacks; i++) {
                try {
                    /* Send the callback to each registered receiver. */
                    m_callbacks.getBroadcastItem(i).onEstimateNflEvent(nfl);
                } catch (Throwable t) {
                    Log.e(TAG, "sendEstimateNflEventCallback", t);
                }
            }
            m_callbacks.finishBroadcast();
        }
    }

    /**
     * Sends event data from the local cache to all registered callbacks.
     */
    private void sendLiveAudioQualityEventCallbackFromLocalStore() {
        if (V) Log.d(TAG, "sendLiveAudioQualityEventCallbackFromLocalStore");
        sendLiveAudioQualityEventCallback(FmReceiverServiceState.mRssi, FmReceiverServiceState.mSnr);
    }

    /**
     * Sends the contents of a world region event to all registered callbacks.
     *
     * @param worldRegion
     *            the current frequency band region in use.
     */
    private void sendLiveAudioQualityEventCallback(int rssi, int snr) {
        if (V) {
            if (V) Log.d(TAG, "sendLiveAudioQualityEventCallback rssi:"+rssi+", snr:"+snr);
        }
        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            Intent i = new Intent(FmReceiver.ACTION_ON_AUDIO_QUAL);
            i.putExtra(FmReceiver.EXTRA_RSSI, rssi);
            i.putExtra(FmReceiver.EXTRA_SNR, snr);
            mContext.sendOrderedBroadcast(i, FmReceiver.FM_RECEIVER_PERM);

        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {
            final int callbacks = m_callbacks.beginBroadcast();
            for (int i = 0; i < callbacks; i++) {
                try {
                    /* Send the callback to each registered receiver. */
                    m_callbacks.getBroadcastItem(i).onLiveAudioQualityEvent(rssi, snr);
                } catch (Throwable t) {
                    Log.e(TAG, "sendLiveAudioQualityEventCallback", t);
                }
            }
            m_callbacks.finishBroadcast();
            if (V) {
                if (V) Log.d(TAG, "radio_state = " + Integer.toString(FmReceiverServiceState.radio_state));
            }
        }
    }

    /**
     * Sends the contents of a FM volume event to all registered callbacks.
     *
     * @param status
     *            equal to 0 if successful. Otherwise returns a non-zero error
     *            code.
     * @param volume
     *            range from 0 to 255
     */
    private void sendVolumeEventCallback(int status, int volume) {
        if (V) {
            if (V) Log.d(TAG, "sendVolumeEventCallback");
        }

        if (FmReceiverServiceConfig.USE_BROADCAST_INTENTS == true) {
            Intent i = new Intent(FmReceiver.ACTION_ON_VOL);
            i.putExtra(FmReceiver.EXTRA_STATUS, status);
            i.putExtra(FmReceiver.EXTRA_VOL, volume);
            mContext.sendOrderedBroadcast(i, FmReceiver.FM_RECEIVER_PERM);

        } else if (FmReceiverServiceConfig.USE_CALLBACKS == true) {
            final int callbacks = m_callbacks.beginBroadcast();
            for (int i = 0; i < callbacks; i++) {
                try {
                    /* Send the callback to each registered receiver. */
                    m_callbacks.getBroadcastItem(i).onVolumeEvent(status, volume);
                } catch (Throwable t) {
                    Log.e(TAG, "sendVolumeEventCallback", t);
                }
            }
            m_callbacks.finishBroadcast();
        }
    }

    /**
     * Sends event data from the local cache to all registered callbacks.
     */
    public void checkForPendingResponses() {
        if (V) Log.d(TAG, "checkForPendingResponses");
        sendLiveAudioQualityEventCallback(FmReceiverServiceState.mRssi, FmReceiverServiceState.mSnr);
    }

    private native void initNativeDataNative();

    /**
     * Turns on the radio and plays audio using the specified functionality
     * mask.
     * <p>
     * After executing this function, the application should wait for a
     * confirmatory status event callback before calling further API functions.
     * Furthermore, applications should call the {@link #turnOffRadio()}
     * function before shutting down.
     *
     * @param functionalityMask
     *            is a bitmask comprised of one or more of the following fields:
     *            {@link FmReceiver#FUNC_REGION_NA},
     *            {@link FmReceiver#FUNC_REGION_JP},
     *            {@link FmReceiver#FUNC_REGION_JP_II},
     *            {@link FmReceiver#FUNC_REGION_EUR},
     *            {@link FmReceiver#FUNC_RDS}, {@link FmReceiver#FUNC_RDBS} and
     *            {@link FmReceiver#FUNC_AF}
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int turnOnRadio(int functionalityMask) {
        Log.d(TAG, "turnOnRadio........()");

        int returnStatus = FmReceiver.STATUS_OK;
        int requestedRegion = functionalityMask & FmReceiverServiceState.FUNC_REGION_BITMASK;
        int requestedRdsFeatures = functionalityMask & FmReceiverServiceState.FUNC_RDS_BITMASK;

        /* Sanity check of parameters. */
        if ((requestedRegion != FmReceiver.FUNC_REGION_EUR)
                && (requestedRegion != FmReceiver.FUNC_REGION_JP)
                && (requestedRegion != FmReceiver.FUNC_REGION_JP_II)
                && (requestedRegion != FmReceiver.FUNC_REGION_NA)) {
            if (V) Log.d(TAG, "Illegal parameter");
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if ((0 != (requestedRdsFeatures & FmReceiver.FUNC_RDS))
                && (0 != (requestedRdsFeatures & FmReceiver.FUNC_RBDS))) {
            if (V) Log.d(TAG, "Illegal parameter (2)");
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_OFF != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.mFuncMask = functionalityMask;
            int oldState = FmReceiverServiceState.radio_state;
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_STARTING;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_ENABLE;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_STARTUP);

            /* Trim excess data from parameters. */
            functionalityMask = functionalityMask & FmReceiverServiceState.FUNC_BITMASK;

            try {
                PowerManager fmPwrMgr = PowerManager.getProxy();
                if (fmPwrMgr == null) {
                    Log.e(TAG, "UNABLE TO TURN ON FM!!!!");
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                } else {

                    fmPwrMgr.enableFm();
                    Log.d(TAG, "Sending onEnabled() Callback");
                    Log.d(TAG, "Current Fm Power State : "  +  fmPwrMgr.isfmEnabled()) ;

                    FmServiceManager.onEnabled();

                if (V) Log.d(TAG, "Calling enableNative()");

                if (enableNative(functionalityMask)) {
                    returnStatus = FmReceiver.STATUS_OK;
                } else {
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                }
               }
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "turnOnRadioNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK) {
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
                FmReceiverServiceState.radio_state = oldState;
            }
        }

        return returnStatus;
    }

    private native boolean enableNative(int functionalityMask);

    /**
     * Turns off the radio.
     * <p>
     * After executing this function, the application should wait for a
     * confirmatory status event callback before shutting down.
     *
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int turnOffRadio() {
        if (V) Log.d(TAG, "turnOffRadio()");

        int returnStatus = FmReceiver.STATUS_OK;

        if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state)
            /* Should allow forcing the FM shutdown operation. */
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_STOPPING;
        /* Set timer to check that this did not lock the state machine. */
        Message msg = Message.obtain();
        msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
        msg.arg1 = FmReceiverServiceState.OPERATION_DISABLE;
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        operationHandler.sendMessageDelayed(msg,
            FmReceiverServiceState.OPERATION_TIMEOUT_SHUTDOWN);

        try {

            if (disableNative(false)) {
                returnStatus = FmReceiver.STATUS_OK;
            }
            else {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            }
        } catch (Exception e) {
            returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            Log.e(TAG, "turnOnRadioNative failed", e);
        }

        if (returnStatus != FmReceiver.STATUS_OK) {
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }
        /*
         * State machine is reset immediately since there is risk of stack
         * failure which would lock state machine in STOPPING state.
         */

        FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_OFF;

        return returnStatus;
    }

    private native boolean disableNative(boolean bForcing);

    /**
     * Tunes the radio to a specific frequency. If successful results in a
     * status event callback.
     *
     * @param freq
     *            the frequency to tune to.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int tuneRadio(int freq) {
        if (V) Log.d(TAG, "tuneRadio()");

        int returnStatus = FmReceiver.STATUS_OK;

        /* Sanity check of parameters. */
        if ((freq < FmReceiverServiceState.MIN_ALLOWED_FREQUENCY_CODE)
                || (freq > FmReceiverServiceState.MAX_ALLOWED_FREQUENCY_CODE)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;

        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_GENERIC;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_GENERIC);

            try {
                if (tuneNative(freq))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "tuneNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    private native boolean tuneNative(int freq);

    /**
     * Gets current radio status. This results in a status event callback.
     *
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int getStatus() {
        if (V) Log.d(TAG, "getStatus()");

        int returnStatus = FmReceiver.STATUS_OK;

        if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            /* Return latest known data. */
            sendStatusEventCallbackFromLocalStore();
        }

        return returnStatus;
    }

    /**
     * Mutes/unmutes radio audio. If muted the hardware will stop sending audio.
     * This results in a status event callback.
     *
     * @param mute
     *            TRUE to mute audio, FALSE to unmute audio.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int muteAudio(boolean mute) {
        if (V) Log.d(TAG, "muteAudio()");

        int returnStatus = FmReceiver.STATUS_OK;

        if (FmReceiverServiceState.RADIO_STATE_STARTING == FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_STARTING;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_GENERIC;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_GENERIC);

            try {
                if (muteNative(mute))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "muteAudio failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    private native boolean muteNative(boolean mute);

    /**
     * Scans FM toward higher/lower frequency for next clear channel. Will
     * result in a seek complete event callback.
     *
     * @param scanMode
     *            see {@link FmReceiver#SCAN_MODE_NORMAL},
     *            {@link FmReceiver#SCAN_MODE_DOWN},
     *            {@link FmReceiver#SCAN_MODE_UP} and
     *            {@link FmReceiver#SCAN_MODE_FULL}.
     * @param minSignalStrength
     *            minimum signal strength, default =
     *            {@link FmReceiver#MIN_SIGNAL_STRENGTH_DEFAULT}
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int seekStation(int scanMode, int minSignalStrength) {
        if (V) Log.d(TAG, "seekStation():1");

        int returnStatus = FmReceiver.STATUS_OK;

        /* Sanity check of parameters. */
        if ((minSignalStrength < FmReceiverServiceState.MIN_ALLOWED_SIGNAL_STRENGTH_NUMBER)
                || (minSignalStrength > FmReceiverServiceState.MAX_ALLOWED_SIGNAL_STRENGTH_NUMBER)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_SEARCH;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_SEARCH);

            /* Trim excess data from parameters. */
            scanMode = scanMode & FmReceiverServiceState.SCAN_MODE_BITMASK;

            try {
                if (searchNative(scanMode, minSignalStrength, FmReceiver.RDS_COND_NONE, 0))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "searchNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    /**
     * Scans FM toward higher/lower frequency for next clear channel depending on the scanDirection.
     * will do wrap around when reached to mMaxFreq/mMinFreq,
     * when no wrap around is needed, use the low_bound or high_bound as endFrequency.
     * Will result in a seek complete event callback.
     *
     * @param startFrequency
     *            Starting frequency of search operation range.
     * @param endFrequency
     *            Ending frequency of search operation
     * @param minSignalStrength
     *            Minimum signal strength, default =
     *            {@link #MIN_SIGNAL_STRENGTH_DEFAULT}
     * @param scanDirection
     *            the direction to search in, it can only be either
     *            {@link #SCAN_MODE_UP} and {@link #SCAN_MODE_DOWN}.
     * @param scanMethod
     *            see {@link #SCAN_MODE_NORMAL}, {@link #SCAN_MODE_FAST},
     * @param multi_channel
     *            Is multiple channels are required, or only find next valid channel(seek).
     * @param rdsType
     *            the type of RDS condition to scan for.
     * @param rdsTypeValue
     *            the condition value to match.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int seekStationCombo (int startFreq, int endFreq, int minSignalStrength, int direction, int scanMethod,
            boolean multiChannel, int rdsType, int rdsTypeValue) {
        if (V) Log.d(TAG, "seekStationCombo()");

        int returnStatus = FmReceiver.STATUS_OK;

        /* Sanity check of parameters. */
        if ((minSignalStrength < FmReceiverServiceState.MIN_ALLOWED_SIGNAL_STRENGTH_NUMBER)
                || (minSignalStrength > FmReceiverServiceState.MAX_ALLOWED_SIGNAL_STRENGTH_NUMBER)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Log.d(TAG, "seekStationCombo(), OPERATION_TIMEOUT_SEARCH:"+FmReceiverServiceState.OPERATION_TIMEOUT_SEARCH);
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_SEARCH;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg, FmReceiverServiceState.OPERATION_TIMEOUT_SEARCH);

            /* Trim excess data from parameters. */
            //scanMode = scanMode & FmReceiverServiceState.SCAN_MODE_BITMASK;

            if (V) Log.d(TAG, "seekStationCombo(), startFreq:"+startFreq+" endFeq:"+endFreq+" rssi:"+minSignalStrength+
                    " direction:"+direction+" scanMethod:"+scanMethod+" multiChannel:"+multiChannel+
                    " rdsType:"+rdsType+" rdsTypeValue:"+rdsTypeValue);


            try {
                if (comboSearchNative(startFreq, endFreq, minSignalStrength, direction, scanMethod, multiChannel, rdsType, rdsTypeValue))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "comboSearchNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    private native boolean comboSearchNative(int startFreq, int endFreq, int rssiThreshold, int direction, int scanMethod,
		boolean multiChannel, int rdsType, int rdsTypeValue);

    /**
     * Scans FM toward higher/lower frequency for next clear channel. Will
     * result in a seek complete event callback. Will seek out a station that
     * matches the requested value for the desired RDS functionality support.
     *
     * @param scanMode
     *            see {@link FmReceiver#SCAN_MODE_NORMAL},
     *            {@link FmReceiver#SCAN_MODE_DOWN},
     *            {@link FmReceiver#SCAN_MODE_UP} and
     *            {@link FmReceiver#SCAN_MODE_FULL}.
     * @param minSignalStrength
     *            Minimum signal strength, default =
     *            {@link FmReceiver#MIN_SIGNAL_STRENGTH_DEFAULT}
     * @param rdsCondition
     *            the type of RDS condition to scan for.
     * @param rdsValue
     *            the condition value to match.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int seekRdsStation(int scanMode, int minSignalStrength, int rdsCondition,
            int rdsValue) {
        if (V) Log.d(TAG, "seekRdsStation():1");

        int returnStatus = FmReceiver.STATUS_OK;

        /* Sanity check of parameters. */
        if ((minSignalStrength < FmReceiverServiceState.MIN_ALLOWED_SIGNAL_STRENGTH_NUMBER)
                || (minSignalStrength > FmReceiverServiceState.MAX_ALLOWED_SIGNAL_STRENGTH_NUMBER)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if ((rdsValue < FmReceiverServiceState.MIN_ALLOWED_RDS_CONDITION_VALUE)
                || (rdsValue > FmReceiverServiceState.MAX_ALLOWED_RDS_CONDITION_VALUE)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if ((rdsCondition != FmReceiver.RDS_COND_NONE)
                && (rdsCondition != FmReceiver.RDS_COND_PTY)
                && (rdsCondition != FmReceiver.RDS_COND_TP)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_SEARCH;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_SEARCH);

            /* Trim excess data from parameters. */
            scanMode = scanMode & FmReceiverServiceState.SCAN_MODE_BITMASK;

            try {
                if (searchNative(scanMode, minSignalStrength, rdsCondition, rdsValue))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "searchNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    private native boolean searchNative(int scanMode, int rssiThreshold, int condVal, int condType);

    /**
     * Aborts the current station seeking operation if any. Will result in a
     * seek complete event containing the last scanned frequency.
     * <p>
     *
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int seekStationAbort() {
        if (V) Log.d(TAG, "seekStationAbort()");

        int returnStatus = FmReceiver.STATUS_OK;

        if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_GENERIC;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_GENERIC);

            try {
                if (searchAbortNative())
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "searchAbortNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    private native boolean searchAbortNative();

    /**
     * Enables/disables RDS/RDBS feature and AF algorithm. Will result in an RDS
     * mode event callback.
     * <p>
     *
     * @param rdsMode
     *            Turns on the RDS or RBDS. See {@link FmReceiver#RDS_MODE_OFF},
     *            {@link FmReceiver#RDS_MODE_DEFAULT_ON},
     *            {@link FmReceiver#RDS_MODE_RDS_ON},
     *            {@link FmReceiver#RDS_MODE_RDBS_ON}
     * @param rdsFields
     *            the mask specifying which types of RDS data to signal back.
     * @param afmode
     *            enables AF algorithm if True. Disables it if False
     * @param afThreshold
     *            the RSSI threshold when the AF should jump frequencies.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int setRdsMode(int rdsMode, int rdsFeatures, int afMode, int afThreshold) {
        int returnStatus = FmReceiver.STATUS_OK;

        if (V) Log.d(TAG, "setRdsMode()");

        /* Sanity check of parameters. */
        if ((afThreshold < FmReceiverServiceState.MIN_ALLOWED_AF_JUMP_THRESHOLD)
                || (afThreshold > FmReceiverServiceState.MAX_ALLOWED_AF_JUMP_THRESHOLD)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            FmReceiverServiceState.radio_op_state = FmReceiverServiceState.RADIO_OP_STATE_NONE;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_GENERIC;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_GENERIC);

            /* Trim excess data from parameters and convert to native. */
            rdsMode = rdsMode & FmReceiverServiceState.RDS_MODE_BITMASK;
            afMode = afMode & FmReceiverServiceState.AF_MODE_BITMASK;
            rdsFeatures = rdsFeatures & FmReceiverServiceState.RDS_FEATURES_BITMASK;
            boolean rdsOnNative = (rdsMode != FmReceiver.RDS_MODE_OFF);
            boolean afOnNative = (afMode != FmReceiver.AF_MODE_OFF);

            /* Set RDS/RBDS from parameters. */
            int rdsModeNative = rdsMode & FmReceiverServiceState.RDS_RBDS_BITMASK;

            /* Execute commands. */
            try {
                if (setRdsModeNative(rdsOnNative, afOnNative, rdsModeNative)) {
                    returnStatus = FmReceiver.STATUS_OK;
                } else {
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                }
            } catch (Exception e) {
                Log.e(TAG, "setRdsNative failed", e);
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    private native boolean setRdsModeNative(boolean rdsOn, boolean afOn, int rdsType);

    /**
     * Configures FM audio mode to be mono, stereo or blend. Will result in an
     * audio mode event callback.
     *
     * @param audioMode
     *            the audio mode such as stereo or mono. The following values
     *            should be used {@link FmReceiver#AUDIO_MODE_AUTO},
     *            {@link FmReceiver#AUDIO_MODE_STEREO},
     *            {@link FmReceiver#AUDIO_MODE_MONO} or
     *            {@link FmReceiver#AUDIO_MODE_BLEND}.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int setAudioMode(int audioMode) {
        int returnStatus = FmReceiver.STATUS_OK;

        if (V) Log.d(TAG, "setAudioMode()");

        if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_GENERIC;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_GENERIC);

            /* Trim excess data from parameters. */
            audioMode = audioMode & FmReceiverServiceState.AUDIO_MODE_BITMASK;

            try {
                if (setAudioModeNative(audioMode))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "setAudioModeNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        }

        return returnStatus;
    }

    private native boolean setAudioModeNative(int audioMode);

    /**
     * Configures FM audio path to AUDIO_PATH_NONE, AUDIO_PATH_SPEAKER,
     * AUDIO_PATH_WIRED_HEADSET or AUDIO_PATH_DIGITAL. Will result in an audio
     * path event callback.
     *
     * @param audioPath
     *            the audio path such as AUDIO_PATH_NONE, AUDIO_PATH_SPEAKER,
     *            AUDIO_PATH_WIRED_HEADSET or AUDIO_PATH_DIGITAL. The following
     *            values should be used {@link FmReceiver#AUDIO_PATH_NONE},
     *            {@link FmReceiver#AUDIO_PATH_SPEAKER},
     *            {@link FmReceiver#AUDIO_PATH_WIRED_HEADSET} or
     *            {@link FmReceiver#AUDIO_PATH_DIGITAL}.
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int setAudioPath(int audioPath) {
        int returnStatus = FmReceiver.STATUS_OK;

        if (V) Log.d(TAG, "setAudioPath(" + Integer.toString(audioPath) + ")");

        if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_GENERIC;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_GENERIC);

            /* Trim excess data from parameters. */
            audioPath = audioPath & FmReceiverServiceState.AUDIO_PATH_BITMASK;

            try {
                if (setAudioPathNative(audioPath))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "setAudioPathNative failed", e);
            }
        }
        if (returnStatus != FmReceiver.STATUS_OK)
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        Log.d(TAG, "setAudioPathNative returnStatus="+returnStatus);
        if (FmReceiverServiceConfig.USE_FM_AUDIO_ROUTING)
        {
	/*
            if (audioPath == FmReceiver.AUDIO_PATH_NONE)
                AudioSystem.setParameters("fm_route=disabled");
            else if (audioPath == FmReceiver.AUDIO_PATH_SPEAKER)
                AudioSystem.setParameters("fm_route=fm_speaker");
            else if (audioPath == FmReceiver.AUDIO_PATH_WIRE_HEADSET)
                AudioSystem.setParameters("fm_route=fm_headset");*/
            Log.d(TAG, "FM audio system switch audioPath="+audioPath);
/*            Intent intent = new Intent(Intent.ACTION_FM);
            intent.putExtra("state", audioPath == FmReceiver.AUDIO_PATH_NONE ? 0 : 1);
            intent.putExtra("speaker", audioPath == FmReceiver.AUDIO_PATH_SPEAKER ? 1 : 0);

            mContext.sendBroadcast(intent, null);*/
            AudioManager am = ((AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE));
            if (audioPath == FmReceiver.AUDIO_PATH_NONE) {
                am.setWiredDeviceConnectionState(AudioManager.DEVICE_OUT_FM_SPEAKER, AudioSystem.DEVICE_STATE_UNAVAILABLE, "");
                am.setWiredDeviceConnectionState(AudioManager.DEVICE_OUT_FM_HEADSET, AudioSystem.DEVICE_STATE_UNAVAILABLE, "");
            } else if (audioPath == FmReceiver.AUDIO_PATH_SPEAKER) {
                am.setWiredDeviceConnectionState(AudioManager.DEVICE_OUT_FM_HEADSET, AudioSystem.DEVICE_STATE_UNAVAILABLE, "");
                am.setWiredDeviceConnectionState(AudioManager.DEVICE_OUT_FM_SPEAKER, AudioSystem.DEVICE_STATE_AVAILABLE, "");
            } else if (audioPath == FmReceiver.AUDIO_PATH_WIRE_HEADSET) {
                am.setWiredDeviceConnectionState(AudioManager.DEVICE_OUT_FM_SPEAKER, AudioSystem.DEVICE_STATE_UNAVAILABLE, "");
                am.setWiredDeviceConnectionState(AudioManager.DEVICE_OUT_FM_HEADSET, AudioSystem.DEVICE_STATE_AVAILABLE, "");
            }
        }

        return returnStatus;
    }

    private native boolean setAudioPathNative(int audioPath);

    /**
     * Sets the minimum frequency step size to use when scanning for stations.
     * This function does not result in a status callback and the calling
     * application should therefore keep track of this setting.
     *
     * @param stepSize
     *            a frequency interval set to
     *            {@link FmReceiver#FREQ_STEP_100KHZ} or
     *            {@link FmReceiver#FREQ_STEP_50KHZ}.
     *
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int setStepSize(int stepSize) {
        if (V) Log.d(TAG, "setStepSize()");

        int returnStatus = FmReceiver.STATUS_OK;

        /* Sanity check of parameters. */
        if ((stepSize != FmReceiver.FREQ_STEP_50KHZ) && (stepSize != FmReceiver.FREQ_STEP_100KHZ)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_GENERIC;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_GENERIC);

            try {
                if (setScanStepNative(stepSize))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "setScanStepNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    private native boolean setScanStepNative(int stepSize);

    /**
     * Sets the volume to use.
     *
     * @param volume
     *            range from 0 to 255
     *
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int setFMVolume(int volume) {
        if (V) Log.d(TAG, "setFMVolume()");

        int returnStatus = FmReceiver.STATUS_OK;

        /* Sanity check of parameters. */
        if ((volume < 0) || (volume > 255)) {
            if (V) Log.d(TAG, "volume is illegal =" + volume);
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_GENERIC;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_GENERIC);
            if (V) Log.d(TAG, "setFMVolume =" + volume);
            try {
                if (setFMVolumeNative(volume))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "setFMVolumeNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    private native boolean setFMVolumeNative(int volume);

    /**
     * Sets a the world frequency region and deemphasis time. This results in a
     * world region event callback.
     *
     * @param worldRegion
     *            the world region the FM receiver is located. Set to
     *            {@link FmReceiver#FUNC_REGION_NA},
     *            {@link FmReceiver#FUNC_REGION_EUR},
     *            {@link FmReceiver#FUNC_REGION_JP} or
     *            {@link FmReceiver#FUNC_REGION_JP_II}.
     * @param deemphasisTime
     *            the deemphasis time that can be set to either
     *            {@link FmReceiver#DEEMPHASIS_50U} or
     *            {@link FmReceiver#DEEMPHASIS_75U}.
     *
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int setWorldRegion(int worldRegion, int deemphasisTime) {
        if (V) Log.d(TAG, "setWorldRegion()");

        int returnStatus = FmReceiver.STATUS_OK;

        /* Sanity check of parameters. */
        if ((worldRegion != FmReceiver.FUNC_REGION_NA)
                && (worldRegion != FmReceiver.FUNC_REGION_EUR)
                && (worldRegion != FmReceiver.FUNC_REGION_JP)
                && (worldRegion != FmReceiver.FUNC_REGION_JP_II)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if ((deemphasisTime != FmReceiver.DEEMPHASIS_50U)
                && (deemphasisTime != FmReceiver.DEEMPHASIS_75U)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_GENERIC;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_GENERIC);

            try {
                if (setRegionNative(worldRegion) && configureDeemphasisNative(deemphasisTime))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "setRdsNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    private native boolean setRegionNative(int region);

    private native boolean configureDeemphasisNative(int time);

    /**
     * Estimates the current frequencies noise floor level. Generates an
     * Estimated NFL Event when complete. The returned NFL value can be used to
     * determine which minimum signal strength to use seeking stations.
     *
     * @param estimatedNoiseFloorLevel
     *            Estimate noise floor to {@link FmReceiver#NFL_LOW},
     *            {@link FmReceiver#NFL_MED} or {@link FmRecei-vver#NFL_FINE}.
     *
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int estimateNoiseFloorLevel(int nflLevel) throws RemoteException {
        if (V) Log.d(TAG, "estimateNoiseFloorLevel()");

        int returnStatus = FmReceiver.STATUS_OK;

        /* Sanity check of parameters. */
        if ((nflLevel != FmReceiver.NFL_FINE) && (nflLevel != FmReceiver.NFL_MED)
                && (nflLevel != FmReceiver.NFL_LOW)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_BUSY;
            /* Set timer to check that this did not lock the state machine. */
            Message msg = Message.obtain();
            msg.what = FmReceiverServiceState.OPERATION_TIMEOUT;
            msg.arg1 = FmReceiverServiceState.OPERATION_NFL;
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            operationHandler.sendMessageDelayed(msg,
                FmReceiverServiceState.OPERATION_TIMEOUT_NFL);

            try {
                if (estimateNoiseFloorNative(nflLevel))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "estimateNoiseFloorNative failed", e);
            }
            if (returnStatus != FmReceiver.STATUS_OK)
                operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        }

        return returnStatus;
    }

    private native boolean estimateNoiseFloorNative(int level);

    /**
     * Sets the live audio polling function that can provide RSSI data on the
     * currently tuned frequency at specified intervals.
     *
     * @param liveAudioPolling
     *            enable/disable live audio data quality updating.
     * @param signalPollInterval
     *            time between RSSI signal polling in milliseconds.
     *
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int setLiveAudioPolling(boolean liveAudioPolling, int signalPollInterval)
            throws RemoteException {
        int returnStatus = FmReceiver.STATUS_OK;

        if (V) Log.d(TAG, "setLiveAudioPolling()");

        /* Sanity check of parameters. */
        if (liveAudioPolling
                && ((signalPollInterval < FmReceiverServiceState.MIN_ALLOWED_SIGNAL_POLLING_TIME) || (signalPollInterval > FmReceiverServiceState.MAX_ALLOWED_SIGNAL_POLLING_TIME))) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;

            try {
                if (getAudioQualityNative(liveAudioPolling)
                        && configureSignalNotificationNative(signalPollInterval))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "setLiveAudioPolling failed", e);
            }

            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
        return returnStatus;
    }

    private native boolean getAudioQualityNative(boolean enabled);

    private native boolean configureSignalNotificationNative(int interval);

    /**
     * Sets the SNR threshold for the subsequent FM frequency tuning.
     * This value will be used by BTA stack internally.
     *
     * @param signalPollInterval
     *           SNR Threshold value (0 ~ 31 (BTA_FM_SNR_MAX) )
     *
     * @return STATUS_OK = 0 if successful. Otherwise returns a non-zero error
     *         code.
     */
    public synchronized int setSNRThreshold( int snrThreshold)
            throws RemoteException {
        int returnStatus = FmReceiver.STATUS_OK;

        if (V) Log.d(TAG, "setSNRThreshold() - "+snrThreshold);

        /* Sanity check of parameters. */
        if ((snrThreshold < FmReceiverServiceState.MIN_ALLOWED_SNR_THRESHOLD) || (snrThreshold > FmReceiverServiceState.MAX_ALLOWED_SNR_THRESHOLD)) {
            returnStatus = FmReceiver.STATUS_ILLEGAL_PARAMETERS;
        } else if (FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND != FmReceiverServiceState.radio_state) {
            if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
            returnStatus = FmReceiver.STATUS_ILLEGAL_COMMAND;
        } else {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;

            try {
                if (setSNRThresholdNative(snrThreshold))
                    returnStatus = FmReceiver.STATUS_OK;
                else
                    returnStatus = FmReceiver.STATUS_SERVER_FAIL;
            } catch (Exception e) {
                returnStatus = FmReceiver.STATUS_SERVER_FAIL;
                Log.e(TAG, "setSNRThreshold failed", e);
            }

            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        if (V) Log.d(TAG, "STATE = " + Integer.toString(FmReceiverServiceState.radio_state));
        return returnStatus;
    }

    private native boolean setSNRThresholdNative(int snrThreshold);


    /* JNI BTA Event callback functions. */
    public void onRadioOnEvent(int status) {
        if (V) Log.d(TAG, "onRadioOnEvent()");
        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mRadioIsOn = true;
        }

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (FmReceiverServiceState.RADIO_STATE_STARTING == FmReceiverServiceState.radio_state) {
            if (FmReceiverServiceState.mRadioIsOn) {
                FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
            } else {
                FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_OFF;
            }
        }

        /* Update local cache. */
        sendStatusEventCallbackFromLocalStore();
    }

    public void onRadioOffEvent(int status) {
        if (V) Log.d(TAG, "onRadioOffEvent()");
        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        //Power down the FM radio after we are complete
        PowerManager fmPwrMgr = PowerManager.getProxy();
        if (fmPwrMgr == null) {
            Log.e(TAG, "UNABLE TO TURN OFF FM!!!!");
        } else {
            fmPwrMgr.disableFm();
        }
        //End power down

        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mRadioIsOn = false;
        }

        /* Update state machine. */
        if (FmReceiverServiceState.RADIO_STATE_STOPPING == FmReceiverServiceState.radio_state) {
            if (!FmReceiverServiceState.mRadioIsOn) {
                FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_OFF;
            } else {
                FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
            }
        }
        if(FmReceiverServiceState.RADIO_STATE_OFF != FmReceiverServiceState.radio_state)
        {
        	FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_OFF;
        }

        /* Callback. */
        sendStatusEventCallbackFromLocalStore();
    }

    private boolean isRadioInOffProcess() {
        return FmReceiverServiceState.radio_state == FmReceiverServiceState.RADIO_STATE_STOPPING
                || FmReceiverServiceState.radio_state == FmReceiverServiceState.RADIO_STATE_OFF;
    }

    public void onRadioMuteEvent(int status, boolean muted) {
        if (V) Log.d(TAG, "onRadioMuteEvent()");

        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mIsMute = muted;
        }

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        /* Callback. */
        sendStatusEventCallbackFromLocalStore();
    }

    public void onRadioTuneEvent(int status, int rssi, int snr, int freq) {
        if (V) Log.d(TAG, "onRadioTuneEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mRssi = rssi;
            FmReceiverServiceState.mFreq = freq;
            FmReceiverServiceState.mSnr = snr;
        }
        sendStatusEventCallbackFromLocalStore();
    }

    public void onRadioSearchEvent(int rssi, int snr, int freq) {
        if (V) Log.d(TAG, "onRadioSearchEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        /* Update local cache. */
        FmReceiverServiceState.mRssi = rssi;
        FmReceiverServiceState.mFreq = freq;
        FmReceiverServiceState.mSnr = snr;
        FmReceiverServiceState.mSeekSuccess = true;
        /* DON'T DO CALLBACK HERE - wait for onRadioSearchCompleteEvent*/
        //sendSeekCompleteEventCallbackFromLocalStore();
    }

    public void onRadioSearchCompleteEvent(int status, int rssi, int snr, int freq) {
        if (V) Log.d(TAG, "onRadioSearchCompleteEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mRssi = rssi;
            FmReceiverServiceState.mFreq = freq;
            FmReceiverServiceState.mSnr = snr;
            FmReceiverServiceState.mSeekSuccess = true;
        }
        sendSeekCompleteEventCallbackFromLocalStore();
    }

    public void onRadioAfJumpEvent(int status, int rssi, int freq) {
        if (V) Log.d(TAG, "onRadioAfJumpEvent(status = " + Integer.toString(status) + ", rssi = "
                + Integer.toString(rssi) + ", freq = " + Integer.toString(freq) + ")");
        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mRssi = rssi;
            FmReceiverServiceState.mFreq = freq;
            FmReceiverServiceState.mSeekSuccess = true;
        }
        sendSeekCompleteEventCallbackFromLocalStore();
        /* Is this of interest internally without knowing the new frequency? */
    }

    public void onRadioAudioModeEvent(int status, int mode) {
        if (V) Log.d(TAG, "onRadioAudioModeEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mAudioMode = mode;
        }
        sendAudioModeEventCallbackFromLocalStore();
    }

    public void onRadioAudioPathEvent(int status, int path) {
        if (V) Log.d(TAG, "onRadioAudioPathEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mAudioPath = path;
        }
        sendAudioPathEventCallbackFromLocalStore();
    }

    public void onRadioAudioDataEvent(int status, int rssi, int mode, int snr) {
        if (V) Log.d(TAG, "onRadioAudioDataEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mRssi = rssi;
            FmReceiverServiceState.mAudioMode = mode;
            FmReceiverServiceState.mSnr = snr;
        }
        sendLiveAudioQualityEventCallbackFromLocalStore();
    }

    public void onRadioRdsModeEvent(int status, boolean rdsOn, boolean afOn, int rdsType) {
        if (V) Log.d(TAG, "onRadioRdsModeEvent()");

        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mRdsOn = rdsOn;
            FmReceiverServiceState.mAfOn = afOn;
            FmReceiverServiceState.mRdsType = rdsType;
            if (V) Log.d(TAG, "onRadioRdsModeEvent( rdsOn " + Boolean.toString(rdsOn) + ", afOn"
                    + Boolean.toString(afOn) + ","
                    + Integer.toString(FmReceiverServiceState.mRdsType) + ")");
        }

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
        /* Transmit event upwards to app. */
        sendRdsModeEventCallbackFromLocalStore();
        FmReceiverServiceState.radio_op_state = FmReceiverServiceState.RADIO_OP_STATE_NONE;
        /* Indicate that this command in the sequence is finished. */
    }

    // Should not be needed anymore
    public void onRadioRdsTypeEvent(int status, int rdsType) {
        if (V) Log.d(TAG, "onRadioRdsTypeEvent()");

        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mRdsType = rdsType;
        }

        if (FmReceiverServiceState.RADIO_OP_STATE_STAGE_1 == FmReceiverServiceState.radio_op_state) {
            /* Update state machine. */
            if (!isRadioInOffProcess()) {
                FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
            }

            /* This response indicates that system is alive and well. */
            operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);
            /* Transmit event upwards to app. */
            sendRdsModeEventCallbackFromLocalStore();
            FmReceiverServiceState.radio_op_state = FmReceiverServiceState.RADIO_OP_STATE_NONE;
        } else {
            /* Indicate that this command in the sequence is finished. */
            FmReceiverServiceState.radio_op_state = FmReceiverServiceState.RADIO_OP_STATE_STAGE_2;
        }
    }

    public void onRadioRdsUpdateEvent(int status, int data, int index, String text) {
        if (V) Log.d(TAG, "onRadioRdsUpdateEvent(" + Integer.toString(status) + ","
                + Integer.toString(data) + "," + Integer.toString(index) + "," + text + ")");

        if (FmReceiverServiceState.BTA_FM_OK == status) {
            /* Update local cache. (For retrieval in status commands.) */
            switch (data) {
                case FmReceiverServiceState.RDS_ID_PTY_EVT:
                    FmReceiverServiceState.mRdsProgramType = index;
                    break;
                case FmReceiverServiceState.RDS_ID_PTYN_EVT:
                    FmReceiverServiceState.mRdsProgramTypeName = text;
                    break;
                case FmReceiverServiceState.RDS_ID_RT_EVT:
                    FmReceiverServiceState.mRdsRadioText = text;
                    break;
                case FmReceiverServiceState.RDS_ID_PS_EVT:
                    FmReceiverServiceState.mRdsProgramService = text;
                    break;

                default:
                    break;
            }
            ;

            /* Transmit individual message to app. */
            sendRdsDataEventCallback(data, index, text);
        }
    }

    public void onRadioDeemphEvent(int status, int time) {
        if (V) Log.d(TAG, "onRadioDeemphEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }
    }

    public void onRadioScanStepEvent(int stepSize) {
        if (V) Log.d(TAG, "onRadioScanStepEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }
    }

    public void onRadioRegionEvent(int status, int region) {
        if (V) Log.d(TAG, "onRadioRegionEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        /* Update local cache. */
        if (FmReceiverServiceState.BTA_FM_OK == status) {
            FmReceiverServiceState.mWorldRegion = region;
            sendWorldRegionEventCallbackFromLocalStore();
        }
    }

    public void onRadioNflEstimationEvent(int level) {
        if (V) Log.d(TAG, "onRadioNflEstimationEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        /* Update local cache. */
        FmReceiverServiceState.mEstimatedNoiseFloorLevel = level;
        sendEstimateNflEventCallbackFromLocalStore();
    }

    public void onRadioVolumeEvent(int status, int volume) {
        if (V) Log.d(TAG, "onRadioVolumeEvent()");

        /* This response indicates that system is alive and well. */
        operationHandler.removeMessages(FmReceiverServiceState.OPERATION_TIMEOUT);

        /* Update state machine. */
        if (!isRadioInOffProcess()) {
            FmReceiverServiceState.radio_state = FmReceiverServiceState.RADIO_STATE_READY_FOR_COMMAND;
        }

        sendVolumeEventCallback(status, volume);
    }

    /*
     * ==========================================================================
     * ========================= public synchronized void onRadioOnEvent(int
     * status) { if (V) Log.d(TAG, "onRadioOnEvent()");
     *
     * }
     *
     * public synchronized void onRadioOffEvent(int status) { if (V) Log.d(TAG,
     * "onRadioOffEvent()"); if ( mService != null ) {
     * mService.onRadioOffEvent(status); } }
     *
     * public synchronized void onRadioMuteEvent(int status, boolean muted) {
     * if (V) Log.d(TAG, "onRadioMuteEvent()"); if ( mService != null ) {
     * mService.onRadioMuteEvent(status, muted); } }
     *
     * public synchronized void onRadioTuneEvent(int status, int rssi, int freq)
     * { if (V) Log.d(TAG, "onRadioTuneEvent()"); if ( mService != null ) {
     * mService.onRadioTuneEvent(status, rssi, freq); } }
     *
     * public synchronized void onRadioSearchEvent(int rssi, int freq) {
     * if (V) Log.d(TAG, "onRadioSearchEvent()"); if ( mService != null ) {
     * mService.onRadioSearchEvent(rssi, freq); } }
     *
     * public synchronized void onRadioSearchCompleteEvent(int status, int rssi,
     * int freq) { if (V) Log.d(TAG, "onRadioSearchCompleteEvent()"); if ( mService !=
     * null ) { mService.onRadioSearchCompleteEvent(status, rssi, freq); } }
     *
     * public synchronized void onRadioAfJumpEvent(int status, int rssi, int
     * freq) { if (V) Log.d(TAG, "onRadioAfJumpEvent()"); if ( mService != null ) {
     * mService.onRadioAfJumpEvent(status, rssi, freq); } }
     *
     * public synchronized void onRadioAudioModeEvent(int status, int mode) {
     * if (V) Log.d(TAG, "onRadioAudioModeEvent()"); if ( mService != null ) {
     * mService.onRadioAudioModeEvent(status, mode); } }
     *
     * public synchronized void onRadioAudioPathEvent(int status, int path) {
     * if (V) Log.d(TAG, "onRadioAudioPathEvent()"); if ( mService != null ) {
     * mService.onRadioAudioPathEvent(status, path); } }
     *
     * public synchronized void onRadioAudioDataEvent(int status, int rssi, int
     * mode, int snr) { if (V) Log.d(TAG, "onRadioAudioDataEvent()"); if ( mService != null ) {
     * mService.onRadioAudioDataEvent(status, rssi, mode, snr); } }
     *
     * public synchronized void onRadioRdsModeEvent(int status, boolean rdsOn,
     * boolean afOn,int rdsType) { if (V) Log.d(TAG, "onRadioRdsModeEvent()"); if (
     * mService != null ) { mService. onRadioRdsModeEvent(status, rdsOn, afOn,
     * rdsType); } }
     *
     * public synchronized void onRadioRdsTypeEvent(int status, int rdsType) {
     * if (V) Log.d(TAG, "onRadioRdsTypeEvent()"); if ( mService != null ) {
     * mService.onRadioRdsTypeEvent(status, rdsType); } }
     *
     * public synchronized void onRadioRdsUpdateEvent(int status, int data, int
     * index, String text) { if (V) Log.d(TAG, "onRadioRdsUpdateEvent() "+text); if (
     * mService != null ) { mService.onRadioRdsUpdateEvent(status, data, index,
     * text); } }
     *
     * public synchronized void onRadioDeemphEvent(int status, int time) {
     * if (V) Log.d(TAG, "onRadioDeemphEvent()");
     *
     * if ( mService != null ) { mService.onRadioDeemphEvent(status, time); } }
     *
     * public synchronized void onRadioScanStepEvent(int stepSize) { if (V) Log.d(TAG,
     * "onRadioScanStepEvent()"); if ( mService != null ) {
     * mService.onRadioScanStepEvent(stepSize); } }
     *
     * public synchronized void onRadioRegionEvent(int status, int region) {
     * if (V) Log.d(TAG, "onRadioRegionEvent()"); if ( mService != null ) {
     * mService.onRadioRegionEvent(status, region); } }
     *
     * public synchronized void onRadioNflEstimationEvent(int level) {
     * if (V) Log.d(TAG, "onRadioNflEstimationEvent()"); if ( mService != null ) {
     * mService.onRadioNflEstimationEvent(level); } }
     *
     * public synchronized void onRadioVolumeEvent(int status, int volume) {
     * if (V) Log.d(TAG, "onRadioVolumeEvent()"); if ( mService != null ) {
     * mService.onRadioVolumeEvent(status, volume); } }
     */


    public synchronized void onStateChangeEvent(boolean started) {
        if (mListener != null) {
            mIsStarted=started;
            if (mIsStarted) {
                mListener.onStart();
            } else {
                mListener.onStop();
            }
        }
        /*
        Intent i = new Intent(FmServiceConfig.FM_SVC_STATE_CHANGE_ACTION);
        i.putExtra(FmServiceConfig.EXTRA_FM_SVC_NAME, getName());
        i.putExtra(FmServiceConfig.EXTRA_FM_SVC_STATE, started?
                FmServiceConfig.SVC_STATE_STARTED: FmServiceConfig.SVC_STATE_STOPPED);
        mContext.sendBroadcast(i);
        */
    }
}
