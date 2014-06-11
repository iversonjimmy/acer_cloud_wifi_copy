/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: UpnpTool.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.util;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;

import org.cybergarage.upnp.Action;
import org.cybergarage.upnp.ActionList;
import org.cybergarage.upnp.Argument;
import org.cybergarage.upnp.ArgumentList;
import org.cybergarage.upnp.Device;
import org.cybergarage.upnp.DeviceList;
import org.cybergarage.upnp.Service;
import org.cybergarage.upnp.ServiceList;
import org.cybergarage.upnp.ServiceStateTable;
import org.cybergarage.upnp.StateVariable;
import org.cybergarage.upnp.ssdp.SSDPPacket;

import com.acer.ccd.upnp.dmr.DMRTool;

/**
 * The Class UpnpTestMethod.
 * 
 * @author chaozhong li
 */
public final class UpnpTool {

    /** The Constant tag. */
    private final static String tag = "UpnpTool";

    /** The Constant DBG. */
    private final static boolean DBG = false;

    /**
     * Adds the arg.
     * 
     * @param argumentList the argument list
     * @param command the command
     * @param value the value
     */
    @SuppressWarnings("unchecked")
    public static void addArg(ArgumentList argumentList, String command, boolean value) {
        if (null == argumentList || null == command) {
            return;
        }
        Argument arg = new Argument();
        arg.setName(command);
        arg.setValue(value);
        argumentList.add(arg);
    }

    /**
     * Adds the arg.
     * 
     * @param argumentList the argument list
     * @param command the command
     * @param value the value
     */
    @SuppressWarnings("unchecked")
    public static void addArg(ArgumentList argumentList, String command, int value) {
        if (null == argumentList || null == command) {
            return;
        }
        Argument arg = new Argument();
        arg.setName(command);
        arg.setValue(value);
        argumentList.add(arg);
    }

    /**
     * Adds the arg.
     * 
     * @param argumentList the argument list
     * @param name the name
     * @param value the value
     */
    @SuppressWarnings("unchecked")
    static public void addArg(ArgumentList argumentList, String name, String value) {
        if (null == argumentList || null == name) {
            return;
        }
        Argument arg = new Argument();
        arg.setName(name);
        arg.setValue(value);
        argumentList.add(arg);
    }

    /**
     * Creates the file.
     * 
     * @param filename the filename
     * @param bytes the bytes
     */
    static public void createFile(String filename, byte[] bytes) {
        try {
            String s = new String(bytes, "UTF-8");
            File f = new File("/sdcard/clear.fi/" + filename + ".txt");
            try {
                FileWriter fw = new FileWriter(f);
                fw.write(s);
                fw.flush();
                fw.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
    }

    /**
     * Creates the file.
     * 
     * @param filename the filename
     * @param s the s
     */
    static public void createFile(String filename, String s) {
        File f = new File("/sdcard/clear.fi/" + filename + ".txt");
        try {
            FileWriter fw = new FileWriter(f);
            fw.write(s);
            fw.flush();
            fw.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    /**
     * Gets the time second.
     * 
     * @param time the time
     * @return the time second
     */
    static public long getTimeSecond(String time) {
        if (null == time) {
            return -1;
        }
        String split[] = time.split(DMRTool.colonSign);
        int len = split.length;
        
        try {
            if (len == 1) {
                return (Long.parseLong(split[0]));
            } else if (len == 2) {
                return (Long.parseLong(split[0]) * 60 + Long.parseLong(split[1]));
            } else if (len == 3) {
                // WMP will always publish format as hh:mm:ss.000
                return ((Long.parseLong(split[0]) * 24 * 60) + (Long.parseLong(split[1]) * 60) + (Long.parseLong(split[2].substring(0, Math.min(2, split[2].length())))));
            }
        } catch (Exception e) {
        }

        // Special case, we don't support incorrect time format string 
        Logger.i(tag, "getTimeSecond() wrong time format. time=" + time);
        return -1;
    }

    /**
     * Input stream2 string.
     * 
     * @param is the is
     * @return the string
     */
    static public String inputStream2String(InputStream is) {
        BufferedReader in = new BufferedReader(new InputStreamReader(is));
        StringBuffer buffer = new StringBuffer();
        String line = "";
        try {
            while ((line = in.readLine()) != null) {
                buffer.append(line);
                buffer.append("\n");
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return buffer.toString();
    }

    /**
     * Prints the device.
     * 
     * @param dev the dev
     */
    static public void printAllDevice(Device dev) {
        if (!DBG || null == dev) {
            return;
        }
        String devName = dev.getFriendlyName();
        Logger.i(tag, "[Device Name]" + devName);
        DeviceList childDevList = dev.getDeviceList();
        int nChildDevs = childDevList.size();
        for (int n = 0; n < nChildDevs; n++) {
            Device childDev = childDevList.getDevice(n);
            printAllDevice(childDev);
        }
    }

    /**
     * Prints the arg content.
     * 
     * @param argTag the arg tag
     * @param argList the arg list
     * @param argName the arg name
     */
    static public void printArgContent(String argTag, ArgumentList argList, String argName) {
        if (!DBG || null == argList) {
            return;
        }
        Argument arg = argList.getArgument(argName);
        if (null != arg) {
            Logger.v(tag, "ActionName:" + argTag + "  argName(" + argName + "):" + arg.getValue());
        }
    }

    /**
     * Prints the device detail info.
     * 
     * @param dev the dev
     */
    static public void printDeviceDetailInfo(Device dev) {
        if (!DBG || null == dev) {
            return;
        }

        Logger.v(tag, "==========printDeviceDetailInfo=============Start");
        String AbsoluteURL = new String();
        Logger.v(tag, "AbsoluteURL:" + dev.getAbsoluteURL(AbsoluteURL));
        Logger.v(tag, "DescriptionFilePath:" + dev.getDescriptionFilePath());
        Logger.v(tag, "DeviceType:" + dev.getDeviceType());
        Logger.v(tag, "ElapsedTime:" + dev.getElapsedTime());
        Logger.v(tag, "FriendlyName:" + dev.getFriendlyName());
        Logger.v(tag, "HTTPPort:" + dev.getHTTPPort());
        Logger.v(tag, "InterfaceAddress:" + dev.getInterfaceAddress());
        Logger.v(tag, "LeaseTime:" + dev.getLeaseTime());
        Logger.v(tag, "Location:" + dev.getLocation());
        Logger.v(tag, "Manufacture:" + dev.getManufacture());
        Logger.v(tag, "ManufactureURL:" + dev.getManufactureURL());
        Logger.v(tag, "ModelDescription:" + dev.getModelDescription());
        Logger.v(tag, "ModelName:" + dev.getModelName());
        Logger.v(tag, "ModelNumber:" + dev.getModelNumber());
        Logger.v(tag, "ModelURL:" + dev.getModelURL());
        Logger.v(tag, "MulticastIPv4Address:" + dev.getMulticastIPv4Address());
        Logger.v(tag, "MulticastIPv6Address:" + dev.getMulticastIPv6Address());
        Logger.v(tag, "PresentationURL:" + dev.getPresentationURL());
        Logger.v(tag, "ParentDevice:" + dev.getParentDevice());
        Logger.v(tag, "SerialNumber:" + dev.getSerialNumber());
        Logger.v(tag, "SSDPAnnounceCount:" + dev.getSSDPAnnounceCount());
        Logger.v(tag, "SSDPIPv4MulticastAddress:" + dev.getSSDPIPv4MulticastAddress());
        Logger.v(tag, "SSDPIPv6MulticastAddress:" + dev.getSSDPIPv6MulticastAddress());
        Logger.v(tag, "SSDPPort:" + dev.getSSDPPort());
        Logger.v(tag, "TimeStamp:" + dev.getTimeStamp());
        Logger.v(tag, "UDN:" + dev.getUDN());
        Logger.v(tag, "UPC:" + dev.getUPC());
        Logger.v(tag, "URLBase:" + dev.getURLBase());

        ServiceList serviceList = dev.getServiceList();
        int serviceCnt = serviceList.size();
        for (int n = 0; n < serviceCnt; n++) {
            Service service = serviceList.getService(n);
            Logger.i(tag, "service Type[" + n + "]" + service.getServiceType());
            ActionList actionList = service.getActionList();
            int actionCnt = actionList.size();
            for (int i = 0; i < actionCnt; i++) {
                Action action = actionList.getAction(i);
                Logger.i(tag, "service[" + i + "]" + action.getName());
            }
            ServiceStateTable stateTable = service.getServiceStateTable();
            int varCnt = stateTable.size();
            for (int i = 0; i < varCnt; i++) {
                StateVariable stateVar = stateTable.getStateVariable(i);
                Logger.i(tag, "StateVariable[" + i + "]" + stateVar.getName());
            }
        }

        Logger.v(tag, "==========printDeviceDetailInfo=============end");
    }

    /**
     * Prints the ssdp packet.
     * 
     * @param ssdpPacket the ssdp packet
     */
    static public void printSsdpPacket(SSDPPacket ssdpPacket) {
        if (!DBG || null == ssdpPacket) {
            return;
        }
        Logger.v(tag, "==========printSsdpPacket=============Start");
        Logger.v(tag, "Host:" + ssdpPacket.getHost());
        Logger.v(tag, "LeaseTime:" + ssdpPacket.getLeaseTime());
        Logger.v(tag, "LocalAddress:" + ssdpPacket.getLocalAddress());
        Logger.v(tag, "MAN:" + ssdpPacket.getMAN());
        Logger.v(tag, "MX:" + ssdpPacket.getMX());
        Logger.v(tag, "NT:" + ssdpPacket.getNT());
        Logger.v(tag, "NTS:" + ssdpPacket.getNTS());
        Logger.v(tag, "RemoteAddress:" + ssdpPacket.getRemoteAddress());
        Logger.v(tag, "RemotePort:" + ssdpPacket.getRemotePort());
        Logger.v(tag, "Server:" + ssdpPacket.getServer());
        Logger.v(tag, "ST:" + ssdpPacket.getST());
        Logger.v(tag, "TimeStamp:" + ssdpPacket.getTimeStamp());
        Logger.v(tag, "USN:" + ssdpPacket.getUSN());
        Logger.v(tag, "==========printSsdpPacket=============End");
    }

    /**
     * String2 input stream.
     * 
     * @param str the str
     * @return the input stream
     */
    static public InputStream String2InputStream(String str) {
        ByteArrayInputStream stream = new ByteArrayInputStream(str.getBytes());
        return stream;
    }
}
