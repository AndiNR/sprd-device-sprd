/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;

import android.media.AudioManager;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View.OnClickListener;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.AbsListView;
import android.widget.Button;
import android.widget.Toast;
import android.widget.ImageButton;
import android.text.Editable;
import android.text.InputFilter;
import android.text.Selection;
import android.text.Spanned;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;


public class EditStationList extends Activity implements AbsListView.OnScrollListener {
    private static final String LOGTAG = "EditList";

    private TextView mTitleTextView;
    private AlertDialog mEditDialog;
    private AlertDialog mAddDialog;
    private AlertDialog mClearDialog;
    private AlertDialog mDeleteDialog;

    private static final int TEXT_MAX_LENGTH = 128;

    private static final int EDIT_DIALOG = 1;
    private static final int DELETE_DIALOG = 2;
    private static final int ADD_DIALOG = 3;
    private static final int CLEAR_DIALOG = 4;
    private static final int FREQ_MAX_LENGTH = 5;

    private ListView mEditStationList;

    private ImageButton mOver;
    private ImageButton mAdd;

    private EditListItem mItemData;

    private FMPlaySharedPreferences mStationList;

    private float mItemFreq;
    private int mTopItem;

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_FM)) {
                if (intent.hasExtra("state")) {
                    if(intent.getIntExtra("state", 0) == 0) {
                        finish();
                    }
                }
            } else if (action.equals(Intent.ACTION_HEADSET_PLUG)) {
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
        setContentView(R.layout.edit_list);

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_FM);
        filter.addAction(Intent.ACTION_HEADSET_PLUG);//mm04 fix bug 8001
        registerReceiver(mReceiver, filter);

        mTitleTextView = (TextView) findViewById(R.id.title);
        mTitleTextView.setText(R.string.edit_title);

        if (!getComponentsValue()) {
            return;
        }

        mStationList = FMPlaySharedPreferences.getInstance(this);

        setActionListeners();

        mTopItem = 0;
    }

    protected void onStart() {
        super.onStart();
        Intent intent = new Intent(FMplayService.ACTION_COUNT);
        intent.putExtra("count", 1);
        sendBroadcast(intent);
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()) {
        case Menu.FIRST + 1:
            showDialog(CLEAR_DIALOG);
            break;
        default:
        }
        return false;
    }

    protected void onResume() {
        super.onResume();
        mStationList.load();
        showRadioList();
    }

    protected void onPause() {
        mStationList.save();
        super.onPause();
    }

    protected void onStop() {
        Intent intent = new Intent(FMplayService.ACTION_COUNT);
        intent.putExtra("count", -1);
        sendBroadcast(intent);
        super.onStop();
    }

    protected void onDestroy() {
        unregisterReceiver(mReceiver);
        super.onDestroy();
    }

    @Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		// TODO Auto-generated method stub
        Resources res = getResources();
        PresetStationList radioList = FMPlaySharedPreferences.getStationList(0);
		if (radioList.getCount() > 0) {
			menu.getItem(Menu.NONE).setVisible(true);
		}else{
			menu.getItem(Menu.NONE).setVisible(false);
		}
		return true;
	}

	public boolean onCreateOptionsMenu(Menu menu) {
        Resources res = getResources();
		menu.add(Menu.NONE, Menu.FIRST + 1, 1,
					res.getText(R.string.delete_all)).setIcon(
					R.drawable.menu_delete_all);
        return true;
    }

    protected void showRadioList() {
        ArrayList<HashMap<String, String>> list = new ArrayList<HashMap<String, String>>();
        PresetStationList radioList = FMPlaySharedPreferences.getStationList(0);
        int position = -1;
        int count = radioList.getCount();
        for (int i = 0; i < count; ++i) {
            HashMap<String, String> item = new HashMap<String, String>();
            PresetStation station = radioList.getStation(i);
            if (station == null) {
                Log.e(LOGTAG, "get unexpected null station object");
                return;
            }
            item.put("name", station.getStationName());
            Float freq = new Float(station.getStationFreq());
            if (station.getStationFreq() == FMPlaySharedPreferences.getTunedFreq()) {
                position = i;
            }
            item.put("freq", freq.toString());
            list.add(item);
        }

        OnClickListener[] listeners = new OnClickListener[] {
                new OnClickListener() {
                    public void onClick(View view) {
                        ImageButton btn = (ImageButton) view;
                        mItemData = (EditListItem) btn.getTag();
                        showDialog(EDIT_DIALOG);
                    }
                }, new OnClickListener() {
                    public void onClick(View view) {
                        ImageButton btn = (ImageButton) view;
                        mItemData = (EditListItem) btn.getTag();
                        showDialog(DELETE_DIALOG);
                    }
                }
        };
        TextButtonAdapter adapter = new TextButtonAdapter(this, list, R.layout.edit_list_item,
                new String[] {
                        "name", "freq"
                }, new int[] {
                        R.id.name, R.id.freq
                }, new int[] {
                        R.id.edit, R.id.delete
                }, listeners);
        adapter.setSelectedListItem(position);
        mEditStationList.setAdapter(adapter);
    }

    protected void setupLengthFilter(EditText editText, int length){
        InputFilter[] filters = new InputFilter[1];
        final int maxLength = length;
        filters[0] = new InputFilter.LengthFilter(maxLength){
//            private final int TOAST_INTERVAL = 2000;
//            private long mToastTime = 0;
//            @Override
//            public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend){
//                if(source.length() > 0 && dest.length() <= maxLength && dest.toString().startsWith(".")){
//                    Toast.makeText(EditStationList.this, getResources().getText(R.string.range_format_error), Toast.LENGTH_SHORT).show();
//                    }else if(source.length() > 0 && dest.length() == maxLength && System.currentTimeMillis() - mToastTime > TOAST_INTERVAL){
//                        mToastTime = System.currentTimeMillis();
//                    if(maxLength==5){
//                        Toast.makeText(EditStationList.this, getResources().getText(R.string.input_too_many_error), Toast.LENGTH_SHORT).show();
//                    }else{
//                        Toast.makeText(EditStationList.this, getResources().getText(R.string.input_too_many_chars), Toast.LENGTH_SHORT).show();
//                    }
//                }
//                return super.filter(source, start, end, dest, dstart, dend);
//            }
        };
        editText.setFilters(filters);
    }

    protected Dialog onCreateDialog(int id) {
        final Toast nameInputInfo = Toast.makeText(this, getResources().getString(R.string.name_input_info), Toast.LENGTH_SHORT);
        final Toast nameError = Toast.makeText(this, getResources().getString(R.string.name_error), Toast.LENGTH_SHORT);
        final Toast nameisempty = Toast.makeText(this, getResources().getString(R.string.name_notis_empty), Toast.LENGTH_SHORT);
        final Toast freqInputInfo = Toast.makeText(this, getResources().getString(R.string.freq_input_info),Toast.LENGTH_SHORT);
        final Toast rangeError = Toast.makeText(this, getResources().getString(R.string.range_error),Toast.LENGTH_SHORT);
        final Toast freqError = Toast.makeText(this, getResources().getString(R.string.freq_error),Toast.LENGTH_SHORT);
        final Toast have_been_completed = Toast.makeText(this, getResources().getString(R.string.have_been_completed), Toast.LENGTH_SHORT);// mm04 fix bug 7032
        switch (id) {
            case EDIT_DIALOG:
                {
                    LayoutInflater inflater = LayoutInflater.from(this);
                    AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    View view = inflater.inflate(R.layout.name_freq_alert_dialog, null);
                    TextView title = (TextView) view.findViewById(R.id.title);
                    title.setText(R.string.edit_title);
                    final EditText name = (EditText) view.findViewById(R.id.edit_name);
                    setupLengthFilter(name, TEXT_MAX_LENGTH);
                    final EditText freq = (EditText) view.findViewById(R.id.edit_freq);
                    setupLengthFilter(freq, FREQ_MAX_LENGTH);
                    Button btOK = (Button) view.findViewById(R.id.bt_ok);
                    Button btCancel = (Button) view.findViewById(R.id.bt_cancel);
                    btOK.setOnClickListener(new Button.OnClickListener() {
                        public void onClick(View v) {
                        	String strFreq = freq.getText().toString();
                            String strName = name.getText().toString().trim();
                            if(strFreq.length() == 0){
                            	freqInputInfo.show();
                            }else{
                                if (strFreq.charAt(0) == '.') {
                            	    strFreq = '0' + strFreq;
                                    }
                                float fFreq = Float.parseFloat(strFreq);
                                if (fFreq < WheelConfig.RADIO_MIN_FREQUENCY
                                        || fFreq > WheelConfig.RADIO_MAX_FREQUENCY) {
                                	rangeError.show();
                                }else {
                                    if ((mItemFreq != fFreq) && FMPlaySharedPreferences.exists(0, fFreq)) {
                                        freqError.show();
                                    } else {
                                        if (strName.length() == 0) {
                                            nameInputInfo.show();
                                            }else {
                                                if(strName.replaceAll(" ", "").length()==0){
                                                    nameisempty.show();
                                                } else if (FMPlaySharedPreferences.exists(0, strName)) {
                                                    if(FMPlaySharedPreferences.getStationName(0, mItemFreq).equals(strName)){
                                                        FMPlaySharedPreferences.setStationFreqandName(0, mItemFreq,fFreq,strName);
                                                        mStationList.save();
                                                        mStationList.load();
                                                        showRadioList();
                                                        mEditDialog.cancel();
                                                        have_been_completed.show();
                                                    } else {
                                                        nameError.show();
                                                    }
                                                }else{
                                                    FMPlaySharedPreferences.setStationFreqandName(0, mItemFreq,fFreq,strName);
                                                    mStationList.save();
                                                    mStationList.load();
                                                    showRadioList();
                                                    mEditDialog.cancel();
                                                    have_been_completed.show();
                                                }
                                            }
                                		}
                                	}
                            }
                        }
                    });
                    btCancel.setOnClickListener(new Button.OnClickListener() {
                        public void onClick(View v) {
                            mEditDialog.cancel();
                        }
                    });
                    mEditDialog = builder.setView(view).create();
                    return mEditDialog;
                }
            case DELETE_DIALOG:
                {
                    LayoutInflater inflater = LayoutInflater.from(this);
                    AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    View view = inflater.inflate(R.layout.delete_alert_dialog, null);
                    Button btOK = (Button) view.findViewById(R.id.bt_ok);
                    Button btCancel = (Button) view.findViewById(R.id.bt_cancel);
                    btOK.setOnClickListener(new Button.OnClickListener() {
                        public void onClick(View v) {
                            View item = mItemData.getListItem();
                            TextView name = (TextView) item.findViewById(R.id.name);
                            TextView freq = (TextView) item.findViewById(R.id.freq);
                            if (name == null || freq == null) {
                                return;
                            }
                            String strName = name.getText().toString();
                            String strFreq = freq.getText().toString();
                            float fFreq = Float.parseFloat(strFreq);
                            PresetStation station = new PresetStation(strName, fFreq);
                            FMPlaySharedPreferences.removeStation(0, station);

                            showRadioList();
                            mDeleteDialog.cancel();
                            have_been_completed.show();
                        }
                    });
                    btCancel.setOnClickListener(new Button.OnClickListener() {
                        public void onClick(View v) {
                            mDeleteDialog.cancel();
                        }
                    });
                    mDeleteDialog = builder.setView(view).create();
                    return mDeleteDialog;
                }
            case ADD_DIALOG:
                {
                    LayoutInflater inflater = LayoutInflater.from(this);
                    AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    View view = inflater.inflate(R.layout.name_freq_alert_dialog, null);
                    TextView title = (TextView) view.findViewById(R.id.title);
                    title.setText(R.string.add_title);
                    final EditText freq = (EditText) view.findViewById(R.id.edit_freq);
                    setupLengthFilter(freq, FREQ_MAX_LENGTH);
                    final EditText name = (EditText) view.findViewById(R.id.edit_name);
                    setupLengthFilter(name, TEXT_MAX_LENGTH);
                    Button btOK = (Button) view.findViewById(R.id.bt_ok);
                    Button btCancel = (Button) view.findViewById(R.id.bt_cancel);
                    btOK.setOnClickListener(new Button.OnClickListener() {
                        public void onClick(View v) {
                            String strFreq = freq.getText().toString();
                            String strName = name.getText().toString().trim();
                            if (strFreq.length() == 0) {
                                freqInputInfo.show();
                            } else {
                                if (strFreq.charAt(0) == '.') {
                                    strFreq = '0' + strFreq;
                                }
                                float fFreq = Float.parseFloat(strFreq);
                                if (fFreq < WheelConfig.RADIO_MIN_FREQUENCY
                                    || fFreq > WheelConfig.RADIO_MAX_FREQUENCY) {
                                    rangeError.show();
                                } else {
                                    if (FMPlaySharedPreferences.exists(0, fFreq)) {
                                        freqError.show();
                                    } else {
                                        if (strName.length() == 0) {
                                            nameInputInfo.show();
                                        } else {
                                        	if(strName.replaceAll(" ", "").length()==0){
                                        		nameisempty.show();
                                        	}else if (FMPlaySharedPreferences.exists(0, strName)) {
                                                nameError.show();
                                            } else {
                                                FMPlaySharedPreferences.addStation(0, strName, fFreq, true);
                                                showRadioList();
                                                freq.setText("");
                                                name.setText("");
                                                mAddDialog.cancel();
                                                have_been_completed.show();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    });
                    btCancel.setOnClickListener(new Button.OnClickListener() {
                        public void onClick(View v) {
                            freq.setText("");
                            name.setText("");
                            mAddDialog.cancel();
                        }
                    });

                    mAddDialog = builder.setView(view).create();
                    return mAddDialog;
                }
            case CLEAR_DIALOG:
                {
                    LayoutInflater inflater = LayoutInflater.from(this);
                    AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    View view = inflater.inflate(R.layout.clear_alert_dialog, null);
                    Button btOK = (Button) view.findViewById(R.id.bt_ok);
                    Button btCancel = (Button) view.findViewById(R.id.bt_cancel);
                    btOK.setOnClickListener(new Button.OnClickListener() {
                        public void onClick(View v) {
                            FMPlaySharedPreferences.clearList(0);
                            showRadioList();
                            mClearDialog.cancel();
                        }
                    });
                    btCancel.setOnClickListener(new Button.OnClickListener() {
                        public void onClick(View v) {
                            mClearDialog.cancel();
                        }
                    });
                    mClearDialog = builder.setView(view).create();
                    return mClearDialog;
                }
            default:
        }

        return null;
    }

    protected void onPrepareDialog(int id, Dialog dialog) {
        switch (id) {
            case EDIT_DIALOG:
                String strName = "";
                if (mItemData != null) {
                    View view = mItemData.getListItem();
                    TextView name = (TextView) view.findViewById(R.id.name);
                    if (name != null) {
                        strName = name.getText().toString();
                    }

                    TextView freq = (TextView) view.findViewById(R.id.freq);
                    if (freq != null) {
                        String strFreq = freq.getText().toString();
                        mItemFreq = Float.parseFloat(strFreq);
                    }
                    EditText editFreq = (EditText) dialog.findViewById(R.id.edit_freq);
                    editFreq.setEnabled(true);
                    editFreq.setFocusable(true);
                    editFreq.setText(Float.toString(mItemFreq));
                    EditText editName = (EditText) dialog.findViewById(R.id.edit_name);
                    editName.setText(strName);
                    Editable edit = editName.getText();
                    Selection.setSelection(edit, edit.length());
                }
                break;
            case ADD_DIALOG:
                EditText editFreq = (EditText) dialog.findViewById(R.id.edit_freq);
                editFreq.requestFocus();
                break;
            default:
        }
    }


    protected boolean getComponentsValue() {
        mEditStationList = (ListView) findViewById(R.id.edit_list);
        if (mEditStationList == null) {
            Log.e(LOGTAG, "onCreate: getComponents Failed to get edit_list ListView");
            return false;
        }

        mOver = (ImageButton) findViewById(R.id.over);
        if (mOver == null) {
            Log.e(LOGTAG, "onCreate: getComponents Failed to get over ImageButton");
            return false;
        }

        mAdd = (ImageButton) findViewById(R.id.add);
        if (mAdd == null) {
            Log.e(LOGTAG, "onCreate: getComponents Failed to get add ImageButton");
            return false;
        }

        return true;
    }

    protected void setActionListeners() {
        mOver.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                finish();
            }
        });

        mAdd.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showDialog(ADD_DIALOG);
            }
        });

        mEditStationList.setOnScrollListener(this);
    }

    public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount,
            int totalItemCount) {
        if (visibleItemCount > 0) {
            mTopItem = firstVisibleItem;
        }
    }

    public void onScrollStateChanged(AbsListView view, int scrollState) {

    }
}
