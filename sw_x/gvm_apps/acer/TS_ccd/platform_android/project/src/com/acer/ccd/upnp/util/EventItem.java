package com.acer.ccd.upnp.util;

public class EventItem {
    private String mTransportState = null;
    private String mCurrentTrack = null;
    
    public void setTransportState(String transportState){
        mTransportState = transportState;
    }
    public void setCurrentTrack(String currentTrack){
        mCurrentTrack = currentTrack;
    }
    
    public String getTransportState(){
        return mTransportState;
    }
    
    public String getCurrentTrack(){
        return mCurrentTrack;
    }
}
