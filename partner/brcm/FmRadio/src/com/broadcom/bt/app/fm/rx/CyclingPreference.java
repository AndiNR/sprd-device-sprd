package com.broadcom.bt.app.fm.rx;

import android.content.Context;
import android.preference.ListPreference;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;
import com.broadcom.bt.app.fm.R;

public class CyclingPreference extends ListPreference {

	private TextView mValueText;

	public CyclingPreference(Context context, AttributeSet attrs) {
		super(context, attrs);
		setLayoutResource(R.layout.preference_with_value);
	}

	public CyclingPreference(Context context) {
		super(context, null);
		setLayoutResource(R.layout.preference_with_value);
	}

	@Override
	protected void onBindView(View view) {
		super.onBindView(view);
		mValueText = (TextView) view.findViewById(R.id.preference_value);
		if (mValueText != null) {
			mValueText.setText(getEntry());
		}
	}

	@Override
	public void setValue(String value) {
		super.setValue(value);
		if (mValueText != null) {
			mValueText.setText(getEntry());
		}
	}

	@Override
	public void onClick() {
		int newIndex = findIndexOfValue(getValue());
		newIndex++;
		if (newIndex == getEntries().length)
			newIndex = 0;
		setValueIndex(newIndex);
	}
}
