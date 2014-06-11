package com.acer.ccd.cache.data;

import com.acer.ccd.cache.DBContent;
import com.acer.ccd.debug.L;

import android.database.Cursor;

/**
 * The object that contains the information what a Container needs.
 */
public class DlnaContainer {
    private static final String TAG = "DlnaContainer";
    private String mContainerId;
    private String mTitle;
    private long mDbId;
    private long mChildCount;
    private String mParentContainerId;
    private String mAlbumUrl;

    public void dump() {
        L.t(TAG, "mDbID = %d, mTitle = %s, mContainerId = %s , mAlbumUrl = %s",
                mDbId, mTitle, mContainerId, mAlbumUrl);
    }

    public DlnaContainer() {
        super();
    }

    public DlnaContainer(String containerId, String title, long childCount) {
        mContainerId = containerId;
        mTitle = title;
        mChildCount = childCount;
    }

    public void setContainerId(String mContainId) {
        this.mContainerId = mContainId;
    }

    public String getContainerId() {
        return mContainerId;
    }

    public void setTitle(String title) {
        this.mTitle = title;
    }

    public String getTitle() {
        return mTitle;
    }

    public void setDbId(long dbId) {
        this.mDbId = dbId;
    }

    public long getDbId() {
        return mDbId;
    }

    public void setChildCount(long childCount) {
        this.mChildCount = childCount;
    }

    public long getChildCount() {
        return mChildCount;
    }

    /**
     * @param parentContainerId the mParentContainerId to set
     */
    public void setParentContainerId(String parentContainerId) {
        this.mParentContainerId = parentContainerId;
    }

    /**
     * @return the mParentContainerId
     */
    public String getParentContainerId() {
        return mParentContainerId;
    }

    public String getAlbumUrl() {
        return mAlbumUrl;
    }

    public void setAlbumUrl(String albumUrl) {
        this.mAlbumUrl = albumUrl;
    }

    public boolean setDataFromDB(Cursor cursor)
    {
        boolean bResult = true;
        if (cursor == null)
        {
            bResult = false;
            throw new IllegalArgumentException("Invalid cursor.");
        }

        try {
            setContainerId(cursor.getString(DBContent.ColumnName.COL_CONTAINERID.ordinal()));
            setTitle(cursor.getString(DBContent.ColumnName.COL_TITLE.ordinal()));
            setDbId(cursor.getLong(DBContent.ColumnName.COL_ID.ordinal()));
            setChildCount(cursor.getLong(DBContent.ColumnName.COL_CHILDCOUNT.ordinal()));
            setParentContainerId(cursor.getString(DBContent.ColumnName.COL_PARENTCONTAINERID.ordinal()));
            setAlbumUrl(cursor.getString(DBContent.ColumnName.COL_ALBUMURL.ordinal()));
        } catch (Exception e) {
            bResult = false;
        }

        return bResult;
    }
}
