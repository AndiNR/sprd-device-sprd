/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.util.Log;
import java.util.List;
import java.util.Map;
import android.widget.SimpleAdapter;
import android.widget.ImageView;
import android.widget.TextView;
import android.text.TextUtils.TruncateAt;

public class HighlightSelectedAdapter extends SimpleAdapter {
    private static final String LOGTAG = "HighlightSelectionAdapater";

    private Context mContext;

    private List<? extends Map<String, ?>> mListData;

    private int mResource;

    private String [] mFromRes;

    private int [] mToLayout;

    private int mSelectedListItem;

    public HighlightSelectedAdapter(Context context, 
                                     List<? extends Map<String, ?>> data, 
                                     int resource, 
                                     String[] from, 
                                     int[] to) {
        super(context, data, resource, from, to);
        mContext = context;
        mListData = data;
        mResource = resource;
        mFromRes = from;
        mToLayout = to;
        mSelectedListItem = -1;
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        View row = convertView;
        if (row == null) {
            LayoutInflater inflater = LayoutInflater.from(mContext);
            if (inflater == null) {
                Log.e(LOGTAG, "fail to obtain inflator");
                return null;
            }
            row = inflater.inflate(mResource, null);
        }

        Map<String, ?> map = mListData.get(position);
        for (int i = 0; i < mToLayout.length; ++i) {
            TextView view = (TextView) row.findViewById(mToLayout[i]);
            view.setText(map.get(mFromRes[i]).toString());
        }
        ImageView status = (ImageView) row.findViewById(R.id.status);
        TextView name = (TextView) row.findViewById(R.id.name);
        if (position == mSelectedListItem) {
            status.setVisibility(View.VISIBLE);
            name.setEllipsize(TruncateAt.MARQUEE);
        } else {
            status.setVisibility(View.INVISIBLE);
            name.setEllipsize(TruncateAt.END);
        }

        return row;
    }

    public void setSelectedItem(int item) {
        mSelectedListItem = item;
    }
}
