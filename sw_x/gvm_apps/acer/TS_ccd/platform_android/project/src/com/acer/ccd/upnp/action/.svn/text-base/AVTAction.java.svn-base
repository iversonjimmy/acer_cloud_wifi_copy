/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: AVTAction.java
 *
 *	Revision:
 *
 *	2011-3-10
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.action;

import org.cybergarage.upnp.Action;
import org.cybergarage.upnp.ArgumentList;
import org.cybergarage.upnp.Device;
import org.cybergarage.upnp.UPnPStatus;

import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;
import com.acer.ccd.upnp.util.UpnpTool;

// TODO: Auto-generated Javadoc
/**
 * The Class AVTAction.
 * 
 * @author acer
 */
public class AVTAction extends BaseAction {

    /** The tag. */
    private final static String TAG = "AVTAction";

    /**
     * Instantiates a new aVT action.
     * 
     * @param dev the dev
     */
    public AVTAction(Device dev) {
        super(dev);
        // TODO Auto-generated constructor stub
    }

    /**
     * Sets the av transport uri.
     * 
     * @param InstanceID the instance id
     * @param currentURI the current uri
     * @param currentURIMetaData the current uri meta data
     * @return true, if successful
     */
    public boolean SetAVTransportURI(int InstanceID, String currentURI, String currentURIMetaData) {
        org.cybergarage.upnp.Service dmrservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmrservice == null) {
            return false;
        }

        Action action = dmrservice.getAction(Upnp.AVTSAction.AVTS_ACTION_SET_AVTRANSPORT_URI);
        if (action == null) {
            return false;
        }

        Logger.v(TAG, "SetAVTransportURI(" + InstanceID + ", " + currentURI + ", "
                + currentURIMetaData + ")");

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.SetAVTransportURI.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        argumentList.getArgument(Upnp.AVTSArgVariable.SetAVTransportURI.AVTS_VARIABLE_IN_CURRENT_URI).setValue(
                currentURI);
        argumentList.getArgument(Upnp.AVTSArgVariable.SetAVTransportURI.AVTS_VARIABLE_IN_CURRENT_URI_META_DATA)
                .setValue(currentURIMetaData);
        
        boolean hi = action.postControlAction();
        
        return hi;

        //return action.postControlAction();
    }

    /**
     * Sets the next av transport uri.
     * 
     * @param InstanceID the instance id
     * @param nextURI the next uri
     * @param nextURIMetaData the next uri meta data
     * @return true, if successful
     */
    public boolean setNextAVTransportURI(int InstanceID, String nextURI, String nextURIMetaData) {
        Logger.v(TAG, "setNextAVTransportURI(" + InstanceID + ", " + nextURI + ", "
                + nextURIMetaData + ")");
        org.cybergarage.upnp.Service dmrservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmrservice == null) {
            return false;
        }

        Action action = dmrservice.getAction(Upnp.AVTSAction.AVTS_ACTION_SET_NEXT_AVTRANSPROT_URI);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.SetNextAVTransportURI.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        argumentList.getArgument(Upnp.AVTSArgVariable.SetNextAVTransportURI.AVTS_VARIABLE_IN_NEXT_URI).setValue(
                nextURI);
        argumentList.getArgument(Upnp.AVTSArgVariable.SetNextAVTransportURI.AVTS_VARIABLE_IN_NEXT_URI_META_DATA)
                .setValue(nextURIMetaData);

        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Gets the media info.
     * 
     * @param InstanceID the instance id
     * @return the media info
     */
    public ArgumentList getMediaInfo(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.GetMediaInfo.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo.AVTS_VARIABLE_OUT_NR_TRACKS);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo.AVTS_VARIABLE_OUT_MEDIA_DURATION);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo.AVTS_VARIABLE_OUT_CURRENT_URI);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo.AVTS_VARIABLE_OUT_CURRENT_URI_META_DATA);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo.AVTS_VARIABLE_OUT_NEXT_URI);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo.AVTS_VARIABLE_OUT_NEXT_URI_META_DATA);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo.AVTS_VARIABLE_OUT_PLAY_MEDIUM);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo.AVTS_VARIABLE_OUT_RECORD_MEDIUM);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo.AVTS_VARIABLE_OUT_WRITE_STATUS);
        return outArgList;
    }

    /**
     * Gets the media info_ ext.
     * 
     * @param InstanceID the instance id
     * @return the media info_ ext
     */
    public ArgumentList getMediaInfo_Ext(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_OUT_CURRENT_TYPE);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_OUT_NR_TRACKS);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_OUT_MEDIA_DURATION);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_OUT_CURRENT_URI);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_OUT_CURRENT_URI_META_DATA);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_OUT_NEXT_URI);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_OUT_NEXT_URI_META_DATA);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_OUT_PLAY_MEDIUM);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_OUT_RECORD_MEDIUM);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_MEDIA_INFO_EXT, outArgList,
                Upnp.AVTSArgVariable.GetMediaInfo_Ext.AVTS_VARIABLE_OUT_WRITE_STATUS);
        return outArgList;
    }

    /**
     * Gets the transport info.
     * 
     * @param InstanceID the instance id
     * @return the transport info
     */
    public ArgumentList getTransportInfo(int InstanceID) {
        Logger.v(TAG, "getTransportInfo(" + InstanceID + ")");

        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_GET_TRANSPORT_INFO);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.GetTransportInfo.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_TRANSPORT_INFO, outArgList,
                Upnp.AVTSArgVariable.GetTransportInfo.AVTS_VARIABLE_OUT_CURRENT_TRANSPORT_STATE);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_TRANSPORT_INFO, outArgList,
                Upnp.AVTSArgVariable.GetTransportInfo.AVTS_VARIABLE_OUT_CURRENT_TRANSPORT_STATUS);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_TRANSPORT_INFO, outArgList,
                Upnp.AVTSArgVariable.GetTransportInfo.AVTS_VARIABLE_OUT_CURRENT_SPEED);
        return outArgList;
    }

    /**
     * Gets the position info.
     * 
     * @param InstanceID the instance id
     * @return the position info
     */
    public ArgumentList getPositionInfo(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_GET_POSITION_INFO);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_POSITION_INFO, outArgList,
                Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_OUT_TRACK);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_POSITION_INFO, outArgList,
                Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_OUT_TRACK_DURATION);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_POSITION_INFO, outArgList,
                Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_OUT_TRACK_META_DATA);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_POSITION_INFO, outArgList,
                Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_OUT_TRACK_URI);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_POSITION_INFO, outArgList,
                Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_OUT_REL_TIME);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_POSITION_INFO, outArgList,
                Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_OUT_ABS_TIME);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_POSITION_INFO, outArgList,
                Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_OUT_REL_COUNT);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_POSITION_INFO, outArgList,
                Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_OUT_ABS_COUNT);
        return outArgList;
    }

    /**
     * Gets the device capabilities.
     * 
     * @param InstanceID the instance id
     * @return the device capabilities
     */
    public ArgumentList getDeviceCapabilities(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_GET_DEVICE_CAPABILITIES);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.GetDeviceCapabilities.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_DEVICE_CAPABILITIES, outArgList,
                Upnp.AVTSArgVariable.GetDeviceCapabilities.AVTS_VARIABLE_OUT_PLAY_MEDIA);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_DEVICE_CAPABILITIES, outArgList,
                Upnp.AVTSArgVariable.GetDeviceCapabilities.AVTS_VARIABLE_OUT_REC_MEDIA);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_DEVICE_CAPABILITIES, outArgList,
                Upnp.AVTSArgVariable.GetDeviceCapabilities.AVTS_VARIABLE_OUT_REC_QUALITY_MODES);
        return outArgList;
    }

    /**
     * Gets the transport settings.
     * 
     * @param InstanceID the instance id
     * @return the transport settings
     */
    public ArgumentList getTransportSettings(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_GET_TRANSPORT_SETTINGS);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.GetTransportSettings.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_TRANSPORT_SETTINGS, outArgList,
                Upnp.AVTSArgVariable.GetTransportSettings.AVTS_VARIABLE_OUT_PLAY_MODE);
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_TRANSPORT_SETTINGS, outArgList,
                Upnp.AVTSArgVariable.GetTransportSettings.AVTS_VARIABLE_OUT_REC_QUALITY_MODE);
        return outArgList;
    }

    /**
     * Stop.
     * 
     * @param InstanceID the instance id
     * @return true, if successful
     */
    public boolean stop(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_STOP);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.Stop.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Play.
     * 
     * @param InstanceID the instance id
     * @param speed the speed
     * @return true, if successful
     */
    public boolean play(int InstanceID, String speed) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_PLAY);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.Play.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        argumentList.getArgument(Upnp.AVTSArgVariable.Play.AVTS_VARIABLE_IN_SPEED).setValue(speed);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Pause.
     * 
     * @param InstanceID the instance id
     * @return true, if successful
     */
    public boolean pause(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_PAUSE);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.Pause.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Record.
     * 
     * @param InstanceID the instance id
     * @return true, if successful
     */
    public boolean record(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_RECORD);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.Record.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Seek.
     * 
     * @param InstanceID the instance id
     * @param unit the unit
     * @param target the target
     * @return true, if successful
     */
    public boolean seek(int InstanceID, String unit, String target) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_SEEK);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.Seek.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        argumentList.getArgument(Upnp.AVTSArgVariable.Seek.AVTS_VARIABLE_IN_UNIT).setValue(unit);
        argumentList.getArgument(Upnp.AVTSArgVariable.Seek.AVTS_VARIABLE_IN_TARGET).setValue(target);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Next.
     * 
     * @param InstanceID the instance id
     * @return true, if successful
     */
    public boolean next(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_NEXT);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.Next.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Previous.
     * 
     * @param InstanceID the instance id
     * @return true, if successful
     */
    public boolean previous(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_PREVIOUS);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.Previous.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Sets the play mode.
     * 
     * @param InstanceID the instance id
     * @param newPlayMode the new play mode
     * @return true, if successful
     */
    public boolean setPlayMode(int InstanceID, String newPlayMode) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_SET_PLAY_MODE);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.SetPlayMode.AVTS_VARIABLE_IN_INSTANCE_ID)
                .setValue(InstanceID);
        argumentList.getArgument(Upnp.AVTSArgVariable.SetPlayMode.AVTS_VARIABLE_IN_NEW_PLAY_MODE).setValue(
                newPlayMode);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Sets the record quality mode.
     * 
     * @param InstanceID the instance id
     * @param newRecordQualityMode the new record quality mode
     * @return true, if successful
     */
    public boolean setRecordQualityMode(int InstanceID, String newRecordQualityMode) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_SET_RECORD_QUALITY_MODE);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.SetRecordQualityMode.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        argumentList.getArgument(Upnp.AVTSArgVariable.SetRecordQualityMode.AVTS_VARIABLE_IN_NEW_RECORD_QUALITY_MODE)
                .setValue(newRecordQualityMode);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Gets the current transport actions.
     * 
     * @param InstanceID the instance id
     * @return the current transport actions
     */
    public ArgumentList getCurrentTransportActions(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_GET_CURRENT_TRANSPORT_ACTIONS);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.GetCurrentTransportActions.AVTS_VARIABLE_IN_INSTANCE_ID)
                .setValue(InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_CURRENT_TRANSPORT_ACTIONS, outArgList,
                Upnp.AVTSArgVariable.GetCurrentTransportActions.AVTS_VARIABLE_OUT_ACTIONS);
        return outArgList;
    }

    /**
     * Gets the dRM state.
     * 
     * @param InstanceID the instance id
     * @return the dRM state
     */
    public ArgumentList getDRMState(int InstanceID) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_GET_DRM_STATE);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.GetDRMState.AVTS_VARIABLE_IN_INSTANCE_ID)
                .setValue(InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_DRM_STATE, outArgList,
                Upnp.AVTSArgVariable.GetDRMState.AVTS_VARIABLE_OUT_CURRENT_DRM_STATE);
        return outArgList;
    }

    /**
     * Gets the state variables.
     * 
     * @param InstanceID the instance id
     * @param stateVariableList the state variable list
     * @return the state variables
     */
    public ArgumentList getStateVariables(int InstanceID, String stateVariableList) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_GET_STATE_VARIABLES);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.GetStateVariables.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        argumentList.getArgument(Upnp.AVTSArgVariable.GetStateVariables.AVTS_VARIABLE_IN_STATE_LIST)
                .setValue(stateVariableList);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_GET_STATE_VARIABLES, outArgList,
                Upnp.AVTSArgVariable.GetStateVariables.AVTS_VARIABLE_OUT_STATE_VALUE_PAIRS);
        return outArgList;
    }

    /**
     * Sets the state variables.
     * 
     * @param InstanceID the instance id
     * @param AVTransportUDN the aV transport udn
     * @param serviceType the service type
     * @param serviceId the service id
     * @param stateVariableValuePairs the state variable value pairs
     * @return the argument list
     */
    public ArgumentList setStateVariables(int InstanceID, String AVTransportUDN,
            String serviceType, String serviceId, String stateVariableValuePairs) {
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_AVTS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.AVTSAction.AVTS_ACTION_SET_STATE_VARIABLES);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.AVTSArgVariable.SetStateVariables.AVTS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        argumentList.getArgument(Upnp.AVTSArgVariable.SetStateVariables.AVTS_VARIABLE_IN_AVTRANSPORT_UDN).setValue(
                AVTransportUDN);
        argumentList.getArgument(Upnp.AVTSArgVariable.SetStateVariables.AVTS_VARIABLE_IN_SERVICE_TYPE).setValue(
                serviceType);
        argumentList.getArgument(Upnp.AVTSArgVariable.SetStateVariables.AVTS_VARIABLE_IN_SERVICE_ID).setValue(
                serviceId);
        argumentList.getArgument(Upnp.AVTSArgVariable.SetStateVariables.AVTS_VARIABLE_IN_STATE_VALUE_PAIRS)
                .setValue(stateVariableValuePairs);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.AVTSAction.AVTS_ACTION_SET_STATE_VARIABLES, outArgList,
                Upnp.AVTSArgVariable.SetStateVariables.AVTS_VARIABLE_OUT_STATE_LIST);
        return outArgList;
    }

}
