/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import java.util.ArrayList;
import java.util.List;

public class PresetStationList {
    private List<PresetStation> mStationList;

    private String mStationName;

    public PresetStationList(String name) {
        mStationName = name;
        mStationList = new ArrayList<PresetStation>();
    }

    public String getStationName() {
        return mStationName;
    }

    public void setmStationName(String name) {
        mStationName = name;
    }

    public String toString() {
        return mStationName;
    }

    public synchronized int getCount() {
        return mStationList.size();
    }

    public synchronized PresetStation getStation(float freq) {
        PresetStation value = null;
        int count = mStationList.size();
        for (int i = 0; i < count; ++i) {
            if (mStationList.get(i).getFreq() == freq) {
                value = mStationList.get(i);
                break;
            }
        }

        return value;
    }

    public synchronized PresetStation getStation(int index) {
        PresetStation value = null;
        if (index >= 0 && index < mStationList.size()) {
            value = mStationList.get(index);
        }

        return value;
    }

    public synchronized void addStation(PresetStation station) {
        if (station != null) {
            addStation(station.getStationName(), station.getStationFreq());
        }
    }

    public synchronized void addStation(String name, float freq) {
        PresetStation tmp;
        int index = 0;
        int count = mStationList.size();
        do {
            if (index >= count) {
                mStationList.add(new PresetStation(name, freq));
                return;
            }

            tmp = mStationList.get(index);

            if (tmp.getFreq() >= freq) {
                if (tmp.getFreq() == freq) {
                    mStationList.remove(index);
                }
                mStationList.add(index, new PresetStation(name, freq));

                return;
            }

            ++index;
        } while (true);
    }

    public synchronized void addStation(String name, float freq, boolean status) {
        PresetStation tmp;
        int index = 0;
        int count = mStationList.size();
        do {
            if (index >= count) {
                mStationList.add(new PresetStation(name, freq, status));
                return;
            }

            tmp = mStationList.get(index);

            if (tmp.getFreq() >= freq) {
                if (tmp.getFreq() == freq) {
                    mStationList.remove(index);
                }
                mStationList.add(index, new PresetStation(name, freq, status));

                return;
            }

            ++index;
        } while (true);
    }

    public synchronized void clearUneditedStations() {
        int size = mStationList.size();
        List<PresetStation> uneditedStations = new ArrayList<PresetStation>();
        for (int i=0; i<size; ++i) {
            PresetStation station = mStationList.get(i);
            if (!station.getStationEditStatus()) {
                uneditedStations.add(station);
            }
        }

        if (uneditedStations.size()>0) {
            mStationList.removeAll(uneditedStations);
        }
    }

    public void setName(String name) {
        mStationName = name;
    }

    public synchronized void clear() {
        mStationList.clear();
    }

    public synchronized void removeStation(float freq) {
        for (int i = 0; i < mStationList.size(); ++i) {
            PresetStation station = mStationList.get(i);
            if (station.getFreq() == freq) {
                mStationList.remove(i);
                return;
            }
        }
    }


    public synchronized void removeStation(int index) {
        int count = mStationList.size();
        if (index >= 0 && index < count) {
            mStationList.remove(index);
        }
    }

    public synchronized void removeStation(PresetStation station) {
        removeStation(station.getFreq());
    }

    public synchronized boolean exists(float freq) {
        int count = mStationList.size();
        for (int i = 0; i < count; ++i) {
            if (mStationList.get(i).getFreq() == freq) {
                return true;
            }
        }

        return false;
    }

    public synchronized boolean exists(String name) {
        int count = mStationList.size();
        for (int i = 0; i < count; ++i) {
            if (mStationList.get(i).getStationName().equals(name)) {
                return true;
            }
        }

        return false;
    }
}
