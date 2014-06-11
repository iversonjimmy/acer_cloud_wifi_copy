package com.acer.ccd.cache.data;

import android.database.Cursor;

import com.acer.ccd.cache.DBContent;
import com.acer.ccd.cache.DBSearchResult;
import com.acer.ccd.debug.L;

/**
 * The object that contains the information what an Audio needs.
 */
public class DlnaAudio extends DlnaMedia {
    private static final String TAG = "DlnaAudio";

    private String mArtist;
    private String mGenre;
    private long mDuration;
    private int mTrackNo;

    @Override
    public void dump() {
        super.dump();
        L.t(TAG, "mArtist = %s, mGenre = %s, mDuration = %d, mRecordingYear = %s, mTrackNo = %d",
                mArtist, mGenre, mDuration, mTrackNo);
    }

    public DlnaAudio() {
        super();
    }

    public void setArtist(String artist) {
        this.mArtist = artist;
    }

    public String getArtist() {
        return mArtist;
    }

    public void setGenre(String genre) {
        this.mGenre = genre;
    }

    public String getGenre() {
        return mGenre;
    }

    public void setDuration(long duration) {
        this.mDuration = duration;
    }

    public long getDuration() {
        return mDuration;
    }

    public void setTrackNo(int trackNo) {
        this.mTrackNo = trackNo;
    }

    public int getTrackNo() {
        return mTrackNo;
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
            setAlbum(cursor.getString(DBContent.ColumnName.COL_ALBUM.ordinal()));
            setDateTaken(cursor.getString(DBContent.ColumnName.COL_DATE_TAKEN.ordinal()));
            setFileSize(cursor.getLong(DBContent.ColumnName.COL_FILE_SIZE.ordinal()));
            setFormat(cursor.getString(DBContent.ColumnName.COL_FORMAT.ordinal()));
            setDuration(cursor.getLong(DBContent.ColumnName.COL_DURATION.ordinal()));
            setTrackNo(cursor.getInt(DBContent.ColumnName.COL_TRACK_NO.ordinal()));
            setAlbumUrl(cursor.getString(DBContent.ColumnName.COL_ALBUMURL.ordinal()));
            setDeviceName(cursor.getString(DBContent.ColumnName.COL_DEVICE_NAME.ordinal()));
            setDeviceUuid(cursor.getString(DBContent.ColumnName.COL_DEVICE_UUID.ordinal()));
            setDbId(cursor.getLong(DBContent.ColumnName.COL_ID.ordinal()));
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
            setArtist(cursor.getString(DBSearchResult.ColumnName.COL_ARTIST.ordinal()));
            setAlbum(cursor.getString(DBSearchResult.ColumnName.COL_MUSICALBUM.ordinal()));
            setDuration(cursor.getLong(DBSearchResult.ColumnName.COL_MUSICDURATION.ordinal()));
            setAlbumUrl(cursor.getString(DBSearchResult.ColumnName.COL_ALBUMURL.ordinal()));
            setDeviceName(cursor.getString(DBSearchResult.ColumnName.COL_DEVICENAME.ordinal()));
            setDeviceUuid(cursor.getString(DBSearchResult.ColumnName.COL_DEVICEUUID.ordinal()));
            setDbId(cursor.getLong(DBSearchResult.ColumnName.COL_ID.ordinal()));
            setParentContainerId(cursor.getString(DBSearchResult.ColumnName.COL_PARENTCONTAINERID.ordinal()));
        } catch (Exception e) {
            e.printStackTrace();
            bResult = false;
        }

        return bResult;
    }
}
