/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: CMSAction.java
 *
 *	Revision:
 *
 *	2011-3-10
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.action;

import org.cybergarage.upnp.Action;
import org.cybergarage.upnp.Argument;
import org.cybergarage.upnp.ArgumentList;
import org.cybergarage.upnp.Device;
import org.cybergarage.upnp.UPnPStatus;

import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;

// TODO: Auto-generated Javadoc
/**
 * The Class CMSAction.
 * 
 * @author acer
 */
public class CMSAction extends BaseAction {

    /** The Constant TAG. */
    private final static String TAG = "CMSAction";

    /**
     * Instantiates a new cMS action.
     * 
     * @param dev the dev
     */
    public CMSAction(Device dev) {
        super(dev);
        // TODO Auto-generated constructor stub
    }

    /**
     * Get device protocol info.
     * 
     * @return string list.
     */
    public ArgumentList getProtocolInfo() {
        Logger.v(TAG, "getProtocolInfo()");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CMS1);
        if (null == service) {
            return null;
        }
        Action action = service.getAction(Upnp.CMSAction.CMS_ACTION_GET_PROTOCOL_INFO);
        if (null == action) {
            return null;
        }

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            Argument argSource = outArgList
                    .getArgument(Upnp.CMSArgVariable.GetProtocolInfo.CMS_VARIABLE_OUT_SOURCE);
            Argument argSinke = outArgList.getArgument(Upnp.CMSArgVariable.GetProtocolInfo.CMS_VARIABLE_OUT_SINK);
            try {
                Logger.v(TAG, Upnp.CMSArgVariable.GetProtocolInfo.CMS_VARIABLE_OUT_SOURCE + ":"
                        + argSource.getValue());
                Logger.v(TAG, Upnp.CMSArgVariable.GetProtocolInfo.CMS_VARIABLE_OUT_SINK + ":"
                        + argSinke.getValue());
            } catch (NullPointerException e) {
                e.printStackTrace();
            }

            // parse result
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Prepare dms connectiong to dmr.
     * 
     * @param remoteProtocolInfo the remote protocol info
     * @param peerConnectionManager the peer connection manager
     * @param peerConnectionID the peer connection id
     * @param direction the direction
     * @return connection info if connect successful.
     */
    public ArgumentList prepareForConnection(String remoteProtocolInfo,
            String peerConnectionManager, String peerConnectionID, String direction) {
        Logger.v(TAG, "prepareForConnection(" + remoteProtocolInfo + "," + peerConnectionManager
                + "," + peerConnectionID + "," + direction + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CMS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CMSAction.CMS_ACTION_PREPARE_FOR_CONNECTION);
        if (null == action) {
            return null;
        }
        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_IN_DIRECTION).setValue(
                direction);
        argumentList.getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_IN_PEER_CONNECTION_ID)
                .setValue(peerConnectionID);
        argumentList.getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_IN_PEER_CONNECTION_MANAGER)
                .setValue(peerConnectionManager);
        argumentList.getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_IN_REMOTE_PROTOCOL_INFO)
                .setValue(remoteProtocolInfo);

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            Argument argavTransportID = outArgList
                    .getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_AVTRANSPORT_ID);

            Argument argconnectionID = outArgList
                    .getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_CONNECTION_ID);

            Argument argrcsID = outArgList
                    .getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_RCS_ID);

            Logger.v(TAG, Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_AVTRANSPORT_ID + ":"
                    + argavTransportID);
            Logger.v(TAG, Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_CONNECTION_ID + ":"
                    + argconnectionID);
            Logger.v(TAG, Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_RCS_ID + ":" + argrcsID);

            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * invoked if connection complete.
     * 
     * @param connectionID the connection id
     * @return true, if successful
     */
    public boolean connectionComplete(int connectionID) {
        Logger.v(TAG, "connectionComplete(" + connectionID + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CMS1);
        if (null == service) {
            return false;
        }

        Action action = service.getAction(Upnp.CMSAction.CMS_ACTION_CONNECTION_COMPLETE);
        if (null == action) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CMSArgVariable.ConnectionComplete.CMS_VARIABLE_IN_CONNECTION_ID).setValue(
                connectionID);
        if (action.postControlAction()) {
            // complete successfully
            return true;
        }
        return false;
    }

    /**
     * Get current connection info.
     * 
     * @return integer list.
     */
    public ArgumentList getCurrentConnectionIDs() {
        Logger.v(TAG, "getCurrentConnectionIDs()");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CMS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CMSAction.CMS_ACTION_GET_CURRENT_CONNECTION_IDS);
        if (null == action) {
            return null;
        }

        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            Argument currentConnectionIDs = outArgList
                    .getArgument(Upnp.CMSArgVariable.GetCurrentConnectionIDs.CMS_VARIABLE_OUT_CONNECTION_IDS);
            // currentConnectionIDs.getValue();
            try {
                Logger.v(TAG, Upnp.CMSArgVariable.GetCurrentConnectionIDs.CMS_VARIABLE_OUT_CONNECTION_IDS + ":"
                        + currentConnectionIDs.getValue());
            } catch (NullPointerException e) {
                e.printStackTrace();
            }
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Get current connection info.
     * 
     * @param connectionID the connection id
     * @return connection info object.
     */
    public ArgumentList getCurrenConnectionInfo(int connectionID) {
        Logger.v(TAG, "getCurrenConnectionInfo(" + connectionID + ")");
        org.cybergarage.upnp.Service service = mDevice.getService(Upnp.Service.SERVICE_CMS1);
        if (null == service) {
            return null;
        }

        Action action = service.getAction(Upnp.CMSAction.CMS_ACTION_GET_CURRENT_CONNECTION_INFO);
        if (null == action) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_IN_CONNECTION_ID)
                .setValue(connectionID);
        if (action.postControlAction()) {
            ArgumentList outArgList = action.getOutputArgumentList();
            // too many info...
            printConnInfoOutArg(outArgList);
            return outArgList;
        } else {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }
    }

    /**
     * Prints the conn info out arg.
     * 
     * @param outArgList the out arg list
     */
    private void printConnInfoOutArg(ArgumentList outArgList) {
        Logger.v(TAG, Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_AVTRANSPORT_ID
                + ":"
                + outArgList.getArgument(
                        Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_AVTRANSPORT_ID).getValue());
        Logger.v(TAG, Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_DIRECTION
                + ":"
                + outArgList.getArgument(Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_DIRECTION)
                        .getValue());
        Logger.v(TAG, Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_PEER_CONNECTION_ID
                + ":"
                + outArgList.getArgument(
                        Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_PEER_CONNECTION_ID)
                        .getValue());
        Logger.v(TAG, Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_PEER_CONNECTION_MANAGER
                + ":"
                + outArgList.getArgument(
                        Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_PEER_CONNECTION_MANAGER)
                        .getValue());
        Logger.v(TAG, Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_PROTOCOL_INFO
                + ":"
                + outArgList.getArgument(
                        Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_PROTOCOL_INFO).getValue());
        Logger.v(TAG, Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_RCS_ID
                + ":"
                + outArgList.getArgument(Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_RCS_ID)
                        .getValue());
        Logger.v(TAG, Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_STATUS
                + ":"
                + outArgList.getArgument(Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_STATUS)
                        .getValue());
    }
}
