package com.acer.ccd.provider;

import com.acer.ccd.util.CcdSdkDefines;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.net.Uri;

public class ThumbnailProvider extends ContentProvider {
    private Context mContext;
    private ThumbsDatabase mDb;
    
    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        SQLiteDatabase db = mDb.getWritableDatabase();
        int result = db.delete(DBThumbs.TABLE_NAME, selection, selectionArgs);
        return result;
    }

    @Override
    public String getType(Uri uri) {
        return null;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        Uri result = null;
        SQLiteDatabase db = mDb.getWritableDatabase();
        long rowId = db.insert(DBThumbs.TABLE_NAME, null, values);
        if (rowId != -1) 
            result =  ContentUris.withAppendedId(CcdSdkDefines.THUMBNAIL_CONTENT_URI, rowId);
        //db.close();
        return result;
    }

    @Override
    public boolean onCreate() {
        mContext = getContext();
        String path = mContext.getExternalCacheDir().toString();
        mDb = new ThumbsDatabase(mContext, path);
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        SQLiteDatabase db = mDb.getReadableDatabase();
        Cursor cursor = db.query(DBThumbs.TABLE_NAME, projection, selection, selectionArgs, null, null, null);
        //db.close();
        return cursor;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        SQLiteDatabase db = mDb.getWritableDatabase();
        int result = db.update(DBThumbs.TABLE_NAME, values, selection, selectionArgs);
        //db.close();
        return result;
    }
    
    public static class DBThumbs {
        
        public static final String TABLE_NAME = "DBThumbs";
        
        public static final String ID = "_id";
        public static final String IDENTIFY = "_identify";
        public static final String THUMB_DATA = "_thumb_data";
        
        public static final String CREATE_TABLE_SQL = String.format(
                "CREATE TABLE %s (%s INTEGER PRIMARY KEY, %s text, %s BLOB)",
                TABLE_NAME, ID, IDENTIFY, THUMB_DATA);
    }
    
    private class ThumbsDatabase extends SQLiteOpenHelper {
        
        private static final int DATABASE_VERSION = 1;
        private static final String DATABASE_NAME = "/ThumbsDatabases";

        public ThumbsDatabase(Context context, String dbPath) {
            super(context, dbPath + DATABASE_NAME, null, DATABASE_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL(DBThumbs.CREATE_TABLE_SQL);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            
        }
        
    }

}
