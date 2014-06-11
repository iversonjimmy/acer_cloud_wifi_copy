/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: ConnectionMeta.java
 *
 *	Revision:
 *
 *	2011-3-31
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.dmr;

import org.cybergarage.upnp.Argument;
import org.cybergarage.upnp.ArgumentList;
import org.cybergarage.upnp.Device;

import com.acer.ccd.upnp.action.CMSAction;
import com.acer.ccd.upnp.device.DeviceAction;
import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;

// TODO: Auto-generated Javadoc
/**
 * The Class ConArgument.
 */
public class ConnectionMeta {

    /** The TAG. */
    private final String TAG = "ConnectionMeta";

    /** The dms uuid. */
    private String dmsUuid;

    /** The dms protocol. */
    private String dmsProtocolList;

    /** The dms connection id. */
    private int dmsConnectionID = -1;

    /** The dms av transport id. */
    private int dmsAVTransportID = -1;

    /** The dmr uuid. */
    private String dmrUuid;

    /** The dmr protocol. */
    private String dmrProtocolList;

    /** The dmr connection id. */
    private int dmrConnectionID = -1;;

    /** The dmr av transport id. */
    private int dmrAVTransportID = -1;

    /** The rcs id. */
    private int dmrRcsID = -1;

    /** The protoco info. */
    private String mRemoteProtocolInfo;

    /**
     * Connection argument.
     */
    public ConnectionMeta() {
    }

    /**
     * Release connection.
     */
    public void clearConnection() {
        releaseCurConnection();
        dmsUuid = DMRTool.spaceSign;

        dmsProtocolList = DMRTool.spaceSign;

        dmsConnectionID = -1;

        dmsAVTransportID = -1;

        dmrUuid = DMRTool.spaceSign;

        dmrProtocolList = DMRTool.spaceSign;

        dmrConnectionID = -1;

        dmrAVTransportID = -1;

        dmrRcsID = -1;

        mRemoteProtocolInfo = DMRTool.spaceSign;
    }

    /**
     * Inits the connection.
     * 
     * @param dmsUuid the dms uuid
     * @param dmrUuid the dmr uuid
     * @return true, if successful
     */
    public boolean initConnection(String dmsUuid, String dmrUuid) {
        
        //clearConnection();
        
        Device dms = DeviceAction.getDMSDevice(dmsUuid);
        Device dmr = DeviceAction.getDMRDevice(dmrUuid);
        if (null == dms || null == dmr) {
            Logger.w(TAG, "uuid can't match device(dmr/dms)");
            setDmsUuid("");
            setDmrUuid("");
            return false;
        }

        setDmsUuid(dmsUuid);
        setDmrUuid(dmrUuid);

        // check device service
        CMSAction dmsAction = new CMSAction(dms);
        CMSAction dmrAction = new CMSAction(dmr);

        // get dms protocol info list
        if (dmsAction.checkAction(Upnp.Service.SERVICE_CMS1, Upnp.CMSAction.CMS_ACTION_GET_PROTOCOL_INFO)) {
            ArgumentList dmsArgList = dmsAction.getProtocolInfo();
            if (null != dmsArgList) {
                Argument dmsArg = dmsArgList
                        .getArgument(Upnp.CMSArgVariable.GetProtocolInfo.CMS_VARIABLE_OUT_SOURCE);
                if (null != dmsArg) {
                    String argSource = dmsArg.getValue();
                    if (null != argSource) {
                        setDmsProtocolList(argSource);
                    }
                    Logger.v(TAG, "initConnection dms protocol list:" + argSource);
                }
            }
        }

        // get dmr protocol info list
        if (dmrAction.checkAction(Upnp.Service.SERVICE_CMS1, Upnp.CMSAction.CMS_ACTION_GET_PROTOCOL_INFO)) {
            Logger.v(TAG, "DMRAction:getProtocolInfo:DMRAction support GetProtocolInfo");

            ArgumentList dmrArgList = dmrAction.getProtocolInfo();
            if (null != dmrArgList) {
                Argument dmrArg = dmrArgList
                        .getArgument(Upnp.CMSArgVariable.GetProtocolInfo.CMS_VARIABLE_OUT_SINK);
                if (null != dmrArg) {
                    String argSink = dmrArg.getValue();
                    argSink = argSink.replaceAll("\t\t\t   ", "");
                    if (null != argSink) {
                        setDmrProtocolList(argSink);
                    }
                    Logger.v(TAG, "initConnection dmr protocol list:" + argSink);
                }
            }
        }
        return true;
    }

    /**
     * Clean cur connection.
     */
    public void releaseCurConnection() {
        Device dms = DeviceAction.getDevice(getDmsUuid());
        if (null != dms && dmsConnectionID > 0) {
            CMSAction action = new CMSAction(dms);
            action.connectionComplete(dmsConnectionID);
        }
        Device dmr = DeviceAction.getDevice(getDmrUuid());
        if (null != dmr && dmrConnectionID > 0) {
            CMSAction action = new CMSAction(dmr);
            action.connectionComplete(dmrConnectionID);
        }

        dmsConnectionID = -1;
        dmsAVTransportID = -1;
        dmrConnectionID = -1;
        dmrAVTransportID = -1;
        dmrRcsID = -1;
        mRemoteProtocolInfo = "";
    }

    /**
     * Gets the dMS remote protocol info.
     * 
     * @param contentProtocolInfo the content protocol info
     * @return the dMS remote protocol info
     */
    private String getDMSRemoteProtocolInfo(String contentProtocolInfo) {
        String remoteProtocolInfo = null;
        if (null == contentProtocolInfo)
            return null;

        String[] contentInfo = contentProtocolInfo.split(DMRTool.cutSign);
        String dmsProtocolInfo = getDmrProtocolList();
        if (null != dmsProtocolInfo && !dmsProtocolInfo.equals(DMRTool.spaceSign)) {
            // compare with the dms protocol info.
            String protocolinfo[] = dmsProtocolInfo.split(DMRTool.commaSign);
            for (int i = 0; i < protocolinfo.length; i++) {
                String[] info = protocolinfo[i].split(DMRTool.cutSign);
                if (info[0].equals(contentInfo[0]) && info[2].equals(contentInfo[2])) {
                    remoteProtocolInfo = protocolinfo[i];
                    break;
                }
            }
        } else {
        }
        return remoteProtocolInfo;
    }

    /**
     * Gets the dMR remote protocol info.
     * 
     * @param contentProtocolInfo the content protocol info
     * @return the dMR remote protocol info
     */
    @SuppressWarnings("unused")
    private String getDMRRemoteProtocolInfo(String contentProtocolInfo) {
        String remoteProtocolInfo = null;
        if (null == contentProtocolInfo)
            return null;

        String[] contentInfo = contentProtocolInfo.split(DMRTool.cutSign);
        String dmrProtocolInfo = getDmrProtocolList();
        if (null != dmrProtocolInfo) {
            Logger.v(TAG, "dmrProtocolInfo !=null");

            // compare with the dmr protocol info.
            String protocolinfo[] = dmrProtocolInfo.split(DMRTool.commaSign);
            for (int i = 0; i < protocolinfo.length; i++) {
                String[] info = protocolinfo[i].split(DMRTool.cutSign);
                if (info[0].equals(contentInfo[0]) && info[2].equals(contentInfo[2])) {
                    remoteProtocolInfo = protocolinfo[i];
                    break;
                }
            }
        } else {
        }
        return remoteProtocolInfo;
    }

    /**
     * Check cur connection.
     * 
     * @param protocolInfo the protocol info
     * @return true, if successful
     */
    public boolean checkCurConnection(String protocolInfo) {
        // check dms connectionid
        boolean bConValid = false;
        int iDmsConnectionId = getDmsConnectionID();
        
        Logger.i(TAG, "checkcur iDmsConnectionid = " + iDmsConnectionId);
        
        if (iDmsConnectionId >= 0) {
            Device dmsDev = DeviceAction.getDMSDevice(getDmsUuid());
            String dmsConnectionId = String.valueOf(iDmsConnectionId);
            if (null == dmsDev || null == dmsConnectionId
                    || dmsConnectionId.equals(DMRTool.spaceSign)) {
                Logger.v(TAG, "checkCurConnection dms(device) is invalid");
                return false;
            }

            CMSAction dmsAction = new CMSAction(dmsDev);
            ArgumentList dmsArgList = dmsAction.getCurrentConnectionIDs();
            if (null == dmsArgList) {
                Logger.w(TAG, "checkCurConnection dms(dmsArgList) is invalid");
                return false;
            }
            Argument dmsArg = dmsArgList
                    .getArgument(Upnp.CMSArgVariable.GetCurrentConnectionIDs.CMS_VARIABLE_OUT_CONNECTION_IDS);
            if (null == dmsArg) {
                if (0 != iDmsConnectionId) {
                    Logger.w(TAG, "checkCurConnection dms(dmsArg) is invalid");
                    return false;
                } else {
                    Logger.i(TAG, "checkCurConnection dms(dmsArg)(iDmsConnectionId) is 0");
                }
            } else {
                String dmsIds = dmsArg.getValue();
                Logger.v(TAG, "checkCurConnection dms(dmsIds) is " + dmsIds);
                if (null == dmsIds) {
                    if (0 != iDmsConnectionId) {
                        Logger.v(TAG, "checkCurConnection dms(dmsIds) is invalid");
                        return false;
                    } else {
                        Logger.i(TAG, "checkCurConnection dms(dmsIds)(iDmsConnectionId) is 0");
                    }
                } else {
                    Logger.v(TAG, "dms connection id list:" + dmsIds);
                    String[] dmsIdList = dmsIds.split(DMRTool.commaSign);
                    bConValid = false;
                    for (int i = 0; i < dmsIdList.length; ++i) {
                        if (dmsConnectionId.equalsIgnoreCase(dmsIdList[i])) {
                            bConValid = true;
                            break;
                        }
                    }
                    if (!bConValid) {
                        Logger.v(TAG, "checkCurConnection dms is invalid");
                        return false;
                    } else {
                        Logger.i(TAG, "checkCurConnection dms(iDmsConnectionId) is 0");
                    }
                }
            }
        } else {
            Logger.v(TAG, "checkCurConnection dms iDmsConnectionId is invalid");
            return false;
        }

        // check dmr connectionid
        int iDmrConnectionId = getDmrConnectionID();
        if (iDmrConnectionId >= 0) {
            Device dmrDev = DeviceAction.getDMRDevice(getDmrUuid());
            String dmrConnectionId = String.valueOf(iDmrConnectionId);
            if (null == dmrDev || null == dmrConnectionId
                    || dmrConnectionId.equals(DMRTool.spaceSign)) {
                Logger.v(TAG, "checkCurConnection dmr(device) is invalid");
                return false;
            }

            CMSAction dmrAction = new CMSAction(dmrDev);
            ArgumentList dmrArgList = dmrAction.getCurrentConnectionIDs();
            if (null == dmrArgList) {
                Logger.i(TAG, "checkCurConnection getCurrentConnectionIDs dmrArgList is invalid");
                return false;
            }

            Argument dmrConIdsArg = dmrArgList
                    .getArgument(Upnp.CMSArgVariable.GetCurrentConnectionIDs.CMS_VARIABLE_OUT_CONNECTION_IDS);
            if (null == dmrConIdsArg) {
                if (0 != iDmrConnectionId) {
                    Logger.i(TAG,
                            "checkCurConnection getCurrentConnectionIDs dmrConIdsArg is invalid");
                    return false;
                } else {
                    Logger.i(TAG, "checkCurConnection dmr(dmrConIdsArg)(iDmrConnectionId) is 0");
                }
            } else {
                String dmrIds = dmrConIdsArg.getValue();
                Logger.v(TAG, "checkCurConnection dmr(dmrIds) is " + dmrIds);
                Logger.v(TAG, "dmr connection id list:" + dmrIds);
                if (null == dmrIds) {
                    if (0 != iDmrConnectionId) {
                        Logger.v(TAG, "checkCurConnection dmr(dmrIds) is invalid");
                        return false;
                    } else {
                        Logger.i(TAG, "checkCurConnection dmr(dmrIds)(iDmrConnectionId) is 0");
                    }
                } else {
                    String[] dmrIdList = dmrIds.split(DMRTool.commaSign);
                    bConValid = false;
                    for (int i = 0; i < dmrIdList.length; ++i) {
                        if (dmrConnectionId.equalsIgnoreCase(dmrIdList[i])) {
                            bConValid = true;
                            break;
                        }
                    }
                    if (!bConValid) {
                        Logger.v(TAG, "checkCurConnection dmr is invalid");
                        return false;
                    } else {
                        Logger.i(TAG, "checkCurConnection dmr(iDmrConnectionId) is 0");
                    }
                }
            }
        } else {
            Logger.v(TAG, "checkCurConnection dmr iDmrConnectionId is invalid");
            return false;
        }

        String curProtocolInfo = getRemoteProtocolInfo();
        String checkedProtocolInfoTmp = getDMSRemoteProtocolInfo(protocolInfo);
        if (null == curProtocolInfo || !curProtocolInfo.equals(checkedProtocolInfoTmp)) {
            Logger.v(TAG, "checkCurConnection curProtocolInfo is invalid (Current protocol:"
                    + curProtocolInfo + ")" + "(Checked protocol:" + checkedProtocolInfoTmp + ")");
            return false;
        }

        return true;
    }

    /**
     * Establish connection.
     * 
     * @param protocolInfo the protocol info
     * @return true, if successful
     */
    public boolean establishConnection(String protocolInfo) {
        Logger.v(TAG, "establishConnection(" + protocolInfo + ")");

        setDmsAVTransportID(-1);
        setDmsConnectionID(-1);
        setDmrAVTransportID(-1);
        setDmrConnectionID(-1);
        setDmrRcsID(-1);

        if (null == protocolInfo) {
            Logger.e(TAG, "establishConnection():" + "The argument is null pointer");
            return false;
        }

        Device dms = DeviceAction.getDMSDevice(getDmsUuid());
        Device dmr = DeviceAction.getDMRDevice(getDmrUuid());
        if (null == dms || null == dmr) {
            Logger.w(TAG, "establishConnection():" + "uuid can't match device(dmr/dms)");
            return false;
        }

        // check device service
        CMSAction dmsAction = new CMSAction(dms);
        CMSAction dmrAction = new CMSAction(dmr);
        
        mRemoteProtocolInfo = getDMSRemoteProtocolInfo(protocolInfo);

        // prepareForConnection in DMS
        if (dmsAction.checkAction(Upnp.Service.SERVICE_CMS1, Upnp.CMSAction.CMS_ACTION_PREPARE_FOR_CONNECTION)) {
            Logger.v(TAG, "establishConnection():" + "DMS support PrepareForConnection");

            // invoke prepareForConnection on dms.
            String peerConnectionManager = dms.getUDN() + DMRTool.slashSign
                    + dms.getService(Upnp.Service.SERVICE_CMS1).getServiceID();

            ArgumentList argList = dmsAction.prepareForConnection(mRemoteProtocolInfo,
                    peerConnectionManager, DMRTool.defaultConnectionID, DMRTool.outDirection);

            if (null != argList) {
                // DMS connectionID
                Argument dmsConArg = argList
                        .getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_CONNECTION_ID);
                if (null != dmsConArg) {
                    String dmsconnectionID = dmsConArg.getValue();
                    if (null != dmsconnectionID) {
                        Logger.v(TAG, "establishConnection():" + "dmsconnectionID:"
                                + dmsconnectionID);
                        try {
                            setDmsConnectionID(Integer.parseInt(dmsconnectionID));
                        } catch (NumberFormatException e) {
                            e.printStackTrace();
                            setDmsConnectionID(0);
                        }
                    } else {
                        Logger.i(TAG, "establishConnection():" + "dmsconnectionID failure");
                        setDmsConnectionID(0);
                    }
                } else {
                    Logger.i(TAG, "establishConnection():" + "dmsArg failure");
                    setDmsConnectionID(0);
                }

                // DMS AVT Instance
                Argument dmsAvt = argList
                        .getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_AVTRANSPORT_ID);
                if (null != dmsAvt) {
                    String dmsavTransportID = dmsAvt.getValue();
                    if (null != dmsavTransportID) {
                        Logger.v(TAG, "establishConnection():" + "dmsavTransportID:"
                                + dmsavTransportID);
                        try {
                            setDmsAVTransportID(Integer.parseInt(dmsavTransportID));
                        } catch (NumberFormatException e) {
                            e.printStackTrace();
                            setDmsAVTransportID(0);
                        }
                    } else {
                        setDmsAVTransportID(0);
                    }
                } else {
                    setDmsAVTransportID(0);
                }
            } else {
                Logger.i(TAG, "establishConnection():" + "prepareForConnection failure");
                setDmsConnectionID(0);
                setDmsAVTransportID(0);
            }
        } else {
            setDmsConnectionID(0);
            setDmsAVTransportID(0);
        }

        // prepareForConnection in DMR
        if (dmrAction.checkAction(Upnp.Service.SERVICE_CMS1, Upnp.CMSAction.CMS_ACTION_PREPARE_FOR_CONNECTION)) {
            // invoke prepareForConnection on dmr.
            Logger.v(TAG, "establishConnection():" + "DMR support PrepareForConnection");

            mRemoteProtocolInfo = getDMSRemoteProtocolInfo(protocolInfo);
            String peerConnectionManager = dmr.getUDN() + DMRTool.slashSign
                    + dmr.getService(Upnp.Service.SERVICE_CMS1).getServiceID();

            ArgumentList dmrArgList = dmrAction.prepareForConnection(mRemoteProtocolInfo,
                    peerConnectionManager, DMRTool.defaultConnectionID, DMRTool.inputDirection);

            if (null != dmrArgList) {
                // DMR Connection ID
                Argument dmrConArg = dmrArgList
                        .getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_CONNECTION_ID);
                if (null != dmrConArg) {
                    String dmrconnectionID = dmrConArg.getValue();
                    if (null != dmrconnectionID) {
                        Logger.v(TAG, "establishConnection():" + "dmrconnectionID:"
                                + dmrconnectionID);
                        try {
                            setDmrConnectionID(Integer.parseInt(dmrconnectionID));
                        } catch (NumberFormatException e) {
                            e.printStackTrace();
                            setDmrConnectionID(0);
                        }
                    } else {
                        setDmrConnectionID(0);
                    }
                } else {
                    setDmrConnectionID(0);
                }

                // DMR AVT ID
                Argument dmrAvtArg = dmrArgList
                        .getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_AVTRANSPORT_ID);
                if (null != dmrAvtArg) {
                    String dmravTransportID = dmrAvtArg.getValue();
                    if (null != dmravTransportID) {
                        Logger.v(TAG, "establishConnection():" + "dmravTransportID:"
                                + dmravTransportID);
                        try {
                            setDmrAVTransportID(Integer.parseInt(dmravTransportID));
                        } catch (NumberFormatException e) {
                            e.printStackTrace();
                            setDmrAVTransportID(0);
                        }
                    } else {
                        setDmrAVTransportID(0);
                    }
                } else {
                    setDmrAVTransportID(0);
                }

                // DMR RCS ID
                Argument dmrRcsArg = dmrArgList
                        .getArgument(Upnp.CMSArgVariable.PrepareForConnection.CMS_VARIABLE_OUT_RCS_ID);
                if (null != dmrRcsArg) {
                    String rcsID = dmrRcsArg.getValue();
                    if (null != rcsID) {
                        Logger.v(TAG, "establishConnection():" + "rcsID:" + rcsID);
                        try {
                            setDmrRcsID(Integer.parseInt(rcsID));
                        } catch (NumberFormatException e) {
                            e.printStackTrace();
                            setDmrRcsID(0);
                        }
                    } else {
                        setDmrRcsID(0);
                    }
                } else {
                    setDmrRcsID(0);
                }
            } else {
                setDmrAVTransportID(0);
                setDmrConnectionID(0);
                setDmrRcsID(0);
            }
        } else {
            setDmrAVTransportID(0);
            setDmrConnectionID(0);
            setDmrRcsID(0);
        }
        
        
        Logger.i(TAG, "create dmsConnectionid = " + getDmsConnectionID());
        
        return true;
    }

    /**
     * Gets the dmr av transport id.
     * 
     * @return the dmr av transport id
     */
    public int getDmrAVTransportID() {
        return dmrAVTransportID;
    }

    /**
     * Gets the dmr connection id.
     * 
     * @return the dmr connection id
     */
    public int getDmrConnectionID() {
        return dmrConnectionID;
    }

    /**
     * Gets the dmr protocol list.
     * 
     * @return the dmr protocol list
     */
    public String getDmrProtocolList() {
        return dmrProtocolList;
    }

    /**
     * Gets the dmr rcs id.
     * 
     * @return the dmr rcs id
     */
    public int getDmrRcsID() {
        return dmrRcsID;
    }

    /**
     * Gets the dmr uuid.
     * 
     * @return the dmr uuid
     */
    public String getDmrUuid() {
        return dmrUuid;
    }

    /**
     * Gets the dms av transport id.
     * 
     * @return the dms av transport id
     */
    public int getDmsAVTransportID() {
        return dmsAVTransportID;
    }

    /**
     * Gets the dms connection id.
     * 
     * @return the dms connection id
     */
    public int getDmsConnectionID() {
        return dmsConnectionID;
    }

    /**
     * Gets the dms protocol list.
     * 
     * @return the dms protocol list
     */
    public String getDmsProtocolList() {
        return dmsProtocolList;
    }

    /**
     * Gets the dms uuid.
     * 
     * @return the dms uuid
     */
    public String getDmsUuid() {
        return dmsUuid;
    }

    /**
     * Gets the remote protocol info.
     * 
     * @return the remote protocol info
     */
    public String getRemoteProtocolInfo() {
        return mRemoteProtocolInfo;
    }

    /**
     * Sets the dmr av transport id.
     * 
     * @param dmrAVTransportID the new dmr av transport id
     */
    public void setDmrAVTransportID(int dmrAVTransportID) {
        this.dmrAVTransportID = dmrAVTransportID;
    }

    /**
     * Sets the dmr connection id.
     * 
     * @param dmrConnectionID the new dmr connection id
     */
    public void setDmrConnectionID(int dmrConnectionID) {
        this.dmrConnectionID = dmrConnectionID;
    }

    /**
     * Sets the dmr protocol list.
     * 
     * @param dmrProtocolList the new dmr protocol list
     */
    public void setDmrProtocolList(String dmrProtocolList) {
        this.dmrProtocolList = dmrProtocolList;
    }

    /**
     * Sets the dmr rcs id.
     * 
     * @param dmrRcsID the new dmr rcs id
     */
    public void setDmrRcsID(int dmrRcsID) {
        this.dmrRcsID = dmrRcsID;
    }

    /**
     * Sets the dmr uuid.
     * 
     * @param dmrUuid the new dmr uuid
     */
    public void setDmrUuid(String dmrUuid) {
        this.dmrUuid = dmrUuid;
    }

    /**
     * Sets the dms av transport id.
     * 
     * @param dmsAVTransportID the new dms av transport id
     */
    public void setDmsAVTransportID(int dmsAVTransportID) {
        this.dmsAVTransportID = dmsAVTransportID;
    }

    /**
     * Sets the dms connection id.
     * 
     * @param dmsConnectionID the new dms connection id
     */
    public void setDmsConnectionID(int dmsConnectionID) {
        this.dmsConnectionID = dmsConnectionID;
    }

    /**
     * Sets the dms protocol list.
     * 
     * @param dmsProtocolList the new dms protocol list
     */
    private void setDmsProtocolList(String dmsProtocolList) {
        this.dmsProtocolList = dmsProtocolList;
    }

    /**
     * Sets the dms uuid.
     * 
     * @param dmsUuid the new dms uuid
     */
    private void setDmsUuid(String dmsUuid) {
        this.dmsUuid = dmsUuid;
    }

    /**
     * Sets the remote protocol info.
     * 
     * @param remoteProtocolInfo the new remote protocol info
     */
    public void setRemoteProtocolInfo(String remoteProtocolInfo) {
        this.mRemoteProtocolInfo = remoteProtocolInfo;
    }

}
