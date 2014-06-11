/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: Logger.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.util;

import android.util.Log;

// TODO: Auto-generated Javadoc
/**
 * The Class Logger.
 * 
 * @author chaozhong li
 */
public final class Logger {

    /** The Level. */
    public static int Level = 1;// 1:V 2:D 3:I 4:W 5:E

    /**
     * Instantiates a new logger.
     */
    public Logger() {
    }

    /**
     * V.
     * 
     * @param tag the tag
     * @param msg the msg
     */
    public static final void v(String tag, String msg) {
        if (Level <= 1)
            Log.v(tag, msg);
    }

    /**
     * D.
     * 
     * @param tag the tag
     * @param msg the msg
     */
    public static final void d(String tag, String msg) {
        if (Level <= 2)
            Log.d(tag, msg);
    }

    /**
     * I.
     * 
     * @param tag the tag
     * @param msg the msg
     */
    public static final void i(String tag, String msg) {
        if (Level <= 3)
            Log.i(tag, msg);
    }

    /**
     * W.
     * 
     * @param tag the tag
     * @param msg the msg
     */
    public static final void w(String tag, String msg) {
        if (Level <= 4)
            Log.w(tag, msg);
    }

    /**
     * E.
     * 
     * @param tag the tag
     * @param msg the msg
     */
    public static final void e(String tag, String msg) {
        if (Level <= 5)
            Log.e(tag, msg);
    }
}
