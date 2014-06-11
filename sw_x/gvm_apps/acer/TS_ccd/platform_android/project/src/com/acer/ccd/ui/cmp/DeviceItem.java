/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.ui.cmp;

public class DeviceItem {
    
    public static final int DEVICE_ID_ACER = 0x50;
    public static final int DEVICE_ID_OTHERS = DEVICE_ID_ACER+1;
    
    public static final int DEVICE_TYPE_PHONE = 0x60;
    public static final int DEVICE_TYPE_TABLET = DEVICE_TYPE_PHONE + 1;
    public static final int DEVICE_TYPE_PC = DEVICE_TYPE_PHONE + 2;
    
    private int mDeviceId;
    private String mDeviceName;
    private int mDeviceType;
    private boolean mIsLink;
    private boolean mIsOnline;
    private String mItemTitle = "";
    
    
    public int getDeviceId() {
        return mDeviceId;
    }
    public void setDeviceId(int mDeviceId) {
        this.mDeviceId = mDeviceId;
    }
    
    public String getDeviceName() {
        return mDeviceName;
    }
    public void setDeviceName(String deviceName) {
        this.mDeviceName = deviceName;
    }
    
    public int getDeviceType() {
        return mDeviceType;
    }
    public void setDeviceType(int deviceType) {
        this.mDeviceType = deviceType;
    }
    
    public boolean isIsLink() {
        return mIsLink;
    }
    public void setIsLink(boolean isLink) {
        this.mIsLink = isLink;
    }
    
    public String toString() {
        return String.format("DeviceItem: DeviceId=(%s), DeviceName=(%S), DeviceType=(%d), ItemTitle=(%s)", mDeviceId, mDeviceName, mDeviceType, mItemTitle);
    }
    public boolean getIsOnline() {
        return mIsOnline;
    }
    public void setIsOnline(boolean isOnline) {
        this.mIsOnline = isOnline;
    }
    public String getItemTitle() {
        return mItemTitle;
    }
    public void setItemTitle(String title) {
        this.mItemTitle = title;
    }
    
}
