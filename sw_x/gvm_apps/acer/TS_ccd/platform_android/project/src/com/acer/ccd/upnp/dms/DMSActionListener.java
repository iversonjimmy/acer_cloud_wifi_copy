/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: DMSActionListener.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.dms;

// TODO: Auto-generated Javadoc
/**
 * The listener interface for receiving DMSAction events. The class that is
 * interested in processing a DMSAction event implements this interface, and the
 * object created with that class is registered with a component using the
 * component's <code>addDMSActionListener<code> method. When
 * the DMSAction event occurs, that object's appropriate
 * method is invoked.
 * 
 * @author chaozhong li
 */
public interface DMSActionListener {

    /**
     * Dms notify.
     * 
     * @param command the command
     * @param total the total
     * @param tableId the table id
     * @param Uuid the uuid
     * @param errCode the err code
     */
    public void dmsNotify(int command, int total, int tableId, String Uuid, int errCode);
}
