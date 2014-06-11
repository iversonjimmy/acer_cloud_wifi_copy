/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: DeviceListener.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.device;

import com.acer.ccd.upnp.common.DlnaListener;

// TODO: Auto-generated Javadoc
/**
 * The listener interface for receiving device events. The class that is
 * interested in processing a device event implements this interface, and the
 * object created with that class is registered with a component using the
 * component's <code>addDeviceListener<code> method. When
 * the device event occurs, that object's appropriate
 * method is invoked.
 * 
 * @author chaozhong li
 */
public interface DeviceListener extends DlnaListener {

    /**
     * Dev notify received.
     * 
     * @param callbackid the callbackid
     * @param total the total
     */
    void devNotifyReceived(int callbackid, int total, int errCode);
}
