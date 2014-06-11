package com.acer.ccd.cache.data;

import android.database.Cursor;

import com.acer.ccd.cache.DBContent;
import com.acer.ccd.cache.DBSearchResult;
import com.acer.ccd.debug.L;

/**
 * The object that contains the information what an Image needs.
 */
public class DlnaImage extends DlnaMedia {
    private static final String TAG = "DlnaImage";

    private String mResolution;
    private String mThumbnailUrl;

    @Override
    public void dump() {
        super.dump();
        L.t(TAG, "mResolution = %s, mThumbnailUrl = %s",
                mResolution, mThumbnailUrl);
    }

    public DlnaImage() {
        super();
    }

    public void setResolution(String resolution) {
        this.mResolution = resolution;
    }

    public String getResolution() {
        return mResolution;
    }

    /**
     * @param thumbnailUrl the mThumbnailUrl to set
     */
    public void setThumbnailUrl(String thumbnailUrl) {
        this.mThumbnailUrl = thumbnailUrl;
    }

    /**
     * @return the mThumbnailUrl
     */
    public String getThumbnailUrl() {
        return mThumbnailUrl;
    }

    @Override
    public boolean setDataFromDB(Cursor cursor) {
        boolean bResult = true;
        if (cursor == null)
        {
            bResult = false;
            throw new IllegalArgumentException("Invalid cursor.");
        }

        try {
            setUrl(cursor.getString(DBContent.ColumnName.COL_URL.ordinal()));
            setTitle(cursor.getString(DBContent.ColumnName.COL_TITLE.ordinal()));
            setCreator(cursor.getString(DBContent.ColumnName.COL_CREATOR.ordinal()));
            setDescription(cursor.getString(DBContent.ColumnName.COL_DESCRIPTION.ordinal()));
            setPublisher(cursor.getString(DBContent.ColumnName.COL_PUBLISHER.ordinal()));
            setAlbum(cursor.getString(DBContent.ColumnName.COL_ALBUM.ordinal()));
            setDateTaken(cursor.getString(DBContent.ColumnName.COL_DATE_TAKEN.ordinal()));
            setFileSize(cursor.getLong(DBContent.ColumnName.COL_FILE_SIZE.ordinal()));
            setFormat(cursor.getString(DBContent.ColumnName.COL_FORMAT.ordinal()));
            setResolution(cursor.getString(DBContent.ColumnName.COL_RESOLUTION.ordinal()));
            setAlbumUrl(cursor.getString(DBContent.ColumnName.COL_ALBUMURL.ordinal()));
            setDeviceName(cursor.getString(DBContent.ColumnName.COL_DEVICE_NAME.ordinal()));
            setDeviceUuid(cursor.getString(DBContent.ColumnName.COL_DEVICE_UUID.ordinal()));
            setDbId(cursor.getInt(DBContent.ColumnName.COL_ID.ordinal()));
            setProtocolName(cursor.getString(DBContent.ColumnName.COL_PROTOCOLNAME.ordinal()));
            setThumbnailUrl(cursor.getString(DBContent.ColumnName.COL_THUMBNAILURL.ordinal()));
            setParentContainerId(cursor.getString(DBContent.ColumnName.COL_PARENTCONTAINERID.ordinal()));
        } catch (Exception e) {
            e.printStackTrace();
            bResult = false;
        }

        return bResult;
    }

    @Override
    public boolean setDataFromSearchDB(Cursor cursor) {
        boolean bResult = true;
        if (cursor == null)
        {
            bResult = false;
            throw new IllegalArgumentException("Invalid cursor.");
        }

        try {
            setUrl(cursor.getString(DBSearchResult.ColumnName.COL_URL.ordinal()));
            setTitle(cursor.getString(DBSearchResult.ColumnName.COL_TITLE.ordinal()));
            setAlbum(cursor.getString(DBSearchResult.ColumnName.COL_PHOTOALBUM.ordinal()));
            setAlbumUrl(cursor.getString(DBSearchResult.ColumnName.COL_ALBUMURL.ordinal()));
            setDeviceName(cursor.getString(DBSearchResult.ColumnName.COL_DEVICENAME.ordinal()));
            setDeviceUuid(cursor.getString(DBSearchResult.ColumnName.COL_DEVICEUUID.ordinal()));
            setDbId(cursor.getInt(DBSearchResult.ColumnName.COL_ID.ordinal()));
            setParentContainerId(cursor.getString(DBSearchResult.ColumnName.COL_PARENTCONTAINERID.ordinal()));
        } catch (Exception e) {
            e.printStackTrace();
            bResult = false;
        }

        return bResult;
    }
}
