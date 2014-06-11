/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: CBCommand.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.dx.util;

// TODO: Auto-generated Javadoc
/**
 * The Class CBCommand.
 * 
 * @author chaozhong li
 */
public class CBCommand {

    /**
     * The Interface DevNotifyID.
     */
    public interface DevNotifyID {
        // public final static int DevNotifyID = 10000;
        /** The Constant addDevice. */
        public final static int ADD_DEVICE = 10000;

        /** The Constant removeDevice. */
        public final static int REMOVE_DEVICE = 10001;

        /** The Constant searchDevice. */
        public final static int SEARCH_DEVICE = 10002;
    }

    /**
     * The Interface DMSActionID.
     */
    public interface DMSActionID {
        // public final static int DMSActionID = 20000;
        /** The Constant browseAction. */
        public final static int BROWSE_ACTION = 20000;

        /** The Constant browseAction. */
        public final static int STOP_BROWSE_ACTION = 20001;

        /** The Constant searchAction. */
        public final static int SEARCH_ACTION = 21001;

        /** The Constant searchAction. */
        public final static int STOP_SEARCH_ACTION = 21002;

        /** The Constant updateContainerAlbum. */
        public final static int UPDATE_CONTAINER_ALBUM = 22001;
    }

    /**
     * The Interface DMRActionID.
     */
    public interface DMRActionID {
        // public final static int DMRActionID = 30000;
        /** The Constant attach. */
        public final static int ATTACH = 30000;

        /** The Constant detach. */
        public final static int DETACH = 30001;

        /** The Constant setAvVolume. */
        public final static int SET_AV_VOLUME = 30002;

        /** The Constant setAvMute. */
        public final static int SET_AV_MUTE = 30003;

        /** The Constant seekTo. */
        public final static int SEEK_TO = 30004;
        
        /** The Constant seekTo. */
        public final static int AV_SEEKTO_TRACK = 30005;
        
        /** The Constant AV_POSITION */
        public final static int AV_POSITION = 30006;

        /** The Constant seekToPhoto */
        public final static int SEEK_TO_PHOTO = 30007;

        /** The Constant openPhoto. */
        public final static int OPEN_PHOTO = 30008;

        /** The Constant openAv. */
        public final static int OPEN_AV = 30009;

        /** The Constant addNextPhoto. */
        public final static int ADD_NEXT_PHOTO = 30010;

        /** The Constant addNextAv. */
        public final static int ADD_NEXT_AV = 30011;

        /** The Constant playNextPhoto. */
        public final static int PLAY_NEXT_PHOTO = 30012;

        /** The Constant playNextAv. */
        public final static int PLAY_NEXT_AV = 30013;

        /** The Constant playPrevPhoto. */
        public final static int PLAY_PREV_PHOTO = 30014;

        /** The Constant playPrevAv. */
        public final static int PLAY_PREV_AV = 30015;

        /** The Constant playPhoto. */
        public final static int PLAY_PHOTO = 30016;

        /** The Constant playAv. */
        public final static int PLAY_AV = 30017;

        /** The Constant pausePhoto. */
        public final static int PAUSE_PHOTO = 30018;

        /** The Constant pauseAv. */
        public final static int PAUSE_AV = 30019;

        /** The Constant stopPhoto. */
        public final static int STOP_PHOTO = 30020;

        /** The Constant stopAv. */
        public final static int STOP_AV = 30021;

        /** The Constant record. */
        public final static int RECORD = 30022;

        /** The Constant dmrStatus. */
        public final static int DMR_STATUS = 30023;
        
        public static final int STOPPED = 30024;
        public static final int PLAYING = 30025;
        public static final int TRANSITIONING = 30026;
        public static final int PAUSED_PLAYBACK = 30027;
        public static final int PAUSED_RECORDING = 30028;
        public static final int RECORDING = 30029;
        public static final int NO_MEDIA_PRESENT = 30030;
        public static final int CURRENT_TRACK = 31000;
    }

    /**
     * The Interface ErrorID.
     */
    public interface ErrorID {

        /** The Constant ok. */
        public final static int OK = 40000;

        /** The Constant nullPointer. */
        public final static int NULL_POINTER = 40001;

        /** The Constant unInitialized. */
        public final static int UNINITIALIZED = 40002;

        /** The Constant argumentError. */
        public final static int ARGUMENT_ERROR = 40003;

        /** The Constant deviceDisappear. */
        public final static int DEVICE_DISAPPEAR = 40004;

        /** The Constant protocolError. */
        public final static int PROTOCOL_ERROR = 40005;

        /** The Constant record. */
        public final static int RECORDING = 40006;

        /** The Constant dbError. */
        public final static int DB_ERROR = 40007;
    }
    
    
}
