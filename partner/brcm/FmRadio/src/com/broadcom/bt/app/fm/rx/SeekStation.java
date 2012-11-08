/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.broadcom.bt.app.fm.rx;

import java.lang.Thread.State;
import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.IntentFilter;
import android.content.DialogInterface.OnMultiChoiceClickListener;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.view.WindowManager;
import android.content.res.Configuration;
import com.broadcom.bt.app.fm.FmConstants;
import com.broadcom.bt.app.fm.R;
import com.broadcom.bt.service.fm.FmReceiver;

public class SeekStation extends Activity {
    private static final String TAG = "SeekStation";

    private static final int MSG_UPDATE = 1;

    private static final int MSG_SEARCH_FINISH = 2;

    private static final int STOP_SEARCHING = 3;

    private static final int SELECT_DIALOG = 4;

    private TextView mTitle;

    private TextView mStation;

    private TextView mFreq;

    private ProgressBar mProgress;

    private Button mStop;

    private CheckBox cb;

    private StationManage mStationManage;

    private PresetStation[] stationArray;
    private String[] stationNameArray;
    private boolean[] isSelectArray;

    private Dialog mSelectDialog;

    public static Searcher mSearcher = null;

    private BroadcastReceiver mReceiver = new BroadcastReceiver(){

		@Override
		public void onReceive(Context context, Intent intent) {
			Log.d(TAG,"onReceive action="+intent.getAction());
		    if (intent.getAction().equalsIgnoreCase(Intent.ACTION_HEADSET_PLUG)) {
		        int state = intent.getIntExtra("state", 0);
		        if(state == 0){
		            if (mSearcher != null && mSearcher.isSearching()) {
		                mSearcher.stopSearching();
		                try {
		                    mSearcher.join();
		                } catch(InterruptedException e) {
		                    e.printStackTrace();
		                }
		            }
		        }
		    }else if(intent.getAction().equalsIgnoreCase(AudioManager.ACTION_AUDIO_BECOMING_NOISY)){
                    if (mSearcher != null && mSearcher.isSearching()) {
		                mSearcher.stopSearching();
		                try {
		                    mSearcher.join();
		                } catch(InterruptedException e) {
		                    e.printStackTrace();
		                }
		            }
		    }else if(intent.getAction().equals(Intent.ACTION_AIRPLANE_MODE_CHANGED)){
				boolean isAirplane = Settings.System.getInt(getContentResolver(),Settings.System.AIRPLANE_MODE_ON,0) == 1;
				if(isAirplane){
					   if (mSearcher != null && mSearcher.isSearching()) {
			                mSearcher.stopSearching();
			                try {
			                    mSearcher.join();
			                } catch(InterruptedException e) {
			                    e.printStackTrace();
			                }
			            }
				}
			}

		}

    };

    public static boolean isSearching(){
    	boolean result = (mSearcher != null) && (mSearcher.isSearching());
    	return result;
    }

	public void onCreate(Bundle savedInstanceState) {
    	Log.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.seek_station);

        if (!getComponents()) {
            return;
        }
        setListeners();
        mStationManage = FmRadio.getStationManage();

        if(SeekStation.isSearching()){
            SeekStation.mSearcher.resetHost(this, mHandler);
	    } else{
	       SeekStation.mSearcher = new Searcher(this, mHandler);
	       SeekStation.mSearcher.start();
        }

  	  IntentFilter filter = new IntentFilter();
	  filter.addAction(Intent.ACTION_HEADSET_PLUG);
//	  filter.addAction(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
	  filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
	  registerReceiver(mReceiver,filter);
    }

    protected void onDestroy() {
        Log.d(TAG, "onDestroy()");
        mHandler.removeMessages(MSG_SEARCH_FINISH);
        mHandler.removeMessages(MSG_UPDATE);

        unregisterReceiver(mReceiver);

        if(SeekStation.isSearching()){
            SeekStation.mSearcher.removeHost(this);
        } else if (SeekStation.mSearcher != null){
            SeekStation.mSearcher.dispose();
            SeekStation.mSearcher = null;
        }

        super.onDestroy();
    }

    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    public void onBackPressed() {

        if (mSearcher != null && mSearcher.isSearching()) {
            mSearcher.stopSearching();
            try {
                mSearcher.join();
            } catch(InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    protected boolean getComponents() {
        mTitle = (TextView) findViewById(R.id.title);
        if (mTitle == null) {
            Log.e(TAG, "fail to get title");
            return false;
        }

        mStation =(TextView) findViewById(R.id.station);
        if (mStation == null) {
            Log.e(TAG, "fail to get station");
            return false;
        }

        mFreq =(TextView) findViewById(R.id.freq);
        if (mFreq == null) {
            Log.e(TAG, "fail to get freq");
            return false;
        }

        mProgress =(ProgressBar) findViewById(R.id.progress);
        if (mProgress == null) {
            Log.e(TAG, "fail to get progress");
            return false;
        }
        int range = FmConstants.MAX_FREQUENCY_US_EUROPE-FmConstants.MIN_FREQUENCY_US_EUROPE;
        mProgress.setMax(range);
        mProgress.setProgress(0);

        mStop =(Button) findViewById(R.id.stop);
        if (mStop == null) {
            Log.e(TAG, "fail to get stop");
            return false;
        }

        return true;
    }

    protected void setListeners() {
        mStop.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	Log.d(TAG, "setOnClickListener, Thread: " + Thread.currentThread().getName());
                if(mSearcher == null || mSearcher.mStopSearching){
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
                    	mHandler.sendEmptyMessage(STOP_SEARCHING);
                    }else {
                    	mHandler.sendMessage(mHandler.obtainMessage(MSG_SEARCH_FINISH));
                    }
				} catch (Exception e) {
					// TODO Auto-generated catch block
					//e.printStackTrace();
					Log.d(TAG, "setOnClickListener, Thread: "+e.getMessage());
				}
            }
        });
    }

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case MSG_UPDATE:
                Bundle data = msg.getData();
                int freq = data.getInt("freq");
                String name = data.getString("name");
                if (mFreq != null && mStation != null) {
                    mFreq.setText(freq/100.0 + "MHz");
                    mStation.setText(name);
                }
                Log.d("StationList", "setprogress freq="+freq);
                mProgress.setProgress(freq - FmConstants.MIN_FREQUENCY_US_EUROPE);
                break;
            case MSG_SEARCH_FINISH:
//                Log.d(TAG,"MSG_CLOSE recieved!");
            	if(stationArray==null || stationArray.length==0
            		|| stationNameArray==null || stationNameArray.length==0
            		|| isSelectArray==null || isSelectArray.length==0){
            		finish();
            	}else{
            		showDialog(SELECT_DIALOG);
            	}

                break;
            case STOP_SEARCHING:
            	if(mSearcher.getState() != State.TERMINATED){
            		mHandler.sendEmptyMessageDelayed(STOP_SEARCHING, 200);
            	}else{
            		mHandler.sendMessage(mHandler.obtainMessage(MSG_SEARCH_FINISH));
            	}

            	break;
            default:
            }
        }
    };

    protected Dialog onCreateDialog(int id) {
    	switch(id){
    	case SELECT_DIALOG:
//    		LayoutInflater inflater = LayoutInflater.from(this);
//    		View view = inflater.inflate(R.layout.collection_radio_title, null);
//            cb = (CheckBox) view.findViewById(R.id.total_collection);
//            cb.setOnCheckedChangeListener(new OnCheckedChangeListener(){
//				@Override
//				public void onCheckedChanged(CompoundButton buttonView,
//						boolean isChecked) {
//					Log.d(TAG," cb isChecked="+isChecked);
//					for(int i=0;i<isSelectArray.length;i++){
//						isSelectArray[i]=isChecked;
//					}
//					mSelectDialog.invalidateOptionsMenu();
//				}
//            });
            return new AlertDialog.Builder(SeekStation.this)
                    .setCancelable(false) // this dialog should not be canceled until user make it's decision.
    				.setTitle(R.string.collection_radio)
		            .setMultiChoiceItems(stationNameArray,isSelectArray,
		            		new OnMultiChoiceClickListener(){
								@Override
								public void onClick(DialogInterface dialog,
										int which, boolean isChecked) {
									//...
									Log.d(TAG,"which="+which+"  isChecked="+isChecked);

								}
		            })
					.setPositiveButton(R.string.collection,
							new DialogInterface.OnClickListener() {
								public void onClick(DialogInterface dialog,
										int whichButton) {
									mStationManage.clearStation();
									for(int i=0;i<isSelectArray.length;i++){
										if(isSelectArray[i]){
											mStationManage.addStation(stationArray[i]);
										}
									}
									mStationManage.save();
									finish();
								}
							})
					.setNegativeButton(R.string.cancel,
							new DialogInterface.OnClickListener() {
								public void onClick(DialogInterface dialog,
										int whichButton) {
									//.....
									finish();
								}
							})
					.create();
    	default:
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
    		stationNameArray[i] = ps.getName()+" ("+ps.getFreq()/100.0+"MHz)";
    		isSelectArray[i] = true;
    	}

    }

    /**
     * A thread used to search stations.
     * It'll be a static member of class StationSearch and runs at application level.
     * @author zhang dawei
     *
     */
    public class Searcher extends Thread {

        private String mSyncRoot = "SyncRoot";

        private List<PresetStation> mStationListFound = null;

        private Handler mHandler = null;

        private boolean mIsSearching = false;

        private boolean mStopSearching = false;

        private SeekStation mHost = null;

        private int SEEKSTATION_RETRYNUM = 3;

        private int mLastSearchedStationFreq = FmConstants.MIN_FREQUENCY_US_EUROPE;

        private String mLastSearchedStationName = "";

        public Searcher(SeekStation host, Handler handler){
        	mStationListFound =  new ArrayList<PresetStation>();
            resetHost(host, handler);
        }

        public boolean isSearching(){
            return mIsSearching;
        }

        public void stopSearching(){
        	Log.d(TAG, "stopSearching, Thread: " + Thread.currentThread().getName());
            mStopSearching = true;
        }

        public void resetHost(SeekStation host, Handler handler){
            synchronized(mSyncRoot){
                mHandler = handler;
                mHost = host;
                if(mHandler != null){
                    Bundle data = new Bundle();
                    data.putInt("freq", mLastSearchedStationFreq);
                    data.putString("name", mLastSearchedStationName);
                    Message msg = mHandler.obtainMessage(MSG_UPDATE);
                    msg.setData(data);
                    mHandler.sendMessage(msg);
                }
            }
        }

        //Warning: this method currently works well, but needs to be improved.
        // We need a better solution to fix the situation below:
        // An older StationSearch activity may destroyed after a newer StationSearch activity.
        // we test if mHost equals to host, so we can avoid that older host removes newer host in its onDestroy method;
        public void removeHost(SeekStation host){
            if(mHost != host){
                return;
            }else{
                mHost = null;
                mHandler = null;
            }
        }

        @Override
        public void start(){
            Log.d(TAG, "Starting search and try bind to service!");

            if(mHandler == null){
                Log.e(TAG, "Trying to start seaching with out mHandler set.");
                return;
            }
            super.start();
        }

        public void run() {
           mStationListFound.clear();
           int freq = FmConstants.MIN_FREQUENCY_US_EUROPE;
           int prev = 0;
           int stationNumber = 1;
           mStationManage.tuneRadio(freq);
           boolean value = false;
           mIsSearching = true;
           Log.d(TAG, "begin searching");
           int retryNum = 0;
           while (!mStopSearching && freq <= FmConstants.MAX_FREQUENCY_US_EUROPE) {
               value = mStationManage.startSeek(FmReceiver.SCAN_MODE_UP);

               if (!value) { // failed
            	   Log.w(TAG, "WARNING , fm device is busy! try later.");
	               try {
	            	   retryNum ++;
	            	   Thread.sleep(500);
				   } catch (InterruptedException e) {
	                   e.printStackTrace();
	        	   }

				   if(retryNum >= SEEKSTATION_RETRYNUM){
					   Log.e(TAG, "Failed three consecutive times to search when searching from" + prev);
	                   break;
				   }else{
					   Log.d(TAG, "Retry Search when searching from" + prev);
					   continue;
				   }
               }

               retryNum = 0;
               freq = mStationManage.getFreq();
               if (freq <= prev || freq >= FmConstants.MAX_FREQUENCY_US_EUROPE) {
                   Log.e(TAG, "current frequency is unnormal value=" + freq +","+ prev);
                   break;
               }

               if (freq != prev) {
            	   Log.d(TAG, "result freq=" + freq);
            	   String name = getResources().getString(R.string.station_info)+ stationNumber;
            	   mStationListFound.add(new PresetStation(name, freq));
            	   synchronized(mSyncRoot){
            		   if(mHandler != null){
            			   Bundle data = new Bundle();
            			   data.putInt("freq", freq);
            			   data.putString("name", name);
            			   Message msg = mHandler.obtainMessage(MSG_UPDATE);
            			   msg.setData(data);
            			   mHandler.sendMessage(msg);
            		   }
            	   }
            	   mLastSearchedStationFreq = freq;
            	   mLastSearchedStationName = name;

            	   ++stationNumber;
                   prev = freq;
               }
           }//while end

           Log.d(TAG, "end searching");

           //Tune to first station, or the minimum frequency if no station saved
           int firstFreq = (mStationListFound.size() == 0) ? FmConstants.MIN_FREQUENCY_US_EUROPE : mStationListFound.get(0).getFreq();
           Log.d(TAG, "first Freq:" + firstFreq);
           mStationManage.tuneRadio(firstFreq);
           mStationManage.setTunedFreq(firstFreq);

           initStationArray(mStationListFound);

           mIsSearching = false;
           mStopSearching = false;

           if(mHandler != null && mStationListFound.size()!=0){
               Log.d(TAG,"Sending MSG_SEARCH_FINISH!");
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
