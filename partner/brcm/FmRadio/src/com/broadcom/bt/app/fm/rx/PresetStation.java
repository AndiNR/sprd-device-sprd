package com.broadcom.bt.app.fm.rx;

public class PresetStation {
	private String name;
    private int freq;
    private boolean isFavorites;

	public PresetStation(String name, int freq) {
		super();
		this.name = name;
		this.freq = freq;
		this.isFavorites = false;
	}

	public String getName() {
		return name;
	}
	public void setName(String name) {
		this.name = name;
	}
	public int getFreq() {
		return freq;
	}
	public void setFreq(int freq) {
		this.freq = freq;
	}
	public boolean isFavorites() {
		return isFavorites;
	}
	public void setFavorites(boolean isFavorites) {
		this.isFavorites = isFavorites;
	}

}
