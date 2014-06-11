/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: ActionThreadCore.java
 *
 *	Revision:
 *
 *	2011-4-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.common;

import org.cybergarage.upnp.ArgumentList;

import com.acer.ccd.upnp.util.Logger;

// TODO: Auto-generated Javadoc
/**
 * The Class ActionThreadCore.
 */
public abstract class ActionThreadCore implements Runnable {

    /** The m thread object. */
    private java.lang.Thread mThreadObject = null;

    /** The argument list. */
    private ArgumentList mArgumentList = null;

    /** The is runnable. */
    boolean isRunnable = false;

    private int token = 0;

    /**
     * Instantiates a new thread core.
     */
    public ActionThreadCore() {
    }

    /**
     * Gets the m argument list.
     * 
     * @return the m argument list
     */
    public ArgumentList getmArgumentList() {
        return mArgumentList;
    }

    /**
     * Gets the token.
     * 
     * @return the token
     */
    public int getToken() {
        return token;
    }

    /**
     * Gets the thread object.
     * 
     * @return the thread object
     */
    public java.lang.Thread getThreadObject() {
        return mThreadObject;
    }

    /**
     * Checks if is runnable.
     * 
     * @return true, if is runnable
     */
    public boolean isRunnable() {
        return isRunnable;
    }

    /**
     * Restart.
     * 
     * @param argumentList the argument list
     */
    public void restart(ArgumentList argumentList) {
        Logger.v("DMSAction", "thread core restart");
        stop();
        start(argumentList);
    }

    /*
     * (non-Javadoc)
     * @see java.lang.Runnable#run()
     */
    abstract public void run();

    /**
     * Sets the m argument list.
     * 
     * @param mArgumentList the new m argument list
     */
    public void setmArgumentList(ArgumentList mArgumentList) {
        this.mArgumentList = mArgumentList;
    }

    public void setToken(int token) {
        this.token = token;
    }

    /**
     * Sets the thread object.
     * 
     * @param obj the new thread object
     */
    public void setThreadObject(java.lang.Thread obj) {
        mThreadObject = obj;
    }

    /**
     * Start.
     * 
     * @param argumentList the argument list
     */
    public void start(ArgumentList argumentList) {
        setToken(0);
        Logger.v("DMSAction", "thread core start");
        isRunnable = true;
        java.lang.Thread threadObject = getThreadObject();
        if (threadObject == null) {
            threadObject = new java.lang.Thread(this, "Clearfi.ActionThreadCore");
            setThreadObject(threadObject);
            setmArgumentList(argumentList);
            threadObject.start();
        }
    }

    /**
     * Stop.
     */
    public void stop() {
        Logger.v("DMSAction", "thread core stop");
        java.lang.Thread threadObject = getThreadObject();
        if (threadObject != null) {
            Logger.v("DMSAction", "Thread-ID-" + threadObject.getId() + ":interrupt");
            threadObject.interrupt();
            setThreadObject(null);
        }
        isRunnable = false;
    }
}
