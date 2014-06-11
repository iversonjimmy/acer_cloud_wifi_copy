package com.acer.ccd.debug;

import android.app.Activity;
import android.content.Context;
import android.util.DisplayMetrics;

import com.acer.ccd.util.CcdSdkDefines;
import com.acer.ccd.util.Network;

/**
 * Debug utility class and debug related definition
 */
public final class Debug {

    private Debug() { }
    /** Debug build flag should be false in release build */
    public static final boolean DEBUG = true;
    /** Skip those annoying setting dialog at application's start */
    public static final boolean SKIP_SETTING = false;
    /** Add a cache clear menu item in service / media view */
    public static final boolean CACHE_DEBUG = false;
    /** by pass Manufacturer checking */
    public static final boolean ACER_DEVICE_ONLY = true;
    /** Strict mode */
    public static final boolean STRICTMODE = false;
    /** Don't launch DMS at startup */
    public static final boolean NO_DMS = false;
    private static final String TAG = "Debug";

    public static void showDisplay(Context context) {
        DisplayMetrics metrics = new DisplayMetrics();
        ((Activity) context).getWindowManager().getDefaultDisplay().getMetrics(metrics);
        L.t(TAG, "density = %f, dpi = %d, Pixels = (%d x %d), scaledDensity = %f, (x,y)dpi = (%f,%f)",
                metrics.density,
                metrics.densityDpi,
                metrics.heightPixels,
                metrics.widthPixels,
                metrics.scaledDensity,
                metrics.xdpi,
                metrics.ydpi);
    }

    public static void showinfo(Context context) {
        if (DEBUG == false)
            return;

        showDisplay(context);

        // wifi info
        if (Network.isWifiEnabled(context) == true)
            L.t(TAG, "wifi enabled");
        else
            L.t(TAG, "wifi disabled");
    }

    private static long timeCost = 0;
    public static final int TIME_START = 0;
    public static final int TIME_END = 1;
    /**
     * Easy time measure function
     * @param msg if msg is null, it tells timeMeasure to start the time measuring, if not null,
     *    use it as part of measure message.
     */
    public static void timeMeasure(String msg) {
        if (msg == null)
        {
            timeCost = System.nanoTime();
            return;
        }

        if (timeCost == 0)
            return;

        L.t(CcdSdkDefines.PTAG, "%s: %d", msg, (System.nanoTime() - timeCost) / 1000000);
        timeCost = 0;
    }
}
