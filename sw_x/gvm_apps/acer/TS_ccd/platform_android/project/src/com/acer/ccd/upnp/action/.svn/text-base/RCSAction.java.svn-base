/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: RCSAction.java
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
 * The Class RCSAction.
 * 
 * @author acer
 */
public class RCSAction extends BaseAction {

    /** The tag. */
    private final static String TAG = "RCSAction";

    /**
     * Instantiates a new rCS action.
     * 
     * @param dev the dev
     */
    public RCSAction(Device dev) {
        super(dev);
        // TODO Auto-generated constructor stub
    }

    /**
     * Gets the brightness.
     * 
     * @param InstanceID the instance id
     * @return the brightness
     */
    public ArgumentList getBrightness(int InstanceID) {
        Logger.v(TAG, "getBrightness(" + InstanceID + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.RCSAction.RCS_ACTION_GET_BRIGHTNESS);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSArgVariable.GetBrightness.RCS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.RCSAction.RCS_ACTION_SET_STATE_VARIABLES, outArgList,
                Upnp.RCSArgVariable.GetBrightness.RCS_VARIABLE_OUT_CURRENT_BRIGHTNESS);
        return outArgList;
    }

    /**
     * Sets the brightness.
     * 
     * @param InstanceID the instance id
     * @param desiredBrightness the desired brightness
     * @return true, if successful
     */
    public boolean setBrightness(int InstanceID, short desiredBrightness) {
        Logger.v(TAG, "setBrightness(" + InstanceID + "," + desiredBrightness + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.RCSAction.RCS_ACTION_SET_BRIGHTNESS);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSArgVariable.SetBrightness.RCS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        argumentList.getArgument(Upnp.RCSArgVariable.SetBrightness.RCS_VARIABLE_IN_DESIRED_BRIGHTNESS).setValue(
                desiredBrightness);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Gets the mute.
     * 
     * @param InstanceID the instance id
     * @param channel the channel
     * @return the mute
     */
    public ArgumentList getMute(int InstanceID, String channel) {
        Logger.v(TAG, "getMute(" + InstanceID + "," + channel + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.RCSAction.RCS_ACTION_GET_MUTE);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSArgVariable.GetMute.RCS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        argumentList.getArgument(Upnp.RCSArgVariable.GetMute.RCS_VARIABLE_IN_CHANNEL).setValue(channel);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.RCSAction.RCS_ACTION_GET_MUTE, outArgList,
                Upnp.RCSArgVariable.GetMute.RCS_VARIABLE_OUT_CURRENT_MUTE);
        return outArgList;
    }

    /**
     * Sets the mute.
     * 
     * @param InstanceID the instance id
     * @param channel the channel
     * @param desiredMute the desired mute
     * @return true, if successful
     */
    public boolean setMute(int InstanceID, String channel, boolean desiredMute) {
        Logger.v(TAG, "setMute(" + InstanceID + "," + channel + "," + desiredMute + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.RCSAction.RCS_ACTION_SET_MUTE);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSArgVariable.SetMute.RCS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        argumentList.getArgument(Upnp.RCSArgVariable.SetMute.RCS_VARIABLE_IN_CHANNEL).setValue(channel);
        argumentList.getArgument(Upnp.RCSArgVariable.SetMute.RCS_VARIABLE_IN_DESIRED_MUTE).setValue(desiredMute);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Gets the volume db range.
     * 
     * @param InstanceID the instance id
     * @param channel the channel
     * @return the volume db range
     */
    public ArgumentList getVolumeDBRange(int InstanceID, String channel) {
        Logger.v(TAG, "getVolumeDBRange(" + InstanceID + "," + channel + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.RCSAction.RCS_ACTION_GET_VOLUME_DB_RANGE);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSArgVariable.GetVolumeDBRange.RCS_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        argumentList.getArgument(Upnp.RCSArgVariable.GetVolumeDBRange.RCS_VARIABLE_IN_CHANNEL).setValue(channel);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.RCSAction.RCS_ACTION_GET_VOLUME_DB_RANGE, outArgList,
                Upnp.RCSArgVariable.GetVolumeDBRange.RCS_VARIABLE_OUT_MIN_VALUE);
        UpnpTool.printArgContent(Upnp.RCSAction.RCS_ACTION_GET_VOLUME_DB_RANGE, outArgList,
                Upnp.RCSArgVariable.GetVolumeDBRange.RCS_VARIABLE_OUT_MAX_VALUE);
        return outArgList;
    }

    /**
     * Gets the volume.
     * 
     * @param InstanceID the instance id
     * @param channel the channel
     * @return the volume
     */
    public ArgumentList getVolume(int InstanceID, String channel) {
        Logger.v(TAG, "getVolume(" + InstanceID + "," + channel + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.RCSAction.RCS_ACTION_GET_VOLUME);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSArgVariable.GetVolume.RCS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        argumentList.getArgument(Upnp.RCSArgVariable.GetVolume.RCS_VARIABLE_IN_CHANNEL).setValue(channel);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.RCSAction.RCS_ACTION_GET_VOLUME, outArgList,
                Upnp.RCSArgVariable.GetVolume.RCS_VARIABLE_OUT_CURRENT_VOLUME);
        return outArgList;
    }

    /**
     * Sets the volume.
     * 
     * @param InstanceID the instance id
     * @param channel the channel
     * @param desiredVolume the desired volume
     * @return true, if successful
     */
    public boolean setVolume(int InstanceID, String channel, int desiredVolume) {
        Logger.v(TAG, "setVolume(" + InstanceID + "," + channel + "," + desiredVolume + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.RCSAction.RCS_ACTION_SET_VOLUME);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSArgVariable.SetVolume.RCS_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        argumentList.getArgument(Upnp.RCSArgVariable.SetVolume.RCS_VARIABLE_IN_CHANNEL).setValue(channel);
        argumentList.getArgument(Upnp.RCSArgVariable.SetVolume.RCS_VARIABLE_IN_DESIRED_VOLUME).setValue(
                desiredVolume);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Gets the zoom for acer.
     * 
     * @param InstanceID the instance id
     * @return the zoom for acer
     */
    public ArgumentList getZoomForAcer(int InstanceID) {
        Logger.v(TAG, "getZoomForAcer(" + InstanceID + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.RCSAcerAction.RCS_ACER_ACTION_GET_ZOOM);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSAcerArgVariable.GetZoom.RCS_ACER_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.RCSAcerAction.RCS_ACER_ACTION_GET_ZOOM, outArgList,
                Upnp.RCSAcerArgVariable.GetZoom.RCS_ACER_VARIABLE_OUT_CURRENT_ZOOM);
        return outArgList;
    }

    /**
     * Sets the zoom for acer.
     * 
     * @param InstanceID the instance id
     * @param desiredZoom the desired zoom
     * @return true, if successful
     */
    public boolean setZoomForAcer(int InstanceID, int desiredZoom) {
        Logger.v(TAG, "setZoomForAcer(" + InstanceID + "," + desiredZoom + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.RCSAcerAction.RCS_ACER_ACTION_SET_ZOOM);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSAcerArgVariable.SetZoom.RCS_ACER_VARIABLE_IN_INSTANCE_ID).setValue(InstanceID);
        argumentList.getArgument(Upnp.RCSAcerArgVariable.SetZoom.RCS_ACER_VARIABLE_IN_DESIRED_ZOOM).setValue(
                desiredZoom);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Gets the rotate for acer.
     * 
     * @param InstanceID the instance id
     * @return the rotate for acer
     */
    public ArgumentList getRotateForAcer(int InstanceID) {
        Logger.v(TAG, "getRotateForAcer(" + InstanceID + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.RCSAcerAction.RCS_ACER_ACTION_GET_ROTATE);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSAcerArgVariable.GetRotate.RCS_ACER_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.RCSAcerAction.RCS_ACER_ACTION_GET_ROTATE, outArgList,
                Upnp.RCSAcerArgVariable.GetRotate.RCS_ACER_VARIABLE_OUT_CURRENT_ROTATE);
        return outArgList;
    }

    /**
     * Sets the rotate for acer.
     * 
     * @param InstanceID the instance id
     * @param desiredRotate the desired rotate
     * @return true, if successful
     */
    public boolean setRotateForAcer(int InstanceID, int desiredRotate) {
        Logger.v(TAG, "setRotateForAcer(" + InstanceID + "," + desiredRotate + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.RCSAcerAction.RCS_ACER_ACTION_SET_ROTATE);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSAcerArgVariable.SetRotate.RCS_ACER_VARIABLE_IN_INSTANCE_ID).setValue(
                InstanceID);
        argumentList.getArgument(Upnp.RCSAcerArgVariable.SetRotate.RCS_ACER_VARIABLE_IN_DESIRED_ROTATE).setValue(
                desiredRotate);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }

    /**
     * Gets the dolby for acer.
     * 
     * @param InstanceID the instance id
     * @return the dolby for acer
     */
    public ArgumentList getDolbyForAcer(int InstanceID) {
        Logger.v(TAG, "getDolbyForAcer(" + InstanceID + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return null;
        }

        Action action = dmsservice.getAction(Upnp.RCSAcerAction.RCS_ACER_ACTION_GET_DOLBY);
        if (action == null) {
            return null;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSAcerArgVariable.GetDolby.RCS_ACER_VARIABLE_IN_INSTANCE_ID)
                .setValue(InstanceID);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return null;
        }

        ArgumentList outArgList = action.getOutputArgumentList();
        UpnpTool.printArgContent(Upnp.RCSAcerAction.RCS_ACER_ACTION_GET_DOLBY, outArgList,
                Upnp.RCSAcerArgVariable.GetDolby.RCS_ACER_VARIABLE_OUT_CURRENT_DOLBY);
        return outArgList;
    }

    /**
     * Sets the dolby for acer.
     * 
     * @param InstanceID the instance id
     * @param desiredDolby the desired dolby
     * @return true, if successful
     */
    public boolean setDolbyForAcer(int InstanceID, int desiredDolby) {
        Logger.v(TAG, "setDolbyForAcer(" + InstanceID + "," + desiredDolby + ")");
        org.cybergarage.upnp.Service dmsservice = mDevice.getService(Upnp.Service.SERVICE_RCS1);
        if (dmsservice == null) {
            return false;
        }

        Action action = dmsservice.getAction(Upnp.RCSAcerAction.RCS_ACER_ACTION_SET_DOLBY);
        if (action == null) {
            return false;
        }

        ArgumentList argumentList = action.getArgumentList();
        argumentList.getArgument(Upnp.RCSAcerArgVariable.SetDolby.RCS_ACER_VARIABLE_IN_INSTANCE_ID)
                .setValue(InstanceID);
        argumentList.getArgument(Upnp.RCSAcerArgVariable.SetDolby.RCS_ACER_VARIABLE_IN_DESIRED_DOLBY).setValue(
                desiredDolby);
        if (!action.postControlAction()) {
            UPnPStatus err = action.getControlStatus();
            Logger.e(TAG, "Error Code = " + err.getCode());
            Logger.e(TAG, "Error Desc = " + err.getDescription());
            return false;
        }
        return true;
    }
}
