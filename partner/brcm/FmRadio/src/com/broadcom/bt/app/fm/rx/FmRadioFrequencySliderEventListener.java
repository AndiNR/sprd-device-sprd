package com.broadcom.bt.app.fm.rx;

public interface FmRadioFrequencySliderEventListener {

	public void onSliderDown();	
	public void onSliderDrag(int freq);
	public void onSliderSet(int freq);
	public void onSliderCancel();
}
