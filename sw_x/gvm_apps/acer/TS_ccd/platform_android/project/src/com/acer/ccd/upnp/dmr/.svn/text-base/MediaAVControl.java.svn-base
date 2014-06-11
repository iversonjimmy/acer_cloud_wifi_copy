/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: MediaAVControl.java
 *
 *	Revision:
 *
 *	2011-3-22
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.dmr;

import org.cybergarage.upnp.Argument;
import org.cybergarage.upnp.ArgumentList;
import org.cybergarage.upnp.Device;

import com.acer.ccd.upnp.action.AVTAction;
import com.acer.ccd.upnp.util.Upnp;
import com.acer.ccd.upnp.util.UpnpTool;

/**
 * The Class MediaAVControl.
 */
public class MediaAVControl extends MediaBaseControl {

    /**
     * MediaAVControl action.
     * 
     * @param dev the dev
     * @param InstanceID the instance id
     */
    public MediaAVControl(Device dev, int InstanceID) {
        super(dev, InstanceID);
    }

    /**
     * seek current audio(video) position pos: seek position value.
     * 
     * @param pos the pos
     * @return true, if successful
     */
    public boolean seekTo(int pos) {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }

        long duration = duration();
        if (pos > duration) {
            return false;
        }

        long hour = pos / 3600;
        long minute = (pos - (hour * 3600)) / 60;
        long second = pos % 60;
        StringBuffer time = new StringBuffer();
        time.append(hour);
        time.append(":");
        if (minute < 10) {
            time.append("0");
            time.append(minute);
        } else {
            time.append(minute);
        }
        time.append(":");
        if (second < 10) {
            time.append("0");
            time.append(second);
        } else {
            time.append(second);
        }
        return avtAction.seek(mInstanceID, "REL_TIME", time.toString());
    }

    /**
     * get current audio(video) position return: position.
     * 
     * @return the long
     */
    public long position() {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return -1;
        }

        ArgumentList outList = avtAction.getPositionInfo(mInstanceID);
        if (outList == null) {
            return -1;
        }

        Argument arg = outList.getArgument(Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_OUT_REL_TIME);
        if (arg == null) {
            return -1;
        }
        return UpnpTool.getTimeSecond(arg.getValue());
    }

    /**
     * get current audio(video) duration return: duration.
     * 
     * @return the long
     */
    public long duration() {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return -1;
        }

        ArgumentList outList = avtAction.getPositionInfo(mInstanceID);
        if (outList == null) {
            return -1;
        }

        Argument arg = outList.getArgument(Upnp.AVTSArgVariable.GetPositionInfo.AVTS_VARIABLE_OUT_TRACK_DURATION);
        if (arg == null) {
            return -1;
        }

        return UpnpTool.getTimeSecond(arg.getValue());
    }

    /**
     * get current audio(video) is playing return: true: playing, false: no
     * play.
     * 
     * @return true, if is playing
     */
    public boolean isPlaying() {
        AVTAction avtAction = new AVTAction(mDevice);
        if (avtAction == null) {
            return false;
        }

        ArgumentList outList = avtAction.getTransportInfo(mInstanceID);
        if (outList == null) {
            return false;
        }

        Argument arg = outList
                .getArgument(Upnp.AVTSArgVariable.GetTransportInfo.AVTS_VARIABLE_OUT_CURRENT_TRANSPORT_STATE);
        if (arg == null) {
            return false;
        }

        if (!arg.getValue().toUpperCase().equals("PLAYING")) {
            return false;
        }
        return true;
    }

    /**
     * get current audio(video) Buffer Percentage return: Percentage.
     * 
     * @return the buffer percentage
     */
    public int getBufferPercentage() {
        return 0;
    }
}
