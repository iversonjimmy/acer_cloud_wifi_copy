package com.acer.ccd.cache.data;

import com.acer.ccd.cache.DBSearchResult;
import com.acer.ccd.debug.L;

import android.database.Cursor;

/**
 * DlnaSearchResult class
 */
public class DlnaSearchResult {
    private static final String TAG = "DlnaSearchResult";
    private String mPhotoAlbum;
    private String mPhoto;
    private String mArtist;
    private String mMusicAlbum;
    private String mMusic;
    private String mVideoAlbum;
    private String mVideo;
    private String mGroupId;
    private long mMusicDuration;
    private String mDeviceName;
    private String mDeviceUuid;
    private long mDbId;
    private String mParentContainerId;
    private String mAlbumUrl;
    private String mUrl;
    private String mContainerName;
    private String mItemName;

    /**
     * The value set for GroupId of search result
     */
    public static enum GroupId {
        PHOTO_ALBUM,
        PHOTO,
        ARTIST,
        MUSIC_ALBUM,
        MUSIC,
        VIDEO_ALBUM,
        VIDEO,
        NONE
    }

    public void dump()
    {
        L.t(TAG, "mPhotoAlbum = %s , mPhoto = %s , mArtist = %s , mMusicAlbum = %s",
                mPhotoAlbum, mPhoto, mArtist, mMusicAlbum);
        L.t(TAG, "mMusic = %s , mVideoAlbum = %s , mVideo = %s , mMusicDuration = %d",
                mMusic, mVideoAlbum, mVideo, mMusicDuration);
        L.t(TAG, "mDeviceName = %s , mDeviceUuid = %s , mParentContainerId = %s , mAlbumUrl = %s , mUrl = %s",
                mDeviceName, mDeviceUuid, mParentContainerId, mAlbumUrl, mUrl);
        L.t(TAG, "mAlbumUrl = %s , mUrl = %s , mContainerName = %s , mItemName = %s",
                mAlbumUrl, mUrl, mContainerName, mItemName);
    }

    public DlnaSearchResult() {
        super();
    }

    public void setPhotoAlbum(String photoAlbum) {
        this.mPhotoAlbum = photoAlbum;
    }

    public String getPhotoAlbum() {
        return mPhotoAlbum;
    }

    public void setPhoto(String photo) {
        this.mPhoto = photo;
    }

    public String getPhoto() {
        return mPhoto;
    }

    public void setArtist(String artist) {
        this.mArtist = artist;
    }

    public String getArtist() {
        return mArtist;
    }

    public void setMusicAlbum(String musicAlbum) {
        this.mMusicAlbum = musicAlbum;
    }

    public String getMusicAlbum() {
        return mMusicAlbum;
    }

    public void setMusic(String music) {
        this.mMusic = music;
    }

    public String getMusic() {
        return mMusic;
    }

    public void setVideoAlbum(String videoAlbum) {
        this.mVideoAlbum = videoAlbum;
    }

    public String getVideoAlbum() {
        return mVideoAlbum;
    }

    public void setVideo(String video) {
        this.mVideo = video;
    }

    public String getVideo() {
        return mVideo;
    }

    public void setGroupId(String groupId) {
        this.mGroupId = groupId;
    }

    public String getGroupId() {
        return mGroupId;
    }

    public void setMusicDuration(long musicDuration) {
        this.mMusicDuration = musicDuration;
    }

    public long getMusicDuration() {
        return mMusicDuration;
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

    public void setParentContainerId(String parentContainerId) {
        this.mParentContainerId = parentContainerId;
    }

    public String getParentContainerId() {
        return mParentContainerId;
    }

    public void setAlbumUrl(String iconPath) {
        this.mAlbumUrl = iconPath;
    }

    public String getAlbumUrl() {
        return mAlbumUrl;
    }

    public String getUrl() {
        return mUrl;
    }

    public void setUrl(String url) {
        this.mUrl = url;
    }

    /**
     * @param containerName the mContainerName to set
     */
    public void setContainerName(String containerName) {
        this.mContainerName = containerName;
    }

    /**
     * @return the mContainerName
     */
    public String getContainerName() {
        return mContainerName;
    }

    /**
     * @param itemName the mItemName to set
     */
    public void setItemName(String itemName) {
        this.mItemName = itemName;
    }

    /**
     * @return the mItemName
     */
    public String getItemName() {
        return mItemName;
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
            setPhotoAlbum(cursor.getString(DBSearchResult.ColumnName.COL_PHOTOALBUM.ordinal()));
            setPhoto(cursor.getString(DBSearchResult.ColumnName.COL_PHOTO.ordinal()));
            setArtist(cursor.getString(DBSearchResult.ColumnName.COL_ARTIST.ordinal()));
            setMusicAlbum(cursor.getString(DBSearchResult.ColumnName.COL_MUSICALBUM.ordinal()));
            setMusic(cursor.getString(DBSearchResult.ColumnName.COL_MUSIC.ordinal()));
            setVideoAlbum(cursor.getString(DBSearchResult.ColumnName.COL_VIDEOALBUM.ordinal()));
            setVideo(cursor.getString(DBSearchResult.ColumnName.COL_VIDEO.ordinal()));
            setGroupId(cursor.getString(DBSearchResult.ColumnName.COL_GROUPID.ordinal()));
            setMusicDuration(cursor.getLong(DBSearchResult.ColumnName.COL_MUSICDURATION.ordinal()));
            setDeviceName(cursor.getString(DBSearchResult.ColumnName.COL_DEVICENAME.ordinal()));
            setDeviceUuid(cursor.getString(DBSearchResult.ColumnName.COL_DEVICEUUID.ordinal()));
            setDbId(cursor.getInt(DBSearchResult.ColumnName.COL_ID.ordinal()));
            setParentContainerId(cursor.getString(DBSearchResult.ColumnName.COL_PARENTCONTAINERID.ordinal()));
            setAlbumUrl(cursor.getString(DBSearchResult.ColumnName.COL_ALBUMURL.ordinal()));
            setUrl(cursor.getString(DBSearchResult.ColumnName.COL_URL.ordinal()));
            setContainerName(cursor.getString(DBSearchResult.ColumnName.COL_CONTAINERNAME.ordinal()));
            setItemName(cursor.getString(DBSearchResult.ColumnName.COL_ITEMNAME.ordinal()));
        } catch (Exception e) {
            e.printStackTrace();
            bResult = false;
        }

        return bResult;
    }
}
