/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: ClearfiCP.java
 *
 *	Revision:
 *
 *	2011-3-11
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.device;

import java.io.ByteArrayInputStream;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import org.cybergarage.upnp.ControlPoint;
import org.cybergarage.upnp.event.EventListener;
import org.xml.sax.InputSource;
import org.xml.sax.XMLReader;
import com.acer.ccd.upnp.util.CBCommand;
import com.acer.ccd.upnp.util.EventItem;
import com.acer.ccd.upnp.util.EventXmlHandler;
import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;
import org.cybergarage.upnp.Service;

// TODO: Auto-generated Javadoc
/**
 * The Class ClearfiCP.
 * 
 * @author acer
 */
public class ClearfiCP extends ControlPoint implements EventListener {

    /** The tag. */
    private static final String TAG = "ClearfiCP";

    private DeviceAction mDeviceAction = null;

    private List<Service> serviceList = new ArrayList<Service>();

    /**
     * Instantiates a new clearfi cp.
     */
    public ClearfiCP(DeviceAction deviceAction) {
        // TODO Auto-generated constructor stub

        mDeviceAction = deviceAction;
        addEventListener(this);

    }

    /**
     * Instantiates a new clearfi cp.
     * 
     * @param ssdpPort the ssdp port
     * @param httpPort the http port
     */
    public ClearfiCP(int ssdpPort, int httpPort) {
        // super(ssdpPort, httpPort);
        // TODO Auto-generated constructor stub
    }

    /**
     * Instantiates a new clearfi cp.
     * 
     * @param ssdpPort the ssdp port
     * @param httpPort the http port
     * @param binds the binds
     */
    public ClearfiCP(int ssdpPort, int httpPort, InetAddress[] binds) {
        // super(ssdpPort, httpPort, binds);
        // TODO Auto-generated constructor stub
    }

    @Override
    public void eventNotifyReceived(String uuid, long seq, String varName, String value) {
        Logger.v(TAG, "eventNotifyReceived(" + uuid + ", " + seq + ", " + varName + ", " + value
                + ")");

        List<EventItem> list = parseEvent(value);
        if (null != list) {
            for (int i = 0; i < list.size(); i++) {
                String transportState = list.get(i).getTransportState();
                Logger.i(TAG, "TransportState = " + transportState);
                String currentTrack = list.get(i).getCurrentTrack();
                Logger.i(TAG, "currentTrack = " + currentTrack);
                if (transportState == null || null == currentTrack) {
                    continue;
                }
                for (int j = 0; j < serviceList.size(); j++) {
                    Service service = serviceList.get(j);
                    String sid = service.getSID();
                    if (sid.equals(uuid)) {
                        String devUuid = service.getDevice().getUDN();

                        if(transportState != null){
                            int state = 0;
                            if (transportState.equals(
                                    Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_STOPPED)) {
                                state = CBCommand.DMRActionID.STOPPED;
                                Logger.i(TAG, "notify STOPPED event from " + devUuid);
                            }
                            else if(transportState.equals(
                                    Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_PLAYING)){
                                state = CBCommand.DMRActionID.PLAYING;
                                Logger.i(TAG, "notify PLAYING event from " + devUuid);
                            }
                            else if (transportState.equals(
                                    Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_TRANSITIONING)) {
                                state = CBCommand.DMRActionID.TRANSITIONING;
                                Logger.i(TAG, "notify TRANSITIONING event from " + devUuid);
                            }
                            else if (transportState.equals(
                                    Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_PAUSED_PLAYBACK)) {
                                state = CBCommand.DMRActionID.PAUSED_PLAYBACK;
                                Logger.i(TAG, "notify PAUSED_PLAYBACK event from " + devUuid);
                            }
                            else if (transportState.equals(
                                    Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_PAUSED_RECORDING)) {
                                state = CBCommand.DMRActionID.PAUSED_RECORDING;
                                Logger.i(TAG, "notify PAUSED_RECORDING event from " + devUuid);
                            }
                            else if (transportState.equals(
                                    Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_RECORDING)) {
                                state = CBCommand.DMRActionID.RECORDING;
                                Logger.i(TAG, "notify RECORDING event from " + devUuid);
                            }
                            else if (transportState.equals(
                                    Upnp.AVTSArgVariable.stateVariable.TransportState.AVTS_VARIABLE_NO_MEDIA_PRESENT)) {
                                state = CBCommand.DMRActionID.NO_MEDIA_PRESENT;
                                Logger.i(TAG, "notify NO_MEDIA_PRESENT event from " + devUuid);
                            }
                            
                            if(state > 0){
                                mDeviceAction.notifyService(state, devUuid,
                                        CBCommand.ErrorID.OK);
                            }
                        }
                        
                        if(currentTrack != null){
                            mDeviceAction.notifyService(
                                    CBCommand.DMRActionID.CURRENT_TRACK + 
                                    Integer.valueOf(currentTrack), devUuid, 
                                    CBCommand.ErrorID.OK);
                        }
                    }
                }
            }
        }
    }

    // //////////////////////////////////////////////
    // M-SEARCH
    // //////////////////////////////////////////////
    public void refresh() {
        // delete old devices
        removeAllDevices();

        Logger.i(TAG, "refresh");

        // search devices
        search();
    }

    // //////////////////////////////////////////////
    // Device List
    // //////////////////////////////////////////////
    /**
     * Removes the all devices.
     */
    private void removeAllDevices() {
        devNodeList.removeAllElements();
    }
    
    public void clearServiceList(){
        org.cybergarage.upnp.Service service = null;
        for(int i = 0; i < serviceList.size(); i ++){
            service = serviceList.get(i);
            unsubscribe(service);
        }
        serviceList.clear();
    }

    public static List<EventItem> parseEvent(String valueStr) {
        if (null == valueStr) {
            return null;
        }

        EventXmlHandler xmlHandler = new EventXmlHandler();
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            SAXParser sp = spf.newSAXParser();

            /* Get the XMLReader of the SAXParser we created. */
            XMLReader xr = sp.getXMLReader();
            /* Create a new ContentHandler and apply it to the XML-Reader */
            xr.setContentHandler(xmlHandler);

            /* Parse the xml-data from our URL. */
            xr.parse(new InputSource(new ByteArrayInputStream(valueStr.getBytes("UTF-8"))));
        } catch (Exception e) {
        }
        return xmlHandler.getItemList();

    }

    public void addService(Service service) {
        serviceList.add(service);
    }

    public void removeService(Service service) {
        serviceList.remove(service);
    }

}
