package com.acer.ccd.cache;

import android.provider.BaseColumns;

/**
 * The schema of Media Content table in DB
 */
public abstract class DBContent implements BaseColumns {
    public static final String MEDIA_TYPE = "_media_type";
    public static final String URL = "_url";
    public static final String TITLE = "_title";
    public static final String CREATOR = "_creator";
    public static final String DESCRIPTION = "_description";
    public static final String PUBLISHER = "_pubilsher";
    public static final String ALBUM = "_album";
    public static final String DATE_TAKEN = "_date_taken";
    public static final String FILE_SIZE = "_file_size";
    public static final String FORMAT = "_format";
    public static final String ALBUM_URL = "_album_url";
    public static final String DEVICE_NAME = "_device_name";
    public static final String DEVICE_UUID = "_device_uuid";
    public static final String GENRE = "_genre";
    public static final String ARTIST = "_artist";
    public static final String RESOLUTION = "_resolution";
    public static final String DURATION = "_druation";
    public static final String TRACK_NO = "_track_no";
    public static final String ACTOR = "_actor";
    public static final String BIT_RATE = "_bit_rate";
    public static final String CONTAINER_ID = "_container_id";
    public static final String PROTOCOL_NAME = "_protocol_name";
    public static final String CHILD_COUNT = "_child_count";
    public static final String THUMBNAIL_URL = "_thumbnail_url";
    public static final String PARENT_CONTAINER_ID = "_parent_container_id";

    /**
     * The index value of the columns of this table
     */
    public static enum ColumnName {
        COL_ID,
        COL_MEDIA_TYPE,
        COL_URL,
        COL_TITLE,
        COL_CREATOR,
        COL_DESCRIPTION,
        COL_PUBLISHER,
        COL_ALBUM,
        COL_DATE_TAKEN,
        COL_FILE_SIZE,
        COL_FORMAT,
        COL_ALBUMURL,
        COL_DEVICE_NAME,
        COL_DEVICE_UUID,
        COL_GENRE,
        COL_ARTIST,
        COL_RESOLUTION,
        COL_DURATION,
        COL_TRACK_NO,
        COL_ACTOR,
        COL_BIT_RATE,
        COL_CONTAINERID,
        COL_PROTOCOLNAME,
        COL_CHILDCOUNT,
        COL_THUMBNAILURL,
        COL_PARENTCONTAINERID
    };

    public static final String CREATE_TABLE_SCHEMA =
        " (" + _ID  + " INTEGER primary key,"
        + " " + MEDIA_TYPE + " text,"
        + " " + URL + " text,"
        + " " + TITLE + " text,"
        + " " + CREATOR + " text,"
        + " " + DESCRIPTION + " text,"
        + " " + PUBLISHER + " text,"
        + " " + ALBUM + " text,"
        + " " + DATE_TAKEN + " text,"
        + " " + FILE_SIZE + " INTEGER,"
        + " " + FORMAT + " text,"
        + " " + ALBUM_URL + " text,"
        + " " + DEVICE_NAME + " text,"
        + " " + DEVICE_UUID + " text,"
        + " " + GENRE + " text,"
        + " " + ARTIST + " text,"
        + " " + RESOLUTION + " text,"
        + " " + DURATION + " INTEGER,"
        + " " + TRACK_NO + " text,"
        + " " + ACTOR + " text,"
        + " " + BIT_RATE + " text,"
        + " " + CONTAINER_ID + " text,"
        + " " + PROTOCOL_NAME + " text,"
        + " " + CHILD_COUNT + " text,"
        + " " + THUMBNAIL_URL + " text,"
        + " " + PARENT_CONTAINER_ID + " text)";

    public static final String ORDER_BY_NAME_ASC = TITLE + " ASC";
}
