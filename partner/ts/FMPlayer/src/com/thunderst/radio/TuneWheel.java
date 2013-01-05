/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

public class TuneWheel extends View {

    public interface OnTuneWheelValueChangedListener {
        public void onTuneWheelValueChanged(View v, float changedBy);
    }

    private static final String LOGTAG = "TuneWheel";

    private static final int LINE_SPAN = 8;
    private static final int DRAG_THRESHOLD = 5;
    private static final float CHANGE_THRESHOLD = 0.1f;
    public static final int DIRECTION_PREV = 1;
    public static final int DIRECTION_NEXT = 2;

    private float mLastDragPos;
    private boolean mIsMoving = false;

    private float mCurrentPos = 0;
    private float mCurrentChange = 0;
    private int mDragEnable = DIRECTION_PREV | DIRECTION_NEXT;

    private OnTuneWheelValueChangedListener mListener = null;

    public void setOnValueChangedListener(OnTuneWheelValueChangedListener listener) {
        mListener = listener;
    }

    public void setDragEnable(int direction, boolean enable) {
        if (enable) {
            mDragEnable = mDragEnable | direction;
        } else {
            mDragEnable = mDragEnable ^ direction;
        }
    }

    public boolean getDragEnable(int direction) {
        return (mDragEnable & direction) != 0;
    }

    public TuneWheel(Context context) {
        super(context);
    }

    protected void onDraw(Canvas canvas) {
        Paint mLinePaint = new Paint();
        mLinePaint.setARGB(0xEC, 0x00, 0, 0);
        mLinePaint.setStrokeWidth(0.5f);
        for (int pos = 0; pos + mCurrentPos < getWidth(); pos += LINE_SPAN) {
            canvas.drawLine(pos + mCurrentPos, 0, pos + mCurrentPos, getHeight(), mLinePaint);
        }
    }

    public TuneWheel(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public TuneWheel(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public boolean onTouchEvent(MotionEvent event) {
        if (!this.isEnabled())
            return false;

        int action = event.getAction();
                
        switch(action) {
        case MotionEvent.ACTION_DOWN:
            mLastDragPos = event.getX();
            break;
        case MotionEvent.ACTION_MOVE:
            float currentDragPos = event.getX();
            
            if (!getDragEnable(getDirection(currentDragPos, mLastDragPos)))
                return false;

            if (!mIsMoving) {
                if (Math.abs(currentDragPos - mLastDragPos) > DRAG_THRESHOLD) {
                    mIsMoving = true;
                    mLastDragPos = currentDragPos;
                } else {
                    return false;
                }
            } else {
                float tempPos = (mCurrentPos + currentDragPos - mLastDragPos) % LINE_SPAN;

                mCurrentChange += calculateChange(currentDragPos, mLastDragPos);
                if (Math.abs(mCurrentChange) > CHANGE_THRESHOLD) {
                    if (mListener != null) {
                        mListener.onTuneWheelValueChanged(this, mCurrentChange);
                    }
                    mCurrentChange = 0;
                }

                mCurrentPos = tempPos;
                invalidate();

                mLastDragPos = currentDragPos;
            }
            break;
        case MotionEvent.ACTION_UP:
            if (mIsMoving) {
                mIsMoving = false;
            }
            break;
        }

        return true;
    }

    private int getDirection(float current, float last) {
        return (int)(Math.signum(current - last) + 3) / 2;
    }
 
    private float calculateChange(float current, float last) {
        float sub = current - last;
        return Math.signum(sub) * (float)Math.pow(Math.abs(sub), 1.3) / 100;
    }
}
