package com.acer.ccd.cache.data;

import com.acer.ccd.debug.L;

import android.database.Cursor;

/**
 * The object that contains the common parts of information for Image, Audio and Video objects.
 */
public abstract class DlnaMedia {
    private static final String TAG = "DlnaMedia";

    private String mUrl;
    private String mTitle;
    private String mCreator;
    private String mDescription;
    private String mPublisher;
    private String mAlbum;
    private String mDateTaken;
    private long mFileSize;
    private String mFormat;
    private String mAlbumUrl;
    private String mDeviceName;
    private String mDeviceUuid;
    private long mDbId;
    private String mProtocolName;
    private String mParentContainerId;

    /** dump basic media element */ 
    public void dump()
    {
        L.t(TAG, "mUrl = %s , mTitle = %s , mCreator = %s , mDescription = %s",
                mUrl, mTitle,  mCreator, mDescription);
        L.t(TAG, "mPublisher = %s , mAlbum = %s , mDateTaken = %s , mFileSize = %d",
                mPublisher, mAlbum,  mDateTaken, mFileSize);
        L.t(TAG, "mFormat = %s , mAlbumUrl = %s , mParentContainerId = %s",
                mFormat, mAlbumUrl, mParentContainerId);
        L.t(TAG, "mDeviceName = %s , mDeviceUuid = %s, mDbId = %d mProtocolName = %s",
                mDeviceName, mDeviceUuid, mDbId, mProtocolName);
    }

    public DlnaMedia() {
        super();
    }

    public abstract boolean setDataFromDB(Cursor cursor);
    public abstract boolean setDataFromSearchDB(Cursor cursor);

    public void setUrl(String url) {
        this.mUrl = url;
    }

    public String getUrl() {
        return mUrl;
    }

    public void setTitle(String title) {
        this.mTitle = title;
    }

    public String getTitle() {
        return mTitle;
    }

    public void setCreator(String artist) {
        this.mCreator = artist;
    }

    public String getCreator() {
        return mCreator;
    }

    public void setDescription(String description) {
        this.mDescription = description;
    }

    public String getDescription() {
        return mDescription;
    }

    public void setPublisher(String publisher) {
        this.mPublisher = publisher;
    }

    public String getPublisher() {
        return mPublisher;
    }

    public void setAlbum(String album) {
        this.mAlbum = album;
    }

    public String getAlbum() {
        return mAlbum;
    }

    public void setDateTaken(String dateTaken) {
        this.mDateTaken = dateTaken;
    }

    public String getDateTaken() {
        return mDateTaken;
    }

    public void setFileSize(long fileSize) {
        this.mFileSize = fileSize;
    }

    public long getFileSize() {
        return mFileSize;
    }

    public void setFormat(String format) {
        this.mFormat = format;
    }

    public String getFormat() {
        return mFormat;
    }

    public void setAlbumUrl(String iconPath) {
        this.mAlbumUrl = iconPath;
    }

    public String getAlbumUrl() {
        return mAlbumUrl;
    }

    public void setDeviceName(String deviceName) {
        this.mDeviceName = deviceName;
    }

    public String getDeviceName() {
        return mDeviceName;
    }

    public void setDbId(long dbId) {
        this.mDbId = dbId;
    }

    public long getDbId() {
        return mDbId;
    }

    public void setDeviceUuid(String deviceUuid) {
        this.mDeviceUuid = deviceUuid;
    }

    public String getDeviceUuid() {
        return mDeviceUuid;
    }

    public void setProtocolName(String protocolName) {
        this.mProtocolName = protocolName;
    }

    public String getProtocolName() {
        return mProtocolName;
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
}
