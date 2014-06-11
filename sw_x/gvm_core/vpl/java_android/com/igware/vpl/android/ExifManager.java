package com.igware.vpl.android;

import java.io.IOException;
import android.media.ExifInterface;
import android.os.Bundle;

import android.util.Log;

public class ExifManager
{
    private static final String logTag = "ExifManager";

    private ExifManager() { } 

    /**
     * Note that this function is called from the native code.
     * @return success(0) or fail(-1)
     */
    public static String getDateTimeTag(String filename) {
        Log.i(logTag, "Parsing " + filename);
        try {
            ExifInterface exif = new ExifInterface(filename);
            String tag = exif.getAttribute(ExifInterface.TAG_DATETIME);
            Log.i(logTag, "TAG_DATETIME:" + tag);
            return tag;
        } catch (IOException e) {
            Log.e(logTag, "creating ExifInterface fail");
            return null; // VPL_ERR_IO
        }
    }

}
