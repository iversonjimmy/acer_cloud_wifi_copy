package com.acer.ccd.cache;

import android.net.Uri;
import android.provider.BaseColumns;

/**
 * Schema of DBMusicPlaylist table of the database
 */
public final class DBMusicPlaylist implements BaseColumns {
    public static final String URI_TAIL = "DBMusicPlaylist";
    public static final Uri CONTENT_URI = Uri.withAppendedPath(DBProvider.BASE_URI, URI_TAIL);
    public static final String TABLE_NAME = "DBMusicPlaylist";
    public static final String PLAYLIST_NAME = "_playlist_name";
    public static final String TITLE = "_title";
    public static final String URL = "_url";
    public static final String PLAYLIST_ID = "_playlist_id";
    public static final String DURATION = "_duration";
    public static final String DESCRIPTION = "_description";
    public static final String FILE_SIZE = "_file_size";
    public static final String IS_PLAYLIST = "_is_playlist";
    public static final String PROTOCOL_NAME = "_protocol_name";
    public static final String ALBUM_URL = "_album_url";

    private DBMusicPlaylist() {

    }

    /**
     * Help get the index of a column
     */
    public static enum ColumnName {
        COL_ID,
        COL_PLAYLIST_ID,
        COL_PLAYLISTNAME,
        COL_ISPLAYLIST,
        COL_TITLE,
        COL_URL,
        COL_DURATION,
        COL_DESCRIPTION,
        COL_FILESIZE,
        COL_PROTOCOLNAME,
        COL_ALBUMURL
    };

    public static final String CREATE_TABLE_SQL = "CREATE TABLE " + TABLE_NAME
        + " (" + _ID + " INTEGER primary key, "
        + " " + PLAYLIST_ID + " INTEGER,"
        + " " + PLAYLIST_NAME + " text,"
        + " " + IS_PLAYLIST + " text,"
        + " " + TITLE + " text,"
        + " " + URL + " text,"
        + " " + DURATION + " INTEGER,"
        + " " + DESCRIPTION + " INTEGER,"
        + " " + FILE_SIZE + " INTEGER,"
        + " " + PROTOCOL_NAME + " text,"
        + " " + ALBUM_URL + " text)";

    public static final String DELETE_TABLE_SQL  = "DROP TABLE IF EXISTS " + TABLE_NAME;

    public static final String ORDER_BY_ID_ASC = _ID + " ASC";
//    public static Uri getContentUri(String volumeName) {
//        if (volumeName == null)
//            return CONTENT_URI;
//
//        else
//            return Uri.parse(DBProvider.BASE_URI + volumeName + URI_TAIL);
//    }
}
