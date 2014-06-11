//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.acer.dx.util.igware;

import java.io.*;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.preference.PreferenceManager;
import android.util.Log;

import com.acer.dx.R;


public class Utils {

    private static final String LOG_TAG = "Utils";
    
    public static void logException(Exception e)
    {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        e.printStackTrace(pw);
        Log.e(LOG_TAG, sw.toString());
    }

    public static void cacheUserName(String userName, Context context) {

        SharedPreferences prefs = PreferenceManager
                .getDefaultSharedPreferences(context);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putString(Constants.SETTING_USER_NAME, userName);
        editor.commit();
    }

    public static String getCachedUserName(Context context) {

        SharedPreferences prefs = PreferenceManager
                .getDefaultSharedPreferences(context);
        return prefs.getString(Constants.SETTING_USER_NAME, "");
    }

    public static void cacheFirstTimeUser(boolean firstTimeUser, Context context) {

        SharedPreferences prefs = PreferenceManager
                .getDefaultSharedPreferences(context);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putBoolean(Constants.SETTING_IS_FIRST_TIME_USER, firstTimeUser);
        editor.commit();
    }

    public static boolean isFirstTimeUser(Context context) {

        SharedPreferences prefs = PreferenceManager
                .getDefaultSharedPreferences(context);
        return prefs.getBoolean(Constants.SETTING_IS_FIRST_TIME_USER, true);
    }

    public static void broadcastMediaScanIntent(Context context, String fullPath) {

        File file = new File(fullPath);
        String canonicalPath;
        try {
            // Resolve any symbolic links in the file path, because the media
            // scanner does not handle them correctly.
            canonicalPath = file.getCanonicalPath();
        } catch (IOException e) {
            canonicalPath = fullPath;
        }
        Uri uri = Uri.parse("file://" + canonicalPath);
        Log.i(LOG_TAG, "Broadcasting media scan intent: fullpath=" + fullPath
                + ", uri=" + uri);
        Intent mediaScanIntent = new Intent(
                "android.intent.action.MEDIA_SCANNER_SCAN_FILE");
        mediaScanIntent.setData(uri);
        context.sendBroadcast(mediaScanIntent);
    }

    public static boolean containsChars(String source, String target) {

        // Try to find any of the chars from the source string in the target
        // string.
        for (int i = 0; i < source.length(); ++i) {
            char c = source.charAt(i);
            String string = "" + c;
            if (target.contains(string)) {
                return true;
            }
        }
        return false;
    }

    public static boolean deletePath(File path, boolean deleteSelf) {

        boolean success = true;
        if (path.isDirectory()) {
            File[] files = path.listFiles();
            for (int i = 0; i < files.length; i++) {
                if (!deletePath(files[i], true)) {
                    Log.w(LOG_TAG, "Failed to delete " + files[i].getPath());
                    success = false;
                }
            }
        }
        if (deleteSelf) {
            if (!path.delete()) {
                return false;
            }
        }
        return success;
    }
    
    public static boolean isStringEmpty(String str)
    {
    	return (str == null) || (str.trim().length() == 0);
    }
    
    public static boolean isStringNotEmpty(String str)
    {
    	return ! isStringEmpty(str);
    }

    public static String errorCodeToMessage(int errorCode, Context context) {

        switch (errorCode) {
        case -9032:
            return context.getString(R.string.VPL_ERR_UNREACH);
        case -9607:
            return context.getString(R.string.VPL_ERR_INVALID_LOGIN);
        case -14001:
            return context.getString(R.string.CCDI_ERROR_PARAMETER);
        case -14012:
            return context.getString(R.string.CCDI_ERROR_QUERY_TYPE);
        case -14016:
            return context.getString(R.string.CCDI_ERROR_INVALID_SIZE);
        case -14017:
            return context.getString(R.string.CCDI_ERROR_ADD_CACHE_USER);
        case -14018:
            return context.getString(R.string.CCDI_ERROR_REMOVE_CACHE_USER);
        case -14019:
            return context.getString(R.string.CCDI_ERROR_SHUTDOWN_TITLE);
        case -14020:
            return context.getString(R.string.CCDI_ERROR_FAIL);
        case -14021:
            return context.getString(R.string.CCDI_ERROR_AGAIN);
        case -14022:
            return context.getString(R.string.CCDI_ERROR_ADD_EVENT);
        case -14023:
            return context.getString(R.string.CCDI_ERROR_GET_EVENTS);
        case -14024:
            return context.getString(R.string.CCDI_ERROR_NO_CACHED_USERS);
        case -14025:
            return context.getString(R.string.CCDI_ERROR_LOGGED_IN);
        case -14026:
            return context.getString(R.string.CCDI_ERROR_OUT_OF_MEM);
        case -14028:
            return context.getString(R.string.CCDI_ERROR_NOT_GAME);
        case -14029:
            return context.getString(R.string.CCDI_ERROR_COMMIT_SAVE_DATA);
        case -14030:
            return context.getString(R.string.CCDI_ERROR_BAD_RESPONSE);
        case -14031:
            return context.getString(R.string.CCDI_ERROR_GET_SESSION_DATA);
        case -14033:
            return context.getString(R.string.CCDI_ERROR_REMOVE_USER_TITLE);
        case -14047:
            return context.getString(R.string.CCDI_ERROR_FAKE_USER);
        case -14048:
            return context.getString(R.string.CCDI_ERROR_NO_PASSWORD);
        case -14049:
            return context.getString(R.string.CCDI_ERROR_NO_EVENTS);
        case -14050:
            return context.getString(R.string.CCDI_ERROR_ADD_NOTIFICATION);
        case -14051:
            return context.getString(R.string.CCDI_ERROR_ACHIEVE_INDEX);
        case -14052:
            return context.getString(R.string.CCDI_ERROR_SORT_CRITERIA);
        case -14090:
            return context.getString(R.string.CCDI_ERROR_RPC_FAILURE);
        case -14101:
            return context.getString(R.string.CCD_ERROR_PARAMETER);
        case -14105:
            return context.getString(R.string.CCD_ERROR_HTTP_STATUS);
        case -14106:
            return context.getString(R.string.CCD_ERROR_USER_NOT_FOUND);
        case -14107:
            return context.getString(R.string.CCD_ERROR_FOPEN);
        case -14108:
            return context.getString(R.string.CCD_ERROR_MKDIR);
        case -14109:
            return context.getString(R.string.CCD_ERROR_OPENDIR);
        case -14111:
            return context.getString(R.string.CCD_ERROR_PARSE_CONTENT);
        case -14112:
            return context.getString(R.string.CCD_ERROR_SIGNATURE_INVALID);
        case -14113:
            return context.getString(R.string.CCD_ERROR_NOT_IMPLEMENTED);
        case -14115:
            return context.getString(R.string.CCD_ERROR_RENAME);
        case -14116:
            return context.getString(R.string.CCD_ERROR_MKSTEMP);
        case -14117:
            return context.getString(R.string.CCD_ERROR_SIGNED_IN);
        case -14118:
            return context.getString(R.string.CCD_ERROR_NOT_SIGNED_IN);
        case -14119:
            return context.getString(R.string.CCD_ERROR_OFFLINE_MODE);
        case -14121:
            return context.getString(R.string.CCD_ERROR_WRONG_USER_ID);
        case -14122:
            return context.getString(R.string.CCD_ERROR_TITLE_NOT_FOUND);
        case -14123:
            return context.getString(R.string.CCD_ERROR_NO_CACHED_USERS);
        case -14124:
            return context.getString(R.string.CCD_ERROR_ES_FAIL);
        case -14125:
            return context.getString(R.string.CCD_ERROR_ECDK_FAIL);
        case -14126:
            return context.getString(R.string.CCD_ERROR_NOMEM);
        case -14127:
            return context.getString(R.string.CCD_ERROR_CS_FAIL);
        case -14128:
            return context.getString(R.string.CCD_ERROR_WRONG_STATE);
        case -14129:
            return context.getString(R.string.CCD_ERROR_LOCK_FAILED);
        case -14130:
            return context.getString(R.string.CCD_ERROR_SOCKET_RECV);
        case -14131:
            return context.getString(R.string.CCD_ERROR_INTERNAL);
        case -14132:
            return context.getString(R.string.CCD_ERROR_TITLE_EXISTS);
        case -14133:
            return context.getString(R.string.CCD_ERROR_STATVFS);
        case -14135:
            return context.getString(R.string.CCD_ERROR_DISK_SERIALIZE);
        case -14141:
            return context.getString(R.string.CCD_ERROR_NOT_INIT);
        case -14142:
            return context.getString(R.string.CCD_ERROR_ALREADY_INIT);
        case -14143:
            return context.getString(R.string.CCD_ERROR_BAD_SERVER_RESPONSE);
        case -14147:
            return context.getString(R.string.CCD_ERROR_VSDS_FAIL);
        case -14148:
            return context.getString(R.string.CCD_ERROR_NEED_PRIVILEGE);
        case -14149:
            return context.getString(R.string.CCD_ERROR_NEED_PASSWORD);
        case -14150:
            return context.getString(R.string.CCD_ERROR_PASSWORD_NOT_ALLOWED);
        case -14151:
            return context
                    .getString(R.string.CCD_ERROR_SAVE_LOCATION_NOT_FOUND);
        case -14152:
            return context.getString(R.string.CCD_ERROR_OUT_OF_BVS_DEVICES);
        case -14153:
            return context.getString(R.string.CCD_ERROR_TITLE_RUNNING);
        case -14154:
            return context.getString(R.string.CCD_ERROR_ATTEST_FAILED);
        default:
            return String.format(context.getString(R.string.Msg_ErrorCode),
                    errorCode);
        }
    }
}
