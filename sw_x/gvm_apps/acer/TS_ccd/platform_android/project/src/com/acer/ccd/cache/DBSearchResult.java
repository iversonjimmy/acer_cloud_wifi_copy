package com.acer.ccd.cache;

import android.net.Uri;
import android.provider.BaseColumns;

/**
 * Schema of DBSearchResult table in the database
 */
public final class DBSearchResult implements BaseColumns {
    public static final String URI_TAIL = "DBSearchResult";
    public static final Uri CONTENT_URI = Uri.withAppendedPath(DBProvider.BASE_URI, URI_TAIL);
    public static final String TABLE_NAME = "DBSearchResult";
    public static final String TITLE = "_title";
    public static final String PHOTO_ALBUM = "_photo_album";
    public static final String PHOTO = "_photo";
    public static final String ARTIST = "_artist";
    public static final String MUSIC_ALBUM = "_music_album";
    public static final String MUSIC = "_music";
    public static final String VIDEO_ALBUM = "_video_album";
    public static final String VIDEO = "_video";
    public static final String GROUP_ID = "_group_id";
    public static final String MUSIC_DURATION = "_music_duration";
    public static final String DEVICE_NAME = "_device_name";
    public static final String DEVICE_UUID = "_device_uuid";
    public static final String PARENT_CONTAINER_ID = "_parent_container_id";
    public static final String ALBUM_URL = "_album_url";
    public static final String URL = "_url";
    public static final String CONTAINER_NAME = "_container_name";
    public static final String ITEM_NAME = "_item_name";

    private DBSearchResult() {

    }

    /**
     * Help to get the index of a column
     */
    public static enum ColumnName {
        COL_ID,
        COL_TITLE,
        COL_PHOTOALBUM,
        COL_PHOTO,
        COL_ARTIST,
        COL_MUSICALBUM,
        COL_MUSIC,
        COL_VIDEOALBUM,
        COL_VIDEO,
        COL_GROUPID,
        COL_MUSICDURATION,
        COL_DEVICENAME,
        COL_DEVICEUUID,
        COL_PARENTCONTAINERID,
        COL_ALBUMURL,
        COL_URL,
        COL_CONTAINERNAME,
        COL_ITEMNAME
    }

    public static final String CREATE_TABLE_SQL = "CREATE TABLE " + TABLE_NAME
             + " (" + _ID + " INTEGER primary key,"
             + " " + TITLE + " text,"
             + " " + PHOTO_ALBUM + " text,"
             + " " + PHOTO + " text,"
             + " " + ARTIST + " text,"
             + " " + MUSIC_ALBUM + " text,"
             + " " + MUSIC + " text,"
             + " " + VIDEO_ALBUM + " text,"
             + " " + VIDEO + " text,"
             + " " + GROUP_ID + " text,"
             + " " + MUSIC_DURATION + " INTEGER,"
             + " " + DEVICE_NAME + " text,"
             + " " + DEVICE_UUID + " text,"
             + " " + PARENT_CONTAINER_ID + " text,"
             + " " + ALBUM_URL + " text,"
             + " " + URL + " text,"
             + " " + CONTAINER_NAME + " text,"
             + " " + ITEM_NAME + " text)";

    public static final String DELETE_TABLE_SQL = "DROP TABLE IF EXISTS " + TABLE_NAME;

    public static final String ORDER_BY_NAME_ASC = TITLE + " ASC";

    public static Uri getContentUri(String volumeName) {
        if (volumeName == null)
            return CONTENT_URI;

        else
            return Uri.parse(DBProvider.BASE_URI + volumeName + URI_TAIL);
    }
}
