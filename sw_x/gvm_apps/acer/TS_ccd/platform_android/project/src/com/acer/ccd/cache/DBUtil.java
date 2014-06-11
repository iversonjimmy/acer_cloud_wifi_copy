package com.acer.ccd.cache;

import java.util.ArrayList;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

import com.acer.ccd.debug.L;

/**
 * The database utility to get a single data value
 */
public class DBUtil {
    private static final String TAG = "DBUtil";
    private static Context mPntContext;
    private static ContentResolver mCntR;

    public DBUtil(Context parentContext)
    {
        mPntContext = parentContext;
    }

    private static Uri getTableUri(DbTable table)
    {
        Uri uri = null;
        switch (table)
        {
        case TBL_CURNT:
            uri = DBMediaInfo.CONTENT_URI;
            break;

        case TBL_PREVW:
            uri = DBPreviewContent.CONTENT_URI;
            break;

        case TBL_DEV:
            uri = DBDevice.CONTENT_URI;
            break;

        case TBL_SEARCH:
            uri = DBSearchResult.CONTENT_URI;
            break;

        case TBL_PL:
            uri = DBMusicPlaylist.CONTENT_URI;
            break;

        default:
            break;
        }

        return uri;
    }

    /**
     * Database table index
     */
    public static enum DbTable {
        TBL_CURNT,
        TBL_PREVW,
        TBL_DEV,
        TBL_SEARCH,
        TBL_PL
    }

    /**
     * The flag of the type of data value
     */
    public static enum ValueType {
        VALUE_STRING,
        VALUE_LONG
    }

    private Object getValue(DbTable table, 
                             String mediaType,
                             String columnName,
                             long dbId,
                             ValueType valueType) 
    {
        Uri uriTable = getTableUri(table);
        if (uriTable == null || dbId < 0)
        {
            throw new IllegalArgumentException("Unknown table or database Id.");
        }
        boolean bContent = false;
        if (table.equals(DbTable.TBL_CURNT)
         || table.equals(DbTable.TBL_PREVW))
        {
            if (!(mediaType.equals(DBManager.MEDIA_TYPE_IMAGE)
               || mediaType.equals(DBManager.MEDIA_TYPE_AUDIO)
               || mediaType.equals(DBManager.MEDIA_TYPE_VIDEO)
               || mediaType.equals(DBManager.MEDIA_TYPE_CONTAINER)))
            {
                throw new IllegalArgumentException("Unknown mediaType!");
            }
            bContent = true;
        }

        Object zValueColumn = null;
        StringBuilder sel = new StringBuilder();
        ArrayList<String> selArgs = new ArrayList<String>();
        sel.setLength(0);
        if (bContent)
        {
            sel.append(DBContent.MEDIA_TYPE + " = ? AND ");
            selArgs.add(mediaType);
        }
        sel.append(DBContent._ID + " = ? ");
        selArgs.add(String.valueOf(dbId));

        Cursor cursor = null;
        mCntR = mPntContext.getContentResolver();
        try {
            String selection = sel.toString();
            String[] selectionArgs = new String[selArgs.size()];
            selArgs.toArray(selectionArgs);
            cursor = mCntR.query(uriTable, null, selection, selectionArgs, null);
            if (cursor == null || !cursor.moveToFirst())
            {
                L.e(TAG, "Failed to query id %d", dbId);
                return null;
            }

            int colIndex = cursor.getColumnIndexOrThrow(columnName);
            if (valueType.equals(ValueType.VALUE_STRING))
                zValueColumn = cursor.getString(colIndex);
            else if (valueType.equals(ValueType.VALUE_LONG))
                zValueColumn = cursor.getLong(colIndex);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        } finally {
            sel = null;
            selArgs = null;

            if (cursor != null)
                cursor.close();
        }
        return zValueColumn;
    }

    public String getString(DbTable table,
                             String mediaType,
                             String columnName,
                             long dbId)
    {
        return (String) getValue(table, mediaType, columnName, dbId, ValueType.VALUE_STRING);
    }

    public long getLong(DbTable table,
                         String mediaType,
                         String columnName,
                         long dbId)
    {
        Long value = (Long) getValue(table, mediaType, columnName, dbId, ValueType.VALUE_LONG);
        return value.longValue();
    }

    public String[] getStrings(DbTable table, String mediaType, String columnName, long[] dbIds)
    {
        if (columnName == null || dbIds == null)
        {
            throw new IllegalArgumentException("Invalid columnName or dbIds.");
        }

        String[] zValueColumns = new String[dbIds.length];
        try {
            int i = 0;
            for (long dbId : dbIds)
            {
                zValueColumns[i] = getString(table, mediaType, columnName, dbId);
                if (zValueColumns[i] == null)
                {
                    L.e(TAG, "Cannot find the column value of dbId=[%d].", dbId);
                }
                i++;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return zValueColumns;
    }

    public long[] getLongs(DbTable table, String mediaType, String columnName, long[] dbIds)
    {
        if (columnName == null || dbIds == null)
        {
            throw new IllegalArgumentException("Invalid columnName or dbIds.");
        }

        long[] zValueColumns = new long[dbIds.length];
        try {
            int i = 0;
            for (long dbId : dbIds)
            {
                zValueColumns[i] = getLong(table, mediaType, columnName, dbId);
                if (zValueColumns[i] < Long.MIN_VALUE || zValueColumns[i] > Long.MAX_VALUE)
                {
                    L.e(TAG, "Cannot find the column value of dbId=[%d].", dbId);
                }
                i++;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return zValueColumns;
    }
}
