/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import android.widget.BaseAdapter;
import android.content.Context;
import java.util.List;
import java.util.Map;
import android.view.View;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.widget.TextView;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.text.TextUtils.TruncateAt;
import android.util.Log;

public class TextButtonAdapter extends BaseAdapter {
    private static final String LOGTAG = "TextButtonAdapter";

    private Context mContext;

    private List<? extends Map<String, ?>> mListData;

    private String[] mFromData;
    private int[] mToData;
    private int mResource;
    private int[] mButtonIds;

    private OnClickListener[] mButtonListeners;
    private int mSelectedListItem;

    public TextButtonAdapter(Context context, List<? extends Map<String, ?>> data, int resource,
            String[] from, int[] to, int[] btnIds, OnClickListener[] btnListeners) {
        mContext = context;
        mListData = data;
        mResource = resource;
        mFromData = from;
        mToData = to;
        mButtonIds = btnIds;
        mButtonListeners = btnListeners;
    }

    public Object getItem(int position) {
        return mListData.get(position);
    }

    public void removeItem(int position) {
        if (position >= mListData.size()) {
            return;
        }
        mListData.remove(position);
    }

    public int getCount() {
        return mListData.size();
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
        for (int i = 0; i < mToData.length; ++i) {
            TextView view = (TextView) row.findViewById(mToData[i]);
            view.setText(map.get(mFromData[i]).toString());
        }
        for (int j = 0; j < mButtonListeners.length; ++j) {
            ImageButton btn = (ImageButton) row.findViewById(mButtonIds[j]);
            EditListItem data = new EditListItem();
            data.setListItem(row);
            data.setListItemPosition(position);
            data.setAdapter(this);
            btn.setTag(data);
            btn.setOnClickListener(mButtonListeners[j]);
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

    public long getItemId(int position) {
        return position;
    }

    public void setSelectedListItem(int item) {
        mSelectedListItem = item;
    }
}
