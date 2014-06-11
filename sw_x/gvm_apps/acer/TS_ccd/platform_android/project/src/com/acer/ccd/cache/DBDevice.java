package com.acer.ccd.cache;

import android.net.Uri;
import android.provider.BaseColumns;

/**
 * Column name
 */
public final class DBDevice implements BaseColumns {
    public static final String URI_TAIL = "DBDevice";
    public static final Uri CONTENT_URI = Uri.withAppendedPath(DBProvider.BASE_URI, URI_TAIL);
    public static final String TABLE_NAME = "DBDevice";
    public static final String DEVICE_TYPE = "_device_type";
    public static final String IS_ACER = "_is_acer";
    public static final String DEVICE_NAME = "_device_name";
    public static final String MANUFACTURE = "_manufacture";
    public static final String MANUFACTURER_URL = "_manufacturer_url";
    public static final String MODEL_NAME = "_model_name";
    public static final String UUID = "_uuid";
    public static final String DEVICE_STATUS = "_device_status";
    public static final String ICON = "_icon";

    private DBDevice() {

    }

    /**
     * Help to get the index of a column
     */
    public static enum ColumnName {
        COL_ID,
        COL_DEVICETYPE,
        COL_ISACER,
        COL_DEVICENAME,
        COL_MANUFACTURE,
        COL_MANUFACTURERURL,
        COL_MODELNAME,
        COL_UUID,
        COL_DEVICESTATUS,
        COL_ICON
    };

    public static final String CREATE_TABLE_SQL = "CREATE TABLE " + TABLE_NAME
            + " (" + _ID + " INTEGER primary key, "
            + " " + DEVICE_TYPE + " text,"
            + " " + IS_ACER + " INTEGER,"
            + " " + DEVICE_NAME + " text,"
            + " " + MANUFACTURE + " text,"
            + " " + MANUFACTURER_URL + " text,"
            + " " + MODEL_NAME + " text,"
            + " " + UUID + " text,"
            + " " + DEVICE_STATUS + " INTEGER,"
            + " " + ICON + " text)";

    public static final String DELETE_TABLE_SQL  = "DROP TABLE IF EXISTS " + TABLE_NAME;

    public static final String ORDER_BY_NAME_ASC = DEVICE_NAME + " ASC";

    public static Uri getContentUri(String volumeName) {
        if (volumeName == null)
            return CONTENT_URI;

        else
            return Uri.parse(DBProvider.BASE_URI + volumeName + URI_TAIL);
    }
}
