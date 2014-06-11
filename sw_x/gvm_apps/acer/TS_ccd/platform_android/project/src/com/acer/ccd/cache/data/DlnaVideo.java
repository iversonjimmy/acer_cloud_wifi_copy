package com.acer.ccd.cache.data;

import com.acer.ccd.cache.DBContent;
import com.acer.ccd.cache.DBSearchResult;
import com.acer.ccd.debug.L;

import android.database.Cursor;

/**
 * The object that contains the information what a Video needs.
 */
public class DlnaVideo extends DlnaMedia {
    private static final String TAG = "DlnaVideo";

    private String mGenre;
    private String mActor;
    private String mArtist;
    private String mResolution;
    private long mDuration;
    private String mBitRate;

    @Override
    public void dump() {
        super.dump();
        L.t(TAG, "mGenre = %s, mActor = %s, mArtist = %s, mResolution = %s, mDuration = %d , mBitRate = %s",
                mGenre, mActor, mArtist, mResolution, mDuration, mBitRate);
    }
    public DlnaVideo() {
        super();
    }

    public void setGenre(String genre) {
        this.mGenre = genre;
    }

    public String getGenre() {
        return mGenre;
    }

    public void setArtist(String artist) {
        this.mArtist = artist;
    }

    public String getArtist() {
        return mArtist;
    }

    public void setActor(String actor) {
        this.mActor = actor;
    }

    public String getActor() {
        return mActor;
    }

    public void setResolution(String resolution) {
        this.mResolution = resolution;
    }

    public String getResolution() {
        return mResolution;
    }

    public void setDuration(long duration) {
        this.mDuration = duration;
    }

    public long getDuration() {
        return mDuration;
    }

    public void setBitRate(String bitRate) {
        this.mBitRate = bitRate;
    }

    public String getBitRate() {
        return mBitRate;
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
            setGenre(cursor.getString(DBContent.ColumnName.COL_GENRE.ordinal()));
            setArtist(cursor.getString(DBContent.ColumnName.COL_ARTIST.ordinal()));
            setActor(cursor.getString(DBContent.ColumnName.COL_ACTOR.ordinal()));
            setAlbum(cursor.getString(DBContent.ColumnName.COL_ALBUM.ordinal()));
            setDateTaken(cursor.getString(DBContent.ColumnName.COL_DATE_TAKEN.ordinal()));
            setFileSize(cursor.getLong(DBContent.ColumnName.COL_FILE_SIZE.ordinal()));
            setFormat(cursor.getString(DBContent.ColumnName.COL_FORMAT.ordinal()));
            setResolution(cursor.getString(DBContent.ColumnName.COL_RESOLUTION.ordinal()));
            setDuration(cursor.getLong(DBContent.ColumnName.COL_DURATION.ordinal()));
            setBitRate(cursor.getString(DBContent.ColumnName.COL_BIT_RATE.ordinal()));
            setAlbumUrl(cursor.getString(DBContent.ColumnName.COL_ALBUMURL.ordinal()));
            setDeviceName(cursor.getString(DBContent.ColumnName.COL_DEVICE_NAME.ordinal()));
            setDeviceUuid(cursor.getString(DBContent.ColumnName.COL_DEVICE_UUID.ordinal()));
            setDbId(cursor.getInt(DBContent.ColumnName.COL_ID.ordinal()));
            setProtocolName(cursor.getString(DBContent.ColumnName.COL_PROTOCOLNAME.ordinal()));
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
            setAlbum(cursor.getString(DBSearchResult.ColumnName.COL_VIDEOALBUM.ordinal()));
            setDuration(cursor.getLong(DBSearchResult.ColumnName.COL_MUSICDURATION.ordinal()));
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
