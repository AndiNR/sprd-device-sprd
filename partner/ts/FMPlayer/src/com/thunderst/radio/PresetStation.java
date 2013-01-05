/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

public class PresetStation {
    private String mStationName;

    private float mStationFreq;
    private boolean mIsStationEdited;

    public PresetStation(String name, float freq) {
        this(name, freq, false);
    }

    public PresetStation(String name, float freq, boolean edit) {
        mStationName = name;
        mStationFreq = freq;
        mIsStationEdited = edit;
    }

    public PresetStation(PresetStation station) {
        this(station.mStationName, station.mStationFreq, station.mIsStationEdited);
    }

    public float getStationFreq() {
        return mStationFreq;
    }

    public void setStationName(String name) {
        mStationName = name;
    }

    public String getStationName() {
        return mStationName;
    }

    public boolean getStationEditStatus() {
        return mIsStationEdited;
    }

    public void setStationEditStatus(boolean status) {
        mIsStationEdited = status;
    }

    public boolean equals(PresetStation station) {
        return mStationFreq == station.mStationFreq;
    }

    public float getFreq() {
        return mStationFreq;
    }

    public void setStationFreq(float freq) {
        mStationFreq = freq;
    }

}
