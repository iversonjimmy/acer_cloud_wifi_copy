/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: DMRAction.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.dmr;

import org.cybergarage.upnp.Argument;
import org.cybergarage.upnp.ArgumentList;
import org.cybergarage.upnp.Device;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import com.acer.ccd.upnp.action.CMSAction;
import com.acer.ccd.upnp.common.ActionThreadCore;
import com.acer.ccd.upnp.common.DlnaAction;
import com.acer.ccd.upnp.device.DeviceAction;
import com.acer.ccd.upnp.device.DeviceTool;
import com.acer.ccd.service.DlnaService;
import com.acer.ccd.upnp.util.CBCommand;
import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;
import com.acer.ccd.upnp.util.UpnpTool;

/**
 * The Class DMRAction.
 * 
 * @author chaozhong li
 */
public class DMRAction extends DlnaAction {

    /** The TAG. */
    private final String TAG = "DMRAction";

    /** The m cur connection info. */
    private ConnectionMeta mCurConnectionInfo;

    /** The m control. */
    MediaBaseControl mControl = null;
    
    private Handler mMultiHandler = null;
    private ArgumentList mAttachArgu = new ArgumentList();
    private ArgumentList mOpenArgu = new ArgumentList();
    private ArgumentList mAddNextUriArgu = new ArgumentList();
    private ArgumentList mGetDmrStatusArgu = new ArgumentList();
    private ArgumentList mNextArgu = new ArgumentList();
    private ArgumentList mPauseArgu = new ArgumentList();
    private ArgumentList mPlayArgu = new ArgumentList();
    private ArgumentList mPrevArgu = new ArgumentList();
    private ArgumentList mRecordArgu = new ArgumentList();
    private ArgumentList mSeekArgu = new ArgumentList();
    private ArgumentList mSetMuteArgu = new ArgumentList();
    private ArgumentList mSetVolumeArgu = new ArgumentList();
    private ArgumentList mStopArgu = new ArgumentList();
    private ArgumentList mPositionArgu = new ArgumentList();
    private String mDmsUuid = null;
    private String mDmrUuid = null;
    
    
    private static final int ATTACH = 0;
    private static final int OPEN = 1;
    private static final int ADD_NEXT_URI = 2;
    private static final int GET_DMR_STATUS = 3;
    private static final int NEXT = 4;
    private static final int PAUSE = 5;
    private static final int PLAY = 6;
    private static final int PREV = 7;
    private static final int RECORD = 8;
    private static final int SEEK = 9;
    private static final int SEEKTOTRACK = 10;
    private static final int SET_MUTE = 11;
    private static final int SET_VOLUME = 12;
    private static final int POSITION = 13;
    private static final int STOP = 14;
    private static final int QUIT_LOOPER = 15;
    
    
    private class MultiThread extends Thread{
        public MultiThread(String name){
            super(name);
        }
        public void run() {
            Looper.prepare();
            mMultiHandler = new Handler() {
                @Override
                public void handleMessage(Message msg) {
                    super.handleMessage(msg);
                    
                    switch(msg.what){
                    case ATTACH:
                        attachDmr();
                        
                        break;
                    
                    case OPEN:
                        open();
                        break;
                        
                    case GET_DMR_STATUS:
                        getDmrStatus();
                        break;
                        
                    case NEXT:
                        next();
                        break;
                        
                    case PAUSE:
                        pause();
                        break;
                        
                    case PLAY:
                        play();
                        break;
                        
                    case PREV:
                        prev();
                        break;
                        
                    case RECORD:
                        record();
                        break;
                        
                    case SEEK:
                        seek();
                        break;
                        
                    case SEEKTOTRACK:
                         seekToTrack();
                         break;

                    case SET_MUTE:
                        setMute();
                        break;
                        
                    case SET_VOLUME:
                        setVolume();
                        break;
                        
                    case POSITION:
                        position();
                        break;
                        
                    case STOP:
                        stopDmr();
                        break;
                        
                    case ADD_NEXT_URI:
                        addNextUri();
                        break;
                        
                    case QUIT_LOOPER:
                        Logger.i(TAG, "quit 1");
                        Looper.myLooper().quit();
                        break;
                        

                        
                    
                    
                    default:
                        break;
                    
                    
                    
                        
                        
                    
                    }
                    
                    
                    
                    
                }
            };
            

            Looper.loop();
        }
    }
    
    public DMRAction(DlnaService service){
        init();
        addListener(service);
    }
    
    
    
    
    
    
    /**
     * The Class AddNextURIActionThreadCore.
     */
    class AddNextURIActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId()
                    + " AddNextURIActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " AddNextURIActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();
            String url = inArgumentList.getArgument(
                    Upnp.AVTSArgVariable.SetNextAVTransportURI.InNextURI).getValue();
            String protocolInfo = inArgumentList.getArgument(
                    Upnp.AVTSArgVariable.SetNextAVTransportURI.InNextURIMetaData).getValue();

            if (null == url || null == protocolInfo) {
                Logger.w(TAG, "addNextURI argument error(null pointer)");
                notifyService(command, null, CBCommand.ErrorID.argumentError);
                return;
            }

            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                Logger.w(TAG, "distant device(dmr) is not a dmr device.");
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }

            if (null == mControl) {
                notifyService(command, null, CBCommand.ErrorID.unInitialized);
                return;
            }
            if (mControl.addNextURI(url, protocolInfo)) {
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m add next uri thread. */
    AddNextURIActionThreadCore mAddNextURIThread = new AddNextURIActionThreadCore();

    /**
     * Adds the next uri.
     * 
     * @param command the command
     * @param url the url
     * @param protocolInfo the protocol info
     */
    public void addNextURI(int command, String url, String protocolInfo) {
        Logger.v(TAG, "addNextURI(" + url + ", " + protocolInfo + ")");

        mAddNextUriArgu.clear();
        UpnpTool.addArg(mAddNextUriArgu, Upnp.Common.COMMON_COMMAND, command);
        UpnpTool.addArg(mAddNextUriArgu, Upnp.AVTSArgVariable.SetNextAVTransportURI.AVTS_VARIABLE_IN_NEXT_URI, url);
        UpnpTool.addArg(mAddNextUriArgu, Upnp.AVTSArgVariable.SetNextAVTransportURI.AVTS_VARIABLE_IN_NEXT_URI_META_DATA,
                protocolInfo);
        
        mMultiHandler.sendEmptyMessage(ADD_NEXT_URI);

        /*
        if (mAddNextURIThread.isRunnable()) {
            mAddNextURIThread.restart(argumentList);
        } else {
            mAddNextURIThread.restart(argumentList);
        }
        */
        
        
        
    }
    
    private void addNextUri(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId()
                + " AddNextURIActionThreadCore");

        
        if (null == mAddNextUriArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " AddNextURIActionThreadCore Null Pointer");
            return;
        }

        int command = mAddNextUriArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();
        String url = mAddNextUriArgu.getArgument(
                Upnp.AVTSArgVariable.SetNextAVTransportURI.AVTS_VARIABLE_IN_NEXT_URI).getValue();
        String protocolInfo = mAddNextUriArgu.getArgument(
                Upnp.AVTSArgVariable.SetNextAVTransportURI.AVTS_VARIABLE_IN_NEXT_URI_META_DATA).getValue();

        if (null == url || null == protocolInfo) {
            Logger.w(TAG, "addNextURI argument error(null pointer)");
            notifyService(command, mDmrUuid, CBCommand.ErrorID.ARGUMENT_ERROR);
            return;
        }

        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            Logger.w(TAG, "distant device(dmr) is not a dmr device.");
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }

        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }
        if (mControl.addNextURI(url, protocolInfo)) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * The Class AttachDMRActionThreadCore.
     */
    class AttachDMRActionThreadCore extends ActionThreadCore {
        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger
                    .v(TAG, "Thread-" + Thread.currentThread().getId()
                            + " AttachDMRActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " AttachDMRActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();
            String dmsUuid = inArgumentList.getArgument(Upnp.Common.dmsUuid).getValue();
            String dmrUuid = inArgumentList.getArgument(Upnp.Common.dmrUuid).getValue();

            if (null == dmsUuid || null == dmrUuid || null == mCurConnectionInfo) {
                notifyService(command, null, CBCommand.ErrorID.argumentError);
                Logger.w(TAG, "initDevProtocolInfo argument error(null pointer)");
                return;
            }


            if (mCurConnectionInfo.initConnection(dmsUuid, dmrUuid)) {
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m add next uri thread. */
    AttachDMRActionThreadCore mAttachDMRActionThreadCore = new AttachDMRActionThreadCore();

    /**
     * Attach dmr.
     * 
     * @param command the command
     * @param dmsUuid the dms uuid
     * @param dmrUuid the dmr uuid
     */
    public void attachDMR(int command, String dmsUuid, String dmrUuid) {
        Logger.v(TAG, "initDevProtocolInfo()");
        mAttachArgu.clear();
        UpnpTool.addArg(mAttachArgu, Upnp.Common.COMMON_COMMAND, command);
        UpnpTool.addArg(mAttachArgu, Upnp.Common.COMMON_DMS_UUID, dmsUuid);
        UpnpTool.addArg(mAttachArgu, Upnp.Common.COMMON_DMR_UUID, dmrUuid);
        
        int max = 0;
        while(mMultiHandler == null && max < 50){
            try{
                Thread.sleep(10);
            }
            catch(Exception e){
                e.printStackTrace();
            }
            
            max ++;
        }
        mMultiHandler.sendEmptyMessage(ATTACH);

        /*
        if (mAttachDMRActionThreadCore.isRunnable()) {
            mAttachDMRActionThreadCore.restart(argumentList);
        } else {
            mAttachDMRActionThreadCore.restart(argumentList);
        }
        */
    }
    
    private void attachDmr(){
        Logger
        .v(TAG, "Thread-" + Thread.currentThread().getId()
                + " AttachDMRActionThreadCore");
    
        if (null == mAttachArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " AttachDMRActionThreadCore Null Pointer");
            return;
        }
        
        int command = mAttachArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();
        mDmsUuid = mAttachArgu.getArgument(Upnp.Common.COMMON_DMS_UUID).getValue();
        mDmrUuid = mAttachArgu.getArgument(Upnp.Common.COMMON_DMR_UUID).getValue();
        
        if (null == mDmsUuid || null == mDmrUuid || null == mCurConnectionInfo) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.ARGUMENT_ERROR);
            Logger.w(TAG, "initDevProtocolInfo argument error(null pointer)");
            return;
        }
        
        
        if (mCurConnectionInfo.initConnection(mDmsUuid, mDmrUuid)) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * Detach dmr.
     * 
     * @param command the command
     */
    public void detachDMR(int command) {
        if (null == mCurConnectionInfo) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }

        Logger.i(TAG, "detachDMR");

        //mCurConnectionInfo.clearConnection();
        notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
    }

    /**
     * Duration.
     * 
     * @return the long
     */
    public long duration() {
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            Logger.w(TAG, "duration() device disappear");
            return -1;
        }
        if (null == mControl) {
            Logger.w(TAG, "duration() Control is not initialized");
            return -1;
        }
        return ((MediaAVControl)mControl).duration();
    }

    /**
     * Gets the current connection info.
     * 
     * @param connectionID the connection id
     * @param devUuid the dev uuid
     * @return the current connection info
     */
    public String[] getCurrentConnectionInfo(int connectionID, String devUuid) {
        if (null == devUuid) {
            return null;
        }

        Device dev = DeviceAction.getDevice(devUuid);
        if (null == dev) {
            return null;
        }
        CMSAction dmsAction = new CMSAction(dev);
        if (!dmsAction.checkService(Upnp.Service.SERVICE_CMS1)
                || !dmsAction
                        .checkAction(Upnp.Service.SERVICE_CMS1, Upnp.CMSAction.CMS_ACTION_GET_CURRENT_CONNECTION_IDS)) {
            return null;
        }

        String[] connectionInfo = {};
        ArgumentList argList = dmsAction.getCurrenConnectionInfo(connectionID);
        if (null == argList) {
            return null;
        }
        Argument avtArg = argList
                .getArgument(Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_AVTRANSPORT_ID);
        if (null != avtArg) {
            connectionInfo[0] = avtArg.getValue();
        } else {
            connectionInfo[0] = "0";
        }

        Argument rcsArg = argList
                .getArgument(Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_RCS_ID);
        if (null != rcsArg) {
            connectionInfo[0] = rcsArg.getValue();
        } else {
            connectionInfo[0] = "0";
        }
        Argument peerConIdArg = argList
                .getArgument(Upnp.CMSArgVariable.GetCurrentConnectionInfo.CMS_VARIABLE_OUT_PEER_CONNECTION_ID);
        if (null != peerConIdArg) {
            connectionInfo[0] = peerConIdArg.getValue();
        } else {
            connectionInfo[0] = "0";
        }

        return connectionInfo;
    }

    /**
     * Gets the volume.
     * 
     * @return the volume
     */
    public int getCurVolume() {
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            return -1;
        }

        if (null == mControl) {
            return -1;
        }
        return mControl.getVolume();
    }

    /**
     * Gets the max volume.
     * 
     * @return the max volume
     */
    public int getMaxVolume() {
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            return -1;
        }

        if (null == mControl) {
            return -1;
        }
        return mControl.getMaxVolume();
    }

    /**
     * Gets the min volume.
     * 
     * @return the min volume
     */
    public int getMinVolume() {
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            return -1;
        }

        if (null == mControl) {
            return -1;
        }
        return mControl.getMinVolume();
    }

    /**
     * Gets the mute.
     * 
     * @return the mute
     */
    public boolean getMute() {
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            return false;
        }

        if (null == mControl) {
            return false;
        }
        return mControl.getMute();
    }

    /*
     * (non-Javadoc)
     * @see com.acer.clearfi.common.DlnaAction#init()
     */
    @Override
    public void init() {

        Logger.v(TAG, "init()");
        if (mCurConnectionInfo == null) {
            mCurConnectionInfo = new ConnectionMeta();
        }
        
        MultiThread multiThread = new MultiThread("multiple thread");
        multiThread.start();
    }

    /**
     * Checks if is playing.
     * 
     * @return true, if is playing
     */
    public boolean isPlaying() {
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            return false;
        }

        if (null == mControl) {
            return false;
        }
        try {
            return ((MediaAVControl)mControl).isPlaying();
        } catch (ClassCastException e) {
            Logger.e(TAG, "isPlaying() ClassCastException eception, return fail");
            return false;
        }
    }

    /**
     * The Class NextActionThreadCore.
     */
    class NextActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " NextActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " NextActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();

            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }

            if (null == mControl) {
                notifyService(command, null, CBCommand.ErrorID.unInitialized);
                return;
            }

            if (mControl.next()) {
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m add next uri thread. */
    NextActionThreadCore mNextActionThreadCore = new NextActionThreadCore();

    /**
     * Next.
     * 
     * @param command the command
     */
    public void next(int command) {
        mNextArgu.clear();
        UpnpTool.addArg(mNextArgu, Upnp.Common.COMMON_COMMAND, command);
        
        mMultiHandler.sendEmptyMessage(NEXT);

        /*
        if (mNextActionThreadCore.isRunnable()) {
            mNextActionThreadCore.restart(argumentList);
        } else {
            mNextActionThreadCore.restart(argumentList);
        }
        */
    }
    
    private void next(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " NextActionThreadCore");

        
        if (null == mNextArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " NextActionThreadCore Null Pointer");
            return;
        }

        int command = mNextArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();

        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }

        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }

        if (mControl.next()) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * Notify service.
     * 
     * @param command the command
     * @param uuid the uuid
     * @param errCode the err code
     */
    private void notifyService(int command, String uuid, int errCode) {
        int listenerSize = getListenerlist().size();
        for (int n = 0; n < listenerSize; n++) {
            DMRActionListener listener = (DMRActionListener)getListenerlist().get(n);
            listener.dmrNotify(command, uuid, errCode);
        }
    }

    /**
     * The Class OpenActionThreadCore.
     */
    class OpenActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " OpenActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " OpenActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();
            String url = inArgumentList.getArgument(
                    Upnp.AVTSArgVariable.SetAVTransportURI.InCurrentURI).getValue();
            String protocolInfo = inArgumentList.getArgument(
                    Upnp.AVTSArgVariable.SetAVTransportURI.InCurrentURIMetaData).getValue();

            if (null == url || null == protocolInfo) {
                Logger.w(TAG, "open argument error(null pointer)");
                notifyService(command, null, CBCommand.ErrorID.argumentError);
                return;
            }

            Logger.i(TAG, "dmsconnectionid = " + mCurConnectionInfo.getDmsConnectionID());
            
            if (mCurConnectionInfo.checkCurConnection(protocolInfo)) {
                Logger.i(TAG, "with the old connection");
                // case one: use the existed connection channel
                if (null == mControl) {
                    Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
                    if (device == null) {
                        Logger.w(TAG, "distant device(dmr) is not a dmr device.");
                        notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                        return;
                    }
                    if (protocolInfo.contains(DMRTool.MediaType.audio)) {
                        mControl = new MediaAudioControl(device, mCurConnectionInfo
                                .getDmrAVTransportID());
                    } else if (protocolInfo.contains(DMRTool.MediaType.video)) {
                        mControl = new MediaVideoControl(device, mCurConnectionInfo
                                .getDmrAVTransportID());
                    } else if (protocolInfo.contains(DMRTool.MediaType.image)) {
                        mControl = new MediaImageControl(device, mCurConnectionInfo
                                .getDmrAVTransportID());
                    } else {
                        notifyService(command, null, CBCommand.ErrorID.argumentError);
                        return;
                    }
                }
                
                if((protocolInfo.contains(DMRTool.MediaType.audio) ||
                		protocolInfo.contains(DMRTool.MediaType.video)) &&
                		command != CBCommand.DMRActionID.record){
                	String state = mControl.getCurrentState();

                    if (!Upnp.AVTSArgVariable.stateVariable.TransportState.STOPPED.equalsIgnoreCase(state)) {
                        if (!mControl.stop()) {
                            notifyService(command, null, CBCommand.ErrorID.protocolError);
                            return;
                        }

                        int imax = 50;
                        while (!Upnp.AVTSArgVariable.stateVariable.TransportState.STOPPED
                                .equalsIgnoreCase(state)
                                && imax > 0) {
                            state = mControl.getCurrentState();
                            try {
                                Thread.sleep(10);
                            } catch (InterruptedException e) {
                                // TODO Auto-generated catch block
                                e.printStackTrace();
                                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                                return;
                            }

                            imax--;
                        }

                        if (imax < 0) {
                            notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                            return;
                        }
                    }
                }
                
                boolean openSuccess = mControl.open(url, protocolInfo);
                Logger.i(TAG, "Thread-" + Thread.currentThread().getId() + ", openSuccess = " + openSuccess);
                
                if (openSuccess) {
                    notifyService(command, null, CBCommand.ErrorID.ok);
                } else {
                    notifyService(command, null, CBCommand.ErrorID.protocolError);
                }
            } else {
                Logger.i(TAG, "build  the new connection");
                // case two: build the new connection channel
                // 1. close old connection
                mCurConnectionInfo.releaseCurConnection();

                // 2. prepare the new connection
                mCurConnectionInfo.establishConnection(protocolInfo);

                Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
                if (device == null) {
                    Logger.w(TAG, "distant device(dmr) is not a dmr device.");
                    notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                    return;
                }
                if (protocolInfo.contains(DMRTool.MediaType.audio)) {
                    mControl = new MediaAudioControl(device, mCurConnectionInfo
                            .getDmrAVTransportID());
                } else if (protocolInfo.contains(DMRTool.MediaType.video)) {
                    mControl = new MediaVideoControl(device, mCurConnectionInfo
                            .getDmrAVTransportID());
                } else if (protocolInfo.contains(DMRTool.MediaType.image)) {
                    mControl = new MediaImageControl(device, mCurConnectionInfo
                            .getDmrAVTransportID());
                } else {
                    notifyService(command, null, CBCommand.ErrorID.argumentError);
                    return;
                }

                if (mControl.open(url, protocolInfo)) {
                    notifyService(command, null, CBCommand.ErrorID.ok);
                } else {
                    notifyService(command, null, CBCommand.ErrorID.protocolError);
                }
            }
            */
            
        }
    }

    /** The m add next uri thread. */
    OpenActionThreadCore mOpenActionThreadCore = new OpenActionThreadCore();

    /**
     * Open.
     * 
     * @param command the command
     * @param url the url
     * @param protocolInfo the protocol info
     */
    public void open(int command, String url, String uriMetaData) {
        Logger.v(TAG, "open(" + url + ", " + uriMetaData + ")");
        mOpenArgu.clear();
        UpnpTool.addArg(mOpenArgu, Upnp.Common.COMMON_COMMAND, command);
        UpnpTool.addArg(mOpenArgu, Upnp.AVTSArgVariable.SetAVTransportURI.AVTS_VARIABLE_IN_CURRENT_URI, url);
        UpnpTool.addArg(mOpenArgu, Upnp.AVTSArgVariable.SetAVTransportURI.AVTS_VARIABLE_IN_CURRENT_URI_META_DATA,
        		uriMetaData);
        
        mMultiHandler.sendEmptyMessage(OPEN);

        /*
        if (mOpenActionThreadCore.isRunnable()) {
            mOpenActionThreadCore.restart(argumentList);
        } else {
            mOpenActionThreadCore.restart(argumentList);
        }
        */
    }
    
    private void open(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " OpenActionThreadCore");

        if (null == mOpenArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " OpenActionThreadCore Null Pointer");
            return;
        }

        int command = mOpenArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();
        String url = mOpenArgu.getArgument(
                Upnp.AVTSArgVariable.SetAVTransportURI.AVTS_VARIABLE_IN_CURRENT_URI).getValue();
        String uriMetaData = mOpenArgu.getArgument(
                Upnp.AVTSArgVariable.SetAVTransportURI.AVTS_VARIABLE_IN_CURRENT_URI_META_DATA).getValue();
        String protocolInfo = uriMetaData;
        if (null == url || null == uriMetaData) {
            Logger.w(TAG, "open argument error(null pointer)");
            notifyService(command, mDmrUuid, CBCommand.ErrorID.ARGUMENT_ERROR);
            return;
        }

        Logger.i(TAG, "dmsconnectionid = " + mCurConnectionInfo.getDmsConnectionID());
        
        if(/*(protocolInfo.contains(DMRTool.MediaType.audio) ||
                protocolInfo.contains(DMRTool.MediaType.video)) &&*/
                command != CBCommand.DMRActionID.RECORD &&
                mControl != null){
            String state = mControl.getCurrentState();

            if (!Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_STOPPED.equalsIgnoreCase(state)) {
                if (!mControl.stop()) {
                    notifyService(command, null, CBCommand.ErrorID.PROTOCOL_ERROR);
                    return;
                }

                int imax = 50;
                while (!Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_STOPPED
                        .equalsIgnoreCase(state)
                        && imax > 0) {
                    state = mControl.getCurrentState();
                    try {
                        Thread.sleep(10);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                        notifyService(command, null, CBCommand.ErrorID.DEVICE_DISAPPEAR);
                        return;
                    }

                    imax--;
                }

                if (imax < 0) {
                    notifyService(command, null, CBCommand.ErrorID.DEVICE_DISAPPEAR);
                    return;
                }
            }
        }
        
        if (mCurConnectionInfo.checkCurConnection(protocolInfo)) {
            Logger.i(TAG, "with the old connection");
            // case one: use the existed connection channel
            if (null == mControl) {
                Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
                if (device == null) {
                    Logger.w(TAG, "distant device(dmr) is not a dmr device.");
                    notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
                    return;
                }
                if (protocolInfo.contains(DMRTool.MediaType.audio)) {
                    mControl = new MediaAudioControl(device, mCurConnectionInfo
                            .getDmrAVTransportID());
                } else if (protocolInfo.contains(DMRTool.MediaType.video)) {
                    mControl = new MediaVideoControl(device, mCurConnectionInfo
                            .getDmrAVTransportID());
                } else if (protocolInfo.contains(DMRTool.MediaType.image)) {
                    mControl = new MediaImageControl(device, mCurConnectionInfo
                            .getDmrAVTransportID());
                } else {
                    notifyService(command, mDmrUuid, CBCommand.ErrorID.ARGUMENT_ERROR);
                    return;
                }
            }

            boolean openSuccess = mControl.open(url, uriMetaData);
            Logger.i(TAG, "Thread-" + Thread.currentThread().getId() + ", openSuccess = " + openSuccess);
            
            if (openSuccess) {
                notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
            } else {
                notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
            }
        } else {
            Logger.i(TAG, "build  the new connection");
            // case two: build the new connection channel
            // 1. close old connection
            mCurConnectionInfo.releaseCurConnection();

            // 2. prepare the new connection
            mCurConnectionInfo.establishConnection(protocolInfo);

            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                Logger.w(TAG, "distant device(dmr) is not a dmr device.");
                notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
                return;
            }
            if (protocolInfo.contains(DMRTool.MediaType.audio)) {
                mControl = new MediaAudioControl(device, mCurConnectionInfo
                        .getDmrAVTransportID());
            } else if (protocolInfo.contains(DMRTool.MediaType.video)) {
                mControl = new MediaVideoControl(device, mCurConnectionInfo
                        .getDmrAVTransportID());
            } else if (protocolInfo.contains(DMRTool.MediaType.image)) {
                mControl = new MediaImageControl(device, mCurConnectionInfo
                        .getDmrAVTransportID());
            } else {
                notifyService(command, mDmrUuid, CBCommand.ErrorID.ARGUMENT_ERROR);
                return;
            }

            if (mControl.open(url, uriMetaData)) {
                notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
            } else {
                notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
            }
        }
    }

    /**
     * The Class PauseActionThreadCore.
     */
    class PauseActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " PauseActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " PauseActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();

            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }

            if (null == mControl) {
                notifyService(command, null, CBCommand.ErrorID.unInitialized);
                return;
            }

            if (mControl.pause()) {
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m add next uri thread. */
    PauseActionThreadCore mPauseActionThreadCore = new PauseActionThreadCore();

    /**
     * Pause.
     * 
     * @param command the command
     */
    public void pause(int command) {
        mPauseArgu.clear();
        UpnpTool.addArg(mPauseArgu, Upnp.Common.COMMON_COMMAND, command);
        mMultiHandler.sendEmptyMessage(PAUSE);

        /*
        if (mPauseActionThreadCore.isRunnable()) {
            mPauseActionThreadCore.restart(argumentList);
        } else {
            mPauseActionThreadCore.restart(argumentList);
        }
        */
    }
    
    private void pause(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " PauseActionThreadCore");

        
        if (null == mPauseArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " PauseActionThreadCore Null Pointer");
            return;
        }

        int command = mPauseArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();

        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }

        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }

        if (mControl.pause()) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * The Class PlayActionThreadCore.
     */
    class PlayActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*

            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " PlayActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " PlayActionThreadCore Null Pointer");
                return;
            }
            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();
            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }
            if (null == mControl) {
                notifyService(command, null, CBCommand.ErrorID.unInitialized);
                return;
            }

            String state = mControl.getCurrentState();

            if (!Upnp.AVTSArgVariable.stateVariable.TransportState.STOPPED.equalsIgnoreCase(state) &&
            		!Upnp.AVTSArgVariable.stateVariable.TransportState.PAUSED_PLAYBACK.equalsIgnoreCase(state)) {
                if (!mControl.stop()) {
                    notifyService(command, null, CBCommand.ErrorID.protocolError);
                    return;
                }

                int imax = 50;
                while (!Upnp.AVTSArgVariable.stateVariable.TransportState.STOPPED
                        .equalsIgnoreCase(state)
                        && imax > 0) {
                    state = mControl.getCurrentState();
                    try {
                        Thread.sleep(10);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                        notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                        return;
                    }

                    imax--;
                }

                if (imax < 0) {
                    notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                    return;
                }
            }

            
            if (mControl.play()) {
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m add next uri thread. */
    PlayActionThreadCore mPlayActionThreadCore = new PlayActionThreadCore();

    /**
     * Play.
     * 
     * @param command the command
     */
    public void play(int command) {
        mPlayArgu.clear();
        UpnpTool.addArg(mPlayArgu, Upnp.Common.COMMON_COMMAND, command);
        
        mMultiHandler.sendEmptyMessage(PLAY);

        /*
        if (mPlayActionThreadCore.isRunnable()) {
            mPlayActionThreadCore.restart(argumentList);
        } else {
            mPlayActionThreadCore.restart(argumentList);
        }
        */

    }
    
    private void play(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " PlayActionThreadCore");


        if (null == mPlayArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " PlayActionThreadCore Null Pointer");
            return;
        }
        int command = mPlayArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }
        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }
        /*
        String state = mControl.getCurrentState();

        if (!Upnp.AVTSArgVariable.stateVariable.TransportState.STOPPED.equalsIgnoreCase(state) &&
                !Upnp.AVTSArgVariable.stateVariable.TransportState.PAUSED_PLAYBACK.equalsIgnoreCase(state)) {
            if (!mControl.stop()) {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
                return;
            }

            int imax = 50;
            while (!Upnp.AVTSArgVariable.stateVariable.TransportState.STOPPED
                    .equalsIgnoreCase(state)
                    && imax > 0) {
                state = mControl.getCurrentState();
                try {
                    Thread.sleep(10);
                } catch (InterruptedException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                    notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                    return;
                }

                imax--;
            }

            if (imax < 0) {
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }
        }
        */

        
        if (mControl.play()) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * Position.
     * 
     * @return the long
     */
    public void position(int command) {
        mPositionArgu.clear();
        UpnpTool.addArg(mPositionArgu, Upnp.Common.COMMON_COMMAND, command);
        mMultiHandler.sendEmptyMessage(POSITION);
    }
    
    private void position() {
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        int command = mPositionArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();
        if (device == null) {
            Logger.w(TAG, "position() device disappear");
            notifyService(command, mDmrUuid, -1);
            return;
        }
        if (null == mControl) {
            Logger.w(TAG, "position() control is not initialized");
            notifyService(command, mDmrUuid, -1);
            return;
        }
        
        try {
            notifyService(command, mDmrUuid, (int)(((MediaAVControl)mControl).position()));
        } catch (ClassCastException e) {
            Logger.e(TAG, "position() ClassCastException eception, return fail");
            notifyService(command, mDmrUuid, -1);
        }
    }

    /**
     * The Class PrevActionThreadCore.
     */
    class PrevActionThreadCore extends ActionThreadCore {
        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " PrevActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " PrevActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();

            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }

            if (null == mControl) {
                notifyService(command, null, CBCommand.ErrorID.unInitialized);
                return;
            }
            if (mControl.prev()) {
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m prev action thread core. */
    PrevActionThreadCore mPrevActionThreadCore = new PrevActionThreadCore();

    /**
     * Prev.
     * 
     * @param command the command
     */
    public void prev(int command) {
        mPrevArgu.clear();
        UpnpTool.addArg(mPrevArgu, Upnp.Common.COMMON_COMMAND, command);
        mMultiHandler.sendEmptyMessage(PREV);

        /*
        if (mPrevActionThreadCore.isRunnable()) {
            mPrevActionThreadCore.restart(argumentList);
        } else {
            mPrevActionThreadCore.restart(argumentList);
        }
        */
    }
    
    private void prev(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " PrevActionThreadCore");

        
        if (null == mPrevArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " PrevActionThreadCore Null Pointer");
            return;
        }

        int command = mPrevArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();

        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }

        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }
        if (mControl.prev()) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /*
     * (non-Javadoc)
     * @see com.acer.clearfi.common.DlnaAction#release()
     */
    @Override
    public void release() {
        mMultiHandler.sendEmptyMessage(QUIT_LOOPER);
        mCurConnectionInfo = null;
    }

    /**
     * The Class SeekToActionThreadCore.
     */
    class SeekToActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " SeekToActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " SeekToActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();
            int position = inArgumentList.getArgument(Upnp.AVTSArgVariable.Seek.InTarget)
                    .getIntegerValue();
            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }
            if (null == mControl) {
                notifyService(command, null, CBCommand.ErrorID.unInitialized);
                return;
            }
            if (((MediaAVControl)mControl).seekTo(position)) {
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m add next uri thread. */
    SeekToActionThreadCore mSeekToActionThreadCore = new SeekToActionThreadCore();

    /**
     * Seek to track.
     * 
     * @param command the command
     * @param position the track
     */
    public void seekToTrack(int command, int track) {
        mSeekArgu.clear();
        UpnpTool.addArg(mSeekArgu, Upnp.Common.COMMON_COMMAND, command);
        UpnpTool.addArg(mSeekArgu, Upnp.AVTSArgVariable.Seek.AVTS_VARIABLE_IN_TARGET, track);
        mMultiHandler.sendEmptyMessage(SEEKTOTRACK);
    }
    
    private void seekToTrack(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " SeekToTrackActionThreadCore");

        
        if (null == mSeekArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " SeekToTrackActionThreadCore Null Pointer");
            return;
        }

        int command = mSeekArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();
        int track = mSeekArgu.getArgument(Upnp.AVTSArgVariable.Seek.AVTS_VARIABLE_IN_TARGET)
                .getIntegerValue();
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }
        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }
        if (mControl.seekToTrack(track)) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * Seek to.
     * 
     * @param command the command
     * @param position the position
     */
    public void seekTo(int command, int position) {
        mSeekArgu.clear();
        UpnpTool.addArg(mSeekArgu, Upnp.Common.COMMON_COMMAND, command);
        UpnpTool.addArg(mSeekArgu, Upnp.AVTSArgVariable.Seek.AVTS_VARIABLE_IN_TARGET, position);
        mMultiHandler.sendEmptyMessage(SEEK);

        /*
        if (mSeekToActionThreadCore.isRunnable()) {
            mSeekToActionThreadCore.restart(argumentList);
        } else {
            mSeekToActionThreadCore.restart(argumentList);
        }
        */
    }
    
    private void seek(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " SeekToActionThreadCore");

        
        if (null == mSeekArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " SeekToActionThreadCore Null Pointer");
            return;
        }

        int command = mSeekArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();
        int position = mSeekArgu.getArgument(Upnp.AVTSArgVariable.Seek.AVTS_VARIABLE_IN_TARGET)
                .getIntegerValue();
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }
        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }
        if (((MediaAVControl)mControl).seekTo(position)) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * The Class SetMuteActionThreadCore.
     */
    class SetMuteActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " SetMuteActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " SetMuteActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();
            boolean mute = Boolean.parseBoolean(inArgumentList.getArgument(
                    Upnp.RCSArgVariable.SetMute.InDesiredMute).getValue());

            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }

            if (null == mControl) {
                notifyService(command, null, CBCommand.ErrorID.unInitialized);
                return;
            }
            if (mControl.setMute(mute)) {
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m set mute action thread core. */
    SetMuteActionThreadCore mSetMuteActionThreadCore = new SetMuteActionThreadCore();

    /**
     * Sets the mute.
     * 
     * @param command the command
     * @param mute the mute
     */
    public void setMute(int command, boolean mute) {
        mSetMuteArgu.clear();
        UpnpTool.addArg(mSetMuteArgu, Upnp.Common.COMMON_COMMAND, command);
        UpnpTool.addArg(mSetMuteArgu, Upnp.RCSArgVariable.SetMute.RCS_VARIABLE_IN_DESIRED_MUTE, mute);
        mMultiHandler.sendEmptyMessage(SET_MUTE);

        /*
        if (mSetMuteActionThreadCore.isRunnable()) {
            mSetMuteActionThreadCore.restart(argumentList);
        } else {
            mSetMuteActionThreadCore.restart(argumentList);
        }
        */
    }
    
    private void setMute(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " SetMuteActionThreadCore");

        
        if (null == mSetMuteArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " SetMuteActionThreadCore Null Pointer");
            return;
        }

        int command = mSetMuteArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();
        boolean mute = Boolean.parseBoolean(mSetMuteArgu.getArgument(
                Upnp.RCSArgVariable.SetMute.RCS_VARIABLE_IN_DESIRED_MUTE).getValue());

        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }

        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }
        if (mControl.setMute(mute)) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * The Class SetVolumeActionThreadCore.
     */
    class SetVolumeActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger
                    .v(TAG, "Thread-" + Thread.currentThread().getId()
                            + " SetVolumeActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " SetVolumeActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();
            int volume = inArgumentList.getArgument(Upnp.RCSArgVariable.SetVolume.InDesiredVolume)
                    .getIntegerValue();

            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }

            if (null == mControl) {
                notifyService(command, null, CBCommand.ErrorID.unInitialized);
                return;
            }
            if (mControl.setVolume(volume)) {
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m set volume action thread core. */
    SetVolumeActionThreadCore mSetVolumeActionThreadCore = new SetVolumeActionThreadCore();

    /**
     * Sets the volume.
     * 
     * @param command the command
     * @param volume the volume
     */
    public void setVolume(int command, int volume) {
        mSetVolumeArgu.clear();
        UpnpTool.addArg(mSetVolumeArgu, Upnp.Common.COMMON_COMMAND, command);
        UpnpTool.addArg(mSetVolumeArgu, Upnp.RCSArgVariable.SetVolume.RCS_VARIABLE_IN_DESIRED_VOLUME, volume);
        mMultiHandler.sendEmptyMessage(SET_VOLUME);

        /*
        if (mSetVolumeActionThreadCore.isRunnable()) {
            mSetVolumeActionThreadCore.restart(argumentList);
        } else {
            mSetVolumeActionThreadCore.restart(argumentList);
        }
        */
    }
    
    private void setVolume(){
        Logger
        .v(TAG, "Thread-" + Thread.currentThread().getId()
                + " SetVolumeActionThreadCore");

        
        if (null == mSetVolumeArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " SetVolumeActionThreadCore Null Pointer");
            return;
        }
        
        int command = mSetVolumeArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();
        int volume = mSetVolumeArgu.getArgument(Upnp.RCSArgVariable.SetVolume.RCS_VARIABLE_IN_DESIRED_VOLUME)
                .getIntegerValue();
        
        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }
        
        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }
        if (mControl.setVolume(volume)) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * The Class StopActionThreadCore.
     */
    class StopActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " StopActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " StopActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();

            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }

            if (null == mControl) {
                notifyService(command, null, CBCommand.ErrorID.unInitialized);
                return;
            }
            if (mControl.stop()) {
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m stop action thread core. */
    StopActionThreadCore mStopActionThreadCore = new StopActionThreadCore();

    /**
     * Stop.
     * 
     * @param command the command
     */
    public void stop(int command) {
        mStopArgu.clear();
        UpnpTool.addArg(mStopArgu, Upnp.Common.COMMON_COMMAND, command);
        
        mMultiHandler.sendEmptyMessage(STOP);
        
        /*
        if (mStopActionThreadCore.isRunnable()) {
            mStopActionThreadCore.restart(argumentList);
        } else {
            mStopActionThreadCore.restart(argumentList);
        }
        */

    }
    
    private void stopDmr(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " StopActionThreadCore");

        
        if (null == mStopArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " StopActionThreadCore Null Pointer");
            return;
        }

        int command = mStopArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();

        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }

        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }
        if (mControl.stop()) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * The Class RecordActionThreadCore.
     */
    class RecordActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " RecordActionThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " RecordActionThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();

            Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
            if (device == null) {
                notifyService(command, null, CBCommand.ErrorID.deviceDisappear);
                return;
            }

            if (null == mControl) {
                notifyService(command, null, CBCommand.ErrorID.unInitialized);
                return;
            }
            if (mControl.record()) {
                try {
                    Thread.sleep(Upnp.Common.recordingLoopTime);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                String state = mControl.getCurrentState();
                Logger.i(TAG, "Thread-" + Thread.currentThread().getId()
                        + " RecordActionThreadCore state:" + state);
                while (Upnp.AVTSArgVariable.stateVariable.TransportState.RECORDING
                        .equalsIgnoreCase(state)) {
                    notifyService(command, null, CBCommand.ErrorID.recording);
                    try {
                        Thread.sleep(Upnp.Common.recordingLoopTime);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    state = mControl.getCurrentState();
                    Logger.i(TAG, "Thread-" + Thread.currentThread().getId()
                            + " RecordActionThreadCore state:" + state);
                }
                notifyService(command, null, CBCommand.ErrorID.ok);
            } else {
                notifyService(command, null, CBCommand.ErrorID.protocolError);
            }
            */
        }
    }

    /** The m record action thread core. */
    RecordActionThreadCore mRecordActionThreadCore = new RecordActionThreadCore();

    /**
     * Record.
     * 
     * @param command the command
     */
    public void record(int command) {
        mRecordArgu.clear();
        UpnpTool.addArg(mRecordArgu, Upnp.Common.COMMON_COMMAND, command);
        mMultiHandler.sendEmptyMessage(RECORD);

        /*
        if (mRecordActionThreadCore.isRunnable()) {
            mRecordActionThreadCore.restart(argumentList);
        } else {
            mRecordActionThreadCore.restart(argumentList);
        }
        */
    }
    
    private void record(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " RecordActionThreadCore");

        
        if (null == mRecordArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " RecordActionThreadCore Null Pointer");
            return;
        }

        int command = mRecordArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();

        Device device = DeviceAction.getDMRDevice(mCurConnectionInfo.getDmrUuid());
        if (device == null) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.DEVICE_DISAPPEAR);
            return;
        }

        if (null == mControl) {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.UNINITIALIZED);
            return;
        }
        if (mControl.record()) {
            try {
                Thread.sleep(Upnp.Common.COMMON_RECORDING_LOOP_TIME);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            String state = mControl.getCurrentState();
            Logger.i(TAG, "Thread-" + Thread.currentThread().getId()
                    + " RecordActionThreadCore state:" + state);
            while (Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_RECORDING
                    .equalsIgnoreCase(state)) {
                notifyService(command, mDmrUuid, CBCommand.ErrorID.RECORDING);
                try {
                    Thread.sleep(Upnp.Common.COMMON_RECORDING_LOOP_TIME);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                state = mControl.getCurrentState();
                Logger.i(TAG, "Thread-" + Thread.currentThread().getId()
                        + " RecordActionThreadCore state:" + state);
            }
            notifyService(command, mDmrUuid, CBCommand.ErrorID.OK);
        } else {
            notifyService(command, mDmrUuid, CBCommand.ErrorID.PROTOCOL_ERROR);
        }
    }

    /**
     * The Class GetDMRStatusThreadCore.
     */
    class GetDMRStatusThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            /*
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " GetDMRStatusThreadCore");

            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                        + " GetDMRStatusThreadCore Null Pointer");
                return;
            }

            int command = inArgumentList.getArgument(Upnp.Common.command).getIntegerValue();
            String uuid = inArgumentList.getArgument(Upnp.Common.dmrUuid).getValue();

            notifyService(command, uuid, DeviceTool.getDmrState(uuid));
            */
        }
    }

    /** The m record action thread core. */
    GetDMRStatusThreadCore mGetDMRStatusThreadCore = new GetDMRStatusThreadCore();

    /**
     * Gets the dMR status.
     * 
     * @param command the command
     * @param uuid the uuid
     * @return the dMR status
     */
    public void getDMRStatus(int command, String uuid) {
        mGetDmrStatusArgu.clear();
        UpnpTool.addArg(mGetDmrStatusArgu, Upnp.Common.COMMON_COMMAND, command);
        UpnpTool.addArg(mGetDmrStatusArgu, Upnp.Common.COMMON_DMR_UUID, uuid);
        int max = 0;
//        while(mMultiHandler == null && max < 50){
//            try{
//                Thread.sleep(10);
//            }
//            catch(Exception e){
//                e.printStackTrace();
//            }
//            
//            max ++;
//        }
        if(mMultiHandler != null){
            mMultiHandler.sendEmptyMessage(GET_DMR_STATUS);
        }
        else{
            notifyService(command, uuid, CBCommand.ErrorID.UNINITIALIZED);
        }

        /*
        if (mGetDMRStatusThreadCore.isRunnable()) {
            mGetDMRStatusThreadCore.restart(argumentList);
        } else {
            mGetDMRStatusThreadCore.restart(argumentList);
        }
        */
    }
    
    private void getDmrStatus(){
        Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " GetDMRStatusThreadCore");


        if (null == mGetDmrStatusArgu) {
            Logger.e(TAG, "Thread-" + Thread.currentThread().getId()
                    + " GetDMRStatusThreadCore Null Pointer");
            return;
        }

        int command = mGetDmrStatusArgu.getArgument(Upnp.Common.COMMON_COMMAND).getIntegerValue();
        String uuid = mGetDmrStatusArgu.getArgument(Upnp.Common.COMMON_DMR_UUID).getValue();

        notifyService(command, uuid, DeviceTool.getDmrState(uuid));
    }
    
    public String getDmrUuid(){
        return mDmrUuid;
    }
    
    public String getDmsUuid(){
        return mDmsUuid;
    }
}
