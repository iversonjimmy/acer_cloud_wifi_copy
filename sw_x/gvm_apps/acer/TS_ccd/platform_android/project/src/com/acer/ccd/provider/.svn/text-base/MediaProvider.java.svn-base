/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.provider;

import java.io.File;
import java.io.IOException;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.LinkedList;
import java.util.Locale;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.ContentResolver;
import android.database.Cursor;
import android.media.ExifInterface;
import android.net.Uri;
import android.provider.MediaStore;
import android.util.Log;

import com.acer.ccd.serviceclient.McaClient;
import com.acer.ccd.util.CcdSdkDefines;
import com.acer.ccd.util.InternalDefines;

public class MediaProvider {
    private static final String TAG = "MediaProvider";
    private ArrayList<HashMap<String, String>> mResults = null;
    private ArrayList<HashMap<String, String>> mCloudMedia = null;
    private ArrayList<HashMap<String, String>> mLocalMedia = null;
    private MediaSource mMediaSource;
    private McaClient mBoundService;
    private long mCloudDeviceId;
    private ContentResolver mCR;
    public static String DATASET = "_data_set";

    private long getLocalDataStart = 0;
    private long getLocalDataTime = 0;
    private long getLocalPhotoStart = 0;
    private long getLocalPhotoTime = 0;
    private static ArrayList<CloudIdMapping> mCloudIdMap = new ArrayList<CloudIdMapping>();

    public static enum MediaSource {
        LOCAL,
        CLOUD,
        ALL
    }

    // Used for Tian Shan

    public static final String TIAN_SHAN_METADATA_ALBUM_URL = "AlbumUrl";
    public static final String TIAN_SHAN_METADATA_CONTAINER_ID = "ContainerId";
    public static final String TIAN_SHAN_METADATA_GENRE = "Genre";
    public static final String TIAN_SHAN_METADATA_MEDIA = "Media";
    public static final String TIAN_SHAN_METADATA_THUMBNAIL_URL = "ThumbnailUrl";

    public MediaProvider(McaClient service, ContentResolver cr) {
        mBoundService = service;
        mCR = cr;
    }

    /**
     * Query the given URI and source, returning a HashMap<String, String> ArrayList
     *
     * @param source
     *            the source where the media data is from. It could be CLOUD, LOCAL or ALL.
     * @param mediaUri
     *            The URI, using the content:// scheme, for the content to retrieve.
     * @param cloudDeviceId
     *            The master server id
     * @param projection
     *            A list of which columns to return. Passing null will return nothing.
     * @param selection
     *            A filter declaring which rows to return, formatted as an SQL WHERE clause (excluding the WHERE itself). Passing null will return all rows for the given URI.
     * @param selectionArgs
     *            The values correspond to the sequence of selection
     * @param sortOrder
     *            How to order the rows
     * @return ArrayList<HashMap<String, String>> contains the rows of data
     */
    public ArrayList<HashMap<String, String>> query(MediaSource source, Uri mediaUri, long cloudDeviceId, String[] projection, String selection,
            String[] selectionArgs, int sortOrder) {

        synchronized(this) {
            mResults = new ArrayList<HashMap<String, String>>();
            mCloudMedia = new ArrayList<HashMap<String, String>>();
            mLocalMedia = new ArrayList<HashMap<String, String>>();
            mMediaSource = source;
            mCloudDeviceId = cloudDeviceId;
            switch (source) {
            case LOCAL:
                if (mCR != null) {
                    getLocalDataStart = System.currentTimeMillis();
                    getLocalData(mediaUri, projection, selection, selectionArgs, sortOrder);
                    getLocalDataTime = System.currentTimeMillis() - getLocalDataStart;
                    //Log.i(TAG, "getLocalDataTime: " + getLocalDataTime);
                    if (mLocalMedia.size() > 0) {
                        for (HashMap<String, String> hm : mLocalMedia) {
                            hm.put(DATASET, MediaSource.LOCAL.name());
                            mResults.add(hm);
                        }
                    }
                    break;
                }
            case CLOUD:
                if ((CcdSdkDefines.INVALID_CLOUD_DEVICE_ID != mCloudDeviceId) && (mBoundService != null)) {
                    getCloudData(mediaUri, projection, selection, selectionArgs, sortOrder);
                    if (mCloudMedia.size() > 0) {
                        for (HashMap<String, String> hm : mCloudMedia) {
                            hm.put(DATASET, MediaSource.CLOUD.name());
                            mResults.add(hm);
                        }
                    }
                }
                break;
            case ALL:
                if (mCR != null) {
                    getLocalData(mediaUri, projection, selection, selectionArgs, sortOrder);
                }
                if ((CcdSdkDefines.INVALID_CLOUD_DEVICE_ID != mCloudDeviceId) && (mBoundService != null)) {
                    getCloudData(mediaUri, projection, selection, selectionArgs, sortOrder);
                }
                if ((CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_TIMELINE == sortOrder) &&
                    (isImageUri(mediaUri) || isVideoUri(mediaUri)) &&
                    (mLocalMedia.size() > 0) &&
                    (mCloudMedia.size() > 0)) {
                    sortAllByTimeline();
                } else {
                    if (mLocalMedia.size() > 0) {
                        for (HashMap<String, String> hm : mLocalMedia) {
                            hm.put(DATASET, MediaSource.LOCAL.name());
                            mResults.add(hm);
                        }
                    }
                    if (mCloudMedia.size() > 0) {
                        for (HashMap<String, String> hm : mCloudMedia) {
                            hm.put(DATASET, MediaSource.CLOUD.name());
                            mResults.add(hm);
                        }
                    }
                }
                break;
            }
            if (mResults.size() > 0) {
//                Log.i(TAG, "***media source: " + source);
                int localCount = 0;
                int cloudCount = 0;
                for (HashMap<String, String> hm : mResults) {
//                    Log.i(TAG, "***[result] id: " + hm.get(MediaStore.MediaColumns._ID) + " , " + hm.get(MediaProvider.DATASET));
                    if (hm.get(DATASET).equals(MediaSource.LOCAL.name())) {
                        localCount ++;
                    } else if (hm.get(DATASET).equals(MediaSource.CLOUD.name())) {
                        cloudCount ++;
                    }
                }
                Log.i(TAG, "***local count: " + localCount + ", cloud count: " + cloudCount + ", all: " + mResults.size());
            }
            return mResults;
        }
    }

    /**
     * sort cloud and local media together
     */
    private void sortAllByTimeline() {
        int cloudIndex, localIndex, cloudSize, localSize, totalSize;
        cloudIndex = 0;
        localIndex = 0;
        cloudSize = mCloudMedia.size();
        localSize = mLocalMedia.size();
        totalSize = 0;
        do {
            if ((cloudIndex < cloudSize) && (localIndex < localSize)) {
                long cloudDate = Long.valueOf(mCloudMedia.get(cloudIndex).get(MediaStore.Images.Media.DATE_TAKEN));
                long localDate = Long.valueOf(mLocalMedia.get(localIndex).get(MediaStore.Images.Media.DATE_TAKEN));
                if (cloudDate < localDate) {
                    mCloudMedia.get(cloudIndex).put(DATASET, MediaSource.CLOUD.name());
                    mResults.add(mCloudMedia.get(cloudIndex));
                    localIndex++;
                } else {
                    mLocalMedia.get(localIndex).put(DATASET, MediaSource.LOCAL.name());
                    mResults.add(mLocalMedia.get(localIndex));
                    localIndex++;
                }
            } else {
                if (cloudSize == cloudIndex) {
                    mLocalMedia.get(localIndex).put(DATASET, MediaSource.LOCAL.name());
                    mResults.add(mLocalMedia.get(localIndex));
                    localIndex++;
                } else if (localSize == localIndex) {
                    mCloudMedia.get(cloudIndex).put(DATASET, MediaSource.CLOUD.name());
                    mResults.add(mCloudMedia.get(localIndex));
                    cloudIndex++;
                }
            }
            totalSize++;
        } while (totalSize < (cloudSize + localSize));

    }

    private boolean isImageUri(Uri uri) {
        return uri.equals(MediaStore.Images.Media.EXTERNAL_CONTENT_URI);
    }

    private boolean isVideoUri(Uri uri) {
        return uri.equals(MediaStore.Video.Media.EXTERNAL_CONTENT_URI);
    }

    private boolean isAudioUri(Uri uri) {
        if ((uri.equals(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI)) ||
            (uri.equals(MediaStore.Audio.Albums.EXTERNAL_CONTENT_URI)) ||
            (uri.equals(MediaStore.Audio.Artists.EXTERNAL_CONTENT_URI)) ||
            (uri.equals(MediaStore.Audio.Genres.EXTERNAL_CONTENT_URI)) ||
            (uri.equals(MediaStore.Audio.Playlists.EXTERNAL_CONTENT_URI))) {
                return true;
            }
        return false;
    }

    /**
     * determine whether or not the cloud data meets the where clause in the query
     *
     * @param jsonObj
     *            the json object which saves the cloud data
     * @param selection
     * @param selectionArgs
     * @return true if the cloud meets the where clause, false otherwise.
     */
    private boolean isCloudDataQualified(JSONObject jsonObj, SelectionPair pair) {
        
        if (null == pair) {
            return true;
        }
        
        boolean qualified = false;
        String cloudValue = jsonObj.optString(pair.key, null);
        for (String s : pair.values) {
            Log.i(TAG, "isCloudDataQualified, key: " + pair.key + ", value: " + s + ", cloudValue: " + cloudValue);
            if (pair.key.equals(MediaStore.MediaColumns._ID)) {
                s = getRealCloudId(Long.valueOf(s));
            }
            if (cloudValue.equals(s)) {
                return true;
            }
        }
        return qualified;
    }

    private void parseSqlSelection(String selection, SelectionPair pair) {

        String value = null;
        String[] splits = null;
        Pattern pattern = Pattern.compile("(.*?)(?:\\s+in\\s+|\\s+IN\\s+|\\s*=\\s*)\\(*(.*)\\)*");
        Log.i(TAG, "selection: " + selection);
        Matcher matcher = pattern.matcher(selection);
        if (matcher.find()) {
            //Log.i(TAG, "find!!");
            pair.key = matcher.group(1);
            value = matcher.group(2);
            splits = value.split(",");
//            Log.i(TAG, "splits size: " + splits.length);
//            for (String s : splits) {
//                Log.i(TAG, "splits: " + s);
//            }
            pair.values = new String[splits.length];
            int i = 0;
            for (String s : splits) {
                pattern = Pattern.compile("\'(.*)\'");
                matcher = pattern.matcher(s.trim());
                if (matcher.find()) {
                    pair.values[i++] = matcher.group(1);
                } else {
                    pair.values[i++] = s.trim();
                }
            }
        }
    }

    private long insertCloudId(String id) {
        for (CloudIdMapping c : mCloudIdMap) {
            if (c.cloudId.equals(id)) {
                return c.idToApp;
            }
        }
        long newId = Long.MAX_VALUE - mCloudIdMap.size();
        CloudIdMapping c = new CloudIdMapping(newId, id);
        mCloudIdMap.add(c);
        return newId;
    }

    private String getRealCloudId(long id) {
        for (CloudIdMapping c : mCloudIdMap) {
            if (c.idToApp == id) {
                return c.cloudId;
            }
        }
        return null;
    }

    private void sortCloudPhotoByAlbum(JSONObject jsonObj, HashMap<String, String> hm, HashMap<String, Object> bucketList) {
        String bucket = jsonObj.optString(MediaStore.Images.Media.BUCKET_DISPLAY_NAME, null);
        if (!hm.containsKey(MediaStore.Images.Media.BUCKET_DISPLAY_NAME)) {
            hm.put(MediaStore.Images.Media.BUCKET_DISPLAY_NAME, bucket);
        }
        int count = 0;
        if (!bucketList.containsKey(bucket)) {
            count = 1;
            bucketList.put(bucket, count);
            // only add one data for each album
            mCloudMedia.add(hm);
        } else {
            // keep counting the photo number of an album
            count = (Integer) bucketList.get(bucket);
            count = count + 1;
            bucketList.put(bucket, count);
        }
        for (HashMap<String, String>hm1 : mCloudMedia) {
            if (hm1.get(MediaStore.Images.Media.BUCKET_DISPLAY_NAME).equals(bucket)) {
                // renew the album photo count once it's updated
                hm1.put(CloudMediaColumns.COUNT_IN_ALBUM, Integer.toString(count));
                break;
            }
        }
    }

    private void parseCloudMeta(JSONObject jsonObj, String[] projection, HashMap<String, String> hm) {
        String value = null;
        String id = jsonObj.optString(MediaStore.Images.Media._ID, null);
        for (String s : projection) {
            if (s.equals(CloudMediaColumns.SOURCE)) {
                value = String.valueOf(jsonObj.optInt(CloudMediaColumns.SOURCE, -1));
                if (value.equals("-1")) {
                    value = null;
                }
                hm.put(s, value);
            } else if (s.equals(MediaStore.Images.Media.DATE_TAKEN) || s.equals(MediaStore.Audio.Media.DURATION)) {
                value = String.valueOf(jsonObj.optLong(s, -1));
                if (value.equals("-1")) {
                    value = null;
                }
                hm.put(s, value);
            } else if ((s.equals(CloudMediaColumns.THUMBNAIL_URL)) || (s.equals(MediaStore.Audio.Media.ALBUM_ART))) {
                if (id != null) {
                    value = mBoundService.getContentUrl(mCloudDeviceId, id, 2);
                    hm.put(s, value);
                }
            } else if (s.equals(MediaStore.Images.Media.DATA)) {
                if (id != null) {
                    value = mBoundService.getContentUrl(mCloudDeviceId, id, 1);
                    hm.put(s, value);
                }
            } else if (s.equals(MediaStore.Audio.Genres.NAME)) {
                value = jsonObj.optString(TIAN_SHAN_METADATA_GENRE, null);
                hm.put(s, value);
            } else if (s.equals(MediaStore.MediaColumns._ID)) {
                if (id != null) {
                    hm.put(s, String.valueOf(insertCloudId(id)));
                }
            } else {
                value = jsonObj.optString(s, null);
                hm.put(s, value);
            }
        }
        if (!hm.containsKey(MediaStore.Audio.Media.ALBUM_ID)) {
            value = jsonObj.optString(MediaStore.Audio.Media.ALBUM_ID, null);
            if (value != null) {
                hm.put(MediaStore.Audio.Media.ALBUM_ID, value);
            }
        }
    }
    /**
     * save the wanted cloud data in mCloudMedia list
     *
     * @param mediaUri
     * @param cloudDeviceId
     * @param projection
     * @param selection
     * @param selectionArgs
     * @param sortOrder
     */
    private void getCloudData(Uri mediaUri, String[] projection, String selection, String[] selectionArgs, int sortOrder) {

        // determine the media type for TianShan
        int mediaType = 0;
        if (isImageUri(mediaUri)) {
            mediaType = InternalDefines.TIAN_SHAN_MEDIA_FLAG_PAGE_PHOTO;
            getCloudPhotoVideo(mediaType, projection, selection, sortOrder);
        } else if (isVideoUri(mediaUri)) {
            mediaType = InternalDefines.TIAN_SHAN_MEDIA_FLAG_PAGE_VIDEO;
            getCloudPhotoVideo(mediaType, projection, selection, sortOrder);
        } else if (isAudioUri(mediaUri)) {
            mediaType = InternalDefines.TIAN_SHAN_MEDIA_FLAG_PAGE_MUSIC;
            getCloudAudio(projection, selection, sortOrder);
        }
    }

    private void getCloudAudioAlbum(ArrayList<HashMap<String, String>> albumList, String[] projection) {
        JSONArray jsonAry = new JSONArray();
        int i = 0;
        int len = 0;
        String value = null;
        mBoundService.getMetaData(mCloudDeviceId, InternalDefines.TIAN_SHAN_MEDIA_FLAG_PAGE_MUSIC, CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_ALBUM, jsonAry);
        try {
            len = jsonAry.length();
            Log.i(TAG, "cloud music album number: " + len);
            for (i = 0; i < len; i ++) {
                JSONObject jsonObj = jsonAry.getJSONObject(i);
                //Log.i(TAG, jsonObj.toString(4));
                HashMap<String, String> hm = new HashMap<String, String>();
                for (String s : projection) {
                    if ((s.equals(MediaStore.Audio.Albums.NUMBER_OF_SONGS)) || (s.equals(CloudMediaColumns.ALBUM_DURATION))) {
                        hm.put(s, String.valueOf(0));
                    } else if (s.equals(MediaStore.Audio.Albums.ALBUM)) {
                        value = jsonObj.optString(s, "unknown");
                        hm.put(s, value);
                    } else {
                        value = jsonObj.optString(s, null);
                        hm.put(s, value);
                    }
                }
                if (!hm.containsKey(MediaStore.MediaColumns._ID)) {
                    value = jsonObj.optString(MediaStore.MediaColumns._ID, null);
                    hm.put(MediaStore.MediaColumns._ID, value);
                }
                //Log.i(TAG, "cloud album id: " + hm.get(MediaStore.MediaColumns._ID) + ", cloud album name: " + hm.get(MediaStore.Audio.Albums.ALBUM));

                albumList.add(hm);
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    private void getCloudRawAudio(String[] projection, String selection, ArrayList<HashMap<String, String>> list) {
        JSONArray jsonAry = new JSONArray();
        mBoundService.getMetaData(mCloudDeviceId, InternalDefines.TIAN_SHAN_MEDIA_FLAG_PAGE_MUSIC, CcdSdkDefines.TIAN_SHAN_SORT_FLAG_NONE, jsonAry);

        int len = jsonAry.length();
        Log.i(TAG, "cloud raw audio size: " + len);
        SelectionPair pair = null;

        // parse the selection parameter
        if (null != selection) {
            pair = new SelectionPair();
            parseSqlSelection(selection, pair);
        }

        try {
            for (int i = 0; i < len; i++) {
                JSONObject jsonObj = jsonAry.getJSONObject(i);
                //Log.i(TAG, "cloud raw audio: " + jsonObj.toString(4));
                if (isCloudDataQualified(jsonObj, pair)) {
                    //Log.i(TAG, "cloud id: " + id + " is qualified");
                    HashMap<String, String> hm = new HashMap<String, String>();
                    parseCloudMeta(jsonObj, projection, hm);
                    list.add(hm);
                }
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }
    private void getCloudAudio(String[] projection, String selection, int sortOrder) {

        ArrayList<HashMap<String, String>> albumList = null;
        ArrayList<HashMap<String, String>> artistList = null;
        ArrayList<HashMap<String, String>> rawAudio = null;
        if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_ALBUM == sortOrder) {
            albumList = new ArrayList<HashMap<String, String>>();
            rawAudio = new ArrayList<HashMap<String, String>>();
            getCloudAudioAlbum(albumList, projection);
            String[] myProjection = new String[]{MediaStore.Audio.Media.ALBUM_ID, MediaStore.Audio.Media.DURATION};
            getCloudRawAudio(myProjection, null, rawAudio);

            if (albumList.size() > 0 && rawAudio.size() > 0) {
            // count the number of songs in each album
                for (HashMap<String, String> audio : rawAudio) {
                    String albumId = audio.get(MediaStore.Audio.Media.ALBUM_ID);
                    if (albumId != null) {
                        for (HashMap<String, String> album : albumList) {
                            if (album.get(MediaStore.MediaColumns._ID).equals(albumId)) {
                                int count = Integer.valueOf(album.get(MediaStore.Audio.Albums.NUMBER_OF_SONGS));
                                count ++;
                                album.put(MediaStore.Audio.Albums.NUMBER_OF_SONGS, String.valueOf(count));
                                // count album duration
                                if (album.get(CloudMediaColumns.ALBUM_DURATION) != null) {
                                    long duration = Long.valueOf(album.get(CloudMediaColumns.ALBUM_DURATION));
                                    if (audio.get(MediaStore.Audio.Media.DURATION) != null) {
                                        long audioDuration = Long.valueOf(audio.get(MediaStore.Audio.Media.DURATION));
                                        duration += audioDuration;
                                        album.put(CloudMediaColumns.ALBUM_DURATION, String.valueOf(duration));
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }

            for (HashMap<String, String>hm : albumList) {
                mCloudMedia.add(hm);
            }
        } else if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_ARTIST == sortOrder) {
            artistList = new ArrayList<HashMap<String, String>>();
            ArrayList<String> albums = new ArrayList<String>();
            rawAudio = new ArrayList<HashMap<String, String>>();
            getCloudRawAudio(projection, selection, rawAudio);
            for (HashMap<String, String>audio : rawAudio) {
                boolean newArtist = true;
                boolean newAlbum = true;
                String albumId = audio.get(MediaStore.Audio.Media.ALBUM_ID);
                for (HashMap<String, String>artist : artistList) {
                    String artistName = artist.get(MediaStore.Audio.Artists.ARTIST);
                    if ((artistName != null) && (artistName.equals(audio.get(MediaStore.Audio.Artists.ARTIST)))) {
                        int count = Integer.valueOf(artist.get(MediaStore.Audio.Artists.NUMBER_OF_TRACKS));
                        count ++;
                        artist.put(MediaStore.Audio.Artists.NUMBER_OF_TRACKS, String.valueOf(count));
                        newArtist = false;
                        if (!albums.contains(albumId)) {
                            count = Integer.valueOf(artist.get(MediaStore.Audio.Artists.NUMBER_OF_ALBUMS));
                            count ++;
                            artist.put(MediaStore.Audio.Artists.NUMBER_OF_ALBUMS, String.valueOf(count));
                            albums.add(albumId);
                        }
                        break;
                    }
                }
                if (true == newArtist) {
                    audio.put(MediaStore.Audio.Artists.NUMBER_OF_TRACKS, String.valueOf(1));
                    audio.put(MediaStore.Audio.Artists.NUMBER_OF_ALBUMS, String.valueOf(1));
                    albums.add(albumId);
                    artistList.add(audio);
                }
            }
            for (HashMap<String, String>hm : artistList) {
                mCloudMedia.add(hm);
            }
        } else if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_GENRE == sortOrder) {
            //TODO: genre sorting
        } else if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_PLAYLIST == sortOrder) {
            //TODO: playlist sorting
        } else if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_NONE == sortOrder) {
            rawAudio = new ArrayList<HashMap<String, String>>();
            getCloudRawAudio(projection, selection, rawAudio);
            for (HashMap<String, String>hm : rawAudio) {
                mCloudMedia.add(hm);
            }
            Log.i(TAG, "getCloudAudio, mCloudMedia size: " + mCloudMedia.size());
        }
    }

    private void getCloudPhotoVideo(int mediaType, String[] projection, String selection, int sortOrder) {
        JSONArray jsonAry = new JSONArray();
        mBoundService.getMetaData(mCloudDeviceId, mediaType, sortOrder, jsonAry);

        int len = jsonAry.length();
        Log.i(TAG, "cloud photo/video size: " + len);
        String value = null;
        SelectionPair pair = null;

        // parse the selection parameter
        if (null != selection) {
            pair = new SelectionPair();
            parseSqlSelection(selection, pair);
        }

        HashMap<String, Object> bucketList = new HashMap<String, Object>();
        try {
            for (int i = 0; i < len; i++) {
                JSONObject jsonObj = jsonAry.getJSONObject(i);
                // Log.i(TAG, "cloud: " + jsonObj.toString(4));
                if (isCloudDataQualified(jsonObj, pair)) {
                    //Log.i(TAG, "cloud id: " + id + " is qualified");
                    HashMap<String, String> hm = new HashMap<String, String>();
                    parseCloudMeta(jsonObj, projection, hm);

                    if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_ALBUM == sortOrder) {
                        sortCloudPhotoByAlbum(jsonObj, hm, bucketList);
                    } else {
                        if ((CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_TIMELINE == sortOrder) &&
                            (!hm.containsKey(MediaStore.Images.Media.DATE_TAKEN)) &&
                            (MediaSource.ALL == mMediaSource)) {
                            // DATE_TAKEN will be needed in this case for me to do cloud and local data merge
                            value = String.valueOf(jsonObj.optLong(MediaStore.Images.Media.DATE_TAKEN, -1));
                            if (value.equals("-1")) {
                                value = null;
                            }
                            hm.put(MediaStore.Images.Media.DATE_TAKEN, value);
                        }
                        mCloudMedia.add(hm);
                    }
                }
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    /**
     * get local media data
     *
     * @param mediaUri
     * @param cr
     * @param projection
     * @param selection
     * @param selectionArgs
     * @param sortOrder
     */

    private void getLocalData(Uri mediaUri, String[] projection, String selection, String[] selectionArgs, int sortOrder) {
        ArrayList<String> validProjectionList = new ArrayList<String>();
        LinkedList<HashMap<String, String>> localData = null;
        boolean needThumbnail = false;

        // construct the valid projection for MediaStore query
        for (String s : projection) {
            if (!CloudMediaColumns.isCloudColumn(s)) {
                validProjectionList.add(s);
            } else {
                if (s.equals(CloudMediaColumns.THUMBNAIL_URL)) {
                    needThumbnail = true;
                }
            }
        }

        if (!validProjectionList.contains(MediaStore.MediaColumns._ID)) {
            validProjectionList.add(MediaStore.MediaColumns._ID);
        }

        if (isImageUri(mediaUri) || isVideoUri(mediaUri)) {
            if (!validProjectionList.contains(MediaStore.Images.Media.DATA)) {
                validProjectionList.add(MediaStore.Images.Media.DATA);
            }
            if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_TIMELINE == sortOrder) {
                addLocalColumnsForTimelineSort(validProjectionList);
            } else if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_ALBUM == sortOrder) {
                addLocalColumnsForAlbumSort(validProjectionList);
            }
            String[] validProjection = (String[]) validProjectionList.toArray(new String[validProjectionList.size()]);
            //getLocalPhotoStart = System.currentTimeMillis();
            localData = getLocalPhoto(mediaUri, validProjection, selection, selectionArgs, sortOrder);
            //getLocalPhotoTime = System.currentTimeMillis() - getLocalPhotoStart;
            //Log.i(TAG, "getLocalPhotoTime: " + getLocalPhotoTime);
        } else if (isAudioUri(mediaUri)) {
            localData = getLocalAudio(mediaUri, projection, selection, selectionArgs, sortOrder);
        }
        Log.i(TAG, "localData size: " + localData.size());
        if (localData.size() > 0) {
            setLocalMediaList(mediaUri, localData, needThumbnail);
        }
    }

    private void getThumbnail(Uri mediaUri, LinkedList<HashMap<String, String>> list) {
        String[] idList = new String[list.size()];
        HashMap<String, String> idThumbnailList = new HashMap<String, String>();
        int i = 0;
        for (HashMap<String, String> hm : list) {
            idList[i++] = hm.get(MediaStore.MediaColumns._ID);
//            Log.i(TAG, "getThumbnail, id: " + hm.get(MediaStore.MediaColumns._ID));
        }
        Arrays.sort(idList);
//        Log.i(TAG, "list size: " + list.size() + ", idList size: " + idList.length);
        thumbnailDataForImage(mediaUri, idList, idThumbnailList);
        if (idThumbnailList.size() > 0) {
            for (HashMap<String, String> hm : list) {
//                Log.i(TAG, "thumbnail url: " + idThumbnailList.get(hm.get(MediaStore.MediaColumns._ID)));
                hm.put(CloudMediaColumns.THUMBNAIL_URL, idThumbnailList.get(hm.get(MediaStore.MediaColumns._ID)));
//                Log.i(TAG, "thumbnail_url: " + hm.get(CloudMediaColumns.THUMBNAIL_URL));
            }
        }
    }
    /**
     * add the requested columns which are not MediaStore ones in the mLocalMedia list
     *
     * @param mediaUri
     * @param list
     * @param needThumbnail
     */
    private void setLocalMediaList(Uri mediaUri, LinkedList<HashMap<String, String>> list, boolean needThumbnail) {
        if (needThumbnail) {
            getThumbnail(mediaUri, list);
        }
        for (HashMap<String, String> hm : list) {
            mLocalMedia.add(hm);
        }
    }

    /**
     * add the required columns to get the values that are needed for timeline sorting.
     *
     * @param list
     *            the projection list which will be used in querying MediaStore
     */
    private void addLocalColumnsForTimelineSort(ArrayList<String> list) {
        if (!list.contains(MediaStore.Images.Media.DATE_TAKEN)) {
            list.add(MediaStore.Images.Media.DATE_TAKEN);
        }
        if (!list.contains(MediaStore.Images.Media.DATA)) {
            list.add(MediaStore.Images.Media.DATA);
        }
    }

    private void addLocalColumnsForAlbumSort(ArrayList<String> list) {
        if (!list.contains(MediaStore.Images.Media.BUCKET_DISPLAY_NAME)) {
            list.add(MediaStore.Images.Media.BUCKET_DISPLAY_NAME);
        }
    }
    /**
     * get the wanted local photo or video data by cursor query
     *
     * @param uri
     * @param projection
     * @param selection
     * @param selectionArgs
     * @param sortOrder
     * @return
     */
    private LinkedList<HashMap<String, String>> getLocalAudio(Uri uri, String[] projection, String selection, String[] selectionArgs, int sortOrder) {

        String order = null;
        LinkedList<HashMap<String, String>> local = new LinkedList<HashMap<String, String>>();
        ArrayList<String> validProjectionList = new ArrayList<String>();
        int index = 0;
        Cursor cursor = null;
        boolean needAlbumDuration = false;

        for (String s : projection) {
            if (!CloudMediaColumns.isCloudColumn(s)) {
                validProjectionList.add(s);
            } else {
                if (s.equals(CloudMediaColumns.ALBUM_DURATION)) {
                    needAlbumDuration = true;
                }
            }
        }
        String[] validProjection = (String[]) validProjectionList.toArray(new String[validProjectionList.size()]);
        cursor = mCR.query(uri, validProjection, selection, selectionArgs, order);
        if (cursor != null) {
            try {
                if (cursor.moveToFirst()) {
                    do {
                        HashMap<String, String> hm = new HashMap<String, String>();
                        for (String s : validProjection) {
                            index = cursor.getColumnIndexOrThrow(s);
                            hm.put(s, cursor.getString(index));
                        }
                        local.add(hm);
                    } while (cursor.moveToNext());
                }
            } finally {
                cursor.close();
            }
        }

        if (needAlbumDuration) {
            // get album duration
            String[] myProjection = new String[]{MediaStore.Audio.Media.DURATION};
            for (HashMap<String, String> hm : local) {
                long albumDuration = 0;
                String albumName = hm.get(MediaStore.Audio.Albums.ALBUM);
                if (albumName != null) {
                    cursor = mCR.query(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, myProjection, MediaStore.Audio.Media.ALBUM + " =? ", new String[]{albumName}, null);
                    if (cursor != null) {
                        try {
                            if (cursor.moveToFirst()) {
                                do {
                                    index = cursor.getColumnIndexOrThrow(MediaStore.Audio.Media.DURATION);
                                    long audioDuration = cursor.getLong(index);
                                    albumDuration += audioDuration;
                                } while (cursor.moveToNext());
                                albumDuration = TimeUnit.MILLISECONDS.toSeconds(albumDuration);
                                hm.put(CloudMediaColumns.ALBUM_DURATION, String.valueOf(albumDuration));
                            }
                        } finally {
                            cursor.close();
                        }
                    }
                }
            }
        }
//        else if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_ARTIST == sortOrder) {
//            // TODO: sort by music artist
//        } else if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_GENRE == sortOrder) {
//            // TODO: sort by music genre
//        } else if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_PLAYLIST == sortOrder) {
//            // TODO: sort by music playlist
//        }

        return local;
    }

    /**
     * get the wanted local photo or video data by cursor query
     *
     * @param uri
     * @param projection
     * @param selection
     * @param selectionArgs
     * @param sortOrder
     * @return
     */
    private LinkedList<HashMap<String, String>> getLocalPhoto(Uri uri, String[] projection, String selection, String[] selectionArgs, int sortOrder) {

        String order = null;
        LinkedList<HashMap<String, String>> local = new LinkedList<HashMap<String, String>>();
        int index = 0;
        String bucket_name = null;
        int count_in_bucket = 0;
        ArrayList<String> bucketList = null;

        if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_ALBUM == sortOrder) {
            order = MediaStore.Images.Media.BUCKET_DISPLAY_NAME + " ASC";
            bucketList = new ArrayList<String>();
        }

        Log.i(TAG, "getLocalPhoto, selection: " + selection);
        long localPhotoQueryStart = System.currentTimeMillis();
        Cursor cursor = mCR.query(uri, projection, selection, selectionArgs, order);
        long localPhotoQueryTime = System.currentTimeMillis() - localPhotoQueryStart;
        Log.i(TAG, "localPhotoQueryTime: " + localPhotoQueryTime + ", cursor size: " + cursor.getCount());
        long traverseCursorStart = System.currentTimeMillis();
        if (cursor != null) {
            try {
                if (cursor.moveToFirst()) {
                    do {
                        int dataColumn = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
                        String data = cursor.getString(dataColumn);
                        File f = new File(data);
                        if (f.exists()) {
                            HashMap<String, String> hm = new HashMap<String, String>();
                            for (String s : projection) {
                                index = cursor.getColumnIndexOrThrow(s);
                                hm.put(s, cursor.getString(index));
                                //Log.i(TAG, "[getLocalPhoto()] key: " + s + ", value: " + cursor.getString(index));
                            }
                            if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_TIMELINE == sortOrder) {
                                sortLocalByTimeline(hm, local);
                            } else if (CcdSdkDefines.TIAN_SHAN_SORT_FLAG_SORT_BY_ALBUM == sortOrder) {
                                bucket_name = hm.get(MediaStore.Images.Media.BUCKET_DISPLAY_NAME);
                                if (!bucketList.contains(bucket_name)) {
                                    Cursor cursor1 = mCR.query(uri,
                                                               new String[] {MediaStore.MediaColumns._ID},
                                                               MediaStore.Images.Media.BUCKET_DISPLAY_NAME + " =? ",
                                                               new String[] {bucket_name},
                                                               order);
                                    if (cursor1 != null) {
                                        count_in_bucket = cursor1.getCount();
                                        bucketList.add(bucket_name);
                                        hm.put(CloudMediaColumns.COUNT_IN_ALBUM, Integer.toString(count_in_bucket));
                                        local.add(hm);
                                    }
                                }
                            } else {
                                local.add(hm);
                            }
                        }
                    } while (cursor.moveToNext());
                }
            } finally {
                cursor.close();
            }
        }
        long traverseCursorTime = System.currentTimeMillis() - traverseCursorStart;
        //Log.i(TAG, "traverseCursorTime: " + traverseCursorTime);
        return local;
    }

    private void sortLocalByTimeline(HashMap<String, String> hm, LinkedList<HashMap<String, String>> list) {

        String exifDate = getExifDate(hm.get(MediaStore.Images.Media.DATA));
        if (exifDate != null) {
            hm.put(MediaStore.Images.Media.DATE_TAKEN, exifDate);
        } else {
            // transform the format of date_taken to yyyyMMdd instead of system milliseconds
            long dateTaken = Long.valueOf(hm.get(MediaStore.Images.Media.DATE_TAKEN));
            SimpleDateFormat dateFormat = new SimpleDateFormat("yyyyMMdd", Locale.US);
            hm.put(MediaStore.Images.Media.DATE_TAKEN, dateFormat.format(new Date(dateTaken)));
        }
        int size = list.size();
        boolean alreadyAdd = false;
        for (int i = size - 1; i > -1; i--) {
            if (Long.valueOf(hm.get(MediaStore.Images.Media.DATE_TAKEN)) > Long.valueOf(list.get(i).get(MediaStore.Images.Media.DATE_TAKEN))) {
                list.add(i + 1, hm);
                alreadyAdd = true;
                break;
            }
        }
        if (false == alreadyAdd) {
            list.addFirst(hm);
        }
    }
    /**
     * get thumbnail data for the specific image id list
     * @param id
     * @param idThumbnailList
     * @return
     */
    private void thumbnailDataForImage(Uri mediaUri, String[] id, HashMap<String, String> idThumbnailList) {
        //Log.i(TAG, "id size for thumbnail: " + id.length);
        StringBuilder idList = new StringBuilder();
        Uri uri = null;
        ArrayList<String> idWithoutThumbnail = new ArrayList<String>();

        for (String s : id) {
            idList.append(s).append(',');
        }
        idList.deleteCharAt(idList.length()-1);
        //Log.i(TAG, "id number for thumbnail data: " + id.length);
        if (isImageUri(mediaUri)) {
            uri = MediaStore.Images.Thumbnails.EXTERNAL_CONTENT_URI;
        } else if (isVideoUri(mediaUri)) {
            uri = MediaStore.Video.Thumbnails.EXTERNAL_CONTENT_URI;
        }
//        Log.i(TAG, "thumbnail data for image, idlist: " + idList);
        long start = System.currentTimeMillis();

        Cursor cursor = mCR.query(uri,
                new String[] { MediaStore.Images.Thumbnails.DATA, MediaStore.Images.Thumbnails.IMAGE_ID },
                MediaStore.Images.Thumbnails.IMAGE_ID + " in (" + idList + ")",
                //null,
                null,
                MediaStore.Images.Thumbnails.IMAGE_ID + " ASC");
        long time = System.currentTimeMillis() - start;
        //Log.i(TAG, "time for thumbnail data query: " + time);
        if (cursor != null) {
            try {
                if (cursor.getCount() > 0) {
                    if (cursor.moveToFirst()) {
                        do {
                            int dataColumn = cursor.getColumnIndexOrThrow(MediaStore.Images.Thumbnails.DATA);
                            int idColumn = cursor.getColumnIndex(MediaStore.Images.Thumbnails.IMAGE_ID);
                            long imageId = cursor.getLong(idColumn);
                            String data = cursor.getString(dataColumn);
                            File f = new File(data);
                            if (f.exists()) {
//                                Log.i(TAG, "image id: " + imageId + ", thumbnail data: " + data);
                                idThumbnailList.put(String.valueOf(cursor.getLong(idColumn)), cursor.getString(dataColumn));
                            }
                        } while (cursor.moveToNext());
                    }
                }
            } finally {
                cursor.close();
            }
        }
        for (String s : id) {
            if (!idThumbnailList.containsKey(s)) {
                idWithoutThumbnail.add(s);
            }
        }

        // put original image uri in thumbnail_url for the images which have no thumbnails
        if (idWithoutThumbnail.size() > 0) {
            idList = new StringBuilder();
            for (String s : idWithoutThumbnail) {
                idList.append(s).append(',');
            }
            idList.deleteCharAt(idList.length()-1);
            if (isImageUri(mediaUri)) {
                uri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
            } else if (isVideoUri(mediaUri)) {
                uri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
            }
            cursor = mCR.query(uri,
                    new String[] { MediaStore.Images.Media.DATA, MediaStore.Images.Media._ID },
                    MediaStore.Images.Media._ID + " in (" + idList + ")",
                    null,
                    MediaStore.Images.Media._ID + " ASC");
            if (cursor != null) {
                try {
                    if (cursor.getCount() > 0) {
                        if (cursor.moveToFirst()) {
                            do {
                                int dataColumn = cursor.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
                                int idColumn = cursor.getColumnIndex(MediaStore.Images.Media._ID);
                                idThumbnailList.put(String.valueOf(cursor.getLong(idColumn)), cursor.getString(dataColumn));
                            } while (cursor.moveToNext());
                        }
                    }
                } finally {
                    cursor.close();
                }
            }
        }
    }

    /**
     * get thumbnail data for the specific image id
     *
     * @param cr
     * @param id
     *            the image id whose thumbnail is wanted
     * @return the path where the thumbnail image is
     */
    private String thumbnailDataForImage(String id) {
        String data = null;
        Cursor cursor = mCR.query(MediaStore.Images.Thumbnails.EXTERNAL_CONTENT_URI,
                new String[] { MediaStore.Images.Thumbnails.DATA },
                MediaStore.Images.Thumbnails.IMAGE_ID + " =? ",
                new String[] { id },
                null);
        if (cursor != null) {
            try {
                if ((cursor.getCount() != 0) && (cursor.moveToFirst())) {
                    int index = cursor.getColumnIndexOrThrow(MediaStore.Images.Thumbnails.DATA);
                    data = cursor.getString(index);
                }
            } finally {
                cursor.close();
            }
        }
        return data;
    }

    /**
     * get date_taken from exif
     *
     * @param path the photo path
     * @return date_taken in "yyyyMMdd" format
     */

    private String getExifDate(String path) {
        String dateTakenStr = null;
        String date = null;
        Date dateTaken = null;
//        long millis = -1;
        final DateFormat dateFormat = new SimpleDateFormat("yyyyMMdd");
        try {
            final ExifInterface exif = new ExifInterface(path);
            dateTakenStr = exif.getAttribute(ExifInterface.TAG_DATETIME);

            if (null != dateTakenStr) {
                dateTaken = dateFormat.parse(dateTakenStr);
                date = dateTaken.toString();
                // Calendar calendar = Calendar.getInstance();
                // calendar.setTime(dateTaken);
                // millis = calendar.getTimeInMillis();
            }
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (ParseException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        // return millis;
        return date;
    }

    private class SelectionPair {
        public String key;
        public String[] values;
    }

    private class CloudIdMapping {
        public long idToApp = -1;
        public String cloudId = null;
        public CloudIdMapping(long id, String id_cloud) {
            idToApp = id;
            cloudId = id_cloud;
        }
    }
}
