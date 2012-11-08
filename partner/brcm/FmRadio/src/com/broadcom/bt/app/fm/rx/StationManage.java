package com.broadcom.bt.app.fm.rx;

import java.util.ArrayList;
import java.util.List;
import java.util.Comparator;
import java.util.Collections;

import com.broadcom.bt.app.fm.FmConstants;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.util.Log;

public class StationManage {
	public static final String STATION_NAME = "station_name_";
	public static final String STATION_FREQ = "station_freq_";
	public static final String STATION_ISFAVORITES = "station_isfavorites_";
	public static final String STATION_TOTAL = "station_total";

	private SharedPreferences mSharedPrefs;
	private List<PresetStation> stations;
	private IRadioViewRxTouchEventHandler mTouchEventhandler;
	private int stationTotal;

    class StationComparator implements Comparator {
        public int compare(Object arg0, Object arg1) {
            PresetStation station0 = (PresetStation)arg0;
            PresetStation station1 = (PresetStation)arg1;

            int result = -1;

            if (station0.getFreq() > station1.getFreq()) {
                result = 1;
            } else if (station0.getFreq() == station1.getFreq()) {
                result = 0;
            }

            return result;
        }
    }

    private StationComparator mStationComparator = new StationComparator();

	public StationManage(IRadioViewRxTouchEventHandler eventHandler,SharedPreferences SharedPrefs){
		this.mTouchEventhandler = eventHandler;
		this.mSharedPrefs = SharedPrefs;
		stations = new ArrayList<PresetStation>();
		load();
	}

	public void load() {
		if(stations == null){
			stations = new ArrayList<PresetStation>();
		}
		clearStation();

		stationTotal = mSharedPrefs.getInt(STATION_TOTAL, 0);

		for(int i=0;i<stationTotal;i++){
			PresetStation stationBean = new PresetStation(
					mSharedPrefs.getString(STATION_NAME+i, ""),
					mSharedPrefs.getInt(STATION_FREQ+i, -1));
			if(stationBean.getFreq()!=-1){
				stations.add(stationBean);
			}
		}
	}

	public void save(){
		removeStation();

		stationTotal = stations.size();

		if(stationTotal>0){
			Editor e = mSharedPrefs.edit();
			for(int i=0;i<stationTotal;i++){
				PresetStation ps = stations.get(i);
				if(ps.getFreq()!=-1){
					e.putString(STATION_NAME+i, ps.getName());
					e.putInt(STATION_FREQ+i, ps.getFreq());
					e.putBoolean(STATION_ISFAVORITES+i, ps.isFavorites());
				}
			}
			e.putInt(STATION_TOTAL, stationTotal);
			e.apply();
		}
	}

	private void removeStation(){
		Editor e = mSharedPrefs.edit();
		for(int i=0;i<stationTotal;i++){
			e.remove(STATION_NAME+i);
			e.remove(STATION_FREQ+i);
			e.remove(STATION_ISFAVORITES+i);
		}
		e.commit();
	}

	public boolean startSeek(int direction){
		return mTouchEventhandler.seekSearch(direction);
	}

	public void setFreq(int freq){
		mTouchEventhandler.setFrequency(freq);
	}

	public void tuneRadio(int freq){
		mTouchEventhandler.tuneRadio(freq);
	}

	public int getFreq(){
		return mTouchEventhandler.getFrequency();
	}

	public void setTunedFreq(int freq){
		mTouchEventhandler.setTunedFreq(freq);
	}

	public boolean isExistStation(int freq){
		for(PresetStation ps:stations){
			if(ps.getFreq()==freq){
				return true;
			}
		}
		return false;
	}

	public boolean isExistStation(String name){
		for(PresetStation ps:stations){
			if(ps.getName().equals(name)){
				return true;
			}
		}
		return false;
	}

	public void clearStation(){
		if(!stations.isEmpty()){
			stations.clear();
		}
	}

	public List<PresetStation> getStations() {
		return stations;
	}

	public void setStations(List<PresetStation> stations) {
		this.stations = stations;
        Collections.sort(stations, mStationComparator);
	}

	public void addStation(PresetStation ps){
		stations.add(ps);
        Collections.sort(stations, mStationComparator);
	}

	public void addStation(String name,int freq){
		stations.add(new PresetStation(name,freq));
        Collections.sort(stations, mStationComparator);
	}

	public void deleteStation(String name,int freq){
		PresetStation station = null;
		for(PresetStation ps:stations){
			if(ps.getFreq() == freq){
				station = ps;
				break;
			}
		}
		if(station != null){
			Log.d("StationList","remove station freq="+station.getFreq());
			stations.remove(station);
//			save();
		}
	}

	public String getStationName(int freq){
		String name = "";
		for(PresetStation ps:stations){
			if(ps.getFreq() == freq){
				name = ps.getName();
				break;
			}
		}
		return name;
	}

	public void setStationName(int freq,String name){
		for(PresetStation ps:stations){
			if(ps.getFreq() == freq){
				ps.setName(name);
				break;
			}
		}
	}

}
