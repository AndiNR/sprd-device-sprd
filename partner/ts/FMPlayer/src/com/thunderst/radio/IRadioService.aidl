package com.thunderst.radio;

interface IRadioService
{
    boolean isMuted();
    boolean setFreq(float freq);
    boolean startSeek(boolean up);
    boolean stopSeek();
    float getFreq();    
    float getFreq2(boolean issech);
    boolean routeAudio(int device); 
    boolean setVolume(int volume);
    int getAudioDevice();
    boolean fmOn();
    boolean fmOff();
    boolean isFmOn();
    boolean mute();
    boolean unMute();
}

