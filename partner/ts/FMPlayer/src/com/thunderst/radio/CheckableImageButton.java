package com.thunderst.radio;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.Checkable;
import android.widget.ImageButton;
import android.view.View;
import android.view.View.OnClickListener;

public class CheckableImageButton extends ImageButton implements Checkable,
        OnClickListener {

    public static interface OnCheckedChangedListener {
        public boolean onCheckedChanged(View view, boolean checked);
    }

    boolean mChecked = false;

    int mCheckedDrawable;

    int mUncheckedDrawable;

    int mDisabledDrawable;

    OnCheckedChangedListener mListener = null;

    public void setCheckedChangedListener(OnCheckedChangedListener listener) {
        mListener = listener;
    }

    public CheckableImageButton(Context context) {
        super(context);
        setOnClickListener(this);
    }

    public CheckableImageButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        setOnClickListener(this);
    }

    public CheckableImageButton(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
        setOnClickListener(this);
    }

    public void setDrawable(int checked, int unchecked, int disabled) {
        mCheckedDrawable = checked;
        mUncheckedDrawable = unchecked;
        mDisabledDrawable = disabled;
        updateImageSource();
    }

    @Override
    public void setEnabled(boolean enabled) {
        super.setEnabled(enabled);
        updateImageSource();
    }

    public boolean isChecked() {
        return mChecked;
    }

    public void setChecked(boolean checked) {
        setChecked(checked, true);
    }

    public void setChecked(boolean checked, boolean notifyToListener) {
        if (checked != mChecked) {
            boolean isAllowed = true;
            if (mListener != null && notifyToListener) {
                isAllowed = mListener.onCheckedChanged(this, checked);
            }
            if (isAllowed) {
                mChecked = checked;
                updateImageSource();
            }
        }
    }
    public void toggle() {
        setChecked(!mChecked);
    }

    public void onClick(View v) {
        toggle();
    }

	private void updateImageSource() {
		if (!isEnabled()) {
			setImageResource(mDisabledDrawable);
		} else {
			setImageResource(/* isEnabled() ? ( */mChecked ? mCheckedDrawable
					: mUncheckedDrawable/* ) : mDisabledDrawable */);
		}
	}
}
