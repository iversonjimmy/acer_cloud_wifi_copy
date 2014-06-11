package com.acer.ccd.cache;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.util.Log;

import com.acer.ccd.cache.data.ContainerItem;
import com.acer.ccd.cache.data.DlnaAudio;
import com.acer.ccd.cache.data.DlnaContainer;
import com.acer.ccd.cache.data.DlnaDevice;
import com.acer.ccd.cache.data.DlnaImage;
import com.acer.ccd.cache.data.DlnaSearchResult;
import com.acer.ccd.cache.data.DlnaSearchResult.GroupId;
import com.acer.ccd.cache.data.DlnaVideo;
import com.acer.ccd.cache.data.MusicInfo;
import com.acer.ccd.upnp.util.Dlna;
import com.acer.ccd.util.CcdSdkDefines;
import com.acer.ccd.util.TimeFormatter;

/**
 * DBManager class
 */
public class DBManager {
    private static final String TAG = "DBManager";
    private static final String FORMAT_IMG_YCBCRYUV42 = "image/x-ycbcr-yuv420";
    private static final String URL_QUERY = "\\?";
    private static final String DATABASE_NAME = "clearFi.db";
    private static final String SEARCH_FLAG_VALUE = "0";
    private static final long KILO = 1000L;

    private static Context mParentContext;
    private static ContentResolver mCR;
    private static DBUtil mUtil;

    /**
     *  Content query criteria
     */
    private static enum QueryReq {
        QRY_BY_ID,
        QRY_BY_POS,
        QRY_BY_SEL_ID,
        QRY_SEQ_BY_ID,
        QRY_ALL
    }

    private static String[] searchGroupName = {
        DBSearchResult.PHOTO_ALBUM,
        DBSearchResult.PHOTO,
        DBSearchResult.ARTIST,
        DBSearchResult.MUSIC_ALBUM,
        DBSearchResult.MUSIC,
        DBSearchResult.VIDEO_ALBUM,
        DBSearchResult.VIDEO
    };

    /*
     * Some types of Media, Image, Audio, Video and Container
     */
    public static final String MEDIA_TYPE_IMAGE = "MediaType_Image";
    public static final String MEDIA_TYPE_AUDIO = "MediaType_Audio";
    public static final String MEDIA_TYPE_VIDEO = "MediaType_Video";
    public static final String MEDIA_TYPE_CONTAINER = "MediaType_Container";
    public static final String MEDIA_TYPE_ALL = "MediaType_All";
    /*
     * Device type - DMS, DMR or both
     */
    public static final String DEVICE_TYPE_DMS = Dlna.DeviceType.DLNA_DEVICE_TYPE_DMS;
    public static final String DEVICE_TYPE_DMR = Dlna.DeviceType.DLNA_DEVICE_TYPE_DMR;
    public static final String DEVICE_TYPE_DMS_DMR = Dlna.DeviceType.DLNA_DEVICE_TYPE_DMSDMR;
    /* Get all items in the Media table */
    public static final int GET_WHOLE_TABLE = -100;
    /*
     * Error return value
     */
    public static final long INVALID_DBID = -1;
    public static final int INVALID_ARGUMENT = -1;
    public static final int CURSOR_NONE = -2;

    /**
     *  The flags in ContentTable: current working table & preview table for Container
     */
    public static class CntntTbl {
        /** DBMediaInfo */
        public static final int TBL_CURNT = 0;
        /** DBPreviewContent */
        public static final int TBL_PREVW = 1;
        /** DBContainerFstChild */
        public static final int TBL_CNTNR = 2;
        /** DBDevice */
        public static final int TBL_DEV = 3;
        /** DBSearchResult */
        public static final int TBL_SEARCH = 4;
        /** DBMusicPlaylist */
        public static final int TBL_PL = 5;
    }

    public DBManager(Context context) {
        mParentContext = context;
        setUtil(new DBUtil(mParentContext));
    }

    /**
     * Get the specific table Uri by the table flag
     * @param table The table flag to get its Uri
     * @return The Uri of the table
     */
    private Uri getContentTableUri(int table) {
        Uri tableUri = null;

        switch (table) {
            case CntntTbl.TBL_CURNT:
            case CntntTbl.TBL_CNTNR:
                tableUri = DBMediaInfo.CONTENT_URI;
                break;

            case CntntTbl.TBL_PREVW:
                tableUri = DBPreviewContent.CONTENT_URI;
                break;

            case CntntTbl.TBL_SEARCH:
                tableUri = DBSearchResult.CONTENT_URI;
                break;

            default:
                break;
        }
        return tableUri;
    }

    /**
     * Set all the data to the class DlnaDevice where the current cursor at
     * @param cursor Where the current data is at in the DMS/DMR Device table in DB
     * @param dlnaDevice The class of DlnaDevice where the data could store to
     * @return True for set device data successfully; false for failed.
     */
    private boolean setDeviceData(Cursor cursor, DlnaDevice dlnaDevice) {
        if (dlnaDevice == null)
            dlnaDevice = new DlnaDevice();
        return dlnaDevice.setDataFromDB(cursor);
    }

    /**
     * Set all the data to the class DlnaImage where the current cursor at
     * @param cursor Where the current data is at in the Media Content table in DB
     * @param dlnaImage The class of DlnaImage where the data could store to
     * @param table The specific table to query data.
     * @return True for set image data successfully; false for failed.
     */
    private boolean setImageData(Cursor cursor, DlnaImage dlnaImage, int table) {
        if (dlnaImage == null)
            dlnaImage = new DlnaImage();
        if (table == CntntTbl.TBL_SEARCH)
            return dlnaImage.setDataFromSearchDB(cursor);
        else
            return dlnaImage.setDataFromDB(cursor);
    }

    /**
     * Set all the data to the class DlnaAudio where the current cursor at
     * @param cursor Where the current data is at in the Media Content table in DB
     * @param dlnaAudio The class of DlnaAudio where the data could store to
     * @param table The specific table to query data.
     * @return True for set audio data successfully; false for failed.
     **/
    private boolean setAudioData(Cursor cursor, DlnaAudio dlnaAudio, int table) {
        if (dlnaAudio == null)
            dlnaAudio = new DlnaAudio();
        if (table == CntntTbl.TBL_SEARCH)
            return dlnaAudio.setDataFromSearchDB(cursor);
        else
            return dlnaAudio.setDataFromDB(cursor);
    }

    /**
     * Set all the data to the class DlnaVideo where the current cursor at
     * @param cursor Where the current data is at in the Media Content table in DB
     * @param dlnaVideo The class of DlnaVideo where the data could store to
     * @param table The specific table to query data.
     * @return True for set video data successfully; false for failed.
     **/
    private boolean setVideoData(Cursor cursor, DlnaVideo dlnaVideo, int table) {
        if (dlnaVideo == null)
            dlnaVideo = new DlnaVideo();
        if (table == CntntTbl.TBL_SEARCH)
            return dlnaVideo.setDataFromSearchDB(cursor);
        else
            return dlnaVideo.setDataFromDB(cursor);
    }

    /**
     * Set all the data to the class DlnaContainer where the current cursor at
     * @param cursor Where the current data is at in the Media Content table in DB
     * @param dlnaContainer The class of DlnaContainer where the data could store to
     * @return True for set container data successfully; false for failed.
     **/
    private boolean setContainerData(Cursor cursor, DlnaContainer dlnaContainer) {
        if (dlnaContainer == null)
            dlnaContainer = new DlnaContainer();
        return dlnaContainer.setDataFromDB(cursor);
    }

    /**
     * Set all the data to the class DlnaSearchResult where the current cursor at
     * @param cursor Where the current data is at in the Global SearchResult table in DB
     * @param dlnaSearchResult The class of DlnaSearchResult where the data could store to
     * @return True for set search result data successfully; false for failed.
     **/
    private boolean setSearchResultData(Cursor cursor, DlnaSearchResult dlnaSearchResult) {
        if (dlnaSearchResult == null)
            dlnaSearchResult = new DlnaSearchResult();
        return dlnaSearchResult.setDataFromDB(cursor);
    }

    /**
     * Close the cursor
     * @param cursor The Cursor intended to be closed.
     **/
    private void closeCursor(Cursor cursor) {
        if (cursor != null)
            cursor.close();
        cursor = null;
    }

    /**
     * New create an object by the media type
     * @param mediaType Image, audio, video or container.
     * @return The object for image, audio, video or container.
     */
    private Object newContentData(String mediaType) {
        Object content = null;

//        Log.t(TAG, "new %s data object", mediaType);
        if (mediaType.equals(MEDIA_TYPE_IMAGE))
            content = new DlnaImage();
        else if (mediaType.equals(MEDIA_TYPE_AUDIO))
            content = new DlnaAudio();
        else if (mediaType.equals(MEDIA_TYPE_VIDEO))
            content = new DlnaVideo();
        else if (mediaType.equals(MEDIA_TYPE_CONTAINER))
            content = new DlnaContainer();

        return content;
    }

    /**
     * New create an object array by its media type and the size
     * @param mediaType Image, audio, video or container.
     * @param size The array size.
     * @return The object array for image, audio, video or container.
     */
    private Object[] newContentDataArray(String mediaType, int size) {
        Object[] contents = null;

        if (mediaType.equals(MEDIA_TYPE_IMAGE))
            contents = new DlnaImage[size];
        else if (mediaType.equals(MEDIA_TYPE_AUDIO))
            contents = new DlnaAudio[size];
        else if (mediaType.equals(MEDIA_TYPE_VIDEO))
            contents = new DlnaVideo[size];
        else if (mediaType.equals(MEDIA_TYPE_CONTAINER))
            contents = new DlnaContainer[size];

        return contents;
    }

    /**
     * Create the selection according the given select criteria and where in a Table
     * @param mediaType Image, Audio, Video or Container
     * @param queryCriteria Single, multiple consequent or random selections.
     * @param queryWhereSize How many items have to be matched.
     * @param table The table where to query data; DBMediaInfo, DBPreviewContent and DBSearchResult
     * @return A SQLite syntax string of selection
     */
    private String getSelection(String mediaType, QueryReq queryCriteria, int queryWhereSize, int table) {
        String selection = null;
        StringBuilder sel = new StringBuilder();
        try {
            sel.setLength(0);
            switch(table) {
                case CntntTbl.TBL_CURNT:
                case CntntTbl.TBL_PREVW:
                case CntntTbl.TBL_CNTNR:
                    sel.append(DBContent.MEDIA_TYPE + " = ? OR ");
                    sel.append(DBContent.MEDIA_TYPE + " = ? ");
                    break;

                case CntntTbl.TBL_SEARCH:
                    sel.append(DBSearchResult.GROUP_ID + " = ? OR ");
                    sel.append(DBSearchResult.GROUP_ID + " = ? ");
                    if (mediaType.equals(MEDIA_TYPE_AUDIO))
                    {
                        sel.append("OR ");
                        sel.append(DBSearchResult.GROUP_ID + " = ? ");
                    }
                    break;

                default:
                    break;
            }

            switch(queryCriteria) {
                case QRY_SEQ_BY_ID:
                    sel.append("AND ");
                    sel.append(DBContent._ID + " >= ? ");
                    break;

                case QRY_BY_SEL_ID:
                    sel.append("AND ");
                    for (int i = 0; i < queryWhereSize; i++) {
                        sel.append(DBContent._ID + " = ? ");
                        if (i < queryWhereSize - 1)
                            sel.append("OR ");
                    }
                    break;

                case QRY_BY_ID:
                    sel.append("AND ");
                    sel.append(DBContent._ID + " = ? ");
                    break;

                default:
                    break;
            }
            selection = sel.toString();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            sel = null;
        }

        
        return selection;
    }

    /**
     * Create the selection Arguments according the given select criteria and where in a Table
     * @param mediaType Image, Audio, Video or Container
     * @param queryCriteria Single, multiple consequent or random selections.
     * @param queryWhere How many items have to be matched.
     * @param table The table where to query data; DBMediaInfo, DBPreviewContent and DBSearchResult
     * @return A SQLite syntax string of where argument
     */
    private String[] getSelectionArgs(String mediaType, QueryReq queryCriteria, ArrayList<Long> queryWhere, int table) {
        if (queryWhere == null || queryWhere.size() <= 0) {
            throw new IllegalArgumentException("queryWhere is null or empty.");
        }

        String[] selectionArgs = null;
        ArrayList<String> selArgs = new ArrayList<String>();
        try {
            if (table >= CntntTbl.TBL_CURNT && table <= CntntTbl.TBL_CNTNR) {
                selArgs.add(DBManager.MEDIA_TYPE_CONTAINER);
                selArgs.add(mediaType);
            } else if (table == CntntTbl.TBL_SEARCH) {
                if (mediaType.equals(MEDIA_TYPE_IMAGE)) {
                    selArgs.add(String.valueOf(DlnaSearchResult.GroupId.PHOTO_ALBUM.ordinal()));
                    selArgs.add(String.valueOf(DlnaSearchResult.GroupId.PHOTO.ordinal()));
                } else if (mediaType.equals(MEDIA_TYPE_AUDIO)) {
                    selArgs.add(String.valueOf(DlnaSearchResult.GroupId.ARTIST.ordinal()));
                    selArgs.add(String.valueOf(DlnaSearchResult.GroupId.MUSIC_ALBUM.ordinal()));
                    selArgs.add(String.valueOf(DlnaSearchResult.GroupId.MUSIC.ordinal()));
                } else if (mediaType.equals(MEDIA_TYPE_VIDEO)) {
                    selArgs.add(String.valueOf(DlnaSearchResult.GroupId.VIDEO_ALBUM.ordinal()));
                    selArgs.add(String.valueOf(DlnaSearchResult.GroupId.VIDEO.ordinal()));
                }
            }

            switch(queryCriteria) {
                case QRY_SEQ_BY_ID:
                    selArgs.add(String.valueOf(queryWhere.get(0)));
                    break;

                case QRY_BY_SEL_ID:
                    for (int i = 0; i < queryWhere.size(); i++) {
                        selArgs.add(String.valueOf(queryWhere.get(i)));
                    }
                    break;

                case QRY_BY_ID:
                    selArgs.add(String.valueOf(queryWhere.get(0)));
                    break;

                default:
                    break;
            }

            selectionArgs = new String[selArgs.size()];
            selArgs.toArray(selectionArgs);
        } catch (Exception e) {
            e.printStackTrace();
            selectionArgs = null;
        } finally {
            selArgs = null;
        }

        return selectionArgs;
    }

    private String getOrderBy(QueryReq queryCriteria) {
        String orderBy = null;
        switch(queryCriteria) {
            case QRY_BY_POS:
            case QRY_SEQ_BY_ID:
                orderBy = DBContent.ORDER_BY_NAME_ASC;
                break;

            default:
                break;
        }

        return orderBy;
    }

    /**
     * Get data of Contents from DB in a way of query criteria
     * @param mediaType Image, audio or video
     * @param queryCriteria Query by single ID, query by position, query by the specific IDs or query sequence by ID
     * @param queryWhere It could be a dbId; a startId and the amount for ByPos/SeqById; a list of specific dbIds.
     * @param table The table where the data store to. (DBMediaInfo, DBPreviewContent, DBSearchResult tables)
     * @return An array of image, audio or video objects.
     */
    private Object[] getContentsUtil(String mediaType,  QueryReq queryCriteria,
                                      ArrayList<Long> queryWhere, int table) {
        if (queryCriteria == null) {
            throw new IllegalArgumentException("Invalid query way: null");
        }

        Cursor cursor = null;
        Object[] contents = null;

        mCR = mParentContext.getContentResolver();
        try {
            // set selection and selectionArgs
            String selection = getSelection(mediaType, queryCriteria, queryWhere.size(), table);
            String[] selectionArgs = getSelectionArgs(mediaType, queryCriteria, queryWhere, table);

            // set sorting order
            String orderBy = getOrderBy(queryCriteria);

            // query DBMediaInfo or DBPreviewContent table
            cursor = mCR.query(getContentTableUri(table), null,
                               selection, selectionArgs, orderBy);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            long totalAvail = 1;
            long amount = 0;
            switch(queryCriteria) {
                case QRY_BY_POS:
                {
                    long pos = queryWhere.get(0);
                    amount = queryWhere.get(1);
//                    Log.t(TAG, "pos = %d, amount = %d", pos, amount);
                    if (!cursor.moveToPosition((int) pos)) {
                        Log.e(TAG, String.format("failed to move to position %d", pos));
                        return null;
                    }

                    totalAvail = cursor.getCount() - pos;
                    if (amount != GET_WHOLE_TABLE)
                        totalAvail = (totalAvail < amount) ? totalAvail : amount;
                    Log.d(TAG, String.format("Total available data are %d items", totalAvail));
                    break;
                }

                case QRY_BY_SEL_ID:
                    amount = queryWhere.size();
                    totalAvail = cursor.getCount();
                    totalAvail = (totalAvail <= amount) ? totalAvail : amount;
                    break;

                case QRY_SEQ_BY_ID:
                {
                    amount = queryWhere.get(1);
                    totalAvail = cursor.getCount();
                    if (amount != GET_WHOLE_TABLE) {
                        totalAvail = amount < totalAvail ? amount : totalAvail;
                    }
                    break;
                }

                case QRY_BY_ID:
                    totalAvail = 1;
                    break;

                default:
                    break;
            }

            // allocate an array to store data
            contents = newContentDataArray(mediaType, (int) totalAvail);
            int i = 0;
            do {
                String zItemType = cursor.getString(DBContent.ColumnName.COL_MEDIA_TYPE.ordinal());
                if ((table != CntntTbl.TBL_SEARCH) && (zItemType == null || !zItemType.equals(mediaType)))
                    continue;
                else {
                    // create a new object and set data to it
                    contents[i] = newContentData(mediaType);
                    if (!setContentData(mediaType, cursor, contents[i], table)) {
                        Log.e(TAG, String.format("Failed to find imageId %d", queryWhere.get(i)));
                        contents[i] = null;
                    }
                    i++;
                }
            } while (cursor.moveToNext() && i < totalAvail);
        } catch (Exception e) {
            e.printStackTrace();
            contents = null;
        } finally {
            closeCursor(cursor);
        }

        return contents;
    }

    /**
     * Get a specific data column of Contents from DB in a way of query criteria
     * @param column The index of a column in a DB table.
     * @param mediaType Image, audio or video
     * @param queryCriteria Query by single ID, query by position, query by the specific IDs or query sequence by ID
     * @param queryWhere It could be a dbId; a startId and the amount for ByPos/SeqById; a list of specific dbIds.
     * @param table The table where the data store to. (DBMediaInfo, DBPreviewContent, DBSearchResult tables)
     * @return An array of image, audio or video objects.
     */
    private List<String> getContentsColumnUtil(int column,
                                                String mediaType,
                                                QueryReq queryCriteria,
                                                ArrayList<Long> queryWhere,
                                                int table) {
        if (queryCriteria == null) {
            throw new IllegalArgumentException("Invalid query way: null");
        }

        Cursor cursor = null;
        List<String> contents = new ArrayList<String>();

        mCR = mParentContext.getContentResolver();
        try {
            // set selection and selectionArgs
            String selection = getSelection(mediaType, queryCriteria, queryWhere.size(), table);
            String[] selectionArgs = getSelectionArgs(mediaType, queryCriteria, queryWhere, table);

            // set sorting order
            String orderBy = getOrderBy(queryCriteria);

            // query DBMediaInfo or DBPreviewContent table
            cursor = mCR.query(getContentTableUri(table), null,
                               selection,
                               selectionArgs,
                               orderBy);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            long totalAvail = 1;
            long amount = 0;
            switch(queryCriteria) {
                case QRY_BY_POS:
                {
                    long pos = queryWhere.get(0);
                    amount = queryWhere.get(1);
//                    Log.t(TAG, "pos = %d, amount = %d", pos, amount);
                    if (!cursor.moveToPosition((int) pos)) {
                        Log.e(TAG, String.format("failed to move to position %d", pos));
                        return null;
                    }

                    totalAvail = cursor.getCount() - pos;
                    if (amount != GET_WHOLE_TABLE)
                        totalAvail = (totalAvail < amount) ? totalAvail : amount;
                    Log.d(TAG, String.format("Total available data are %d items", totalAvail));
                    break;
                }

                case QRY_BY_SEL_ID:
                    amount = queryWhere.size();
                    totalAvail = cursor.getCount();
                    totalAvail = (totalAvail <= amount) ? totalAvail : amount;
                    break;

                case QRY_SEQ_BY_ID:
                {
                    amount = queryWhere.get(1);
                    totalAvail = cursor.getCount();
                    if (amount != GET_WHOLE_TABLE) {
                        totalAvail = amount < totalAvail ? amount : totalAvail;
                    }
                    break;
                }

                case QRY_BY_ID:
                    totalAvail = 1;
                    break;

                default:
                    break;
            }

            // allocate an array to store data
            int i = 0;
            do {
                String content = null;
                String zItemType = cursor.getString(DBContent.ColumnName.COL_MEDIA_TYPE.ordinal());
                if (zItemType != null && !zItemType.equals(MEDIA_TYPE_CONTAINER)) {
                    content = cursor.getString(column);
                    if (column == DBContent.ColumnName.COL_URL.ordinal()) {
                        String zFormat = cursor.getString(DBContent.ColumnName.COL_FORMAT.ordinal());
                        if (zFormat != null && zFormat.equals(FORMAT_IMG_YCBCRYUV42))
                            if (content != null)
                                content = content.split(URL_QUERY)[0];
                    }
                }
                contents.add(content);
                i++;
            } while (cursor.moveToNext() && i < totalAvail);
        } catch (Exception e) {
            e.printStackTrace();
            contents = null;
        } finally {
            closeCursor(cursor);
        }

        return contents;
    }

    /**
     * Set the data read from DB by cursor to a specific object type
     * @param mediaType Image, audio, video or container
     * @param cursor Cursor of the data
     * @param content The object where the data is written to.
     * @param table The specific table to query data.
     * @return true for write successfully; false for failure.
     */
    private boolean setContentData(String mediaType, Cursor cursor, Object content, int table) {
        boolean bResult = false;

//        Log.t(TAG, "set %s object data gotten from DB cursor.", mediaType);
        if (mediaType.equals(MEDIA_TYPE_IMAGE))
            bResult = setImageData(cursor, (DlnaImage) content, table);
        else if (mediaType.equals(MEDIA_TYPE_AUDIO))
            bResult = setAudioData(cursor, (DlnaAudio) content, table);
        else if (mediaType.equals(MEDIA_TYPE_VIDEO))
            bResult = setVideoData(cursor, (DlnaVideo) content, table);
        else if (mediaType.equals(MEDIA_TYPE_CONTAINER))
            bResult = setContainerData(cursor, (DlnaContainer) content);

        return bResult;
    }

    /**
     * Get the total amount number of a content table of image, audio, video or container
     * @param mediaType Image, audio, video or container
     * @param table The table read from. Current working table or Container preview table.
     * @return The amount of containers, images, audio or video data in the content table.
     */
    private long getContentsCount(String mediaType, int table) {
        long cursorCount = 0;

        mCR = mParentContext.getContentResolver();
        Cursor cursor = null;
        try {
            cursor = mCR.query(getContentTableUri(table), null,
                               DBContent.MEDIA_TYPE + " = ? ",
                               new String[] {mediaType},
                               DBContent.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst())
                return cursorCount;

            cursorCount = cursor.getCount();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            closeCursor(cursor);
        }

        return cursorCount;
    }

    /**
     * Get a content item by its database ID
     * @param mediaType Image, audio, video or container
     * @param dbId The unique Id provided by database.
     * @param table The table read from. Current working table or Container preview table.
     * @return The object according to the media type.
     */
    private Object getContent(String mediaType, long dbId, int table) {
        if (dbId <= 0)
            throw new IllegalArgumentException("Invalid database ID.");

        ArrayList<Long> getByIdReq = new ArrayList<Long>();
        try {
            getByIdReq.add(dbId);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }

        Object[] contents = getContentsUtil(mediaType, QueryReq.QRY_BY_ID, getByIdReq, table);
        if (contents == null)
            return null;
        return contents[0];
    }

    /**
     * Get an amount of images, audio, video or containers object
     * @param mediaType Image, audio, video or container
     * @param startId The beginning of the read row in DB.
     * @param amount Total amount of read rows.
     * @param table The table read from. Current working table or Container preview table.
     * @return The object array according to the media type
     */
    private Object[] getContents(String mediaType, long startId, long amount, int table) {
        if (startId <= 0 || (amount <= 0 && amount != GET_WHOLE_TABLE))
            throw new IllegalArgumentException("Invalid startImageId: " + startId + " or imagesAmount: " + amount);

        ArrayList<Long> getSeqByIdReq = new ArrayList<Long>();
        try {
            getSeqByIdReq.add(startId);
            getSeqByIdReq.add(amount);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }

        return getContentsUtil(mediaType, QueryReq.QRY_SEQ_BY_ID, getSeqByIdReq, table);
    }

    /**
     * Get an amount of image, audio, video or container objects in sequence by position.
     * @param mediaType Image, audio, video or container
     * @param pos The beginning position of the read row in DB.
     * @param amount Total amount of read rows.
     * @param table The table read from. Current working table or Container preview table.
     * @return The object array started by a position in an amount of data according to the media type
     */
    private Object[] getContentsByPos(String mediaType, int pos, int amount, int table) {
        if (pos < 0 || (amount <= 0 && amount != GET_WHOLE_TABLE))
            throw new IllegalArgumentException("Invalid pos: " + pos + " or invalid amount: " + amount);

        ArrayList<Long> getByPosReq = new ArrayList<Long>();
        try {
            getByPosReq.add((long) pos);
            getByPosReq.add((long) amount);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        return getContentsUtil(mediaType, QueryReq.QRY_BY_POS, getByPosReq, table);
    }

    /**
     * Get an amount of image, audio, video or container objects in sequence by specific dbIds.
     * @param mediaType Image, audio, video or container
     * @param contentIds Some specific database unique IDs
     * @param table The table read from. Current working table or Container preview table.
     * @return The object array for the specific DB IDs according to the media type
     */
    private Object[] getContents(String mediaType, long[] contentIds, int table) {
        if (contentIds == null)
            throw new IllegalArgumentException("Invalid imageIds: null");

        ArrayList<Long> dbIds = new ArrayList<Long>();
        try {
            for (long dbId : contentIds)
                dbIds.add(dbId);
        } catch (Exception e) {
            e.printStackTrace();
        }

        return getContentsUtil(mediaType, QueryReq.QRY_BY_SEL_ID, dbIds, table);
    }

    /**
     * Add a row of content to a specific table in DB
     * @param mediaType Image, audio, video or container.
     * @param content The object of the content with data.
     * @param table The table where the data store to.
     * @return A unique ID given by the DB system.
     */
    private long addContent(String mediaType, Object content, int table) {
        if (content == null)
            throw new IllegalArgumentException("Content is null");

        long dbId = INVALID_DBID;

        if (mediaType.equals(MEDIA_TYPE_IMAGE)) {
            DlnaImage dlnaImage = (DlnaImage) content;
            dbId = addImage(dlnaImage, table);
        } else if (mediaType.equals(MEDIA_TYPE_AUDIO)) {
            DlnaAudio dlnaAudio = (DlnaAudio) content;
            dbId = addAudio(dlnaAudio, table);
        } else if (mediaType.equals(MEDIA_TYPE_VIDEO)) {
            DlnaVideo dlnaVideo = (DlnaVideo) content;
            dbId = addVideo(dlnaVideo, table);
        } else if (mediaType.equals(MEDIA_TYPE_CONTAINER)) {
            DlnaContainer dlnaContainer = (DlnaContainer) content;
            dbId = addContainer(dlnaContainer, table);
        }

        return dbId;
    }

    /**
     * Copy an ArrayList of database IDs of a content from DBMediaInfo table to DBPreviewContent table
     * @param mediaType Image, audio, video or container.
     * @param dbIds Some specific database unique IDs that are intended to copy to the Preview table.
     * @return The copied database IDs in an ArrayList
     */
    private ArrayList<Long> copyContentToPreview(String mediaType, ArrayList<Long>dbIds) {
        if (dbIds == null)
            throw new IllegalArgumentException("Invalid dbIds.");

        ArrayList<Long> previewIds = null;
        Cursor cursor = null;
        StringBuilder sel = new StringBuilder();
        ArrayList<String> selArgs = new ArrayList<String>();

        try {
            sel.setLength(0);
            sel.append(DBContent.MEDIA_TYPE + " = ? ");
            selArgs.add(mediaType);

            int amount = dbIds.size();
            if (amount > 0)
                sel.append("AND ");

            int i = 0;
            do {
                sel.append(DBContent._ID + " = ? ");
                selArgs.add(String.valueOf(dbIds.get(i)));

                if (i < (amount - 1))
                    sel.append("OR ");

                i++;
            } while (i < amount);

            String selection = sel.toString();
            String[] selectionArgs = new String[selArgs.size()];
            selArgs.toArray(selectionArgs);
            Log.d(TAG, String.format("selection = %s", selection));
            for (String selArg : selectionArgs)
                Log.d(TAG, String.format("selectionArgs = %s", selArg));

            mCR = mParentContext.getContentResolver();
            cursor = mCR.query(DBMediaInfo.CONTENT_URI, null,
                               selection,
                               selectionArgs,
                               null);
            if (cursor == null || !cursor.moveToFirst())
                return previewIds;

            previewIds = new ArrayList<Long>();
            Object content = null;
            i = 0;
            do {
                content = newContentData(mediaType);
                if (setContentData(mediaType, cursor, content, CntntTbl.TBL_CURNT))
                    previewIds.add(addContent(mediaType, content, CntntTbl.TBL_PREVW));

                i++;
            } while (i < cursor.getCount());
        } catch (Exception e) {
            previewIds = null;
            e.printStackTrace();
        } finally {
            sel = null;
            selArgs = null;
            closeCursor(cursor);
        }
        return previewIds;
    }

    /**
     * Delete some rows in a Content table
     * @param mediaType Image, audio, video or container
     * @param dbIds An ArrayList of the dbIds which are going to be deleted.
     * @param table The table read from. Current working table or Container preview table.
     * @return The amount of the deleted rows.
     */
    private int deleteContent(String mediaType, ArrayList<Long> dbIds, int table) {
        mCR = mParentContext.getContentResolver();
        StringBuilder sel = new StringBuilder();
        ArrayList<String> selArgs = new ArrayList<String>();
        String selection = null;
        String[] selectionArgs = null;

        try {
            sel.setLength(0);
            sel.append(DBContent.MEDIA_TYPE + " = ? ");

            selArgs.add(mediaType);
            if (dbIds != null && dbIds.size() > 0) {
                sel.append("AND ");
                int i = 0;
                do {
                    sel.append(DBContent._ID + " = ? ");
                    selArgs.add(String.valueOf(dbIds.get(i)));
                    i++;
                    if (i < dbIds.size())
                        sel.append("OR ");
                } while (i < dbIds.size());
            }
            selection = sel.toString();
            selectionArgs = new String[selArgs.size()];
            selArgs.toArray(selectionArgs);
        } catch (Exception e) {
            e.printStackTrace();
        }

        return mCR.delete(getContentTableUri(table), selection, selectionArgs);
    }

    /**
     * Replace a null String to a space character
     * @param string The original text string
     * @return The original string or a space character if it was null
     */
    private String replaceNullString(String string) {
        return (string == null) ? " " : string;
    }

    private DlnaAudio[] getPlMusicUtil(boolean isGetSinglePl, long[] dbIds, long[] playlistIds) {
        if (playlistIds == null || playlistIds.length <= 0)
            throw new IllegalArgumentException("playlistIds is null or empty.");

        if (isGetSinglePl) {
            if (dbIds == null || dbIds.length <= 0)
                throw new IllegalArgumentException("dbIds is null or empty.");
        }

        Cursor cursor = null;
        DlnaAudio[] dlnaAudio = null;
        StringBuilder sel = new StringBuilder();
        ArrayList<String> selArgs = new ArrayList<String>();
        mCR = mParentContext.getContentResolver();
        try {
            sel.setLength(0);

            // Add the first selection argument for playlist Id when the single playlist query.
            if (isGetSinglePl) {
                sel.append(DBMusicPlaylist.PLAYLIST_ID + " = ? AND ");
                selArgs.add(String.valueOf(playlistIds[0]));
            }

            // Add selection argument for !isPlaylist
            sel.append(DBMusicPlaylist.IS_PLAYLIST + " = ? AND ");
            selArgs.add(String.valueOf(false));

            // Add all DB Ids to the selection argument.
            long[] bufIds = null;
            String selColumn = null;
            int i = 0;
            if (isGetSinglePl) {
                bufIds = dbIds;
                selColumn = DBMusicPlaylist._ID;
                
            } else {
                bufIds = playlistIds;
                selColumn = DBMusicPlaylist.PLAYLIST_ID;
            }

            for (long bufId : bufIds) {
                sel.append(selColumn + " = ? ");
                if (i < bufIds.length - 1)
                    sel.append("OR ");
                selArgs.add(String.valueOf(bufId));
                i++;
            }

            String selection = sel.toString();
            String[] selectionArgs = new String[selArgs.size()];
            selArgs.toArray(selectionArgs);

            cursor = mCR.query(DBMusicPlaylist.CONTENT_URI, null, selection, selectionArgs, null);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty.");
                return dlnaAudio;
            }

            long totalAvail = cursor.getCount();
            if (isGetSinglePl)
                totalAvail = (totalAvail <= dbIds.length) ? totalAvail : dbIds.length;
            dlnaAudio = new DlnaAudio[(int) totalAvail];
            i = 0;
            do {
                DlnaAudio audio = new DlnaAudio();
                audio.setDbId(cursor.getLong(DBMusicPlaylist.ColumnName.COL_ID.ordinal()));
                audio.setTitle(cursor.getString(DBMusicPlaylist.ColumnName.COL_TITLE.ordinal()));
                audio.setUrl(cursor.getString(DBMusicPlaylist.ColumnName.COL_URL.ordinal()));
                audio.setDuration(cursor.getLong(DBMusicPlaylist.ColumnName.COL_DURATION.ordinal()));
                audio.setDescription(cursor.getString(DBMusicPlaylist.ColumnName.COL_DESCRIPTION.ordinal()));
                audio.setAlbumUrl(cursor.getString(DBMusicPlaylist.ColumnName.COL_ALBUMURL.ordinal()));
                audio.setFileSize(cursor.getLong(DBMusicPlaylist.ColumnName.COL_FILESIZE.ordinal()));
                audio.setProtocolName(cursor.getString(DBMusicPlaylist.ColumnName.COL_PROTOCOLNAME.ordinal()));
                dlnaAudio[i] = audio;
                i++;
            } while (cursor.moveToNext() && i < totalAvail);
        } catch (Exception e) {
            e.printStackTrace();
            dlnaAudio = null;
        } finally {
            closeCursor(cursor);
            sel = null;
            selArgs = null;
        }
        return dlnaAudio;
    }

    /**
     * Add an item of Device data to the DB     * 
     * @param dlnaDevice The DlnaDevice class contains the data that is going to store to the DB
     * @return The database unique ID of the item.  Pass: id >= 1, failed: id = -1
     */
    public long addDevice(DlnaDevice dlnaDevice) {
        Log.d(TAG, "DBManager, addDevice()");
        long id = INVALID_DBID;
        if (dlnaDevice == null)
            return id;

        Log.d(TAG, String.format("device added %s", dlnaDevice.getUuid()));
        
        mCR = mParentContext.getContentResolver();
        ContentValues values = new ContentValues();

        values.put(DBDevice.DEVICE_TYPE, dlnaDevice.getDeviceType());
        values.put(DBDevice.IS_ACER, dlnaDevice.isAcerDevice());
        values.put(DBDevice.DEVICE_NAME, dlnaDevice.getDeviceName());
        values.put(DBDevice.MANUFACTURE, dlnaDevice.getManufacture());
        values.put(DBDevice.MANUFACTURER_URL, dlnaDevice.getManufacturerUrl());
        values.put(DBDevice.MODEL_NAME, dlnaDevice.getModelName());
        values.put(DBDevice.UUID, dlnaDevice.getUuid());
        values.put(DBDevice.ICON, dlnaDevice.getIconPath());

        Uri uri = mCR.insert(DBDevice.CONTENT_URI, values);
        if (uri != null) {
            id = Integer.valueOf(uri.getLastPathSegment());
            dlnaDevice.setDbId(id);
//            Log.d(TAG, "DbId = %d\n", id);
        }

        return id;
    }

    /**
     * Get the amount of the items in DMS/DMR Device table
     * @param deviceType A specific device type of DMS or DMR or both.
     * @return The amount of total items in DMS/DMR Device table
     **/
    public long getDeviceCount(int deviceType) {
        long cursorCount = 0;
        Cursor cursor = null;
        StringBuilder sel = new StringBuilder();
        ArrayList<String> selArgs = new ArrayList<String>();

        mCR = mParentContext.getContentResolver();
        try {
            // set select and where argument for sql syntax
            sel.setLength(0);

            if ((deviceType & CcdSdkDefines.DEVICE_TYPE_DMR) == CcdSdkDefines.DEVICE_TYPE_DMR) {
                sel.append(DBDevice.DEVICE_TYPE + " = ? OR ");
                sel.append(DBDevice.DEVICE_TYPE + " = ? ");

                selArgs.add(DEVICE_TYPE_DMR);
                selArgs.add(DEVICE_TYPE_DMS_DMR);
            } else {
                sel.append(DBDevice.DEVICE_TYPE + " = ? OR ");
                sel.append(DBDevice.DEVICE_TYPE + " = ? ");

                selArgs.add(DEVICE_TYPE_DMS);
                selArgs.add(DEVICE_TYPE_DMS_DMR);
            }
            String selection = sel.toString();
            String[] selectionArgs = new String[selArgs.size()];
            selArgs.toArray(selectionArgs);

            cursor = mCR.query(DBDevice.CONTENT_URI, null,
                               selection,
                               selectionArgs,
                               DBDevice.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
//                Log.e(TAG, "cursor null or empty");
                return 0;
            }

            cursorCount = cursor.getCount();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            sel = null;
            selArgs = null;
            closeCursor(cursor);
        }

        return cursorCount;
    }

    /**
     * Get the amount of the items with Acer clear.fi in DMR Device table
     * @param deviceType A specific device type of DMS or DMR or both.
     * @return The amount of total items in DMS/DMR Device table
     **/
    public long getAcerDMRCount() {
        long cursorCount = 0;
        Cursor cursor = null;
        StringBuilder sel = new StringBuilder();
        ArrayList<String> selArgs = new ArrayList<String>();

        mCR = mParentContext.getContentResolver();
        try {
            // set select and where argument for sql syntax
            sel.setLength(0);

            sel.append(DBDevice.IS_ACER + " = ? ");
            
            selArgs.add(String.valueOf(CcdSdkDefines.DEVICE_MANUFACTURER_CLCLEARFI15));

            sel.append(" AND ( ");
            sel.append(DBDevice.DEVICE_TYPE + " = ? OR ");
            sel.append(DBDevice.DEVICE_TYPE + " = ? )");

            selArgs.add(DEVICE_TYPE_DMR);
            selArgs.add(DEVICE_TYPE_DMS_DMR);

            String selection = sel.toString();
            String[] selectionArgs = new String[selArgs.size()];
            selArgs.toArray(selectionArgs);

            cursor = mCR.query(DBDevice.CONTENT_URI, null,
                               selection,
                               selectionArgs,
                               DBDevice.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
//                Log.e(TAG, "cursor null or empty");
                return 0;
            }

            cursorCount = cursor.getCount();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            sel = null;
            selArgs = null;
            closeCursor(cursor);
        }

        return cursorCount;
    }

    /**
     * Get an amount of items of data value which is already written to DMS/DMR Device table by position
     * @param pos The start position
     * @param amount The amount of items to be read
     * @param deviceType A specific device type of DMS or DMR or both.
     * @return An array of the data read from DMS/DMR Device table in DB
     */
    public DlnaDevice[] getDevicesByPos(int pos, int amount, int deviceType) {
        if (pos < 0 || amount < 0)
            throw new IllegalArgumentException("Invalid pos: " + pos + " or invalid amount: " + amount);

        Cursor cursor = null;
        DlnaDevice[] dlnaDevices = null;
        StringBuilder sel = new StringBuilder();
        ArrayList<String> selArgs = new ArrayList<String>();

        mCR = mParentContext.getContentResolver();
        try {
            // set select and where argument for sql syntax
            sel.setLength(0);

            if ((deviceType & CcdSdkDefines.DEVICE_TYPE_DMR) == CcdSdkDefines.DEVICE_TYPE_DMR) {
                sel.append(DBDevice.DEVICE_TYPE + " = ? OR ");
                sel.append(DBDevice.DEVICE_TYPE + " = ? ");

                selArgs.add(DEVICE_TYPE_DMR);
                selArgs.add(DEVICE_TYPE_DMS_DMR);
            } else {
                sel.append(DBDevice.DEVICE_TYPE + " = ? OR ");
                sel.append(DBDevice.DEVICE_TYPE + " = ? ");

                selArgs.add(DEVICE_TYPE_DMS);
                selArgs.add(DEVICE_TYPE_DMS_DMR);
            }
            String selection = sel.toString();
            String[] selectionArgs = new String[selArgs.size()];
            selArgs.toArray(selectionArgs);
            Log.d(TAG, String.format("deviceType=%d, selectionArgs[]{%s, %s}", deviceType, selectionArgs[0], selectionArgs[1]));

            cursor = mCR.query(DBDevice.CONTENT_URI, null,
                               selection,
                               selectionArgs,
                               null);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            // move the cursor to position pos
            if (!cursor.moveToPosition(pos)) {
                Log.e(TAG, String.format("failed to move to position %d", pos));
                return null;
            }

            int totalAvail = cursor.getCount() - pos;
            totalAvail = (totalAvail < amount) ? totalAvail : amount;
//            Log.t(TAG, "Total available data are %d items", totalAvail);
            dlnaDevices = new DlnaDevice[totalAvail];

            int i = 0;
            do {
                if (dlnaDevices[i] == null)
                    dlnaDevices[i] = new DlnaDevice();

                if (!setDeviceData(cursor, dlnaDevices[i]))
                    dlnaDevices[i] = null;
//                Log.t(TAG, "Reading pos[%d]...%s device[%s]", pos+i, dlnaDevices[i].getDeviceType(), dlnaDevices[i].getDeviceName());

                i++;
            } while (cursor.moveToNext() && i < totalAvail);
        } catch (Exception e) {
            e.printStackTrace();
            dlnaDevices = null;
        } finally {
//            Log.t(TAG, "finally");
            sel = null;
            selArgs = null;
            closeCursor(cursor);
        }

        return dlnaDevices;
    }

    /**
     * Update the DMR status to a specific device UUID in the DBDevice table
     * @param deviceUuid The specific UUID
     * @param status Device status for a DMR
     * @return True for success; False for failure
     */
    public boolean updateDMRStatus(String deviceUuid, int status) {
        boolean bResult = true;

        if (deviceUuid == null)
            throw new IllegalArgumentException("null deviceId");

        mCR = mParentContext.getContentResolver();
        try {
            ContentValues values = new ContentValues();
            values.put(DBDevice.DEVICE_STATUS, String.valueOf(status));
            int nRows = mCR.update(DBDevice.CONTENT_URI, values, 
                                   DBDevice.UUID + " = ? ",
                                   new String[] {deviceUuid});
            if (nRows <= 0) {
                Log.e(TAG, "Failed to update data");
                bResult = false;
            }
        } catch (Exception e) {
            e.printStackTrace();
            bResult = false;
        }

        return bResult;
    }

    /**
     * Delete a specific UUID of an item from the DMS/DMR Device table
     * @param uuid The UUID that provided by the Service
     * @return The number of rows deleted.
     **/
    public int deleteDeviceByUuid(String uuid) {
        mCR = mParentContext.getContentResolver();
        return mCR.delete(DBDevice.CONTENT_URI,
                          DBDevice.UUID + " = ? ",
                          new String[] {uuid});
    }

    /**
     * Delete all items from the DMS/DMR Device table
     * @return The amount of items been deleted.
     **/
    public int deleteAllDevices() {
        mCR = mParentContext.getContentResolver();
        return mCR.delete(DBDevice.CONTENT_URI, null, null);
    }

    /**
     * Add the data for a Container to the Media Content table in DB 
     * @param dlnaContainer The DlnaContainer class contains the data of Containers that is going to store to the DB
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The database unique ID of the item. Pass: id >= 1, failed: id = -1
     */
    public long addContainer(DlnaContainer dlnaContainer, int table) {
        Log.d(TAG, "addContainer()");
        long id = INVALID_DBID;
        if (dlnaContainer == null)
            return id;

        mCR = mParentContext.getContentResolver();
        ContentValues values = new ContentValues();

        values.put(DBContent.MEDIA_TYPE, MEDIA_TYPE_CONTAINER);
        values.put(DBContent.CONTAINER_ID, dlnaContainer.getContainerId());
        values.put(DBContent.TITLE, dlnaContainer.getTitle());
        values.put(DBContent.CHILD_COUNT, dlnaContainer.getChildCount());
        values.put(DBContent.PARENT_CONTAINER_ID, dlnaContainer.getParentContainerId());
        values.put(DBContent.ALBUM_URL, dlnaContainer.getAlbumUrl());

        Uri uri = mCR.insert(getContentTableUri(table), values);
        if (uri != null) {
            id = Integer.valueOf(uri.getLastPathSegment());
            dlnaContainer.setDbId(id);
//            Log.d(TAG, "DbId = %d\n", id);
        }

        return id;
    }

    /**
     * Get the amount of Container items in Media Content table
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The amount of total Container items in Media Content table
     */
    public long getContainerCount(int table) {
        return getContentsCount(MEDIA_TYPE_CONTAINER, table);
    }

    /**
     * Get an item of a Container data value from the Media Content table from DB through ID
     * @param mediaType Image, audio, video or container.
     * @param dbId The database ID
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The Container data read from Media Content table in DB
     */
    public DlnaContainer getContainer(String mediaType, long dbId, int table) {
        Cursor cursor = null;
        DlnaContainer dlnaContainer = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(getContentTableUri(table), null,
                               DBContent._ID + " = ? AND "
                             + DBContent.MEDIA_TYPE + " = ? ",
                               new String[] {String.valueOf(dbId), MEDIA_TYPE_CONTAINER},
                               DBContent.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

//            Log.t(TAG, "Container(%d) was found", dbId);

            dlnaContainer = new DlnaContainer();
            if (!setContainerData(cursor, dlnaContainer)) {
                dlnaContainer = null;
            }
        } catch (Exception e) {
//            Log.t(TAG, "No container with id %d was found.", dbId);
            e.printStackTrace();
            dlnaContainer = null;
        } finally {
            closeCursor(cursor);
        }

        return dlnaContainer;
    }

    /**
     * Get all database ID and with the info of whether it's a container 
     *  in the table of a type of Media content
     * @param mediaType The specific media type from Image, Audio and Video
     * @param table A specific content table to read data. Current or Preview content table.
     * @return An ArrayList of object ContainerItem 
     *           which contains database Ids and isContainer information
     */
    public ArrayList<ContainerItem> getContentDbId(String mediaType, int table) {
        Cursor cursor = null;
        ArrayList<ContainerItem> dbId = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(getContentTableUri(table), null,
                                   DBContent.MEDIA_TYPE + " = ? OR "
                                 + DBContent.MEDIA_TYPE + " = ? ",
                                   new String[] {mediaType, MEDIA_TYPE_CONTAINER},
                                   DBContent.ORDER_BY_NAME_ASC);

            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            dbId = new ArrayList<ContainerItem>();
            int i = 0;
            ContainerItem contentId = null;
            do {
                boolean isContainer = false;
                String containerId = cursor.getString(DBContent.ColumnName.COL_CONTAINERID.ordinal());
                if (containerId != null && containerId.length() > 0)
                    isContainer = true;

                contentId = new ContainerItem(false);
                contentId.setId(cursor.getLong(DBContent.ColumnName.COL_ID.ordinal()));
                contentId.setContainer(isContainer);
                dbId.add(contentId);
                i++;
            } while (cursor.moveToNext());
        } catch (Exception e) {
            e.printStackTrace();
            dbId = null;
        } finally {
            closeCursor(cursor);
        }

        return dbId;
    }

    /**
     * Check if the Media Content item in a specific database ID is a Container
     * 
     * @param mediaType The specific media type of Image / Audio / Video
     * @param dbId The start position
     * @param table 
     * @return Is a Container or not?
     **/
    public boolean isContainerById(String mediaType, long dbId, int table) {
        boolean bResult = true;
        Cursor cursor = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(getContentTableUri(table), null,
                               DBContent._ID + " = ? "
                             + DBContent.MEDIA_TYPE + " = ? ",
                               new String[] {String.valueOf(dbId), MEDIA_TYPE_CONTAINER},
                               DBContent.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                bResult = false;
            }
        } catch (Exception e) {
            e.printStackTrace();
            bResult = false;
        } finally {
            closeCursor(cursor);
        }
        return bResult;
    }

    /**
     * Get an amount of content data about it's whether an item or a container 
     *  in the table of a type of Media content
     * @param mediaType The specific media type of Image / Audio / Video
     * @param pos The start position
     * @param amount The amount of items to be read
     * @param table A specific content table to read data. Current or Preview content table.
     * @return An array of object DlnaContainer
     **/
    public DlnaContainer[] getContainerPropByPos(String mediaType, int pos, int amount, int table) {
        if (pos < 0 || amount < 0)
            throw new IllegalArgumentException("Invalid pos:" + pos + " or invalid amount: " + amount);

        Cursor cursor = null;
        DlnaContainer[] dlnaContainers = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(getContentTableUri(table), null,
                               DBContent.MEDIA_TYPE + " = ? OR " + DBContent.MEDIA_TYPE + " = ? ",
                               new String[] {mediaType, MEDIA_TYPE_CONTAINER},
                               DBContent.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            // move the cursor to position pos
            if (!cursor.moveToPosition(pos)) {
                Log.e(TAG, String.format("Failed to move to position %d", pos));
                return null;
            }

            int totalAvail = cursor.getCount() - pos;
            totalAvail = (totalAvail < amount) ? totalAvail : amount;
//            Log.t(TAG, "Total available data are %d items", totalAvail);
            dlnaContainers = new DlnaContainer[totalAvail];

            int i = 0;
            do {
                dlnaContainers[i] = new DlnaContainer();
                if (!setContainerData(cursor, dlnaContainers[i])) {
                    dlnaContainers[i] = null;
                }
//                else
//                    Log.t(TAG, "Reading pos[%d]... containerId? [%s]", pos+i, dlnaContainers[i].getContainerId());
                i++;
            } while (cursor.moveToNext() && i < totalAvail);
        } catch (Exception e) {
            e.printStackTrace();
            dlnaContainers = null;
        } finally {
//            Log.t(TAG, "finally");
            closeCursor(cursor);
        }

        return dlnaContainers;
    }

    /**
     * Get an amount of Container items of data value which is already written to Media Content table by database ID
     * 
     * @param startIndex The start database ID
     * @param containersAmount The amount of items to be read
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The data read from Media Content table in DB
     **/
    public DlnaContainer[] getContainers(long startIndex, long containersAmount, int table) {
        return (DlnaContainer[]) getContents(MEDIA_TYPE_CONTAINER, startIndex, containersAmount, table);
    }

    /**
     * Check if the Media Content item in a specific position is a Container
     * @param mediaType Specific the media type within {Image, Audio, Video}
     * @param position The start position
     * @param table A specific content table to read data. Current or Preview content table.
     * @return Is a Container or not?
     **/
    public boolean isContainerByPos(String mediaType, int position, int table) {
        boolean bContainer = false;

        Cursor cursor = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(getContentTableUri(table), null,
                               DBContent.MEDIA_TYPE + " = ? ",
                               new String[] {MEDIA_TYPE_CONTAINER},
                               DBContent.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return bContainer;
            }

            bContainer = cursor.moveToPosition(position);
        } catch (Exception e) {
            e.printStackTrace();
            bContainer = false;
        } finally {
            closeCursor(cursor);
        }

        return bContainer;
    }

    /**
     * Copy Container objects data to the DBPreviewContent table
     * @param dbIds the specific DB Ids
     * @return An ArrayList of the DB Ids in the DBPreviewContent table
     */
    public ArrayList<Long> copyContainersToPreview(ArrayList<Long> dbIds) {
        return copyContentToPreview(MEDIA_TYPE_CONTAINER, dbIds);
    }

    /**
     * Update the albumUrl to a Container to DB
     * @param containerId The title of the Container
     * @param albumUrl The albumUrl updated
     * @param table Content table
     * @return True for success; False for failed
     */
    public boolean updateContainerAlbumUrl(String containerId, String albumUrl, int table) {
        boolean bResult = true;

        if (containerId == null || albumUrl == null)
            throw new IllegalArgumentException("container = " + containerId + "url = " + albumUrl);

        mCR = mParentContext.getContentResolver();
        try {
            ContentValues values = new ContentValues();
            values.put(DBContent.ALBUM_URL, albumUrl);
            int nRows = mCR.update(getContentTableUri(table), values, 
                                   DBContent.CONTAINER_ID + " = ? ", 
                                   new String[] {containerId});
            if (nRows <= 0) {
                Log.e(TAG, "Failed to update data");
                bResult = false;
            }
        } catch (Exception e) {
            e.printStackTrace();
            bResult = false;
        }

        return bResult;
    }

    /**
     * Delete a specific ID of a Container item from the Media Content table
     * @param dbId The database ID
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The number of rows deleted.
     **/
    public int deleteContainerById(long dbId, int table) {
        ArrayList<Long> containerIds = new ArrayList<Long>();
        try {
            containerIds.add(dbId);
        } finally {
            containerIds.clear();
            containerIds = null;
        }
        return deleteContent(MEDIA_TYPE_CONTAINER, containerIds, table);
    }

    /**
     * Delete all Container items from the Media Content table
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The number of rows deleted.
     **/
    public int deleteAllContainers(int table) {
        return deleteContent(MEDIA_TYPE_CONTAINER, null, table);
    }

    /**
     * Add an item of Image data to the Media Content table in DB
     * @param dlnaImage The DlnaImage class contains the data that is going to store to the DB
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The database unique ID of the item.  Pass: id >= 1, failed: id = -1
     **/
    public long addImage(DlnaImage dlnaImage, int table) {
        long id = INVALID_DBID;
        if (dlnaImage == null)
            return id;

        mCR = mParentContext.getContentResolver();
        ContentValues values = new ContentValues();

        values.put(DBContent.MEDIA_TYPE, MEDIA_TYPE_IMAGE);
        values.put(DBContent.TITLE, dlnaImage.getTitle());
        values.put(DBContent.URL, dlnaImage.getUrl());
        values.put(DBContent.CREATOR, dlnaImage.getCreator());
        values.put(DBContent.DESCRIPTION, dlnaImage.getDescription());
        values.put(DBContent.PUBLISHER, dlnaImage.getPublisher());
        values.put(DBContent.ALBUM, dlnaImage.getAlbum());
        values.put(DBContent.DATE_TAKEN, dlnaImage.getDateTaken());
        values.put(DBContent.FILE_SIZE, dlnaImage.getFileSize());
        values.put(DBContent.FORMAT, dlnaImage.getFormat());
        values.put(DBContent.RESOLUTION, dlnaImage.getResolution());
        values.put(DBContent.ALBUM_URL, dlnaImage.getAlbumUrl());
        values.put(DBContent.DEVICE_NAME, dlnaImage.getDeviceName());
        values.put(DBContent.DEVICE_UUID, dlnaImage.getDeviceUuid());
        values.put(DBContent.PROTOCOL_NAME, dlnaImage.getProtocolName());
        values.put(DBContent.THUMBNAIL_URL, dlnaImage.getThumbnailUrl());
        values.put(DBContent.PARENT_CONTAINER_ID, dlnaImage.getParentContainerId());

        Uri uri = mCR.insert(getContentTableUri(table), values);
        if (uri != null) {
            id = Integer.valueOf(uri.getLastPathSegment());
            dlnaImage.setDbId(id);
//            Log.d(TAG, "DbId = %d\n", id);
        }

        return id;
    }

    /**
     * Get the amount of Image items in Media Content table
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The amount of total items in Media Content table
     **/
    public long getImageCount(int table) {
        return getContentsCount(MEDIA_TYPE_IMAGE, table);
    }

    /**
     * Get a Image item of data value from the Media Content table from DB through ID
     * @param dbId The database ID
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The data read from Media Content table in DB
     **/
    public DlnaImage getImage(long dbId, int table) {
        return (DlnaImage) getContent(MEDIA_TYPE_IMAGE, dbId, table);
    }

    /**
     * Get an amount of Image items of data value which is already written to Media Content table by position
     * @param pos The start position
     * @param amount The amount of items to be read
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The data read from Media Content table in DB
     **/
    public DlnaImage[] getImagesByPos(int pos, int amount, int table) {
        return (DlnaImage[]) getContentsByPos(MEDIA_TYPE_IMAGE, pos, amount, table);
    }

    /**
     * Get an amount of Image items of data value which is already written to Media Content table by database ID
     * @param startIndex The start database ID
     * @param imagesAmount The amount of items to be read
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The data read from Media Content table in DB
     **/
    public DlnaImage[] getImages(long startIndex, long imagesAmount, int table) {
        return (DlnaImage[]) getContents(MEDIA_TYPE_IMAGE, startIndex, imagesAmount, table);
    }

    /**
     * Get an amount of Image items of data value with specific database IDs
     * @param imageIds An array of database IDs of images
     * @param table The table where the images data read from.
     * @return The data read from Media Content table in DB.
     */
    public DlnaImage[] getImages(long[] imageIds, int table) {
        return (DlnaImage[]) getContents(MEDIA_TYPE_IMAGE, imageIds, table);
    }

    /**
     * Get the whole list of Image Url data value which is already written to Media Content
     * @param None
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The List of Image Url read from Media Content table in DB
     **/
    public List<String> getImagesUrl(int table, DBContent.ColumnName columnName) {
        ArrayList<Long> queryWhere = new ArrayList<Long>();
        queryWhere.add((long) 1);
        queryWhere.add((long) GET_WHOLE_TABLE);

        return getContentsColumnUtil(columnName.ordinal(),
                                          MEDIA_TYPE_IMAGE,
                                          QueryReq.QRY_SEQ_BY_ID,
                                          queryWhere,
                                          table);
    }

    /**
     * Get the Images' Url data from the Current and Preview Media Cotent table by given DB ids.
     * @param ids a long array of given DB ids.
     * @param table DBMediaInfo or DBPreviewContent table
     * @return A List of Url String
     */
    public List<String> getImagesUrlById(long[] ids, int table, DBContent.ColumnName columnName) {
        if (ids == null || ids.length == 0)
            throw new IllegalArgumentException("Invalid DB Id array.");
        if (table < 0) {
            Log.e(TAG, "Invalid table.");
            return null;
        }

        ArrayList<Long> dbIds = new ArrayList<Long>();
        for (long id : ids) {
            dbIds.add(id);
        }

        return getContentsColumnUtil(columnName.ordinal(),
                                      MEDIA_TYPE_IMAGE,
                                      QueryReq.QRY_BY_SEL_ID,
                                      dbIds,
                                      table);
    }

    /**
     * Copy the data of the specific images by database IDs
     *  from the current content table to the Preview table
     * @param dbIds An ArrayList of the specific database IDs of the images
     * @return The copied database IDs in an ArrayList
     */
    public ArrayList<Long> copyImagesToPreview(ArrayList<Long> dbIds) {
        return copyContentToPreview(MEDIA_TYPE_IMAGE, dbIds);
    }

    /**
     * Delete a specific ID of a Image item from the Media Content table
     * @param index The database ID
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The number of rows deleted.
     **/
    public int deleteImageById(long index, int table) {
        ArrayList<Long> imageIds = new ArrayList<Long>();
        try {
            imageIds.add(index);
        } finally {
            imageIds.clear();
            imageIds = null;
        }
        return deleteContent(MEDIA_TYPE_IMAGE, imageIds, table);
    }

    /**
     * Delete all Image items from the Media Content table.  Container items aren't deleted.
     * @param None
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The number of rows deleted.
     **/
    public int deleteAllImages(int table) {
        return deleteContent(MEDIA_TYPE_IMAGE, null, table);
    }

    /**
     * Add an item of Audio data to the Media Content table in DB
     * @param dlnaAudio The DlnaAudio class contains the data that is going to store to the DB
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The database unique ID of the item.  Pass: id >= 1, failed: id = -1
     **/
    public long addAudio(DlnaAudio dlnaAudio, int table) {
        long id = INVALID_DBID;
        if (dlnaAudio == null)
            return id;

        mCR = mParentContext.getContentResolver();
        ContentValues values = new ContentValues();

        values.put(DBContent.MEDIA_TYPE, MEDIA_TYPE_AUDIO);
        values.put(DBContent.TITLE, dlnaAudio.getTitle());
        values.put(DBContent.URL, dlnaAudio.getUrl());
        values.put(DBContent.CREATOR, dlnaAudio.getCreator());
        values.put(DBContent.DESCRIPTION, dlnaAudio.getDescription());
        values.put(DBContent.PUBLISHER, dlnaAudio.getPublisher());
        values.put(DBContent.GENRE, dlnaAudio.getGenre());
        values.put(DBContent.ARTIST, dlnaAudio.getArtist());
        values.put(DBContent.ALBUM, dlnaAudio.getAlbum());
        values.put(DBContent.DATE_TAKEN, dlnaAudio.getDateTaken());
        values.put(DBContent.FILE_SIZE, dlnaAudio.getFileSize());
        values.put(DBContent.FORMAT, dlnaAudio.getFormat());
        values.put(DBContent.DURATION, dlnaAudio.getDuration());
        values.put(DBContent.TRACK_NO, dlnaAudio.getTrackNo());
        values.put(DBContent.ALBUM_URL, dlnaAudio.getAlbumUrl());
        values.put(DBContent.DEVICE_NAME, dlnaAudio.getDeviceName());
        values.put(DBContent.DEVICE_UUID, dlnaAudio.getDeviceUuid());
        values.put(DBContent.PROTOCOL_NAME, dlnaAudio.getProtocolName());
        values.put(DBContent.PARENT_CONTAINER_ID, dlnaAudio.getParentContainerId());

        Uri uri = mCR.insert(getContentTableUri(table), values);
        if (uri != null) {
            id = Integer.valueOf(uri.getLastPathSegment());
            dlnaAudio.setDbId(id);
//            Log.d(TAG, "DbId = %d\n", id);
        }

        return id;
    }

    /**
     * Get the amount of Audio items in Media Content table
     * @param None
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The amount of total items in Media Content table
     **/
    public long getAudioCount(int table) {
        return getContentsCount(MEDIA_TYPE_AUDIO, table);
    }

    /**
     * Get a Audio item of data value from the Media Content table from DB through ID
     * @param dbId The database ID
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The data read from Media Content table in DB
     **/
    public DlnaAudio getAudio(long dbId, int table) {
        return (DlnaAudio) getContent(MEDIA_TYPE_AUDIO, dbId, table);
    }

    /**
     * Get an amount of Audio items of data value which is already written to Media Content table by position
     * @param pos The start position
     * @param amount The amount of items to be read
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The data read from Media Content table in DB
     **/
    public DlnaAudio[] getAudioByPos(int pos, int amount, int table) {
        return (DlnaAudio[]) getContentsByPos(MEDIA_TYPE_AUDIO, pos, amount, table);
    }

    /**
     * Get an amount of Audio items of data value which is already written to Media Content table by database ID
     * @param startIndex The start database ID
     * @param audioAmount The amount of items to be read
     * @param table 
     * @return The data read from Media Content table in DB
     **/
    public DlnaAudio[] getAudio(long startIndex, long audioAmount, int table) {
        return (DlnaAudio[]) getContents(MEDIA_TYPE_AUDIO, startIndex, audioAmount, table);
    }

    /**
     * Get an amount of Audio items of data values with specific dbIds.
     * @param audioIds An array of the database IDs of the Audio. 
     * @param table The table read from. Current working table or Container preview table.
     * @return The array of Audio object data.
     */
    public DlnaAudio[] getAudio(long[] audioIds, int table) {
        return (DlnaAudio[]) getContents(MEDIA_TYPE_AUDIO, audioIds, table);
    }

    /**
     * Copy the selected Audio content by their dbIds from DBMediaInfo to DBPreviewContent table
     * @param dbIds An ArrayList of the specific database IDs of the Audio
     * @return The copied database IDs in an ArrayList
     */
    public ArrayList<Long> copyAudioToPreview(ArrayList<Long> dbIds) {
        return copyContentToPreview(MEDIA_TYPE_AUDIO, dbIds);
    }

    /**
     * Delete a specific ID of a Audio item from the Media Content table
     * @param index The database ID
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The number of rows deleted.
     **/
    public int deleteAudioById(long index, int table) {
        ArrayList<Long> audioIds = new ArrayList<Long>();
        try {
            audioIds.add(index);
        } finally {
            audioIds.clear();
            audioIds = null;
        }
        return deleteContent(MEDIA_TYPE_AUDIO, audioIds, table);
    }

    /**
     * Delete all Audio items from the Media Content table.  Container items aren't deleted.
     * @param None
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The number of rows deleted.
     **/
    public int deleteAllAudio(int table) {
        return deleteContent(MEDIA_TYPE_AUDIO, null, table);
    }

    /**
     * Add an item of Video data to the Media Content table in DB
     * @param dlnaVideo The DlnaVideo class contains the data that is going to store to the DB
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The database unique ID of the item.  Pass: id >= 1, failed: id = -1
     **/
    public long addVideo(DlnaVideo dlnaVideo, int table) {
        long id = INVALID_DBID;
        if (dlnaVideo == null)
            return id;

        mCR = mParentContext.getContentResolver();
        ContentValues values = new ContentValues();

        values.put(DBContent.MEDIA_TYPE, MEDIA_TYPE_VIDEO);
        values.put(DBContent.TITLE, dlnaVideo.getTitle());
        values.put(DBContent.URL, dlnaVideo.getUrl());
        values.put(DBContent.CREATOR, dlnaVideo.getCreator());
        values.put(DBContent.DESCRIPTION, dlnaVideo.getDescription());
        values.put(DBContent.PUBLISHER, dlnaVideo.getPublisher());
        values.put(DBContent.GENRE, dlnaVideo.getGenre());
        values.put(DBContent.ACTOR, dlnaVideo.getActor());
        values.put(DBContent.ARTIST, dlnaVideo.getArtist());
        values.put(DBContent.ALBUM, dlnaVideo.getAlbum());
        values.put(DBContent.DATE_TAKEN, dlnaVideo.getDateTaken());
        values.put(DBContent.FILE_SIZE, dlnaVideo.getFileSize());
        values.put(DBContent.FORMAT, dlnaVideo.getFormat());
        values.put(DBContent.DURATION, dlnaVideo.getDuration());
        values.put(DBContent.RESOLUTION, dlnaVideo.getResolution());
        values.put(DBContent.DURATION, dlnaVideo.getDuration());
        values.put(DBContent.BIT_RATE, dlnaVideo.getBitRate());
        values.put(DBContent.ALBUM_URL, dlnaVideo.getAlbumUrl());
        values.put(DBContent.DEVICE_NAME, dlnaVideo.getDeviceName());
        values.put(DBContent.DEVICE_UUID, dlnaVideo.getDeviceUuid());
        values.put(DBContent.PROTOCOL_NAME, dlnaVideo.getProtocolName());
        values.put(DBContent.PARENT_CONTAINER_ID, dlnaVideo.getParentContainerId());

        Uri uri = mCR.insert(getContentTableUri(table), values);
        if (uri != null) {
            id = Integer.valueOf(uri.getLastPathSegment());
            dlnaVideo.setDbId(id);
//            Log.d(TAG, "DbId = %d\n", id);
        }

        return id;
    }

    /**
     * Get the amount of Video items in Media Content table
     * @param None
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The amount of total items in Media Content table
     **/
    public long getVideoCount(int table) {
        return getContentsCount(MEDIA_TYPE_VIDEO, table);
    }

    /**
     * Get a Video item of data value from the Media Content table in DB through ID
     * @param dbId The database ID
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The data read from Media Content table in DB
     **/
    public DlnaVideo getVideo(long dbId, int table) {
        return (DlnaVideo) getContent(MEDIA_TYPE_VIDEO, dbId, table);
    }

    /**
     * Get an amount of Video items of data value which is already written to Media Content table by position
     * @param pos The start position
     * @param amount The amount of items to be read
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The data read from Media Content table in DB
     **/
    public DlnaVideo[] getVideoByPos(int pos, int amount, int table) {
        return (DlnaVideo[]) getContentsByPos(MEDIA_TYPE_VIDEO, pos, amount, table);
    }

    /**
     * Get an amount of Video items of data value which is already written to Media Content table by database ID
     * @param startIndex The start database ID
     * @param videoAmount The amount of items to be read
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The data read from Media Content table in DB
     **/
    public DlnaVideo[] getVideo(long startIndex, long videoAmount, int table) {
        return (DlnaVideo[]) getContents(MEDIA_TYPE_VIDEO, startIndex, videoAmount, table);
    }

    /**
     * Get an amount of Video items of data values with specific dbIds.
     * @param videoIds An ArrayList of the dbId of the Audio. 
     * @param table The table read from. Current working table or Container preview table.
     * @return The array of Video object data.
     */
    public DlnaVideo[] getVideo(long[] videoIds, int table) {
        return (DlnaVideo[]) getContents(MEDIA_TYPE_VIDEO, videoIds, table);
    }

    /**
     * Copy the data of the specific video by database IDs
     *  from the current content table to the Preview table
     * @param dbIds An ArrayList of the specific database IDs of the video
     * @return The copied database IDs in an ArrayList
     */
    public ArrayList<Long> copyVideoToPreview(ArrayList<Long> dbIds) {
        return copyContentToPreview(MEDIA_TYPE_VIDEO, dbIds);
    }

    /**
     * Delete a specific ID of a Video item from the Media Content table
     * @param dbId The database ID
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The number of rows deleted.
     **/
    public int deleteVideoById(long dbId, int table) {
        ArrayList<Long> videoIds = new ArrayList<Long>();
        try {
            videoIds.add(dbId);
        } finally {
            videoIds.clear();
            videoIds = null;
        }
        return deleteContent(MEDIA_TYPE_VIDEO, videoIds, table);
    }

    /**
     * Delete all Video items from the Media Content table. Container items aren't deleted.
     * @param None
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The number of rows deleted.
     **/
    public int deleteAllVideo(int table) {
        return deleteContent(MEDIA_TYPE_VIDEO, null, table);
    }

    public int deleteAllPreview() {
        mCR = mParentContext.getContentResolver();
        return mCR.delete(DBPreviewContent.CONTENT_URI, null, null);
    }

    /**
     * Get the total amount of items in Media Content table
     * @param table A specific content table to read data. Current or Preview content table.     * 
     * @return The number of Media Content items
     */
    public long getMediaCount(int table) {
        Cursor cursor = null;
        long count = 0;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(getContentTableUri(table),
                               null, null, null,
                               DBContent.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
//                Log.e(TAG, "cursor null or empty");
                return 0;
            }

            count = cursor.getCount();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            closeCursor(cursor);
        }

        return count;
    }

    /**
     * Delete all kinds of items include Container from the Media Content table
     * @param None
     * @param table A specific content table to read data. Current or Preview content table.
     * @return The number of rows deleted.
     **/
    public int clearContentTable(int table) {
        mCR = mParentContext.getContentResolver();
        return mCR.delete(getContentTableUri(table), null, null);
    }

    /**
     * Get all database ID from the Global SearchResult table in DB
     * @param None
     * @return The ArrayList where the result of database IDs store to
     **/
    public ArrayList<Long> getSearchResultDbId() {
        Cursor cursor = null;
        ArrayList<Long> dbId = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBSearchResult.CONTENT_URI,
                               null, null, null,
                               DBSearchResult.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            dbId = new ArrayList<Long>();
            int i = 0;
            do {
                dbId.add(Long.valueOf(cursor.getLong(DBSearchResult.ColumnName.COL_ID.ordinal())));
                i++;
            } while (cursor.moveToNext() && i < cursor.getCount());
        } catch (Exception e) {
            e.printStackTrace();
            dbId = null;
        } finally {
            closeCursor(cursor);
        }

        return dbId;
    }

    /**
     * Get the DB Ids in a specific column and their value fit the request.
     * @param field The specific column name
     * @param fieldValue The requested value
     * @return An array of DB Ids.
     */
    public long[] getSearchResultsDbId(String field, String fieldValue) {
        long[] dbIds = null;
        Cursor cursor = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBSearchResult.CONTENT_URI, null,
                               field + " = ? ", 
                               new String[] {fieldValue},
                               DBSearchResult.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst())
                Log.d(TAG, "cursor null or empty");
            dbIds = new long[cursor.getCount()];
            int i = 0;
            do {
                dbIds[i] = cursor.getLong(DBSearchResult.ColumnName.COL_ID.ordinal());
                i++;
            } while (cursor.moveToNext() && i < cursor.getCount());
        } catch (Exception e) {
            e.printStackTrace();
            dbIds = null;
        } finally {
            closeCursor(cursor);
        }

        return dbIds;
    }

    /**
     * Add an item of data to the Global SearchResult table in DB
     * @param dlnaSearchResult The DlnaSearchResult class contains the data that is going to store to the DB
     * @param keyWord Search key word
     * @return The database unique ID of the item.  Pass: id >= 1, failed: id = -1
     **/
    public long addSearchResult(DlnaSearchResult dlnaSearchResult, String keyWord) {
        long id = INVALID_DBID;
        if (dlnaSearchResult == null || keyWord == null)
            return id;

        Log.d(TAG, String.format("key word = %s", keyWord));

        mCR = mParentContext.getContentResolver();
        ContentValues values = new ContentValues();

        String zTmp = null;
        GroupId groupId = DlnaSearchResult.GroupId.NONE;

        zTmp = dlnaSearchResult.getPhotoAlbum();
        if (zTmp != null && zTmp.contains(keyWord)) {
            values.put(DBSearchResult.PHOTO_ALBUM, zTmp);
            groupId = DlnaSearchResult.GroupId.PHOTO;
            values.put(DBSearchResult.TITLE, dlnaSearchResult.getPhoto());
        }

        zTmp = dlnaSearchResult.getPhoto();
        if (zTmp != null && zTmp.contains(keyWord)) {
            values.put(DBSearchResult.PHOTO, zTmp);
            if (groupId.equals(DlnaSearchResult.GroupId.NONE)) {
                groupId = DlnaSearchResult.GroupId.PHOTO;
                values.put(DBSearchResult.TITLE, zTmp);
            }
        }

        zTmp = dlnaSearchResult.getArtist();
        if (zTmp != null && zTmp.contains(keyWord)) {
            values.put(DBSearchResult.ARTIST, zTmp);
            if (groupId.equals(DlnaSearchResult.GroupId.NONE)) {
                groupId = DlnaSearchResult.GroupId.MUSIC;
                values.put(DBSearchResult.TITLE, dlnaSearchResult.getMusic());
                values.put(DBSearchResult.MUSIC_ALBUM, dlnaSearchResult.getMusicAlbum());
            }
        }

        zTmp = dlnaSearchResult.getMusicAlbum();
        if (zTmp != null && zTmp.contains(keyWord)) {
            values.put(DBSearchResult.MUSIC_ALBUM, zTmp);
            if (groupId.equals(DlnaSearchResult.GroupId.NONE)) {
                groupId = DlnaSearchResult.GroupId.MUSIC;
                values.put(DBSearchResult.TITLE, dlnaSearchResult.getMusic());
                values.put(DBSearchResult.ARTIST, dlnaSearchResult.getArtist());
            }
        }

        zTmp = dlnaSearchResult.getMusic();
        if (zTmp != null && zTmp.contains(keyWord)) {
            values.put(DBSearchResult.MUSIC, zTmp);
            if (groupId.equals(DlnaSearchResult.GroupId.NONE)) {
                groupId = DlnaSearchResult.GroupId.MUSIC;
                values.put(DBSearchResult.TITLE, zTmp);
            }
        }

        zTmp = dlnaSearchResult.getVideoAlbum();
        if (zTmp != null && zTmp.contains(keyWord)) {
            values.put(DBSearchResult.VIDEO_ALBUM, zTmp);
            if (groupId.equals(DlnaSearchResult.GroupId.NONE)) {
                groupId = DlnaSearchResult.GroupId.VIDEO;
                values.put(DBSearchResult.TITLE, dlnaSearchResult.getVideo());
            }
        }

        zTmp = dlnaSearchResult.getVideo();
        if (zTmp != null && zTmp.contains(keyWord)) {
            values.put(DBSearchResult.VIDEO, zTmp);
            if (groupId.equals(DlnaSearchResult.GroupId.NONE)) {
                groupId = DlnaSearchResult.GroupId.VIDEO;
                values.put(DBSearchResult.TITLE, zTmp);
            }
        }

        values.put(DBSearchResult.GROUP_ID, groupId.ordinal());
        values.put(DBSearchResult.MUSIC_DURATION, dlnaSearchResult.getMusicDuration());
        values.put(DBSearchResult.DEVICE_NAME, dlnaSearchResult.getDeviceName());
        values.put(DBSearchResult.DEVICE_UUID, dlnaSearchResult.getDeviceUuid());
        values.put(DBSearchResult.PARENT_CONTAINER_ID, dlnaSearchResult.getParentContainerId());
        values.put(DBSearchResult.ALBUM_URL, dlnaSearchResult.getAlbumUrl());
        values.put(DBSearchResult.URL, dlnaSearchResult.getUrl());
        values.put(DBSearchResult.CONTAINER_NAME, dlnaSearchResult.getContainerName());
        values.put(DBSearchResult.ITEM_NAME, dlnaSearchResult.getItemName());

        Uri uri = mCR.insert(DBSearchResult.CONTENT_URI, values);
        if (uri != null) {
            id = Integer.valueOf(uri.getLastPathSegment());
            dlnaSearchResult.setDbId(id);
//            Log.d(zTmp, "DbId = %d\n", id);
        }

        return id;
    }

    /**
     * Get the amount of the items in SearchResult table
     * @param None
     * @return The amount of total items in SearchResult table
     **/
    public long getSearchResultCount() {
        long cursorCount = 0;
        Cursor cursor = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBSearchResult.CONTENT_URI,
                               null, null, null,
                               DBSearchResult.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
//                Log.e(TAG, "cursor null or empty");
                return 0;
            }

            cursorCount = cursor.getCount();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            closeCursor(cursor);
        }

        return cursorCount;
    }

    /**
     * Get an item of data value from the Global Search Result in DB through database ID
     * @param index The database ID
     * @param dlnaSearchResult The class of DlnaSearchResult where the data could store to
     * @return The data read from SearchResult table in DB
     **/
    public DlnaSearchResult getSearchResult(long index, DlnaSearchResult dlnaSearchResult) {
        Cursor cursor = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBSearchResult.CONTENT_URI, null,
                               DBSearchResult._ID + " = ? ",
                               new String[] {String.valueOf(index)},
                               DBSearchResult.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            if (!setSearchResultData(cursor, dlnaSearchResult))
                return null;
        } catch (Exception e) {
            e.printStackTrace();
            dlnaSearchResult = null;
        } finally {
            closeCursor(cursor);
        }

        return dlnaSearchResult;
    }

    /**
     * Get an amount of items of data value which is already written to DB by starting with an ID
     * @param startIndex The start database ID
     * @param resultsAmount Requested amount
     * @param listItems A List of HashMap contains <Key, data value> read from Global SearchResult table
     * @param gpCollector An array to collect the groupIds of all found items.
     * @return The read amount of items of search result.
     */
    public int getSearchResults(long startIndex,
                                 long resultsAmount,
                                 List<HashMap<String, Object>> listItems,
                                 short[] gpCollector) {
        if (startIndex <= 0 || resultsAmount <= 0) {
            throw new IllegalArgumentException("Invalid startVideoId: " + startIndex + " or videoAmount: " + resultsAmount);
        }

        int amount = 0;
        Cursor cursor = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBSearchResult.CONTENT_URI, null,
                               DBSearchResult._ID + " >= ? ",
                               new String[] {String.valueOf(startIndex)},
                               DBSearchResult.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst())
                return CURSOR_NONE;

            if (listItems == null) {
                Log.d(TAG, "listItems = null");
                return INVALID_ARGUMENT;
            }

            long totalAvail = cursor.getCount();
            totalAvail = resultsAmount < totalAvail ? resultsAmount : totalAvail;
            if (totalAvail > 0) {
                amount = (int) totalAvail;
                Log.d(TAG, String.format("read %d items from DB", amount));
            }

            int i = 0;
            do {
                String zGroupId = cursor.getString(DBSearchResult.ColumnName.COL_GROUPID.ordinal());
                if (zGroupId != null && !zGroupId.equals(String.valueOf(DlnaSearchResult.GroupId.NONE.ordinal()))) {
                    HashMap<String, Object> map = new HashMap<String, Object>();

                    map.put(CcdSdkDefines.SEARCH_DBID, String.valueOf(cursor.getLong(DBSearchResult.ColumnName.COL_ID.ordinal())));
                    map.put(CcdSdkDefines.SEARCH_GROUPID, zGroupId);
                    map.put(CcdSdkDefines.SEARCH_FLAG, SEARCH_FLAG_VALUE);
                    String url = cursor.getString(DBSearchResult.ColumnName.COL_ALBUMURL.ordinal());
                    if (url == null || url.length() == 0) // if albumUrl is empty, then get the exact url for the image.
                        url = cursor.getString(DBSearchResult.ColumnName.COL_URL.ordinal());
                    map.put(CcdSdkDefines.SEARCH_ICON, replaceNullString(url));
                    map.put(CcdSdkDefines.SEARCH_NAME, replaceNullString(cursor.getString(DBSearchResult.ColumnName.COL_TITLE.ordinal())));
                    gpCollector[0] = (short) (gpCollector[0] | (0x1 << Integer.valueOf(zGroupId)));
                    long duration = cursor.getLong(DBSearchResult.ColumnName.COL_MUSICDURATION.ordinal());
                    if (duration > 0)
                        map.put(CcdSdkDefines.SEARCH_DURATION, TimeFormatter.makeTimeString(duration * KILO));
                    else // set the duration to the empty string if it's 0.
                        map.put(CcdSdkDefines.SEARCH_DURATION, "");
                    listItems.add(map);
                    i++;
                }
            } while (cursor.moveToNext() && i < totalAvail);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            closeCursor(cursor);
        }

        return amount;
    }

    /**
     * Get the item Urls for an array of specific DB Ids or a groupId
     * @param field DBId or groupId
     * @param ids DBId or groupId
     * @return A List of the Url of the gotten items
     */
    public List<String> getSearchResultsUrl(String field, long[] ids, DBSearchResult.ColumnName columnName) {
        if (field == null)
            throw new IllegalArgumentException("field is null.");

        List<String> listUrls = null;
        Cursor cursor = null;
        StringBuilder sel = new StringBuilder();
        ArrayList<String> selArgs = new ArrayList<String>();
        mCR = mParentContext.getContentResolver();
        try {
            // convert the ids array to selection arguments
            sel.setLength(0);
            // query by groupId
            if (field.equals(DBSearchResult.GROUP_ID)) {
                sel.append(DBSearchResult.GROUP_ID + " = ? ");
                selArgs.add(String.valueOf(ids[0]));
            }
            // query by selected Ids
            else if (field.equals(DBSearchResult._ID))
            {
                int j = 0;
                for (long id : ids) {
                    sel.append(DBSearchResult._ID + " = ? ");
                    selArgs.add(String.valueOf(id));
                    if (j < ids.length - 1)
                        sel.append("OR ");
                    j++;
                }
            }
            String selection = sel.toString();
            String[] selectionArgs = new String[selArgs.size()];
            selArgs.toArray(selectionArgs);

            // query
            cursor = mCR.query(DBSearchResult.CONTENT_URI, null,
                               selection, selectionArgs,
                               DBSearchResult.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst())
                Log.d(field, "cursor null or empty.");

            // put data to the List
            int i = 0;
            listUrls = new ArrayList<String>();
            do {
                listUrls.add(cursor.getString(columnName.ordinal()));
                i++;
            } while (cursor.moveToNext() && i < cursor.getCount());
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            sel = null;
            selArgs = null;
            closeCursor(cursor);
        }

        return listUrls;
    }

    /**
     * Get the data that grouped by some fieldName
     * @param listItems A List of HashMap contains <Key, data value> read from Global SearchResult table
     * @param groupId The column in index will be grouped by.
     * @return True for success; false for error.
     */
    public int getSearchGroupedResults(List<HashMap<String, Object>> listItems, DlnaSearchResult.GroupId groupId)
    {
        if (groupId == null) {
            return INVALID_ARGUMENT;
        }

        int groupIndex = groupId.ordinal();
        if (listItems == null
            || groupIndex < DlnaSearchResult.GroupId.PHOTO_ALBUM.ordinal()
            || groupIndex > DlnaSearchResult.GroupId.VIDEO.ordinal()) {
//            throw new IllegalArgumentException("Invalid listItems or fileName.");
            return INVALID_ARGUMENT;
        }

        SQLiteDatabase db = mParentContext.openOrCreateDatabase(
                                DATABASE_NAME, Context.MODE_WORLD_READABLE, null);
        int amount = 0;
        String fieldName = searchGroupName[groupIndex];
        Cursor cursor = null;
        try {
            cursor = db.query(DBSearchResult.TABLE_NAME, null,
                              null, null, fieldName, null, DBSearchResult.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return CURSOR_NONE;
            }

            long totalAvail = cursor.getCount();
            do {
                String searchName = cursor.getString(cursor.getColumnIndex(fieldName));
                if (searchName != null)
                {
                    HashMap<String, Object> map = new HashMap<String, Object>();
    
                    map.put(CcdSdkDefines.SEARCH_DBID, "");
                    map.put(CcdSdkDefines.SEARCH_FLAG, SEARCH_FLAG_VALUE);
                    map.put(CcdSdkDefines.SEARCH_ICON, "");
                    map.put(CcdSdkDefines.SEARCH_NAME, searchName);
                    map.put(CcdSdkDefines.SEARCH_GROUPID, String.valueOf(groupIndex));
                    map.put(CcdSdkDefines.SEARCH_DURATION, "");
    
                    listItems.add(map);
                    amount++;
                }
            } while (cursor.moveToNext() && amount < totalAvail);
        } catch (Exception e) {
            e.printStackTrace();
//            amount = 0;
        } finally {
            cursor.close();
            db.close();
        }

        return amount; 
    }

    /**
     * Get the DB Ids of the same groupId
     * @param groupId A Id of a kind of group, e.g. Photo, Music, Artist, Video Album...etc.
     * @return An array of DB Ids in long type
     */
    public long[] getSearchGroupedResults(DlnaSearchResult.GroupId groupId)
    {
        if (groupId.ordinal() <= 0)
            throw new IllegalArgumentException("Invalid groupId.");

        long[] dbIds = null;
        Cursor cursor = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBSearchResult.CONTENT_URI, null,
                               DBSearchResult.GROUP_ID + " = ? ",
                               new String[] {String.valueOf(groupId.ordinal())},
                               DBSearchResult.ORDER_BY_NAME_ASC);
            if (cursor == null || !cursor.moveToFirst())
                Log.e(TAG, "cursor null or empty.");
            dbIds = new long[cursor.getCount()];
            int i = 0;
            do {
                dbIds[i] = cursor.getLong(DBSearchResult.ColumnName.COL_ID.ordinal());
                i++;
            } while(cursor.moveToNext() && i < cursor.getCount());
        } catch (Exception e) {
            e.printStackTrace();
            dbIds = null;
        } finally {
            closeCursor(cursor);
        }

        return dbIds;
    }

    /**
     * Delete a specific ID of an item from the Global SearchResult table
     * @param index The database ID
     * @return The number of rows deleted.
     **/
    public int deleteSearchResultById(long index) {
        mCR = mParentContext.getContentResolver();
        String selection = DBSearchResult._ID + " = ? ";
        String[] selectionArgs = {String.valueOf(index)};
        return mCR.delete(DBSearchResult.CONTENT_URI, selection, selectionArgs);
    }

    /**
     * Delete all items from the Global Search Result table
     * @param None
     * @return The number of rows deleted.
     **/
    public int deleteAllSearchResults() {
        mCR = mParentContext.getContentResolver();
        return mCR.delete(DBSearchResult.CONTENT_URI, null, null);
    }

    /**
     * Add an item of Music Playlist data to the DB
     * @param listName The new playlist name
     * @param musicIds An array contains the database IDs in the content table that is going to store to the DB
     * @param table DBMediaInfo or DBPreviewContent tables
     * @return The database unique ID of the item. Pass: id >= 1, failed: id = -1
     **/
    public long addNewPlaylist(String listName, long[] musicIds, int table) {
        Log.d(TAG, "addMusicPlaylist()");
        long playlistId = INVALID_DBID;

        mCR = mParentContext.getContentResolver();
        ContentValues values = new ContentValues();

        if (listName == null)
            new String("");

        values.put(DBMusicPlaylist.PLAYLIST_NAME, listName);
        values.put(DBMusicPlaylist.IS_PLAYLIST, String.valueOf(true));

        Uri uri = mCR.insert(DBMusicPlaylist.CONTENT_URI, values);
        if (uri != null) {
            playlistId = Integer.valueOf(uri.getLastPathSegment());
//            Log.d(TAG, "playlist Id = %d", playlistId);
        }

        if (musicIds == null || musicIds.length == 0)
            return playlistId;

        addMusicToPlaylist(playlistId, musicIds, table);

        return playlistId;
    }

    /**
     * Add an amount of music to a playlist by Id
     * @param playlistId The playlist Id
     * @param musics An array of music
     * @return The amount of music been added
     */
    public int addMusicToPlaylist(long playlistId, DlnaAudio[] musics) {
        int count = 0;

        mCR = mParentContext.getContentResolver();
        Uri uri = null;
        ContentValues values = new ContentValues();
        for (int i = 0; i < musics.length; i++) {
            values.clear();
            values.put(DBMusicPlaylist.PLAYLIST_ID, playlistId);
            values.put(DBMusicPlaylist.IS_PLAYLIST, String.valueOf(false));
            values.put(DBMusicPlaylist.TITLE, musics[i].getTitle());
            values.put(DBMusicPlaylist.URL, musics[i].getUrl());
            values.put(DBMusicPlaylist.DESCRIPTION, musics[i].getDescription());
            values.put(DBMusicPlaylist.DURATION, musics[i].getDuration());
            values.put(DBMusicPlaylist.FILE_SIZE, musics[i].getFileSize());
            values.put(DBMusicPlaylist.PROTOCOL_NAME, musics[i].getProtocolName());
            
            uri = mCR.insert(DBMusicPlaylist.CONTENT_URI, values);
            if (uri != null) {
              count++;
          }
        }
        return count;
        
    }

    /**
     * Add an amount of music Ids to a playlist by Id
     * @param playlistId The playlist Id
     * @param musicIds An array of music Ids
     * @param table DBMediaInfo or DBPreviewContent tables
     * @return The amount of music been added
     */
    public int addMusicToPlaylist(long playlistId, long[] musicIds, int table) {
        int count = 0;

        mCR = mParentContext.getContentResolver();
        // Check if the playlist Id is exist in the DB.
        Cursor cursor = mCR.query(DBMusicPlaylist.CONTENT_URI, null,
                                  DBMusicPlaylist._ID + " = ? AND " + DBMusicPlaylist.IS_PLAYLIST + " = ? ",
                                  new String[] {String.valueOf(playlistId), String.valueOf(true)},
                                  null);
        if (cursor == null || cursor.getCount() <= 0) {
            Log.w(TAG, String.format("The playlist[%d] is not found.", playlistId));
            return count;
        }
        closeCursor(cursor);

        // Add music from Media Content table to the Music Playlist table for the playlist Id.
        ContentValues values = new ContentValues();
        Uri uri = null;

        for (int i = 0; i < musicIds.length; i++) {
            try {
                cursor = mCR.query(getContentTableUri(table), null, 
                                   DBContent.MEDIA_TYPE + " = ? AND "
                                 + DBContent._ID + " = ? ", 
                                   new String[] {MEDIA_TYPE_AUDIO, String.valueOf(musicIds[i])}, 
                                   DBContent.ORDER_BY_NAME_ASC);
                if (cursor != null
                    && cursor.moveToFirst()) {
                    values.put(DBMusicPlaylist.PLAYLIST_ID, playlistId);
                    values.put(DBMusicPlaylist.IS_PLAYLIST, String.valueOf(false));
                    values.put(DBMusicPlaylist.TITLE, cursor.getString(DBContent.ColumnName.COL_TITLE.ordinal()));
                    values.put(DBMusicPlaylist.URL, cursor.getString(DBContent.ColumnName.COL_URL.ordinal()));
                    values.put(DBMusicPlaylist.DESCRIPTION, cursor.getString(DBContent.ColumnName.COL_DESCRIPTION.ordinal()));
                    values.put(DBMusicPlaylist.DURATION, cursor.getLong(DBContent.ColumnName.COL_DURATION.ordinal()));
                    values.put(DBMusicPlaylist.FILE_SIZE, cursor.getLong(DBContent.ColumnName.COL_FILE_SIZE.ordinal()));
                    values.put(DBMusicPlaylist.PROTOCOL_NAME, cursor.getString(DBContent.ColumnName.COL_PROTOCOLNAME.ordinal()));
                    
                    uri = mCR.insert(DBMusicPlaylist.CONTENT_URI, values);
                    if (uri != null) {
//                        Log.d(TAG, "music[%d], new id = %d", musicIds[i], musicId);
                        count++;
                    }
                }
                closeCursor(cursor);
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                closeCursor(cursor);
            }
        }

        return count;
    }

    /**
     * Get all playlist Ids
     * @param None
     * @return All Playlist Ids in the Music Playlist table in DB
     */
    public ArrayList<Long> getPlaylists() {
        Cursor cursor = null;
        ArrayList<Long> playlists = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBMusicPlaylist.CONTENT_URI, null,
                               DBMusicPlaylist.IS_PLAYLIST + " = ? ",
                               new String[] {String.valueOf(true)}, null);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            playlists = new ArrayList<Long>();
            int i = 0;
            long listId = INVALID_DBID;
            do {
                listId = cursor.getLong(DBMusicPlaylist.ColumnName.COL_ID.ordinal());
                playlists.add(listId);
                i++;
            } while (cursor.moveToNext() && i < cursor.getCount());
        } catch (Exception e) {
            e.printStackTrace();
            playlists = null;
        } finally {
            closeCursor(cursor);
        }

        return playlists;
    }

    /**
     * Get the Music Ids in a Playlist by playlist Id
     * @param playlistId The playlist Id
     * @return The ArrayList of gotten Music Ids in a Playlist
     */
    public ArrayList<Long> getPlaylistMusic(long playlistId) {
        Cursor cursor = null;
        ArrayList<Long> musicIds = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBMusicPlaylist.CONTENT_URI, null,
                               DBMusicPlaylist.PLAYLIST_ID + " = ? AND " + DBMusicPlaylist.IS_PLAYLIST + " = ? ",
                               new String[] {String.valueOf(playlistId), String.valueOf(false)}, null);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            musicIds = new ArrayList<Long>();

            int i = 0;
            do {
                musicIds.add(cursor.getLong(DBMusicPlaylist.ColumnName.COL_ID.ordinal()));
                i++;
            } while (cursor.moveToNext() && i < cursor.getCount());
        } catch (Exception e) {
            e.printStackTrace();
            musicIds = null;
        } finally {
            closeCursor(cursor);
        }
        
        return musicIds;
    }

    /**
     * Get the playlist name for a playlist Id
     * @param playlistId A specific playlist Id
     * @return The Playlist Name
     **/
    public String getPlaylistName(long playlistId)
    {
        Cursor cursor = null;
        String listName = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBMusicPlaylist.CONTENT_URI, null,
                               DBMusicPlaylist.IS_PLAYLIST + " = ? AND " + DBMusicPlaylist._ID + " = ? ",
                               new String[] {String.valueOf(true), String.valueOf(playlistId)}, null);
            if (cursor == null || !cursor.moveToFirst())
            {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            listName = cursor.getString(DBMusicPlaylist.ColumnName.COL_PLAYLISTNAME.ordinal());
        } catch (Exception e) {
            e.printStackTrace();
            listName = null;
        } finally {
            closeCursor(cursor);
        }

        return listName;
    }

    /**
     * Edit the playlist name for a specific playlist Id.
     * @param playlistId The playlist whose name is edited.
     * @param listName The new name for the listId
     * @return True for edit successfully; false for edit failed.
     */
    public boolean setPlaylistName(long playlistId, String listName) {
        boolean bResult = true;

        if (listName == null)
            listName = new String("");

        mCR = mParentContext.getContentResolver();
        try {
            ContentValues values = new ContentValues();
            values.put(DBMusicPlaylist.PLAYLIST_NAME, listName);
//            values.put(DBMusicPlaylist.IS_PLAYLIST, String.valueOf(true));
            int nRows = mCR.update(DBMusicPlaylist.CONTENT_URI, values, 
                                   DBMusicPlaylist._ID + " = ? AND " + DBMusicPlaylist.IS_PLAYLIST + " = ? ",
                                   new String[] {String.valueOf(playlistId), 
                                                 String.valueOf(true)});
            if (nRows <= 0) {
                Log.e(TAG, "Failed to update data");
                bResult = false;
            }
        } catch (Exception e) {
            e.printStackTrace();
            bResult = false;
        }

        return bResult;
    }

    /**
     * Get an item of MusicInfo by musicId
     * @param musicId The specific music Id
     * @return The object contains MusicInfo of the music Id
     */
    public MusicInfo getPlMusicInfo(long musicId) {
        Cursor cursor = null;
        MusicInfo musicInfo = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBMusicPlaylist.CONTENT_URI, null,
                               DBMusicPlaylist._ID + " = ? AND " + DBMusicPlaylist.IS_PLAYLIST + " = ? ",
                               new String[] {String.valueOf(musicId), String.valueOf(false)}, null);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
                return null;
            }

            musicInfo = new MusicInfo();
            musicInfo.setMusicId(cursor.getLong(DBMusicPlaylist.ColumnName.COL_ID.ordinal()));
            musicInfo.setName(cursor.getString(DBMusicPlaylist.ColumnName.COL_TITLE.ordinal()));
            musicInfo.setUrl(cursor.getString(DBMusicPlaylist.ColumnName.COL_URL.ordinal()));
            musicInfo.setDescription(cursor.getString(DBMusicPlaylist.ColumnName.COL_DESCRIPTION.ordinal()));
            musicInfo.setDuration(cursor.getLong(DBMusicPlaylist.ColumnName.COL_DURATION.ordinal()));
            musicInfo.setFileSize(cursor.getLong(DBMusicPlaylist.ColumnName.COL_FILESIZE.ordinal()));
        } catch (Exception e) {
            e.printStackTrace();
            musicInfo = null;
        } finally {
            closeCursor(cursor);
        }
        return musicInfo;
    }

    /**
     * Get the whole music data by specific playlist Ids
     * @param playlistIds Specific playlist Ids
     * @return An object array of DlnaAudio
     */
    public DlnaAudio[] getPlaylistMusic(long[] playlistIds) {
        if (playlistIds == null || playlistIds.length <= 0) {
            throw new IllegalArgumentException("Invalid playlistIds " + playlistIds);
        }

        return getPlMusicUtil(false, null, playlistIds);
    }

    /**
     * Get the Audio data in DlnaAudio object by specific DB Ids in a Playlist
     * @param dbIds An array of specific 
     * @param playlistId The playlist Id of the Audio items
     * @return An object array of DlnaAudio
     */
    public DlnaAudio[] getPlaylistMusic(long[] dbIds, long playlistId) {
        if (dbIds == null || dbIds.length <= 0 || playlistId < 1) {
            throw new IllegalArgumentException("Invalid dbIds " + dbIds + "or Invalid playlistId " + playlistId);
        }

        long[] playlistIds = new long[1];
        playlistIds[0] = playlistId;
        return getPlMusicUtil(true, dbIds, playlistIds);
    }

    /**
     * Get the database ID of the first Music item in a playlist
     * @param playlistId The DB Id of a specific Music Playlist
     * @return The DB Id of the Music Item gotten
     */
    public long getPlFstMusic(long playlistId) {
        if (playlistId <= 0) {
            return INVALID_DBID;
        }

        long musicId = INVALID_DBID;
        Cursor cursor = null;
        mCR = mParentContext.getContentResolver();
        try {
            cursor = mCR.query(DBMusicPlaylist.CONTENT_URI, null,
                               DBMusicPlaylist.PLAYLIST_ID + " = ? AND "
                             + DBMusicPlaylist.IS_PLAYLIST + " = ? ",
                               new String[] {String.valueOf(playlistId), String.valueOf(false)},
                               DBMusicPlaylist.ORDER_BY_ID_ASC);
            if (cursor == null || !cursor.moveToFirst()) {
                Log.e(TAG, "cursor null or empty");
//                return musicId;
            } else {
                musicId = cursor.getLong(DBMusicPlaylist.ColumnName.COL_ID.ordinal());
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            closeCursor(cursor);
        }

        return musicId;
    }

    /**
     * Delete a specific playlist
     * @param playlistId The playlist Id that is being deleted.
     * @return The amount of the items in Playlist table including the playlist and its music been deleted.
     */
    public int deletePlaylist(long playlistId) {
        int count = 0;
        if (playlistId <= 0) {
            throw new IllegalArgumentException("Invalid argument listId %d" + playlistId);
        }

        mCR = mParentContext.getContentResolver();
        count = mCR.delete(DBMusicPlaylist.CONTENT_URI,
                           DBMusicPlaylist._ID + " = ? AND "
                         + DBMusicPlaylist.IS_PLAYLIST + " = ? ",
                           new String[] {String.valueOf(playlistId), String.valueOf(true)});
        count += mCR.delete(DBMusicPlaylist.CONTENT_URI, 
                            DBMusicPlaylist.PLAYLIST_ID + " = ? AND "
                          + DBMusicPlaylist.IS_PLAYLIST + " = ? ",
                            new String[] {String.valueOf(playlistId), String.valueOf(false)});
        return count;
    }

    /**
     * Delete a mount of music from a specific Playlist by Id
     * @param playlistId The specific playlist Id
     * @param musicIds An array of the deleting music Ids
     * @return The amount of deleted Music Info from the playlist
     */
    public int deleteMusicFromPl(long playlistId, long[] musicIds) {
        int delCount = 0;
        if (musicIds == null) {
            throw new IllegalArgumentException("Invalid argument musicIds = null");
        } else if (musicIds.length == 0) {
            return delCount;
        }
        int idCount = musicIds.length;

        mCR = mParentContext.getContentResolver();
        int i = 0;
        do {
            delCount += mCR.delete(DBMusicPlaylist.CONTENT_URI, 
                                   DBMusicPlaylist.PLAYLIST_ID + " = ? AND "
                                 + DBMusicPlaylist.IS_PLAYLIST + " = ? AND "
                                 + DBMusicPlaylist._ID + " = ? ",
                                   new String[] {String.valueOf(playlistId),
                                                 String.valueOf(false),
                                                 String.valueOf(musicIds[i])});
            i++;
        } while (i < idCount);

        return delCount;
    }

    /**
     * Delete all items from all the tables
     * @param None
     **/
    public void clearCache() {
        deleteAllDevices();
        clearContentTable(CntntTbl.TBL_CURNT);
        clearContentTable(CntntTbl.TBL_PREVW);
        deleteAllSearchResults();
    }

    /** The utilities to get single column value in a table **/

    /**
     * @param util the mUtil to set
     */
    public static void setUtil(DBUtil util) {
        DBManager.mUtil = util;
    }

    /**
     * @return the mUtil
     */
    public static DBUtil getUtil() {
        return mUtil;
    }

}
