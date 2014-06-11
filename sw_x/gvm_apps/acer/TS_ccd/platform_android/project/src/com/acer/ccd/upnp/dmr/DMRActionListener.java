/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: DMRActionListener.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.dmr;

import com.acer.ccd.upnp.common.DlnaListener;

/**
 * The listener interface for receiving DMRAction events. The class that is
 * interested in processing a DMRAction event implements this interface, and the
 * object created with that class is registered with a component using the
 * component's <code>addDMRActionListener<code> method. When
 * the DMRAction event occurs, that object's appropriate
 * method is invoked.
 * 
 * @author chaozhong li
 */
public interface DMRActionListener extends DlnaListener {
    public void dmrNotify(int command, String uuid, int errCode);
}
