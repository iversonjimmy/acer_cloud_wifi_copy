/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: DeviceAction.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.device;

import org.cybergarage.upnp.Device;
import org.cybergarage.upnp.device.DeviceChangeListener;
import org.cybergarage.upnp.device.NotifyListener;
import org.cybergarage.upnp.device.SearchResponseListener;
import org.cybergarage.upnp.ssdp.SSDPPacket;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import com.acer.ccd.cache.data.DlnaDevice;
import com.acer.ccd.upnp.common.DlnaAction;
import com.acer.ccd.upnp.dmr.DMRActionListener;
import com.acer.ccd.upnp.util.CBCommand;
import com.acer.ccd.upnp.util.DBManagerUtil;
import com.acer.ccd.upnp.util.Dlna;
import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;
import com.acer.ccd.upnp.util.UpnpTool;

/**
 * The Class DeviceAction.
 * 
 * @author chaozhong li
 */
public class DeviceAction extends DlnaAction implements DeviceChangeListener,
        SearchResponseListener, NotifyListener {

    /** The TAG. */
    private final static String TAG = "DeviceAction";

    /** The clearficp. */
    private static ClearfiCP clearficp = null;

    /** The m total. */
    private int mTotal = 0;
    
    public static final boolean IS_EVENT_ON = true;
    private static final long EVENT_VALID_TIME = 10000000;
    private long mTimeout = 0;
    private RenewSubcriberThread renewSubThread = null;
    private Handler mRenewSubHandler = null;
    
    private static final int RENEW_SUBCRIBER = 0;
    private static final int QUIT_LOOPER = 1;
    
    class RenewSubcriberThread extends Thread{
        public RenewSubcriberThread(String name){
            super(name);
        }
        
        public void run() {
            Looper.prepare();
            mRenewSubHandler = new Handler() {
                @Override
                public void handleMessage(Message msg) {
                    super.handleMessage(msg);
                    
                    switch(msg.what){
                    case RENEW_SUBCRIBER:
                        while(mTimeout > 0){
                            try{
                                sleep(mTimeout * 1000);
                            }
                            catch(Exception e){
                                e.printStackTrace();
                            }
                            if(clearficp != null){
                                clearficp.renewSubscriberService(EVENT_VALID_TIME);
                            }
                            Logger.i(TAG, "renew success");
                        }
                        
                        break;
                    
                    case QUIT_LOOPER:
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

    

    /*
     * (non-Javadoc)
     * @see com.acer.clearfi.common.DlnaAction#init()
     */
    public void init() {
        Logger.v(TAG, "init()");
        if (null == clearficp) {
            clearficp = new ClearfiCP(this);
            clearficp.addDeviceChangeListener(this);
            clearficp.addSearchResponseListener(this);
            //clearficp.addEventListener(clearficp);

            clearficp.start();
            mTimeout = 0;
            if(renewSubThread == null){
                renewSubThread = new RenewSubcriberThread("renew subcriber");
                renewSubThread.start();
            }
        }
    }

    /*
     * (non-Javadoc)
     * @see com.acer.clearfi.common.DlnaAction#release()
     */
    @Override
    public void release() {
        if (null != clearficp) {
            clearficp.stop();
        }
        
        mTimeout = 0;
        if(mRenewSubHandler != null){
            mRenewSubHandler.sendEmptyMessage(QUIT_LOOPER);
        }
        clearficp = null;
        
    }

    /*
     * (non-Javadoc)
     * @see
     * org.cybergarage.upnp.device.DeviceChangeListener#deviceAdded(org.cybergarage
     * .upnp.Device)
     */
    @Override
    public void deviceAdded(Device dev) {
        Logger.i(TAG, "callback - deviceAdded()");
        addDeviceToDB(dev);
        notifyService(CBCommand.DevNotifyID.ADD_DEVICE);
        //suscribe events of dmr
        if(IS_EVENT_ON){
        	org.cybergarage.upnp.Service service = dev.getService(Upnp.Service.SERVICE_AVTS1);
            if(service != null && clearficp != null){
                boolean success = clearficp.subscribe(service, EVENT_VALID_TIME);
                if(success){
                    String sid = service.getSID();
                    long timeout = service.getTimeout();
                    if(mTimeout == 0 || mTimeout > timeout){
                        mTimeout = timeout;
                    }
                    if(clearficp != null){
                        clearficp.addService(service);
                    }
                    mRenewSubHandler.sendEmptyMessage(RENEW_SUBCRIBER);

                    Logger.i(TAG, "sid = " + sid);
                    Logger.i(TAG, "mTimeout = " + mTimeout);
                }
            }
        }
        
        

        UpnpTool.printAllDevice(dev);
        // UpnpTool.printDeviceDetailInfo(dev);
    }

    /*
     * (non-Javadoc)
     * @seeorg.cybergarage.upnp.device.DeviceChangeListener#deviceRemoved(org.
     * cybergarage.upnp.Device)
     */
    @Override
    public void deviceRemoved(Device dev) {
        Logger.i(TAG, "callback - deviceRemoved()");
        
        org.cybergarage.upnp.Service service = dev.getService(Upnp.Service.SERVICE_AVTS1);
        if(service != null){
            clearficp.unsubscribe(service);
            clearficp.removeService(service);
        }

        removeDeviceFromDB(dev);
        notifyService(CBCommand.DevNotifyID.REMOVE_DEVICE);

        UpnpTool.printAllDevice(dev);
        // UpnpTool.printDeviceDetailInfo(dev);
    }

    /*
     * (non-Javadoc)
     * @seeorg.cybergarage.upnp.device.SearchResponseListener#
     * deviceSearchResponseReceived(org.cybergarage.upnp.ssdp.SSDPPacket)
     */
    @Override
    public void deviceSearchResponseReceived(SSDPPacket ssdpPacket) {
        Logger.i(TAG, "callback - deviceSearchResponseReceived()");
        // notifyService(CBCommand.DevNotifyID.searchDevice);
        UpnpTool.printSsdpPacket(ssdpPacket);
    }

    /**
     * Device notify received.
     * 
     * @param ssdpPacket the ssdp packet
     */
    @Override
    public void deviceNotifyReceived(SSDPPacket ssdpPacket) {
        Logger.i(TAG, "callback - deviceNotifyReceived()");
        UpnpTool.printSsdpPacket(ssdpPacket);
    }

    /**
     * Refresh devices.
     */
    public void refreshDevices() {
        Logger.i(TAG, "refreshDevices()");
        if (null == clearficp) {
            init();
        }
        
        clearficp.clearServiceList();

        // remove all old devices
        removeAllDeviceFromDB();
        clearficp.refresh();
    }

    /**
     * Gets the dMS device.
     * 
     * @param uuid the uuid
     * @return the dMS device
     */
    public static Device getDMSDevice(String uuid) {
        Device dev = null;
        if (null != uuid && null != clearficp) {
            dev = clearficp.getDevice(uuid);
            if (null != dev) {
                String devType = DeviceTool.getDevType(dev);
                if (Dlna.DeviceType.DLNA_DEVICE_TYPE_DMS.equalsIgnoreCase(devType)
                        || Dlna.DeviceType.DLNA_DEVICE_TYPE_DMSDMR.equalsIgnoreCase(devType)) {
                    Logger.i(TAG, "Device( " + uuid + " )" + " is DMS");
                } else {
                    Logger.e(TAG, "Device( " + uuid + " )" + " is not DMS");
                }
            } else {
                Logger.w(TAG, "Device( " + uuid + " )" + " is not existed.");
            }
        } else {
            Logger.e(TAG, "Device( " + uuid + " )" + " failure!"
                    + "<ControlPoint or uuid is null!>");
        }

        return dev;
    }

    /**
     * Gets the dMR device.
     * 
     * @param uuid the uuid
     * @return the dMR device
     */
    public static Device getDMRDevice(String uuid) {
        Device dev = null;
        if (null != uuid && null != clearficp) {
            dev = clearficp.getDevice(uuid);
            if (null != dev) {
                String devType = DeviceTool.getDevType(dev);
                if (Dlna.DeviceType.DLNA_DEVICE_TYPE_DMR.equalsIgnoreCase(devType)
                        || Dlna.DeviceType.DLNA_DEVICE_TYPE_DMSDMR.equalsIgnoreCase(devType)) {
                    Logger.i(TAG, "Device( " + uuid + " )" + " is DMR");
                } else {
                    Logger.e(TAG, "Device( " + uuid + " )" + " is not DMR");
                }
            } else {
                Logger.w(TAG, "Device( " + uuid + " )" + " is not existed.");
            }
        } else {
            Logger.e(TAG, "Device( " + uuid + " )" + " failure!"
                    + "<ControlPoint or uuid is null!>");
        }

        return dev;
    }

    /**
     * Gets the device.
     * 
     * @param uuid the uuid
     * @return the device
     */
    public static Device getDevice(String uuid) {
        Device dev = null;
        if (null != uuid && null != clearficp) {
            dev = clearficp.getDevice(uuid);
        }
        return dev;
    }

    /**
     * Adds the device to db.
     * 
     * @param dev the dev
     */
    private void addDeviceToDB(Device dev) {
        DlnaDevice dlnaDevice = new DlnaDevice();
        String deviceType = DeviceTool.getDevType(dev);
        String deviceName = dev.getFriendlyName();
        String manufacture = dev.getManufacture();
        String manufacturerUrl = dev.getManufactureURL();
        String modelName = dev.getModelName();
        String uuid = dev.getUDN();
        String iconPath = DeviceTool.getDevIconUrl(dev);
        int isAcerDev = DeviceTool.getDevId(dev);

        // UpnpTool.printDeviceDetailInfo(dev);
        Logger.i(TAG, "device type = " + deviceType);
        Logger.i(TAG, "iconpath = " + iconPath);
        dlnaDevice.setAcerDevice(isAcerDev);
        dlnaDevice.setDeviceType(deviceType);
        dlnaDevice.setDeviceName(deviceName);
        dlnaDevice.setManufacture(manufacture);
        dlnaDevice.setManufacturerUrl(manufacturerUrl);
        dlnaDevice.setModelName(modelName);
        dlnaDevice.setUuid(uuid);
        dlnaDevice.setIconPath(iconPath);
        DBManagerUtil.getDBManager().addDevice(dlnaDevice);

        mTotal++;
    }

    /**
     * Removes the device to db.
     * 
     * @param dev the dev
     */
    private void removeDeviceFromDB(Device dev) {
        // TBD(wait for DB support)
        mTotal--;
        DBManagerUtil.getDBManager().deleteDeviceByUuid(dev.getUDN());
    }

    /**
     * Removes the all device from db.
     */
    private void removeAllDeviceFromDB() {
        // TBD(wait for DB support)
        DBManagerUtil.getDBManager().deleteAllDevices();
        mTotal = 0;
    }

    /**
     * Notify service.
     * 
     * @param commandid the commandid
     */
    private void notifyService(int commandid) {
        int listenerSize = getListenerlist().size();
        for (int n = 0; n < listenerSize; n++) {
            DeviceListener listener = (DeviceListener)getListenerlist().get(n);
            listener.devNotifyReceived(commandid, mTotal, CBCommand.ErrorID.OK);
        }
    }
    
    public void notifyService(int command, String uuid, int errCode) {
        int listenerSize = getListenerlist().size();
        for (int n = 0; n < listenerSize; n++) {
            DMRActionListener listener = (DMRActionListener)getListenerlist().get(n);
            listener.dmrNotify(command, uuid, errCode);
        }
    }

}
