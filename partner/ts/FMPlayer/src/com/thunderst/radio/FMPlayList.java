/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import android.app.Activity;

import android.media.AudioManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.AdapterView;
import android.widget.TextView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.content.ServiceConnection;
import android.content.ComponentName;
import android.content.Intent;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.widget.AbsListView;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.view.LayoutInflater;
import android.widget.Button;
import android.app.AlertDialog;
import android.app.Dialog;
import java.util.ArrayList;
import java.util.HashMap;

public class FMPlayList extends Activity implements AbsListView.OnScrollListener {
    private static final String LOGTAG = "FMPlayList";
    
    private static final int CHECK_DELAY = 100;
    private static final int MSG_TUNE = 0;
    private static final int SEARCH_DIALOG = 0;
    private ListView mRadioList;
    private Dialog mSearchDialog;

    private ImageButton mSearch;
    private ImageButton mEdit;

    private HighlightSelectedAdapter mAdapter;
    private RadioServiceStub mService;
    private FMPlaySharedPreferences mStationList;
    
    private boolean mReady;
    
    private ServiceConnection mCallback = new ServiceConnection() {
        public void onServiceConnected(ComponentName name, IBinder service) {
            mReady = true;
        }

        public void onServiceDisconnected(ComponentName name) {
            mReady = false;
        }
    };

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case MSG_TUNE:
                Bundle data = msg.getData();
                removeMessages(MSG_TUNE);
                if (mReady) {
                    float freq = data.getFloat("freq");
                    mService.setFreq(freq);
                    FMPlaySharedPreferences.setTunedFreq(freq);
                    int position = data.getInt("position");
                    mAdapter.setSelectedItem(position);
                    mAdapter.notifyDataSetInvalidated();
                    finish();
                } else {
                    Message message = obtainMessage(MSG_TUNE);
                    message.setData(data);
                    sendMessageDelayed(message, CHECK_DELAY);
                }
                break;
            default:
            }
        }
    };

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_FM)) {
                if (intent.hasExtra("state")) {
                    if (intent.getIntExtra("state", 0) == 0) {
                        finish();
                    }
                }
            }else if (action.equals(Intent.ACTION_HEADSET_PLUG)) {
                if (intent.getIntExtra("state", 0) == 0) {
	                finish();
                } 
            }
        }
    };

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setVolumeControlStream(AudioManager.STREAM_FM);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                , WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.radio_list);

        mStationList = FMPlaySharedPreferences.getInstance(this);

        mService = new RadioServiceStub(this, mCallback);

        if (!getComponentsValue()) {
            return;
        }

        setActionListeners();
        mReady = false;

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_FM);
        filter.addAction(Intent.ACTION_HEADSET_PLUG);//mm04 fix bug 8001
        registerReceiver(mReceiver, filter);
    }

    protected void onDestroy() {
        mHandler.removeMessages(MSG_TUNE);
        unregisterReceiver(mReceiver);
        super.onDestroy();
    }

    protected void onStart() {
        super.onStart();
        if (!mService.bindToService()) {
            Log.e(LOGTAG, "fail to bindToService");
            mService = null;
            return;
        }
        Intent intent = new Intent(FMplayService.ACTION_COUNT);
        intent.putExtra("count", 1);
        sendBroadcast(intent);
    }

    protected void onResume() {
        super.onResume();
        mStationList.load();
        showStationList();
    }

    protected void onPause() {
        mStationList.save();
        super.onPause();
    }

    protected void onStop() {
        Intent intent = new Intent(FMplayService.ACTION_COUNT);
        intent.putExtra("count", -1);
        sendBroadcast(intent);
        if (mService != null) {
            mService.unbindFromService();
        }
        super.onStop();
    }

    protected boolean getComponentsValue() {
        mRadioList = (ListView) findViewById(R.id.radio_list);
        if (mRadioList == null) {
            Log.e(LOGTAG, "onCreate: Failed to get radio_list ListView");
            return false;
        }
        mSearch = (ImageButton) findViewById(R.id.search);
        if (mSearch == null) {
            Log.e(LOGTAG, "onCreate: Failed to get search button");
            return false;
        }
        mEdit = (ImageButton) findViewById(R.id.edit);
        if (mEdit == null) {
            Log.e(LOGTAG, "onCreate: Failed to get edit ImageButton");
            return false;
        }
        return true;
    }

    protected Dialog onCreateDialog(int id) {
        switch(id) {
            case SEARCH_DIALOG:
                {
                    LayoutInflater inflater = LayoutInflater.from(this);
                    AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    View view = inflater.inflate(R.layout.search_alert_dialog, null);
                    Button btOK = (Button) view.findViewById(R.id.bt_ok);
                    Button btCancel = (Button) view.findViewById(R.id.bt_cancel);
                    btOK.setOnClickListener(new Button.OnClickListener() {
                        public void onClick(View v) {
//                            FMPlaySharedPreferences.clearList(0);
                            Intent req = new Intent(getBaseContext(), StationSearch.class);
                            req.putExtra("freq",mService.getFreq());
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
            default:
        }

        return null;
    }

    protected void showStationList() {
        ArrayList<HashMap<String, String>> list = new ArrayList<HashMap<String, String>>();
        PresetStationList radioList = FMPlaySharedPreferences.getStationList(0);
        if (radioList == null) {
            return;
        }
        int count = radioList.getCount();
        int position = -1;
        for (int i = 0; i < count; ++i) {
            HashMap<String, String> item = new HashMap<String, String>();
            PresetStation station = radioList.getStation(i);
            if (station == null) {
                Log.e(LOGTAG, "get unexpected null station object");
                return;
            }
            item.put("name", station.getStationName());
            Float freq = new Float(station.getStationFreq());
            item.put("freq", freq.toString());
            list.add(item);
            if (freq == FMPlaySharedPreferences.getTunedFreq()) {
                position = i;
            }
        }

        mAdapter = new HighlightSelectedAdapter(this, list, R.layout.radio_list_item,
                new String[] {
                        "name", "freq"
                }, new int[] {
                        R.id.name, R.id.freq
                });
        mAdapter.setSelectedItem(position);
        mRadioList.setAdapter(mAdapter);
    }

    protected void setActionListeners() {
        mRadioList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                TextView freq = (TextView) view.findViewById(R.id.freq);
                if (freq == null) {
                    return;
                }
                String strFreq = freq.getText().toString();
                Message msg = mHandler.obtainMessage(MSG_TUNE);
                Bundle data = new Bundle();
                data.putFloat("freq", Float.parseFloat(strFreq));
                data.putInt("position", position);
                msg.setData(data);
                mHandler.sendMessage(msg);
            }
        });

        mSearch.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showDialog(SEARCH_DIALOG);
            }
        });

        mEdit.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Intent req = new Intent(v.getContext(), EditStationList.class);
                startActivity(req);
            }
        });
    }

    public void onScroll(AbsListView view, 
                         int firstVisibleItem, 
                         int visibleItemCount,
                         int totalItemCount) {
    }

    public void onScrollStateChanged(AbsListView view, int scrollState) {

    }
}
