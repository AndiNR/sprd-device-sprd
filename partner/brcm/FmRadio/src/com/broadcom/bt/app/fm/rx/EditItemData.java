/**

* Copyright (C) 2009,2010 Thundersoft Corporation

* All rights Reserved

*/

package com.broadcom.bt.app.fm.rx;

import android.view.View;

import com.broadcom.bt.app.fm.rx.StationList.StationAdapter;

/**
 * a class used to save the data of the item in the ListView
 * @author songkun
 *
 */
public class EditItemData {
    private View mView;

    private int mPosition;

    private StationAdapter mAdapter;

    /** constructor */
    public EditItemData() {
        mView = null;
        mPosition = -1;
        mAdapter = null;
    }

    /** save item view
     *
     * @param view item view
     */
    public void setView(View view) {
        mView = view;
    }

    /** save item position
     *
     * @param position item position in the ListView
     */
    public void setPosition(int position) {
        mPosition = position;
    }

    /** save item adapter
     *
     * @param adapter used by ListView
     */
    public void setAdapter(StationAdapter adapter) {
        mAdapter = adapter;
    }

    /** get item view
     *
     * @return saved item view
     */
    public View getView() {
        return mView;
    }

    /** get item position
     *
     * @return saved item position
     */
    public int getPosition() {
        return mPosition;
    }

    /** get item adapter
     *
     * @return saved item adapter
     */
    public StationAdapter getAdapter() {
        return mAdapter;
    }
}
