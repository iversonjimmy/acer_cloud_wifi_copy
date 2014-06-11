package com.acer.ccd.cache.data;

import com.acer.ccd.debug.L;

/**
 * The object that contains the information what a MusicPlaylist needs.
 */
public class DlnaMusicPlaylist {
    private static final String TAG = "DlnaMusicPlaylist";
    private long mDbId;
    private long mPlaylistId;
    private String mPlName;
    private MusicInfo mMusicInfo = null;

    public void dump()
    {
        L.t(TAG, "mPlaylistId = %d , mPlName = %s", mPlaylistId, mPlName);
        if (mMusicInfo != null)
        {
            L.t(TAG, "mName = %s , mUrl = %s , mMusicId = %d , mDuration = %d",
                    mMusicInfo.getName(), mMusicInfo.getUrl(), mMusicInfo.getMusicId(), mMusicInfo.getDuration());
            L.t(TAG, "mDescription = %s , mFileSize = %d , mIconPath = %s, mDateTaken",
                    mMusicInfo.getDescription(), mMusicInfo.getFileSize(), mMusicInfo.getIconPath(), mMusicInfo.getDateTaken());
        }
    }

    public DlnaMusicPlaylist() {
        super();
    }

    public DlnaMusicPlaylist(long playlistId,
                              String plName,
                              MusicInfo musicInfo)
    {
        mDbId = -1;
        mPlaylistId = playlistId;
        mPlName = plName;
        mMusicInfo = musicInfo;
    }

    public void setDbId(long dbId) {
        this.mDbId = dbId;
    }

    public long getDbId() {
        return mDbId;
    }

    public void setPlaylistId(long playlistId) {
        this.mPlaylistId = playlistId;
    }

    public long getPlaylistId() {
        return mPlaylistId;
    }

    public void setPlaylistName(String playlistName) {
        this.mPlName = playlistName;
    }

    public String getPlaylistName() {
        return mPlName;
    }

    public void setMusicName(String musicName) {
        if (musicName != null)
        {
            if (this.mMusicInfo == null)
                this.mMusicInfo = new MusicInfo();
            this.mMusicInfo.setName(musicName);
        }
    }

    public String getMusicName() {
        if (this.mMusicInfo != null)
            return this.mMusicInfo.getName();
        else
            return null;
    }

    public void setUrl(String url) {
        if (url != null)
        {
            if (this.mMusicInfo == null)
                this.mMusicInfo = new MusicInfo();
            this.mMusicInfo.setUrl(url);
        }
    }

    public String getUrl() {
        if (this.mMusicInfo != null)
            return this.mMusicInfo.getUrl();
        else
            return null;
    }

    public void setMusicId(long musicId) {
        if (this.mMusicInfo == null)
            this.mMusicInfo = new MusicInfo();
        this.mMusicInfo.setMusicId(musicId);
    }

    public long getMusicId() {
        if (this.mMusicInfo != null)
            return this.mMusicInfo.getMusicId();
        else
            return 0;
    }

    public void setDuration(long duration) {
        if (this.mMusicInfo == null)
            this.mMusicInfo = new MusicInfo();
        this.mMusicInfo.setDuration(duration);
    }

    public long getDuration() {
        if (this.mMusicInfo != null)
            return this.mMusicInfo.getDuration();
        else
            return 0;
    }
    
    public void setDescription(String description) {
        if (description != null)
        {
            if (this.mMusicInfo == null)
                this.mMusicInfo = new MusicInfo();
            this.mMusicInfo.setDescription(description);
        }
    }

    public String getDesciption() {
        if (this.mMusicInfo != null)
            return this.mMusicInfo.getDescription();
        else
            return null;
    }

    public void setFileSize(long fileSize) {
        if (this.mMusicInfo == null)
            this.mMusicInfo = new MusicInfo();
        this.mMusicInfo.setFileSize(fileSize);
    }

    public long getFileSize() {
        if (this.mMusicInfo != null)
            return this.mMusicInfo.getFileSize();
        else
            return 0;
    }
}
