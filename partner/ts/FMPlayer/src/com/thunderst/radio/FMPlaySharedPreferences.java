/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

public class FMPlaySharedPreferences {
    
    private static List<PresetStationList> mListOfPlist = new ArrayList<PresetStationList>();

    private static Map<String, String> mNameMap = new HashMap<String, String>();

    private static final String SHARED_PREFERENCES = "radio_preferences";
    private static final String LAST_TUNED_FREQUENCY = "last_frequency";
    private static final String NUMBER_OF_LIST = "number_of_list";
    private static final String NAME_OF_LIST = "name_of_list";
    private static final String NUMBER_OF_STATION = "number_of_station";
    private static final String NAME_OF_STATION = "name_of_station";
    private static final String DEFAULT_NO_NAME = "";
    private static final String EDIT_STATUS_OF_STATION = "edit_status_of_station";
    
    private static final float DEFAULT_NO_FREQUENCY = 87.5f;
    public static float mTunedFreq = 0.0f;
    private static final boolean DEFAULT_EDIT_STATUS = false;
    private static final String STATION_FREQUENCY = "station_freq";

    private static final String LAST_LIST_INDEX = "last_list_index";


    private static FMPlaySharedPreferences mInstance = null;

    private static int mListIndex;

    private Context mContext;
    
    int numberofFreq;
    protected FMPlaySharedPreferences(Context context) {
        mContext = context.getApplicationContext();
        load();
    }

    public static FMPlaySharedPreferences getInstance(Context context) {
        if (mInstance == null) {
            return new FMPlaySharedPreferences(context);
        }

        return mInstance;
    }

    public static int createPresetList(String name) {
        int list = getListIndex(name);
        if (list != -1) {
            return list;
        }

        int count = mListOfPlist.size();
        mListOfPlist.add(new PresetStationList(name));
        mNameMap.put(name, String.valueOf(count));
        return count;
    }

    public void load() {
        if (mContext == null) {
            return;
        }

        if (mListOfPlist.size() != 0) {
            mListOfPlist.clear();
            mNameMap.clear();
        }

        SharedPreferences sp = mContext.getSharedPreferences(SHARED_PREFERENCES,
                Context.MODE_WORLD_READABLE | Context.MODE_WORLD_WRITEABLE);
        mTunedFreq = sp.getFloat(LAST_TUNED_FREQUENCY, DEFAULT_NO_FREQUENCY);

        int listNumber = sp.getInt(NUMBER_OF_LIST, 0);
        int listIndex = 0;
        int stationIndex = 0;
        while (listIndex < listNumber) {
            String listName = sp.getString(NAME_OF_LIST + listIndex, "FM - " + (listIndex + 1));
            int stationNumber = sp.getInt(NUMBER_OF_STATION + listIndex, 0);
            int list = createPresetList(listName);

            PresetStationList stationList = mListOfPlist.get(list);
            stationIndex = 0;
            while (stationIndex < stationNumber) {
                String stationName = sp.getString(NAME_OF_STATION + listIndex + "x" + stationIndex,
                        DEFAULT_NO_NAME);
                float stationFreq = sp.getFloat(STATION_FREQUENCY + listIndex + "x" + stationIndex,
                        DEFAULT_NO_FREQUENCY);
                boolean stationEditStatus = sp.getBoolean(EDIT_STATUS_OF_STATION + listIndex + "x" + stationIndex,
                        DEFAULT_EDIT_STATUS);
                stationList.addStation(stationName, stationFreq, stationEditStatus);

                ++stationIndex;
            }

            ++listIndex;
        }

        mListIndex = sp.getInt(LAST_LIST_INDEX, 0);
        if (mListIndex >= listNumber) {
            mListIndex = 0;
        }
    }
    public void save() {
        if (mContext == null) {
            return;
        }

        int listNumber = mListOfPlist.size();
        SharedPreferences sp = mContext.getSharedPreferences(SHARED_PREFERENCES,
                Context.MODE_WORLD_READABLE | Context.MODE_WORLD_WRITEABLE);
        SharedPreferences.Editor edit = sp.edit();

        edit.putFloat(LAST_TUNED_FREQUENCY, mTunedFreq);
        edit.putInt(NUMBER_OF_LIST, listNumber);
        edit.putInt(LAST_LIST_INDEX, mListIndex);

        int listIndex = 0;
        int stationIndex = 0;
        while (listIndex < listNumber) {
            PresetStationList stationList = mListOfPlist.get(listIndex);
            edit.putString(NAME_OF_LIST + listIndex, stationList.getStationName());
            int stationNumber = stationList.getCount();
            int numStation = 0;
            stationIndex = 0;
            while (stationIndex < stationNumber) {
                PresetStation station = stationList.getStation(stationIndex);
                if (station != null) {
                    edit.putString(NAME_OF_STATION + listIndex + "x" + numStation, station
                            .getStationName());
                    edit.putFloat(STATION_FREQUENCY + listIndex + "x" + numStation, station
                            .getStationFreq());
                    edit.putBoolean(EDIT_STATUS_OF_STATION + listIndex + "x" + numStation, station
                            .getStationEditStatus());
                    ++numStation;
                }

                ++stationIndex;
            }
            edit.putInt(NUMBER_OF_STATION + listIndex, numStation);

            ++listIndex;
        }

        edit.commit();
    }

    public static float getTunedFreq() {
        return mTunedFreq;
    }

    public static void setTunedFreq(float freq) {
        mTunedFreq = freq;
    }

    public static void addStation(int list, PresetStation station) {
        if (checkIndex(list)) {
            mListOfPlist.get(list).addStation(station);
        }
    }

    public static int getStationCount(int list) {
        if (checkIndex(list)) {
            return mListOfPlist.get(list).getCount();
        }

        return -1;
    }

    public static PresetStation getStation(int list, int station) {
        if (checkIndex(list)) {
            return mListOfPlist.get(list).getStation(station);
        }

        return null;
    }

    public static void addStation(int list, String name, float freq) {
        if (checkIndex(list)) {
            mListOfPlist.get(list).addStation(name, WheelConfig.format(freq));
        }
    }

    public static void addStation(int list, String name, float freq, boolean status) {
        if (checkIndex(list)) {
            mListOfPlist.get(list).addStation(name, WheelConfig.format(freq), status);
        }
    }

    public static PresetStation getStation(int list, float freq) {
        if (checkIndex(list)) {
            return mListOfPlist.get(list).getStation(freq);
        }

        return null;
    }

    public static int getListIndex(String name) {
        if (mNameMap.containsKey(name)) {
            return Integer.parseInt(mNameMap.get(name));
        }

        return -1;
    }

    public static String getListName(int list) {
        int count = mListOfPlist.size();
        String name = null;
        if (list >= 0 && list < count) {
            name = mListOfPlist.get(list).getStationName();
        }

        return name;
    }

    public static PresetStation getPrevStation(int list, float freq) {
        PresetStation station = null;
        PresetStation first = getStation(0, 0);
        PresetStation last = getStation(0, getStationCount(0)-1);
        if (first != null) {
            if(freq <= first.getStationFreq()){
                station = last;
                }else{
                    if (checkIndex(list)) {
                        PresetStationList stations = mListOfPlist.get(list);
                        int count = stations.getCount();
                        for (int i=count - 1; i>=0; --i) {
                            if (stations.getStation(i).getStationFreq() < freq) {
                                station = stations.getStation(i);
                                break;
                                }
                            }
                        }
                    }
            }

        return station;
    }

    public static PresetStation getNextStation(int list, float freq) {
        PresetStation station = null;
        PresetStation first = getStation(0, 0);
        PresetStation last = getStation(0, getStationCount(0)-1);
        if(last != null){
            if(freq >= last.getStationFreq()){
                station = first;
                }else{
                    if (checkIndex(list)) {
                        PresetStationList stations = mListOfPlist.get(list);
                        int count = stations.getCount();
                        for (int i=0; i<count; ++i) {
                            if (stations.getStation(i).getStationFreq() > freq) {
                                station = stations.getStation(i);
                                break;
                                }
                            }
                        }
                    }
            }
        return station;
    }

    public static String getStationName(int list, int station) {
        if (checkIndex(list)) {
            PresetStation tmp = mListOfPlist.get(list).getStation(station);
            if (tmp != null) {
                return tmp.getStationName();
            }
        }

        return null;
    }

    public static void removeStation(int list, int station) {
        if (checkIndex(list)) {
            mListOfPlist.get(list).removeStation(station);
        }
    }

    public static String getStationName(int list, float freq) {
        if (checkIndex(list)) {
            PresetStation tmp = mListOfPlist.get(list).getStation(freq);
            if (tmp != null) {
                return tmp.getStationName();
            }
        }

        return null;
    }

    public static void removeStation(int list, PresetStation station) {
        if (checkIndex(list)) {
            mListOfPlist.get(list).removeStation(station);
        }
    }

    public static boolean  getStationEditStatus(int list, int station) {
        if (checkIndex(list)) {
            PresetStation tmp = mListOfPlist.get(list).getStation(station);
            if (tmp != null) {
                return tmp.getStationEditStatus();
            }
        }

        return false;
    }

    public static boolean getStationEditStatus(int list, float freq) {
        if (checkIndex(list)) {
            PresetStation tmp = mListOfPlist.get(list).getStation(freq);
            if (tmp != null) {
                return tmp.getStationEditStatus();
            }
        }

        return false;
    }

    public static PresetStationList getStationList(int list) {
        if (!checkIndex(list)) {
            return null;
        }

        return mListOfPlist.get(list);
    }

    public static void removeStation(int list, float freq) {
        if (checkIndex(list)) {
            mListOfPlist.get(list).removeStation(freq);
        }
    }

    public static boolean exists(int list, float freq) {
        if (!checkIndex(list)) {
            return false;
        }

        return mListOfPlist.get(list).exists(freq);
    }

    public static void renameListName(int list, String name) {
        if (checkIndex(list)) {
            mListOfPlist.get(list).setName(name);
        }
    }

    public static void setListName(int list, String name) {
        if (checkIndex(list)) {
            mListOfPlist.get(list).setName(name);
        }
    }

    public static void setStationName(int list, int station, String name) {
        if (checkIndex(list)) {
            PresetStation tmp = mListOfPlist.get(list).getStation(station);
            if (tmp != null) {
                tmp.setStationName(name);
            }
        }
    }

    public static void setStationName(int list, float freq, String name) {
        if (checkIndex(list)) {
            PresetStation tmp = mListOfPlist.get(list).getStation(freq);
            if (tmp != null) {
                tmp.setStationName(name);
            }
        }
    }
    
    public static void setStationEditStatus(int list, float freq, boolean status) {
        if (checkIndex(list)) {
            PresetStation tmp = mListOfPlist.get(list).getStation(freq);
            if (tmp != null) {
                tmp.setStationEditStatus(status);
            }
        }
    }

    public static void setStationFreqandName(int list,float oldFreq,float newFreq,String name){
    	if (checkIndex(list)) {
            PresetStation tmp = mListOfPlist.get(list).getStation(oldFreq);
            if(tmp != null){
            	tmp.setStationName(name);
            	tmp.setStationFreq(newFreq);
            }
    	}
    }
    
    public static void setStationEditStatus(int list, int station, boolean status) {
        if (checkIndex(list)) {
            PresetStation tmp = mListOfPlist.get(list).getStation(station);
            if (tmp != null) {
                tmp.setStationEditStatus(status);
            }
        }
    }

    public static boolean exists(int list, String name) {
        boolean value = false;
        if (checkIndex(list)) {
            value = mListOfPlist.get(list).exists(name);
        }

        return value;
    }
    
    public static void clearUneditedStationsInList(int list) {
        if (checkIndex(list)) {
            mListOfPlist.get(list).clearUneditedStations();
        }
    }

    public static void clearList(int list) {
        if (checkIndex(list)) {
            mListOfPlist.get(list).clear();
        }
    }

    private static boolean checkIndex(int list) {
        int count = mListOfPlist.size();
        if (list == count) {
            createPresetList(String.valueOf(list));
        }
        count = mListOfPlist.size();
        if (list >= count || list < 0) {
            return false;
        }

        return true;
    }
}
