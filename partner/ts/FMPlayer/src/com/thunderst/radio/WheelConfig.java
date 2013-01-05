/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import java.text.DecimalFormat;
import java.lang.Number;
import java.text.ParseException;

public class WheelConfig {
    public static final float RADIO_MIN_FREQUENCY = 87.5f;
    public static final float RADIO_MAX_FREQUENCY = 108.0f;
    
    private static DecimalFormat mFormat = new DecimalFormat("####.0");
    
    private float mFreqSpan;    
    private float mFreqInterval;
    private float mMinFreq;    
    private float mMaxFreq;
    private float mHeadPadding;
    private float mTailPadding;
    
    public void setSpan(float span) {
        mFreqSpan = format(span);
    }

    public static float format(float f) {
        float value = f;
        try {
            Number digit = mFormat.parse(mFormat.format(f));
            value = digit.floatValue();
        } catch (ParseException e) {
            e.printStackTrace();
        }
        return value;
    }
    
    public WheelConfig() {}

    public void setBand(float start, float end) {       
        mMinFreq = format(start);
        mMaxFreq = format(end);
    }
    
    public void setHeadPadding(float padding) {
        mHeadPadding = padding;
    }
    
    public void setTailPadding(float padding) {
        mTailPadding = padding;
    }

    public float[] getBand() {
        float[] value = new float[2];
        value[0] = mMinFreq;
        value[1] = mMaxFreq;
        
        return value;
    }

    public float getSpan() {
        return mFreqSpan;
    }
    
    public void setInterval(float interval) {
        mFreqInterval = format(interval);
    }
    
    public float getInterval() {
        return mFreqInterval;
    }


    public float getHeadPadding() {
        return mHeadPadding;
    }

    public float getTailPadding() {
        return mTailPadding;
    }
}
