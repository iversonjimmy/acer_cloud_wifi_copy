/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: Device.java
 *
 *	Revision:
 *
 *	2011-3-11
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.device;

import org.cybergarage.upnp.Argument;
import org.cybergarage.upnp.ArgumentList;
import org.cybergarage.upnp.Device;
import org.cybergarage.upnp.Icon;
import org.cybergarage.upnp.IconList;
import org.cybergarage.xml.Node;

import com.acer.ccd.cache.data.DlnaDevice;
import com.acer.ccd.upnp.action.AVTAction;
import com.acer.ccd.upnp.action.CMSAction;
import com.acer.ccd.upnp.dmr.DMRTool;
import com.acer.ccd.upnp.util.CBCommand;
import com.acer.ccd.upnp.util.Dlna;
import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;

/**
 * The Class Device.
 * 
 * @author chaozhong li
 */
public class DeviceTool {

    /** The tag. */
    private static String tag = "DeviceTool";

    /**
     * MediaServer [CDS REQ] [CMS REQ] [AVT OPT] MediaRenderer [RCS REQ] [CMS
     * REQ] [AVT OPT].
     * 
     * @param dev the dev
     * @return the dev type
     */
    /**
     * Gets the dev type.
     * 
     * @param dev the dev
     * @return the dev type
     */
    static public String getDevType(Device dev) {
        if (null == dev) {
            return null;
        }

        boolean bCDS = false;
        boolean bCMS = false;
        boolean bRCS = false;

        if ((null != dev.getService(Upnp.Service.SERVICE_CDS1))
                || (null != dev.getService(Upnp.Service.SERVICE_CDS2))
                || (null != dev.getService(Upnp.Service.SERVICE_CDS3))) {
            bCDS = true;
        }

        if ((null != dev.getService(Upnp.Service.SERVICE_CMS1))
                || (null != dev.getService(Upnp.Service.SERVICE_CMS2))) {
            bCMS = true;
        }

        if ((null != dev.getService(Upnp.Service.SERVICE_AVTS1))
                || (null != dev.getService(Upnp.Service.SERVICE_AVTS2))) {
        }

        if ((null != dev.getService(Upnp.Service.SERVICE_RCS1))
                || (null != dev.getService(Upnp.Service.SERVICE_RCS2))) {
            bRCS = true;
        }

        if (bCDS && bCMS && bRCS) {
            Logger.v(tag, Dlna.DeviceType.DLNA_DEVICE_TYPE_DMSDMR);
            return Dlna.DeviceType.DLNA_DEVICE_TYPE_DMSDMR;
        } else if (bCDS && bCMS) {
            Logger.v(tag, Dlna.DeviceType.DLNA_DEVICE_TYPE_DMS);
            return Dlna.DeviceType.DLNA_DEVICE_TYPE_DMS;
        } else if (bRCS && bCMS) {
            Logger.v(tag, Dlna.DeviceType.DLNA_DEVICE_TYPE_DMR);
            return Dlna.DeviceType.DLNA_DEVICE_TYPE_DMR;
        } else {
            Logger.v(tag, Dlna.DeviceType.DLNA_DEVICE_TYPE_UNKNOWN);
            return Dlna.DeviceType.DLNA_DEVICE_TYPE_UNKNOWN;
        }
    }

    /**
     * Gets the dev icon url.
     * 
     * @param dev the dev
     * @return the dev icon url
     */
    static public String getDevIconUrl(Device dev) {
        if (null == dev) {
            return null;
        }

        Node iconListNode = dev.getDeviceNode().getNode(IconList.ELEM_NAME);
        if (iconListNode == null)
            return null;

        String absoluteURL = "";
        String iconUrl = null;
        int nNode = iconListNode.getNNodes();
        for (int n = 0; n < nNode; n++) {
            Node node = iconListNode.getNode(n);
            if (Icon.isIconNode(node) == false)
                continue;
            Icon icon = new Icon(node);
            absoluteURL = dev.getAbsoluteURL(absoluteURL);
            iconUrl = icon.getURL();
            if(iconUrl.startsWith(("/"))){
                iconUrl = absoluteURL + icon.getURL();
            }
            else{
                iconUrl = absoluteURL + '/' + icon.getURL();
            }
            return iconUrl;
        }
        return null;
    }

    /**
     * Gets the dmr state.
     * 
     * @param uuid the uuid
     * @return the dmr state
     */
    static public int getDmrState(String uuid) {
        Device dev = DeviceAction.getDMRDevice(uuid);
        if (null == dev) {
            return CBCommand.ErrorID.DEVICE_DISAPPEAR;
        }
        // get id l
        CMSAction cmsAction = new CMSAction(dev);
        if (cmsAction == null) {
            return CBCommand.ErrorID.PROTOCOL_ERROR;
        }
        ArgumentList idsList = cmsAction.getCurrentConnectionIDs();
        if (idsList != null) {
            Argument arg = idsList
                    .getArgument(Upnp.CMSArgVariable.GetCurrentConnectionIDs.CMS_VARIABLE_OUT_CONNECTION_IDS);
            if (null != arg) {
                String id = arg.getValue();
                String[] idslst = id.split(DMRTool.commaSign);

                if (idslst.length <= 0) {
                    return CBCommand.ErrorID.PROTOCOL_ERROR;
                } else {
                    AVTAction avtAction = new AVTAction(dev);
                    if (avtAction == null) {
                        return CBCommand.ErrorID.PROTOCOL_ERROR;
                    }

                    try {
                        ArgumentList argList = avtAction.getTransportInfo(Integer
                                .parseInt(idslst[0]));
                        if (argList != null) {
                            Argument avtArg = argList
                                    .getArgument(Upnp.AVTSArgVariable.GetTransportInfo.AVTS_VARIABLE_OUT_CURRENT_TRANSPORT_STATE);
                            if (null != avtArg) {
                                String state = avtArg.getValue();
                                if (null != state) {
                                    if (state
                                            .contains(Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_NO_MEDIA_PRESENT)) {
                                        return Dlna.DmrState.DMR_STATE_NO_MEDIA_PRESENT;
                                    } else if (state
                                            .contains(Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_PAUSED_PLAYBACK)) {
                                        return Dlna.DmrState.DMR_STATE_PAUSED_PLAYBACK;
                                    } else if (state
                                            .contains(Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_PAUSED_RECORDING)) {
                                        return Dlna.DmrState.DMR_STATE_PAUSED_RECORDING;
                                    } else if (state
                                            .contains(Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_PLAYING)) {
                                        return Dlna.DmrState.DMR_STATE_PLAYING;
                                    } else if (state
                                            .contains(Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_RECORDING)) {
                                        return Dlna.DmrState.DMR_STATE_RECORDING;
                                    } else if (state
                                            .contains(Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_STOPPED)) {
                                        return Dlna.DmrState.DMR_STATE_STOPPED;
                                    } else if (state
                                            .contains(Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_TRANSITIONING)) {
                                        return Dlna.DmrState.DMR_STATE_TRANSITIONING;
                                    }
                                }
                            }
                        }
                    } catch (NumberFormatException e) {
                        e.printStackTrace();
                    }
                    return -1;
                }
            } else {
                return CBCommand.ErrorID.PROTOCOL_ERROR;
            }
        } else {
            return CBCommand.ErrorID.PROTOCOL_ERROR;
        }
    }

    /**
     * Gets the dev id.
     * 
     * @param dev the dev
     * @return the dev id
     */
    static public int getDevId(Device dev) {
        if (null == dev) {
            return Dlna.DeviceID.DLNA_DEVICE_ID_NO_ACER;
        }

        String devManufacture = dev.getManufacture();
        String modelName = dev.getModelName();
        if (null == devManufacture) {
            return Dlna.DeviceID.DLNA_DEVICE_ID_NO_ACER;
        }

        if (devManufacture.toUpperCase().contains(Upnp.Common.COMMON_ACER_DEVICE_FLAG) &&
                modelName.equalsIgnoreCase(Upnp.Common.COMMON_ACER_MODLE_NAME)) {
            return Dlna.DeviceID.DLNA_DEVICE_ID_ACER;
        } else if (devManufacture.toUpperCase().contains(Upnp.Common.COMMON_MS_DEVICE_FLAG)) {
            return Dlna.DeviceID.DLNA_DEVICE_ID_MS;
        } else {
            if (modelName.contains(Upnp.Common.COMMON_CLEARFI15_DEVICE_FLAG)) {
                return Dlna.DeviceID.DLNA_DEVICE_ID_CLEARFI15;
            } else if (modelName.contains(Upnp.Common.COMMON_CLEARFI15L_DEVICE_FLAG)) {
                return Dlna.DeviceID.DLNA_DEVICE_ID_CLEARFI15L;
            } else if (modelName.contains(Upnp.Common.COMMON_CLEARFI10_DEVICE_FLAG)) {
                return Dlna.DeviceID.DLNA_DEVICE_ID_CLEARFI10;
            }
        }

        return Dlna.DeviceID.DLNA_DEVICE_ID_NO_ACER;
    }

    /**
     * Prints the db device.
     * 
     * @param tag the tag
     * @param dlnaDeviceDB the dlna device db
     */
    static public void printDBDevice(String tag, DlnaDevice dlnaDeviceDB) {
        if (null == dlnaDeviceDB) {
            return;
        }
        Logger.v(tag, "(DB)dlnaDevice:" + dlnaDeviceDB.getDeviceType());
        Logger.v(tag, "(DB)deviceName:" + dlnaDeviceDB.getDeviceName());
        Logger.v(tag, "(DB)manufacture:" + dlnaDeviceDB.getManufacture());
        Logger.v(tag, "(DB)manufacturerUrl:" + dlnaDeviceDB.getManufacturerUrl());
        Logger.v(tag, "(DB)modelName:" + dlnaDeviceDB.getModelName());
        Logger.v(tag, "(DB)uuid:" + dlnaDeviceDB.getUuid());
        Logger.v(tag, "(DB)iconUrl:" + dlnaDeviceDB.getIconPath());
    }
}
