package com.acer.ccd.cache;

import android.net.Uri;

/**
 * Extends DBContent schema to store the data of items under current working folder
 */
public class DBMediaInfo extends DBContent {
    public static final String URI_TAIL = "DBMediaInfo";
    public static final Uri CONTENT_URI = Uri.withAppendedPath(DBProvider.BASE_URI, URI_TAIL);
    public static final String TABLE_NAME = "DBMediaInfo";

    public static final String CREATE_TABLE_SQL =
        "CREATE TABLE "
        + TABLE_NAME
        + CREATE_TABLE_SCHEMA;

    public static final String DELETE_TABLE_SQL =
        "DROP TABLE IF EXISTS "
        + TABLE_NAME;
}
