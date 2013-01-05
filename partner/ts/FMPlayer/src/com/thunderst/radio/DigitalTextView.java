package com.thunderst.radio;

import java.lang.reflect.Field;
import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;

public class DigitalTextView extends LinearLayout {

    String mDigitalText = "";

    String mResourcePrefix = "";
    private static final String TAG = "DigitalTextView";
    private boolean DEBUG = false;

    public DigitalTextView(Context context) {
        super(context);
        init();
    }

    public DigitalTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        this.setOrientation(LinearLayout.HORIZONTAL);
    }

    int getTimeDrawable(int index){
        int drawableId=-1;
        switch(index){
        case 0:
          drawableId=R.drawable.time_0;
          break;
        case 1:
          drawableId=R.drawable.time_1;
          break;
        case 2:
          drawableId=R.drawable.time_2;
          break;
        case 3:
          drawableId=R.drawable.time_3;
          break;
        case 4:
          drawableId=R.drawable.time_4;
          break;
        case 5:
          drawableId=R.drawable.time_5;
          break;
        case 6:
          drawableId=R.drawable.time_6;
          break;
        case 7:
          drawableId=R.drawable.time_7;
          break;
        case 8:
          drawableId=R.drawable.time_8;
          break;
        case 9:
          drawableId=R.drawable.time_9;
          break;
       }
       return drawableId;
    }

    int getFreqDrawable(int index){
       int drawableId=-1;
       switch(index){
       case 0:
         drawableId=R.drawable.freq_0;
         break;
       case 1:
         drawableId=R.drawable.freq_1;
         break;
       case 2:
         drawableId=R.drawable.freq_2;
         break;
       case 3:
         drawableId=R.drawable.freq_3;
         break;
       case 4:
         drawableId=R.drawable.freq_4;
         break;
       case 5:
         drawableId=R.drawable.freq_5;
         break;
       case 6:
         drawableId=R.drawable.freq_6;
         break;
       case 7:
         drawableId=R.drawable.freq_7;
         break;
       case 8:
         drawableId=R.drawable.freq_8;
         break;
       case 9:
         drawableId=R.drawable.freq_9;
         break;
      }
      return drawableId;
    }

    private int getResourceForChar(char c){
        String fieldName="";
        if(c=='.'){
              if(!mResourcePrefix.equals("time")){
                   return R.drawable.freq_dot;
              }
        } else if(c==':'){
              if(mResourcePrefix.equals("time")){
                   return R.drawable.time_colon;
              }
        } else if(c>='0' && c<='9'){
              if(mResourcePrefix.equals("time")){
                   return getTimeDrawable(c-'0');
              }else{
                   return getFreqDrawable(c-'0');
              }
        } else {
              return -1;
        }
        return -1;
    }
//    private int getResourceForChar(char c) {
//        String fieldName = "";
//        if (c == '.') {
//            fieldName = mResourcePrefix + "_dot";
//        } else if (c == ':') {
//            fieldName = mResourcePrefix + "_colon";
//        } else if (c >= '0' && c <= '9') {
//            fieldName = mResourcePrefix + "_" + c;
//        } else {
//            return -1;
//        }
//
//        Class resClass = R.drawable.class;
//
//        Field field = null;
//        try {
//            field = resClass.getField(fieldName);
//        } catch (Exception e) {
//            e.printStackTrace();
//            return -1;
//        }
//
//        try {
//            return field.getInt(null);
//        } catch (Exception e) {
//            e.printStackTrace();
//            return -1;
//        }
//    }

    private ImageView createImageView() {
        ImageView imageView = new ImageView(getContext());
        LayoutParams param = new LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        imageView.setLayoutParams(param);
        return imageView;
    }

    public void setResourcePrefix(String resourcePrefix) {
        mResourcePrefix = resourcePrefix;
    }

    public void setDigitalText(String text) {
        mDigitalText = text;
        updateView();
    }

    private void updateView() {

        if (DEBUG) {
            Log.d(TAG, "getChildCount()" + getChildCount());
            Log.d(TAG, "mDigitalText" + mDigitalText);
        }

        int startIndex = getChildCount() - mDigitalText.length();
        if(startIndex < 0)
            startIndex = 0;

        for (int i = 0; i < startIndex; i++) {
            getChildAt(i).setVisibility(View.GONE);
        }

        for (int i = 0; i < mDigitalText.length(); i++) {
            int childId = i + startIndex;
            int resId = getResourceForChar(mDigitalText.charAt(i));

            if (resId != -1) {
                if (childId == getChildCount()) {
                    addView(createImageView());
                }

                ImageView child = ((ImageView)getChildAt(childId));
                child.setVisibility(View.VISIBLE);
                child.setImageResource(resId);

            } else {
            }
        }
    }
}
