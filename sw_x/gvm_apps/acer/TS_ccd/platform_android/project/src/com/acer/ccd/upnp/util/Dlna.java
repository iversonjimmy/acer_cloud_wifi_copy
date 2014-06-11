/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: DlnaService.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.util;

// TODO: Auto-generated Javadoc
/**
 * The Class DlnaService.
 * 
 * @author chaozhong li
 */
public class Dlna {

    /**
     * The Interface common.
     */
    public interface common {

        /** The Constant UNKNOWN. */
        public final static String UNKNOWN = "unknown";
    }

    /**
     * The Interface DeviceType.
     */
    public interface DeviceType {

        /** The Constant DMS. */
        public final static String DLNA_DEVICE_TYPE_DMS = "DMS";

        /** The Constant DMR. */
        public final static String DLNA_DEVICE_TYPE_DMR = "DMR";

        /** The Constant DMSDMR. */
        public final static String DLNA_DEVICE_TYPE_DMSDMR = "DMSDMR";

        /** The Constant UNKNOWN. */
        public final static String DLNA_DEVICE_TYPE_UNKNOWN = "UNKNOWN";
    }

    /**
     * The Interface DeviceID.
     */
    public interface DeviceID {

        /** The Constant acerDevice. */
        public final static int DLNA_DEVICE_ID_ACER = 0;

        /** The Constant msDevice. */
        public final static int DLNA_DEVICE_ID_MS = 1;

        /** The Constant notAcerDevice. */
        public final static int DLNA_DEVICE_ID_NO_ACER = 2;
        
        /** The Constant clearfi15Device */
        public final static int DLNA_DEVICE_ID_CLEARFI15 = 3;

        /** The Constant clearfi15Device */
        public final static int DLNA_DEVICE_ID_CLEARFI15L = 4;

        /** The Constant clearfi10Device */
        public final static int DLNA_DEVICE_ID_CLEARFI10 = 5;
    }

    /**
     * The Interface DmrState.
     */
    public interface DmrState {

        /** The Constant unKnown. */
        public final static int DMR_STATE_UNKNOWN = -1;

        /** The Constant STOPPED. */
        public final static int DMR_STATE_STOPPED = 0;

        /** The Constant PLAYING. */
        public final static int DMR_STATE_PLAYING = 1;

        /** The Constant TRANSITIONING. */
        public final static int DMR_STATE_TRANSITIONING = 2;

        /** The Constant PAUSED_PLAYBACK. */
        public final static int DMR_STATE_PAUSED_PLAYBACK = 3;

        /** The Constant RECORDING. */
        public final static int DMR_STATE_RECORDING = 4;

        /** The Constant PAUSED_RECORDING. */
        public final static int DMR_STATE_PAUSED_RECORDING = 5;

        /** The Constant NO_MEDIA_PRESENT. */
        public final static int DMR_STATE_NO_MEDIA_PRESENT = 6;

    }
}
