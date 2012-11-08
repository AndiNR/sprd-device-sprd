package com.broadcom.bt.service;

/**
 *
 * {@hide}
 */
interface IPowerManager
{
    boolean enableFm();	
    boolean enableBt();	
    boolean disableFm();	
    boolean disableBt();
    boolean isfmEnabled();
    boolean isBtEnabled();
}
