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

import java.util.ArrayList;



public class CloudMediaColumns{
    public static final String SOURCE = "Source";
    public static final String MEDIA_TYPE = "Media";
    public static final String THUMBNAIL_URL = "ThumbnailUrl";
    public static final String CONTAINER_ID = "ContainerId";
    public static final String COUNT_IN_ALBUM = "count_in_album";
    public static final String ALBUM_DURATION = "album_duration";
    
    private static ArrayList<String> mColumnList;
    
    static {
        mColumnList = new ArrayList<String>();
        mColumnList.add(CONTAINER_ID);
        mColumnList.add(MEDIA_TYPE);
        mColumnList.add(SOURCE);
        mColumnList.add(THUMBNAIL_URL);
        mColumnList.add(COUNT_IN_ALBUM);
        mColumnList.add(ALBUM_DURATION);
    }
    
    public static boolean isCloudColumn(String s) {
        return mColumnList.contains(s);
    }

}
