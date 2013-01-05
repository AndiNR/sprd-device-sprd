/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import android.R.integer;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.media.AudioManager;
import android.os.Bundle;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.DialogInterface.OnMultiChoiceClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.view.View;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.widget.TextView;
import android.widget.Button;
import android.widget.ProgressBar;


import java.util.ArrayList;
import java.util.List;
import java.lang.Thread;
import java.lang.InterruptedException;
import java.lang.Thread.State;

import android.telephony.TelephonyManager;
import android.util.Log;

public class StationSearch extends Activity {
    private static final String LOGTAG = "StationSearchActivity";

    private static final int AUDIO_DELAY = 500;
    private static final int ZOOM_FACTOR = 10;
    private static final int RSSI_LEVEL = 100;
    private static final int SHOW_DIALOG = 1;

    private static final int MSG_UPDATE = 1;
    private static final int MSG_SEARCH_FINISH = 2;
    private static final int STOP_SEARCHING = 3;

    private float startFreq;
    
    private TextView mTitle;
    private TextView mStation;
    private TextView mFreq;

    private PresetStation[] stationArray;
    private String[] stationNameArray;
    private boolean[] isSelectArray;

    private ProgressBar mProgress;
    private Button mStop;
    private FMPlaySharedPreferences mStationList;
    public static Searcher mSearcher = null;

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_FM)) {
                if (intent.hasExtra("state")) {
                    if (intent.getIntExtra("state", 0) == 0 && StationSearch.isSearching()) {
                        mSearcher.stopSearching();
                        finish();
                    }
                }
            }else if (action.equals(Intent.ACTION_HEADSET_PLUG)) {
                if (intent.getIntExtra("state", 0) == 0 ) {
                    if(StationSearch.isSearching()){
                        mSearcher.dispose();
                    }
                    finish();
                }
            }
        }
    };

    public void onCreate(Bundle savedInstanceState) {
        Log.d(LOGTAG, "onCreate");
        super.onCreate(savedInstanceState);
        setVolumeControlStream(AudioManager.STREAM_FM);
        this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.station_search);

        if (!getComponents()) {
            return;
        }

        mStationList = FMPlaySharedPreferences.getInstance(this);

        if(StationSearch.isSearching()){
            StationSearch.mSearcher.resetHost(this, mHandler);
            } else{
                StationSearch.mSearcher = new Searcher(this, mHandler);
                StationSearch.mSearcher.start();
        }

        setListeners();

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_FM);
        filter.addAction(Intent.ACTION_HEADSET_PLUG);//mm04 fix bug 7998
        registerReceiver(mReceiver, filter);
    }

    protected void onStart (){
        Log.d(LOGTAG, "onStart");
        super.onStart();
        Intent intent = new Intent(FMplayService.ACTION_COUNT);
        intent.putExtra("count", 1);
        sendBroadcast(intent);
    }

    protected void onResume() {
        Log.d(LOGTAG,"onResume");
        super.onResume();
        Intent intent = getIntent();
        startFreq = intent.getFloatExtra("freq", (float) 87.5);
        startFreq-=0.1;
        if(startFreq <= WheelConfig.RADIO_MIN_FREQUENCY){
            startFreq = WheelConfig.RADIO_MIN_FREQUENCY;
        }
        mStationList.load();
    }

    protected void onPause() {
        Log.d(LOGTAG,"onPause");
        if(StationSearch.mSearcher != null){
            if(StationSearch.mSearcher.ismIsFinish()&&StationSearch.mSearcher.mStationListFound.size()==0) {//fix bug 12338
                Log.d(LOGTAG,"Is time telephone no freq.");
                mHandler.sendMessage(mHandler.obtainMessage(MSG_SEARCH_FINISH));
                }
            }
        TelephonyManager tm = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
        int callState = tm.getCallState();
        if (callState == TelephonyManager.CALL_STATE_OFFHOOK || callState == TelephonyManager.CALL_STATE_RINGING){
            Log.d(LOGTAG, "Is time telephone,finish");
            mHandler.sendMessage(mHandler.obtainMessage(MSG_SEARCH_FINISH));
        }
        super.onPause();
    }

    protected void onStop () {
        Log.d(LOGTAG, "onStop");
        Intent intent = new Intent(FMplayService.ACTION_COUNT);
        intent.putExtra("count", -1);
        sendBroadcast(intent);
        super.onStop();
    }

    public void onBackPressed() {
        Log.d(LOGTAG, "onBackPressed");
        if(mSearcher != null){
            if (mSearcher.isSearching()) {
                mSearcher.stopSearching();
                try {
                    mSearcher.join(1000);
                } catch(InterruptedException e) {
                    e.printStackTrace();
                }
                if(mSearcher.getState() != State.TERMINATED){//fix bug 10266
//                mHandler.sendEmptyMessage(STOP_SEARCHING);
                    Message msg = mHandler.obtainMessage(STOP_SEARCHING);
                    msg.obj = mSearcher.getState();
                    mHandler.sendMessage(msg);
                }else {
                    mHandler.sendMessage(mHandler.obtainMessage(MSG_SEARCH_FINISH));
                }
            }else {
                finish();
            }
        }
    }

    public void onDestroy() {
        Log.d(LOGTAG, "onDestroy");
        mHandler.removeMessages(MSG_SEARCH_FINISH);
        mHandler.removeMessages(MSG_UPDATE);
        unregisterReceiver(mReceiver);

        if(StationSearch.isSearching()){
            StationSearch.mSearcher.removeHost(this);
        } else if (StationSearch.mSearcher != null){
            StationSearch.mSearcher.dispose();
            StationSearch.mSearcher = null;
        }
        super.onDestroy();
    }

    public static boolean isSearching(){
        boolean result = (mSearcher != null) && (mSearcher.isSearching());
        return result;
    }

    protected boolean getComponents() {
        mTitle = (TextView) findViewById(R.id.title);
        if (mTitle == null) {
            Log.e(LOGTAG, "fail to get title");
            return false;
        }

        mStation =(TextView) findViewById(R.id.station);
        if (mStation == null) {
            Log.e(LOGTAG, "fail to get station");
            return false;
        }

        mFreq =(TextView) findViewById(R.id.freq);
        if (mFreq == null) {
            Log.e(LOGTAG, "fail to get freq");
            return false;
        }

        mProgress =(ProgressBar) findViewById(R.id.progress);
        if (mProgress == null) {
            Log.e(LOGTAG, "fail to get progress");
            return false;
        }
        float range = WheelConfig.RADIO_MAX_FREQUENCY - WheelConfig.RADIO_MIN_FREQUENCY;
        mProgress.setMax((int)(range * ZOOM_FACTOR));
        mProgress.setProgress(0);

        mStop =(Button) findViewById(R.id.stop);
        if (mStop == null) {
            Log.e(LOGTAG, "fail to get stop");
            return false;
        }

        return true;
    }

    //fix bug 20615 begin
    protected Dialog onCreateDialog(int id){
        switch (id) {
            case SHOW_DIALOG:
                /*AlertDialog mAD = new AlertDialog.Builder(StationSearch.this).setTitle(R.string.suggest)
                .setMessage(R.string.suggest_message)
                .setPositiveButton(R.string.button_ok,new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Intent intent = new Intent();
                        intent.setClass(StationSearch.this, EditStationList.class);
                        startActivity(intent);
                        finish();
                    }
                }).setNegativeButton(R.string.button_cancel,new DialogInterface.OnClickListener(){

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        finish();
                    }
                }).create();

                return mAD;*/
                //add for 24388 cmcc new feature
                return new AlertDialog.Builder(StationSearch.this)
                .setCancelable(false) // this dialog should not be canceled until user make it's decision.
                .setTitle(R.string.collection_radio)
                .setMultiChoiceItems(stationNameArray,isSelectArray,
                        new OnMultiChoiceClickListener(){
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which, boolean isChecked) {
                                //...
                                Log.d(LOGTAG,"which="+which+"  isChecked="+isChecked);

                            }
                })
                .setPositiveButton(R.string.collection,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int whichButton) {
                                if(null != isSelectArray){
                                    for(int i=0;i<isSelectArray.length;i++){
                                        boolean a = isSelectArray[i];
                                                if (isSelectArray[i]) {
                                                    if (!FMPlaySharedPreferences.exists(0,stationArray[i].getStationFreq())) {
                                                        boolean exist = FMPlaySharedPreferences.exists(0, stationArray[i].getStationName());
                                                        if(exist){
                                                            int count = FMPlaySharedPreferences.getStationCount(0);
                                                            count++;
                                                            String name = getResources().getString(R.string.station_info)+count;
                                                            stationArray[i].setStationName(name);
                                                            FMPlaySharedPreferences.addStation(0, stationArray[i]);
                                                        }else {
                                                            FMPlaySharedPreferences.addStation(0, stationArray[i]);
                                                        }
                                                    }
                                                }
                                        }
                                    mStationList.save();
                                    finish();
                                }
                            }
                        })
                .setNegativeButton(R.string.button_cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int whichButton) {
                                //.....
                                finish();
                            }
                        })
                .create();
        }
        return null;
    }

    private void initStationArray(List<PresetStation> stationList){
        if(stationList==null||stationList.size()==0){
            return;
        }
        int size = stationList.size();
        stationArray = new PresetStation[size];
        stationNameArray = new String[size];
        isSelectArray = new boolean[size];

        for(int i=0;i<size;i++){
            PresetStation ps = stationList.get(i);
            stationArray[i] = ps;
            stationNameArray[i] = ps.getStationName()+" ("+ps.getFreq()+"MHz)";
            isSelectArray[i] = true;
        }

    }

    //fix bug 20615 end

    protected void setListeners() {
        Log.v(LOGTAG,"-------aaaaa--");
        mStop.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Log.d(LOGTAG, "setOnClickListener, Thread: " + Thread.currentThread().getName());
                if(mSearcher == null){
                    return;
                }
                if(mSearcher.mStopSearching){
                    mHandler.sendMessage(mHandler.obtainMessage(MSG_SEARCH_FINISH));
                    return;
                    }
                try {
                    if(mSearcher!=null){
                        mSearcher.stopSearching();
                        try {
                            mSearcher.join(1000);
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                                }
                            }
                    
                    if(mSearcher.getState() != State.TERMINATED){
//                        mHandler.sendEmptyMessage(STOP_SEARCHING);
                        Message msg = mHandler.obtainMessage(STOP_SEARCHING);
                        msg.obj = mSearcher.getState();
                        mHandler.sendMessage(msg);
                    }else {
                        mHandler.sendMessage(mHandler.obtainMessage(MSG_SEARCH_FINISH));
                    }
                    } catch (Exception e) {
                       e.printStackTrace();
                       Log.d(LOGTAG, "setOnClickListener, Thread: "+e.getMessage());
                       }
            }
        });
    }

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case MSG_UPDATE:
                Bundle data = msg.getData();
                float freq = data.getFloat("freq");
                String name = data.getString("name");
                if (mFreq != null && mStation != null) {
                    mFreq.setText(freq + "MHz");
                    mStation.setText(name);
                }
                mProgress.setProgress((int)((freq - WheelConfig.RADIO_MIN_FREQUENCY) * ZOOM_FACTOR));
                break;
            case STOP_SEARCHING:
                if(FMplayService.getInstance().isFmOn())
                {
                Log.v(LOGTAG, "-------RadioService.getInstance() "+FMplayService.getInstance());
                if(FMplayService.getInstance() != null){
                    FMplayService.getInstance().ismute = true;
                    FMplayService.getInstance().unMute();
                    }
                }
                State mSearchState = (State)msg.obj;
                if(mSearchState != State.TERMINATED){
//                    mHandler.sendEmptyMessageDelayed(STOP_SEARCHING, 200);
                    Message mMsg = mHandler.obtainMessage(STOP_SEARCHING);
                    mMsg.obj = mSearchState;
                    mHandler.sendMessageDelayed(mMsg,200);
                    }else{
                        mHandler.sendMessage(mHandler.obtainMessage(MSG_SEARCH_FINISH));
                        }
                break;
            case MSG_SEARCH_FINISH:
                if(FMplayService.getInstance().isFmOn())
                {
                    FMplayService.getInstance().ismute = true;
                    FMplayService.getInstance().unMute();
                    }

                Log.d(LOGTAG,"MSG_CLOSE recieved!");
                if(StationSearch.mSearcher != null){
                    StationSearch.mSearcher.setmIsFinish(false);
                }

                //fix bug 20615
                /*if(!StationSearch.this.isFinishing()){
                    showDialog(SHOW_DIALOG);
                }*/
                if(null == stationArray || null == stationNameArray
                        || null == isSelectArray){
                    finish();
                } else {
                    if (!StationSearch.this.isFinishing()) {
                        showDialog(SHOW_DIALOG);
                    }
                }

                break;
            default:
            }
        }
    };


    public class Searcher extends Thread implements ServiceConnection {

        private String mSyncRoot = "SyncRoot";

        private ArrayList<PresetStation> mStationListFound = new ArrayList<PresetStation>();

        private Handler mHandler = null;

        private RadioServiceStub mService = null;

        private boolean mIsSearching = false;

        private boolean mIsFinish = false;

        public boolean ismIsFinish() {
            return mIsFinish;
            }

        public void setmIsFinish(boolean mIsFinish) {
            this.mIsFinish = mIsFinish;
            }

        private boolean mStopSearching = false;

        private StationSearch mHost = null;

        private float mLastSearchedStationFreq = WheelConfig.RADIO_MIN_FREQUENCY;

        private String mLastSearchedStationName = "";

        public Searcher(StationSearch host, Handler handler){
            resetHost(host, handler);
        }

        public RadioServiceStub getService(){
            return mService;
        }

        public boolean isSearching(){
            return mIsSearching;
        }

        public void stopSearching(){
            Log.d(LOGTAG, "stopSearching, Thread: " + Thread.currentThread().getName());
            mStopSearching = true;
            mService.stopSeek();
        }

        public void resetHost(StationSearch host, Handler handler){
            synchronized(mSyncRoot){
                mHandler = handler;
                mHost = host;
                if(mHandler != null){
                    Bundle data = new Bundle();
                    data.putFloat("freq", mLastSearchedStationFreq);
                    data.putString("name", mLastSearchedStationName);
                    Message msg = mHandler.obtainMessage(MSG_UPDATE);
                    msg.setData(data);
                    mHandler.sendMessage(msg);
                }
            }
        }

        public void removeHost(StationSearch host){
            if(mHost != host){
                return;
            }else{
                mHost = null;
                mHandler = null;
            }
        }

        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(LOGTAG,"Bind OK! Now start searchThread!");
            super.start();
        }

        public void onServiceDisconnected(ComponentName name) {
            Log.e(LOGTAG,"FMplayService disconected!");
        }

        @Override
        public void start(){

            Log.d(LOGTAG, "Starting search and try bind to service!");
            FMplayService.getInstance().ismute = false;
            FMplayService.getInstance().mute();

            mService = new RadioServiceStub(getApplication(), this);

            if (!mService.bindToService()) {
                Log.e(LOGTAG, "fail to bindToService");
                mService = null;
                return;
            }

            if(mHandler == null){
                Log.e(LOGTAG, "Trying to start seaching with out mHandler set.");
                return;
            }
        }

        public void run() {
           mStationListFound.clear();
           float freq = startFreq;
           float prev = 0.0f;
           int stationNumber = 1;
           mService.setFreq(freq);
           boolean value = false;
           mIsSearching = true;
           setmIsFinish(true);
           Log.d(LOGTAG, "begin searching");
           while (!mStopSearching && freq <= WheelConfig.RADIO_MAX_FREQUENCY) {
               value = mService.startSeek(true);
               if (!value) {
                   Log.e(LOGTAG, "fail to search when searching from" + prev);
                   break;
               }

               freq = mService.getFreq();
               if (freq <= prev || freq >= WheelConfig.RADIO_MAX_FREQUENCY) {
                   Log.e(LOGTAG, "current frequency is unnormal value=" + freq +","+ prev);
                   break;
               }

               if (freq != prev ) {
//                   if (!FMPlaySharedPreferences.exists(0, freq)) {
                       Log.d(LOGTAG, "result freq=" + freq);
                       String name = getResources().getString(R.string.station_info)
                                   + stationNumber;

                       mStationListFound.add(new PresetStation(name, freq));
                       synchronized(mSyncRoot){
                           if(mHandler != null){
                               Bundle data = new Bundle();
                               data.putFloat("freq", freq);
                               data.putString("name", name);
                               Message msg = mHandler.obtainMessage(MSG_UPDATE);
                               msg.setData(data);
                               mHandler.sendMessage(msg);
                           }
                       }

                       mLastSearchedStationFreq = freq;
                       mLastSearchedStationName = name;

                       ++stationNumber;

//                   }
                   prev = freq;
               }
           }

           Log.d(LOGTAG, "end searching");

           float firstFreq = (mStationListFound.size() == 0) ? WheelConfig.RADIO_MIN_FREQUENCY : mStationListFound.get(0).getStationFreq();
           Log.d(LOGTAG, "first Freq:" + firstFreq);
           mService.setFreq(firstFreq);
           FMPlaySharedPreferences.setTunedFreq(firstFreq);

//           for(PresetStation station:mStationListFound){
//               FMPlaySharedPreferences.addStation(0, station.getStationName(), station.getStationFreq());
//           }

           mStationList.save();
           mService.getFreq2(true);//fix bug 10956

           initStationArray(mStationListFound);

           mIsSearching = false;
           mStopSearching = false;

           mService.unbindFromService();
           if(mHandler != null && mStationListFound.size()!=0){
               Log.d(LOGTAG,"Sending MSG_SEARCH_FINISH!");
               mHandler.sendMessage(mHandler.obtainMessage(MSG_SEARCH_FINISH));
           }
        }

        public void dispose(){
            if(isSearching()){
                stopSearching();
                try {
                    this.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}
