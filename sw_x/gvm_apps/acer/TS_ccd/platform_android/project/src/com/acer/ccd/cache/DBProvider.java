package com.acer.ccd.cache;

import java.util.ArrayList;

import com.acer.ccd.debug.L;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.content.pm.ProviderInfo;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.net.Uri;

/**
 * DBProvider extends ContentProvider
 */
public class DBProvider extends ContentProvider {
    private static final String TAG = "DBProvider";
    private static final String AUTHORITY = "com.acer.ccd.cache.DBProvider";
    private static final String NULL_COLUMN_HACK = "_id";
    private static UriMatcher mUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
    private static ArrayList<String> mTables = new ArrayList<String>();
    private SQLiteOpenHelper mDatabase = null;

    public static final Uri BASE_URI = Uri.parse("content://" + AUTHORITY);

    public static final String[][] DB_URIPATH_TABLE = {
        {"DBDevice", DBDevice.TABLE_NAME},
        {"DBMediaInfo", DBMediaInfo.TABLE_NAME},
        {"DBSearchResult", DBSearchResult.TABLE_NAME},
        {"DBMusicPlaylist", DBMusicPlaylist.TABLE_NAME},
        {"DBPreviewContent", DBPreviewContent.TABLE_NAME}
    };

    @Override
    public boolean onCreate() {
        L.d(TAG, "DBProvider, onCreate()");

        mDatabase = new DBhelper(getContext());
        return false;
    }

    @Override
    public void attachInfo(Context context, ProviderInfo info)
    {
        super.attachInfo(context, info);
        for (int i = 0; i < DB_URIPATH_TABLE.length; i++)
        {
            addTable(AUTHORITY, DB_URIPATH_TABLE[i][0], DB_URIPATH_TABLE[i][1]);
//            L.t(TAG, "Path = %s, table = %s", DB_URIPATH_TABLE[i][0], DB_URIPATH_TABLE[i][1]);
        }
    }

    @Override
    public Cursor query(Uri uri, String[] projection,
                         String selection, String[] selectionArgs,
                         String sortOrder)
    {
        int match = mUriMatcher.match(uri);
        if (match == UriMatcher.NO_MATCH)
            throw new IllegalArgumentException("Invalid URI: " + uri);

        String tableName = mTables.get(match);
        SQLiteDatabase db = mDatabase.getReadableDatabase();
        Cursor cursor = db.query(tableName, projection, selection, selectionArgs, null, null, sortOrder);

        return cursor;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        int match = mUriMatcher.match(uri);
        if (match == UriMatcher.NO_MATCH)
            throw new IllegalArgumentException("Invalid URI: " + uri);
//        L.t(TAG, "match = %d\n", match);

        SQLiteDatabase db = mDatabase.getWritableDatabase();
        String tableName = mTables.get(match);
//        L.t(TAG, "tableName = %s\n", tableName);
        long rowId = db.insert(tableName, NULL_COLUMN_HACK, values);
//        L.t(TAG, "rowId = %d\n", rowId);
        if (rowId == -1)
            throw new SQLException("Failed to insert row at: " + uri);
        else
            return Uri.withAppendedPath(uri, Long.toString(rowId));
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        int count = 0;
        int match = mUriMatcher.match(uri);
        if (match == UriMatcher.NO_MATCH)
            throw new IllegalArgumentException("Invalid URI: " + uri);

        SQLiteDatabase db = mDatabase.getWritableDatabase();
        String tableName = mTables.get(match);
        count = db.delete(tableName, selection, selectionArgs);

        return count;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        int count = 0;
        int match = mUriMatcher.match(uri);
        if (match == UriMatcher.NO_MATCH)
            throw new IllegalArgumentException("Invalid URI: " + uri);

        SQLiteDatabase db = mDatabase.getWritableDatabase();
        String tableName = mTables.get(match);
        count = db.update(tableName, values, selection, selectionArgs);

        return count;
    }

    @Override
    public String getType(Uri uri) {
        return null;
    }

    private void addTable(String authority, String path, String tableName)
    {
        ArrayList<String> tables = mTables;
        UriMatcher matcher = mUriMatcher;
        matcher.addURI(authority, path, tables.size());
        tables.add(new String(tableName));
    }

    /**
     * Database helper by SQLiteOpenHelper
     */
    private static final class DBhelper extends SQLiteOpenHelper {
        private static final String DATABASE_NAME = "clearFi.db";
        private static final int DATABASE_VERSION = 6;

        public DBhelper(Context context) {
            super(context, DATABASE_NAME, null, DATABASE_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            L.d(TAG, "[DBhelper] onCreate()");
            createDataTables(db);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            if (oldVersion != newVersion)
            {
                deleteDataTables(db);
                createDataTables(db);
            }
        }

        private void createDataTables(SQLiteDatabase db)
        {
            L.d(TAG, "[DBhelper] createDataTables()");

            String sql = DBMediaInfo.CREATE_TABLE_SQL;
            db.execSQL(sql);

            sql = "";
            sql = DBDevice.CREATE_TABLE_SQL;
            db.execSQL(sql);

            sql = "";
            sql = DBSearchResult.CREATE_TABLE_SQL;
            db.execSQL(sql);

            sql = "";
            sql = DBMusicPlaylist.CREATE_TABLE_SQL;
            db.execSQL(sql);

            sql = "";
            sql = DBPreviewContent.CREATE_TABLE_SQL;
            db.execSQL(sql);
//            L.t(TAG, "%s", sql);
        }

        private void deleteDataTables(SQLiteDatabase db)
        {
            String sql = DBMediaInfo.DELETE_TABLE_SQL;
            db.execSQL(sql);

            sql = "";
            sql = DBDevice.DELETE_TABLE_SQL;
            db.execSQL(sql);

            sql = "";
            sql = DBSearchResult.DELETE_TABLE_SQL;
            db.execSQL(sql);

            sql = "";
            sql = DBMusicPlaylist.DELETE_TABLE_SQL;
            db.execSQL(sql);

            sql = "";
            sql = DBPreviewContent.DELETE_TABLE_SQL;
            db.execSQL(sql);
        }
    }
}