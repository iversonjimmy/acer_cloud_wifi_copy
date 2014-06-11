/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: DlnaAction.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.common;

import org.cybergarage.util.ListenerList;

/**
 * The Class DlnaAction.
 */
public abstract class DlnaAction {

    /** The dlna listener list. */
    private ListenerList dlnaListenerList = new ListenerList();

    /**
     * Adds the listener.
     * 
     * @param listener the listener
     */
    public void addListener(DlnaListener listener) {
        dlnaListenerList.add(listener);
    }

    /**
     * Removes the listener.
     * 
     * @param listener the listener
     */
    public void removeListener(DlnaListener listener) {
        dlnaListenerList.remove(listener);
    }

    /**
     * Gets the listenerlist.
     * 
     * @return the listenerlist
     */
    public ListenerList getListenerlist() {
        return dlnaListenerList;
    }

    /**
     * Inits the.
     */
    public abstract void init();

    /**
     * Release.
     */
    public abstract void release();
}
