/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: MediaBaseControl.java
 *
 *	Revision:
 *
 *	2011-3-22
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.dmr;

import org.cybergarage.upnp.AllowedValueRange;
import org.cybergarage.upnp.Argument;
import org.cybergarage.upnp.ArgumentList;
import org.cybergarage.upnp.Device;
import org.cybergarage.upnp.StateVariable;

import com.acer.ccd.upnp.action.AVTAction;
import com.acer.ccd.upnp.action.RCSAction;
import com.acer.ccd.upnp.common.Item;
import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;

/**
 * The Class MediaBaseControl.
 */
public class MediaBaseControl {

    /** The TAG. */
    private final String TAG = "MediaBaseControl";

    /** The m device. */
    public Device mDevice = null;

    /** The m instance id. */
    int mInstanceID = 0;

    /**
     * Instantiates a new media base control.
     * 
     * @param mDevice the m device
     * @param mInstanceID the m instance id
     */
    public MediaBaseControl(Device mDevice, int mInstanceID) {
        super();
        this.mDevice = mDevice;
        this.mInstanceID = mInstanceID;
    }

    /**
     * open media(image,audio,video) urlPath: open media path.
     * 
     * @param url the url
     * @param format the format
     * @return true, if successful
     */
    public boolean addNextURI(String url, String format) {
        Logger.v(TAG, "addNextURI(), url: " + url + ", format: " + format);
        if (url == null || format == null) {
            return false;
        }

        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }

        return avtAction.setNextAVTransportURI(mInstanceID, url, format);
    }

    /**
     * get audio Max volume return: Max volume.
     * 
     * @return the max volume
     */
    public int getMaxVolume() {
        try {
            StateVariable sv = mDevice.getStateVariable(Upnp.Service.SERVICE_RCS1,
                    Upnp.CMSArgVariable.stateVariable.CMS_VARIABLE_VOLUME);
            AllowedValueRange avr = sv.getAllowedValueRange();
            return Integer.parseInt(avr.getMaximum());
        } catch (NullPointerException e) {
            e.printStackTrace();
            return -1;
        }
    }

    /**
     * Gets the min volume.
     * 
     * @return the min volume
     */
    public int getMinVolume() {
        try {
            StateVariable sv = mDevice.getStateVariable(Upnp.Service.SERVICE_RCS1,
                    Upnp.CMSArgVariable.stateVariable.CMS_VARIABLE_VOLUME);
            AllowedValueRange avr = sv.getAllowedValueRange();
            return Integer.parseInt(avr.getMinimum());
        } catch (NullPointerException e) {
            e.printStackTrace();
            return -1;
        }
    }

    /**
     * get audio mute state return: mute state.
     * 
     * @return the mute
     */
    public boolean getMute() {
        RCSAction rcsAction = new RCSAction(mDevice);
        if (rcsAction == null) {
            return false;
        }

        ArgumentList outList = rcsAction.getMute(mInstanceID, "Master");
        if (outList == null) {
            return false;
        }

        Argument arg = outList.getArgument(Upnp.RCSArgVariable.GetMute.RCS_VARIABLE_OUT_CURRENT_MUTE);
        if (arg == null) {
            return false;
        }
        return Boolean.parseBoolean(arg.getValue());
    }

    /**
     * get audio volume return: volume value.
     * 
     * @return the volume
     */
    public int getVolume() {
        RCSAction rcsAction = new RCSAction(mDevice);
        if (rcsAction == null) {
            return -1;
        }

        ArgumentList outList = rcsAction.getVolume(mInstanceID, "Master");
        if (outList == null) {
            return -1;
        }

        Argument arg = outList.getArgument(Upnp.RCSArgVariable.GetVolume.RCS_VARIABLE_OUT_CURRENT_VOLUME);
        if (arg == null) {
            return -1;
        }

        try {
            return Integer.parseInt(arg.getValue());
        } catch (NullPointerException e) {
            e.printStackTrace();
            return -1;
        }

    }

    /**
     * next media(image,audio,video) urlPath: next media path.
     * 
     * @return true, if successful
     */
    public boolean next() {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }
        return avtAction.next(mInstanceID);
    }

    /**
     * open media(image,audio,video) urlPath: open media path.
     * 
     * @param item the item
     * @return true, if successful
     */
    public boolean open(Item item) {
        if (item == null) {
            return false;
        }

        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }

        return avtAction.SetAVTransportURI(mInstanceID, item.getUrl(), item.getFormat());
    }

    /**
     * open media(image,audio,video) urlPath: open media path.
     * 
     * @param url the url
     * @param format the format
     * @return true, if successful
     */
    public boolean open(String url, String format) {
        if (url == null || format == null) {
            return false;
        }

        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }

        return avtAction.SetAVTransportURI(mInstanceID, url, format);
    }

    /**
     * pause current media(image,audio,video).
     * 
     * @return true, if successful
     */
    public boolean pause() {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }
        return avtAction.pause(mInstanceID);
    }

    /**
     * play current media(image,audio,video).
     * 
     * @return true, if successful
     */
    public boolean play() {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }

        String speed = "1";
        ArgumentList argList = avtAction.getTransportInfo(mInstanceID);
        if (argList != null) {
            speed = argList.getArgument(Upnp.AVTSArgVariable.GetTransportInfo.AVTS_VARIABLE_OUT_CURRENT_SPEED)
                    .getValue();
        }
        return avtAction.play(mInstanceID, speed);
    }

    /**
     * previous media(image,audio,video) urlPath: previous media path.
     * 
     * @return true, if successful
     */
    public boolean prev() {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }
        return avtAction.previous(mInstanceID);
    }

    /**
     * seek to track
     * 
     * @param track the track
     * @return true, if successful
     */
    public boolean seekToTrack(int track) {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }

        return avtAction.seek(mInstanceID, "TRACK_NR", Integer.toString(track) );
    }

    /**
     * set audio volume state: true: mute, false: has sound.
     * 
     * @param state the state
     * @return true, if successful
     */
    public boolean setMute(boolean state) {
        RCSAction rcsAction = new RCSAction(mDevice);
        if (rcsAction == null) {
            return false;
        }

        return rcsAction.setMute(mInstanceID, "Master", state);
    }

    /**
     * set audio volume volume: volume value.
     * 
     * @param volume the volume
     * @return true, if successful
     */
    public boolean setVolume(int volume) {
        RCSAction rcsAction = new RCSAction(mDevice);
        if (rcsAction == null) {
            return false;
        }

        return rcsAction.setVolume(mInstanceID, "Master", volume);
    }

    /**
     * stop current media(image,audio,video).
     * 
     * @return true, if successful
     */
    public boolean stop() {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }
        return avtAction.stop(mInstanceID);
    }

    /**
     * Record.
     * 
     * @return true, if successful
     */
    public boolean record() {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }
        return avtAction.record(mInstanceID);
    }

    /**
     * Gets the current state.
     * 
     * @return the current state
     */
    public String getCurrentState() {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return null;
        }

        ArgumentList argList = avtAction.getTransportInfo(mInstanceID);
        if (argList != null) {
            Argument arg = argList
                    .getArgument(Upnp.AVTSArgVariable.GetTransportInfo.AVTS_VARIABLE_OUT_CURRENT_TRANSPORT_STATE);
            if (null != arg) {
                return arg.getValue();
            }
        }
        return null;
    }

}
