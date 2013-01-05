/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.thunderst.radio;

import android.view.View;

public class EditListItem {
    private View mItemView;

    private int mItemPosition;

    TextButtonAdapter mAdapter;

    public EditListItem() {
        mItemView = null;
        mItemPosition = -1;
        mAdapter = null;
    }

    public void setListItem(View view) {
        mItemView = view;
    }

    public View getListItem() {
        return mItemView;
    }

    public void setListItemPosition(int position) {
        mItemPosition = position;
    }

    public int getListItemPosition() {
        return mItemPosition;
    }

    public void setAdapter(TextButtonAdapter adapter) {
        mAdapter = adapter;
    }

    public TextButtonAdapter getAdapter() {
        return mAdapter;
    }
}
