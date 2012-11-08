/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.broadcom.bt.app.fm.rx;

import java.util.List;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.text.Editable;
import android.text.Selection;
import android.text.TextUtils.TruncateAt;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import java.text.DecimalFormat;
import java.lang.Number;
import java.text.ParseException;
import com.broadcom.bt.app.fm.FmConstants;
import com.broadcom.bt.app.fm.R;
import android.content.res.Configuration;
import android.view.Menu;
import android.view.MenuItem;
/**
 * a class to show station list
 * @author songkun
 *
 */
public class StationList extends Activity {
    private static final String LOGTAG = "StationList";

    private static final int MSG_TUNE = 0;

    private static final int EDIT_DIALOG = 1;

    private static final int DELETE_DIALOG = 2;

    private static final int ADD_DIALOG = 3;

    private static final int CLEAR_MENU = 0;

    private HeadsetPlugUnplugBroadcastReceiver mHeadsetPlugUnplugBroadcastReceiver;

    private AlertDialog mAddDialog;

    private EditItemData mItemData;

    private ListView mStationList;

    private ImageButton mSearch;

    private ImageButton mAdd;

    private StationAdapter mAdapter;

    private static final int SEARCH_DIALOG = 0;

    private Dialog mDeleteDialog;

    private Dialog mEditDialog;

    private Dialog mSearchDialog;

    private int mItemFreq;

    private StationManage mStationManage;

    private static DecimalFormat mFormat = new DecimalFormat("####.0");

    private static float format(float f) {
        float value = f;
        try {
            Number digit = mFormat.parse(mFormat.format(f));
            value = digit.floatValue();
        } catch (ParseException e) {
            Log.w(LOGTAG, "failed to parse");
        }

        return value;
    }

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case MSG_TUNE:
                Bundle data = msg.getData();
                removeMessages(MSG_TUNE);
                int freq = data.getInt("freq");
                mStationManage.setFreq(freq);
                mStationManage.setTunedFreq(freq);
                int position = data.getInt("position");
                mAdapter.setSelectedItem(position);
                mAdapter.notifyDataSetInvalidated();
                break;
            default:
            }
        }
    };

    public class HeadsetPlugUnplugBroadcastReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			Log.d(LOGTAG,"onReceive action="+intent.getAction());
		    if (intent.getAction().equalsIgnoreCase(Intent.ACTION_HEADSET_PLUG)) {
		        int state = intent.getIntExtra("state", 0);
		        if(state == 0){
		        	StationList.this.finish();
		        }
		    }else if(intent.getAction().equalsIgnoreCase(AudioManager.ACTION_AUDIO_BECOMING_NOISY)){
		    	StationList.this.finish();
		    }else if(intent.getAction().equals(Intent.ACTION_AIRPLANE_MODE_CHANGED)){
				boolean isAirplane = Settings.System.getInt(getContentResolver(),Settings.System.AIRPLANE_MODE_ON,0) == 1;
				if(isAirplane){
					StationList.this.finish();
				}
			}
		}
	}

    /**
     * do some things to create RadioList
     * @param savedInstanceState the last state
     *
     */
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.station_list);

        mStationManage = FmRadio.getStationManage();
        if (null == mStationManage) {
            Log.e(LOGTAG, "onCreate: Failed to get mStationManage");
            return;
        }

        if (!getComponents()) {
            return;
        }
        setListeners();
        mStationManage.load();

        if (mHeadsetPlugUnplugBroadcastReceiver == null){
	    	  mHeadsetPlugUnplugBroadcastReceiver = new HeadsetPlugUnplugBroadcastReceiver();
	      }
		  IntentFilter filter = new IntentFilter();
		  filter.addAction(Intent.ACTION_HEADSET_PLUG);
//		  filter.addAction(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
		  filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
		  registerReceiver(mHeadsetPlugUnplugBroadcastReceiver,filter);
    }

	protected void onDestroy() {
        mHandler.removeMessages(MSG_TUNE);
        unregisterReceiver(mHeadsetPlugUnplugBroadcastReceiver);
        super.onDestroy();
    }

    protected void onResume() {
        super.onResume();
        showStationList();
    }

    protected void onPause() {
    	mStationManage.save();
        super.onPause();
    }

    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        menu.add(0, CLEAR_MENU, 0, R.string.menu_clear).setIcon(R.drawable.edit_delete_normal);
        return true;
    }

    public boolean onPrepareOptionsMenu(Menu menu) {
        MenuItem item = menu.findItem(CLEAR_MENU);
        if (item != null) {
            if (mStationManage != null) {
                List<PresetStation> stations = mStationManage.getStations();
                item.setEnabled(!stations.isEmpty());
            }
        }

        return super.onPrepareOptionsMenu(menu);
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()) {
            case CLEAR_MENU: {
                if (mStationManage != null) {
                    mStationManage.clearStation();
                    if (mAdapter != null) {
                        mAdapter.notifyDataSetInvalidated();
                    }
                }
            } break;
        }

        return super.onOptionsItemSelected(item);
    }

    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    protected boolean getComponents() {
    	mStationList = (ListView) findViewById(R.id.station_list);
        if (mStationList == null) {
            Log.e(LOGTAG, "onCreate: Failed to get radio_list ListView");
            return false;
        }

        mSearch = (ImageButton) findViewById(R.id.search);
        if (mSearch == null) {
            Log.e(LOGTAG, "onCreate: Failed to get search button");
            return false;
        }

        mAdd = (ImageButton) findViewById(R.id.add);
        if (mAdd == null) {
            Log.e(LOGTAG, "onCreate: Failed to get edit ImageButton");
            return false;
        }

        return true;
    }

    protected void setListeners() {
    	mStationList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            	Log.d("StationList","mStationList Item");
                TextView freq = (TextView) view.findViewById(R.id.freq);
                if (freq == null) {
                    return;
                }
                String strFreq = freq.getText().toString();
                Message msg = mHandler.obtainMessage(MSG_TUNE);
                Bundle data = new Bundle();
                data.putInt("freq", (int)(Float.parseFloat(strFreq)*100));
                data.putInt("position", position);
                msg.setData(data);
                mHandler.sendMessage(msg);
            }
        });

        mSearch.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	Log.d("StationList","mSearch button");
                if (mStationManage != null) {
                    List<PresetStation> stations = mStationManage.getStations();
                    if (!stations.isEmpty()) {
                        showDialog(SEARCH_DIALOG);
                        return;
                    }
                }

                Intent req = new Intent(StationList.this, SeekStation.class);
                startActivity(req);
            }
        });

        mAdd.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	Log.d("StationList","mAdd button");
            	showDialog(ADD_DIALOG);
            }
        });
    }

    protected void showStationList() {
    	List<PresetStation> stationList = mStationManage.getStations();
    	if(stationList == null){
    		return;
    	}
    	int position = -1;
    	for(int i=0;i<stationList.size();i++){
    		if(stationList.get(i).getFreq() == mStationManage.getFreq()){
    			position = i;
    			Log.d(LOGTAG,"curr position="+position);
    			break;
    		}
    	}

    	View.OnClickListener listenerEdit = new View.OnClickListener() {
            public void onClick(View view) {
                ImageButton btn = (ImageButton) view;
                mItemData = (EditItemData) btn.getTag();
                Log.d("StationList","listenerEdit button");
                showDialog(EDIT_DIALOG);
            }
    	};
    	View.OnClickListener listenerDelete = new View.OnClickListener() {
            public void onClick(View view) {
                ImageButton btn = (ImageButton) view;
                mItemData = (EditItemData) btn.getTag();
                Log.d("StationList","listenerDelete button");
                showDialog(DELETE_DIALOG);
            }
    	};

        mAdapter = new StationAdapter(this, stationList,listenerEdit,listenerDelete);
        mAdapter.setSelectedItem(position);
        mStationList.setAdapter(mAdapter);

//        for(PresetStation ps:stationList){
//     	   Log.d(LOGTAG,"********************list "+ps.getName()+"  "+ps.getFreq()/100.0);
//        }
    }

    protected void onPrepareDialog(int id, Dialog dialog) {
        switch (id) {
            case EDIT_DIALOG:
                String strName = "";
                if (mItemData != null) {
                    View view = mItemData.getView();
                    TextView name = (TextView) view.findViewById(R.id.name);
                    if (name != null) {
                        strName = name.getText().toString();
                    }

                    TextView freq = (TextView) view.findViewById(R.id.freq);
                    if (freq != null) {
                        String strFreq = freq.getText().toString();
                        mItemFreq = (int)(Float.parseFloat(strFreq)*100);
                    }
                    EditText editFreq = (EditText) dialog.findViewById(R.id.edit_freq);
                    editFreq.setText(freq.getText().toString());
                    Editable edit = editFreq.getText();
                    Selection.setSelection(edit, edit.length());
                    editFreq.requestFocus();
                    EditText editName = (EditText) dialog.findViewById(R.id.edit_name);
                    editName.setText(strName);
                }
                break;
            case ADD_DIALOG:
                EditText editFreq = (EditText) dialog.findViewById(R.id.edit_freq);
                editFreq.requestFocus();
                break;
            default:
        }
    }

	protected Dialog onCreateDialog(int id) {
		final Toast nameInputInfo = Toast.makeText(this, getResources()
				.getString(R.string.name_input_info), Toast.LENGTH_SHORT);
		final Toast nameError = Toast.makeText(this,
				getResources().getString(R.string.name_error),
				Toast.LENGTH_SHORT);
		final Toast nameisempty = Toast.makeText(this, getResources()
				.getString(R.string.name_notis_empty), Toast.LENGTH_SHORT);
		final Toast freqInputInfo = Toast.makeText(this, getResources()
				.getString(R.string.freq_input_info), Toast.LENGTH_SHORT);
		final Toast rangeError = Toast.makeText(this,
				getResources().getString(R.string.range_error),
				Toast.LENGTH_SHORT);
		final Toast freqError = Toast.makeText(this,
				getResources().getString(R.string.freq_error),
				Toast.LENGTH_SHORT);
		final Toast have_been_completed = Toast.makeText(this, getResources()
				.getString(R.string.have_been_completed), Toast.LENGTH_SHORT);
		switch (id) {
		case EDIT_DIALOG: {
			LayoutInflater inflater = LayoutInflater.from(this);
			AlertDialog.Builder builder = new AlertDialog.Builder(this);
			View view = inflater.inflate(R.layout.name_freq_alert_dialog, null);
			TextView title = (TextView) view.findViewById(R.id.title);
			title.setText(R.string.edit_title);
            final EditText freq = (EditText) view.findViewById(R.id.edit_freq);
			final EditText name = (EditText) view.findViewById(R.id.edit_name);
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
                        float value = format(Float.parseFloat(strFreq));
                        int fFreq = (int) (value * 100);
                        if (fFreq < FmConstants.MIN_FREQUENCY_US_EUROPE
                                || fFreq > FmConstants.MAX_FREQUENCY_US_EUROPE) {
                            rangeError.show();
                        } else {
                            if (fFreq != mItemFreq && mStationManage.isExistStation(fFreq)) {
                                freqError.show();
                            } else {
                                if (strName.length() == 0) {
                                    nameInputInfo.show();
                                } else {
                                    if (strName.replaceAll(" ", "").length() == 0) {
                                        nameisempty.show();
                                    } else if (mStationManage.isExistStation(strName) && !mStationManage.getStationName(mItemFreq).equals(strName)) {
                                        nameError.show();
                                    } else {
                                        mStationManage.deleteStation("", mItemFreq);
                                        mStationManage.addStation(strName, fFreq);

                                        if (fFreq != mItemFreq) {
                                            int position = -1;
                                            if (fFreq == mStationManage.getFreq()) {
                                                List<PresetStation> stationList = mStationManage.getStations();
                                                for(int i=0;i<stationList.size();i++){
                                                    if(stationList.get(i).getFreq() == fFreq){
                                                        position = i;
                                                        break;
                                                    }
                                                }
                                            }

                                            if ((mItemFreq == mStationManage.getFreq()) ||
                                                (fFreq == mStationManage.getFreq())) {
                                                mAdapter.setSelectedItem(position);
                                            }
                                        }

                                        mAdapter.notifyDataSetInvalidated();
                                        mEditDialog.cancel();
                                        mStationManage.save();
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

		case DELETE_DIALOG: {
			LayoutInflater inflater = LayoutInflater.from(this);
			AlertDialog.Builder builder = new AlertDialog.Builder(this);
			View view = inflater.inflate(R.layout.delete_alert_dialog, null);
			Button btOK = (Button) view.findViewById(R.id.bt_ok);
			Button btCancel = (Button) view.findViewById(R.id.bt_cancel);
			btOK.setOnClickListener(new Button.OnClickListener() {
				public void onClick(View v) {
					View item = mItemData.getView();
					TextView name = (TextView) item.findViewById(R.id.name);
					TextView freq = (TextView) item.findViewById(R.id.freq);
					if (name == null || freq == null) {
						return;
					}
					String strName = name.getText().toString();
					String strFreq = freq.getText().toString();
					int fFreq = (int) (Float.parseFloat(strFreq) * 100);
					mStationManage.deleteStation(strName, fFreq);
					showStationList();
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
					Intent req = new Intent(StationList.this,
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

		case ADD_DIALOG: {
			LayoutInflater inflater = LayoutInflater.from(this);
			AlertDialog.Builder builder = new AlertDialog.Builder(this);
			View view = inflater.inflate(R.layout.name_freq_alert_dialog, null);
			TextView title = (TextView) view.findViewById(R.id.title);
			title.setText(R.string.add_title);
			final EditText freq = (EditText) view.findViewById(R.id.edit_freq);
			final EditText name = (EditText) view.findViewById(R.id.edit_name);
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
                        float value = format(Float.parseFloat(strFreq));
						int fFreq = (int) (value * 100);
						if (fFreq < FmConstants.MIN_FREQUENCY_US_EUROPE
								|| fFreq > FmConstants.MAX_FREQUENCY_US_EUROPE) {
							rangeError.show();
						} else {
							if (mStationManage.isExistStation(fFreq)) {
								freqError.show();
							} else {
								if (strName.length() == 0) {
									nameInputInfo.show();
								} else {
									if (strName.replaceAll(" ", "").length() == 0) {
										nameisempty.show();
									} else if (mStationManage
											.isExistStation(strName)) {
										nameError.show();
									} else {
										mStationManage.addStation(strName, fFreq);
                                        int position = -1;
                                        List<PresetStation> stationList = mStationManage.getStations();
                                        for(int i=0;i<stationList.size();i++){
                                            if(stationList.get(i).getFreq() == mStationManage.getFreq()){
                                                position = i;
                                                break;
                                            }
                                        }

                                        if (position != -1) {
                                            mAdapter.setSelectedItem(position);
                                        }

										mAdapter.notifyDataSetInvalidated();
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

		default:
		}
		return null;
	}


    class StationAdapter extends BaseAdapter {
        private static final String TAG = "StationAdapter";

        private Context mContext;

        private List<PresetStation> mData;

        private View.OnClickListener mBtnListeners_edit;

        private View.OnClickListener mBtnListeners_delete;

        private int mSelectedItem;

        public StationAdapter(Context context, List<PresetStation> data,
        		OnClickListener listenerEdit,OnClickListener ListenersDelete) {
            mContext = context;
            mData = data;
            mBtnListeners_edit = listenerEdit;
            mBtnListeners_delete = ListenersDelete;
            mSelectedItem = -1;
        }
        /**
         * get data count
         * @return data count
         */
        public int getCount() {
            return mData.size();
        }

        /**
         * get item ID
         * @param position item index
         * @return item ID
         */
        public long getItemId(int position) {
            return position;
        }

        /**
         * get item
         * @param position item index
         * @return item object
         */
        public Object getItem(int position) {
            return mData.get(position);
        }

        /**
         * get item view
         * @param position item index
         * @param convertView the view to be converted
         * @param parent the container that contains the item
         * @return item view
         */
        public View getView(int position, View convertView, ViewGroup parent) {
        	if(convertView==null){
        		LayoutInflater inflater = LayoutInflater.from(mContext);
        		convertView = inflater.inflate(R.layout.station_list_item, null);
        	}
        	if(convertView==null) return null;

        	PresetStation psBean = mData.get(position);

        	TextView viewName = (TextView) convertView.findViewById(R.id.name);
        	viewName.setText(psBean.getName());
        	TextView viewFreq = (TextView) convertView.findViewById(R.id.freq);
        	viewFreq.setText((psBean.getFreq()/100.0)+"");

        	ImageButton btn_edit = (ImageButton) convertView.findViewById(R.id.edit);
        	EditItemData data_edit = new EditItemData();
        	data_edit.setView(convertView);
        	data_edit.setPosition(position);
        	data_edit.setAdapter(this);
        	btn_edit.setTag(data_edit);
        	btn_edit.setOnClickListener(mBtnListeners_edit);

        	ImageButton btn_delete = (ImageButton) convertView.findViewById(R.id.delete);
        	EditItemData data_delete = new EditItemData();
        	data_delete.setView(convertView);
        	data_delete.setPosition(position);
        	data_delete.setAdapter(this);
        	btn_delete.setTag(data_delete);
        	btn_delete.setOnClickListener(mBtnListeners_delete);

            ImageView status = (ImageView) convertView.findViewById(R.id.status);
            if (position == mSelectedItem) {
                status.setVisibility(View.VISIBLE);
                viewName.setEllipsize(TruncateAt.MARQUEE);
            } else {
                status.setVisibility(View.INVISIBLE);
                viewName.setEllipsize(TruncateAt.END);
            }

            return convertView;
        }

        /**
         * remove item
         * @param position item index
         */
        public void removeItem(int position) {
            if (position >= mData.size()) {
                return;
            }
            mData.remove(position);
        }

        /**
         * @param item set selected item position
         */
        public void setSelectedItem(int item) {
            mSelectedItem = item;
        }
    }

}
