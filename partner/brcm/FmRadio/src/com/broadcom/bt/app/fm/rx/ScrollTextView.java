package com.broadcom.bt.app.fm.rx;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.TextView;

public class ScrollTextView extends TextView{

	public ScrollTextView(Context context) {
		super(context);
	}

	public ScrollTextView(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	public ScrollTextView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
	}

	@Override
	public boolean isFocused() {
		return true;
	}

	@Override
	public boolean isInTouchMode() {
		return true;
	}

}
