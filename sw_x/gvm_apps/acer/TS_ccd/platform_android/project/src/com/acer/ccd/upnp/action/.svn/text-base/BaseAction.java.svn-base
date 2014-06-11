/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: BaseAction.java
 *
 *	Revision:
 *
 *	2011-3-10
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.action;

import org.cybergarage.upnp.Action;
import org.cybergarage.upnp.Device;

// TODO: Auto-generated Javadoc
/**
 * The Class BaseAction.
 */
public class BaseAction {

    /** The m device. */
    public Device mDevice = null;

    /**
     * Base action.
     * 
     * @param dev the dev
     */
    public BaseAction(Device dev) {
        mDevice = dev;
    }

    /**
     * Check service.
     * 
     * @param serviceName the service name
     * @return true, if successful
     */
    public final boolean checkService(String serviceName) {
        if (null == mDevice) {
            return false;
        }

        org.cybergarage.upnp.Service service = mDevice.getService(serviceName);
        if (null == service) {
            return false;
        }

        service = null;
        return true;
    }

    /**
     * Check action.
     * 
     * @param serviceName the service name
     * @param actionName the action name
     * @return true, if successful
     */
    public final boolean checkAction(String serviceName, String actionName) {
        if (null == mDevice) {
            return false;
        }

        org.cybergarage.upnp.Service service = mDevice.getService(serviceName);
        if (null == service) {
            return false;
        }

        Action action = service.getAction(actionName);
        service = null;
        if (null == action) {
            return false;
        }
        action = null;
        return true;
    }
}
