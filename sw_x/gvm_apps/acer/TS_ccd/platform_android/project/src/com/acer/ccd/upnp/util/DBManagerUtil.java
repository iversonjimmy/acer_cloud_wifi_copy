/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: DBManagerUtil.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.util;

import com.acer.ccd.cache.DBManager;

import android.content.Context;

// TODO: Auto-generated Javadoc
/**
 * The Class DBManagerUtil.
 * 
 * @author chaozhong li
 */
public class DBManagerUtil {

    /** The m db manager. */
    static DBManager mDBManager = null;

    /** The m context. */
    static Context mContext;

    /**
     * Gets the dB manager.
     * 
     * @return the dB manager
     * @throws NullPointerException the null pointer exception
     */
    public static DBManager getDBManager() throws NullPointerException {
        if (null == mContext) {
            throw new NullPointerException("context is null");
        }
        if (null == mDBManager) {
            mDBManager = new DBManager(mContext);
        }
        return mDBManager;
    }

    /**
     * Sets the context.
     * 
     * @param context the new context
     * @throws NullPointerException the null pointer exception
     */
    public static void setContext(Context context) throws NullPointerException {
        if (null == context) {
            throw new NullPointerException();
        }
        mContext = context;
    }
}
