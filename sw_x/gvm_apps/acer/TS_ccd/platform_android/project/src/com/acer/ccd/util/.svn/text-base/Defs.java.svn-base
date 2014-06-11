package com.acer.ccd.util;

import com.acer.ccd.upnp.util.CBCommand;
import com.acer.ccd.upnp.util.Dlna;

/**
 *  Global definition class
 *  
 *  Other comment terms, used in comment
 *      TBD        To be developed
 *      TBF        To be fixed
 *      TBV        To be verified
 *      XXX        mark for review later
 *      DEBUG    For debug, remove these codes later
 *      TODO    To do
 */
public final class Defs {
    private Defs() { };

    // Feature control
    /** Don't use external cache forever */
    public static final boolean USE_ONLY_INTERNAL = false;
    /** Default vibration duration */
    public static final int VIBRATE_DURATION = 35;
    /** Dolby */
    public static final boolean WITH_DOLBY = true;
    /* 
     * Media view grid view column numbers
     */
    public static final int MEDIAVIEW_COLUMN_NUMBER_PHOTO = 3;
    public static final int MEDIAVIEW_COLUMN_NUMBER_PHOTO_A5 = 3;
    public static final int MEDIAVIEW_COLUMN_NUMBER_MUSIC = 3;
    public static final int MEDIAVIEW_COLUMN_NUMBER_MUSIC_A5 = 3;
    public static final int MEDIAVIEW_COLUMN_NUMBER_VIDEO = 2;
    public static final int MEDIAVIEW_COLUMN_NUMBER_VIDEO_A5 = 2;

    /** Background player height
     *      hard coded, it must be the same as layout/backgroundplayer.xml
     */
    public static final int BGP_HEIGHT = 30;

    /* Player status */
    public static final int PLAYER_STATUS_NORMAL = 0;
    public static final int PLAYER_STATUS_PLAYING = 1;
    public static final int PLAYER_STATUS_SAVING = 2;

    public static final int SERVER_TYPE_EMPTY = 0;
    public static final int SERVER_TYPE_PHONE = 1;
    public static final int SERVER_TYPE_PC = 2;
    public static final int SERVER_TYPE_TABLET = 3;

    /** Animation duration for tab indication */
    public static final int ANI_DURATION = 500;
    /** Disable tab switch for a little longer than animation duration */
    public static final int SWITCH_DURATION = ANI_DURATION + 100;

    /* Page number should start from 0 */
    public static final int PAGE_SERVER = 0;
    public static final int PAGE_PLAYER = 1;
    public static final int DEFAULT_SERVER_PAGE = PAGE_SERVER;

    public static final int PAGE_INVALID = -1;
    public static final int PAGE_PHOTO = 0;
    public static final int PAGE_MUSIC = 1;
    public static final int PAGE_VIDEO = 2;

    public static final int DEFAULT_MEDIA_PAGE = PAGE_PHOTO;
    
    public static final String PATH_DIVIDER = "/ ";
    public static final String PATH_LPAYLIST = "/ Play Lists ";

    //Define column name for Search results
    public static final String SEARCH_FLAG = "Flag";
    public static final String SEARCH_ICON = "Icon";
    public static final String SEARCH_NAME = "ItemName";
    public static final String SEARCH_GROUPID = "ItemGroupID";
    public static final String SEARCH_DURATION = "ItemDuration";
    public static final String SEARCH_DBID = "SearchDBId";

    public static final float SORT_MENU_ICON_SIZE = 48; // dp
    public static final float DIALOG_LIST_MIN_HEIGHT = 72;

    public static final long TIME_EACH_SLIDE   = 5000; // Photo DMC Slideshow time length

    /**
     * Constant wrapper from DLNA service layer
     */
    public static final class Callback {
        private Callback() { }
        // These are defined in com.acer.clearfi.util.CBCommand.DevNotifyID
        public static final int DEVICE_ADD    = CBCommand.DevNotifyID.ADD_DEVICE;
        public static final int DEVICE_REMOVE = CBCommand.DevNotifyID.REMOVE_DEVICE;
        public static final int DEVICE_SEARCH = CBCommand.DevNotifyID.SEARCH_DEVICE;

        // These are defined in com.acer.clearfi.util.CBCommand.DMSActionID
        public static final int ACTION_BROWSE      = CBCommand.DMSActionID.BROWSE_ACTION;
        public static final int ACTION_SEARCH      = CBCommand.DMSActionID.SEARCH_ACTION;
        public static final int ACTION_STOPBROWSE  = CBCommand.DMSActionID.STOP_BROWSE_ACTION;
        public static final int ACTION_UPDATEALBUM = CBCommand.DMSActionID.UPDATE_CONTAINER_ALBUM;

        /**
         * Defined for DMC operation
         */
        public static final int DMC_ATTACH        = CBCommand.DMRActionID.ATTACH;
        public static final int DMC_DETACH        = CBCommand.DMRActionID.DETACH;
        public static final int DMC_AV_SET_VOLUME = CBCommand.DMRActionID.SET_AV_VOLUME;
        public static final int DMC_AV_SET_MUTE   = CBCommand.DMRActionID.SET_AV_MUTE;
        public static final int DMC_AV_SEEKTO     = CBCommand.DMRActionID.SEEK_TO;
        public static final int DMC_AV_SEEKTO_TRACK = CBCommand.DMRActionID.AV_SEEKTO_TRACK;
        public static final int DMC_AV_POSITION   = CBCommand.DMRActionID.AV_POSITION;
        public static final int DMC_AV_OPEN       = CBCommand.DMRActionID.OPEN_AV;
        public static final int DMC_AV_ADD_NEXT   = CBCommand.DMRActionID.ADD_NEXT_AV;
        public static final int DMC_AV_PLAY_NEXT  = CBCommand.DMRActionID.PLAY_NEXT_AV;
        public static final int DMC_AV_PLAY_PREV  = CBCommand.DMRActionID.PLAY_PREV_AV;
        public static final int DMC_AV_PLAY       = CBCommand.DMRActionID.PLAY_AV;
        public static final int DMC_AV_PAUSE      = CBCommand.DMRActionID.PAUSE_AV;
        public static final int DMC_AV_STOP       = CBCommand.DMRActionID.STOP_AV;
        public static final int DMC_PHO_OPEN      = CBCommand.DMRActionID.OPEN_PHOTO;
        public static final int DMC_PHO_ADD_NEXT  = CBCommand.DMRActionID.ADD_NEXT_PHOTO;
        public static final int DMC_PHO_PLAY_NEXT = CBCommand.DMRActionID.PLAY_NEXT_PHOTO;
        public static final int DMC_PHO_PLAY_PREV = CBCommand.DMRActionID.PLAY_PREV_PHOTO;
        public static final int DMC_PHO_PLAY      = CBCommand.DMRActionID.PLAY_PHOTO;
        public static final int DMC_PHO_PUASE     = CBCommand.DMRActionID.PAUSE_PHOTO;
        public static final int DMC_PHO_STOP      = CBCommand.DMRActionID.STOP_PHOTO;
        public static final int DMC_PHO_SEEKTO    = CBCommand.DMRActionID.SEEK_TO_PHOTO;
        public static final int DMR_RECORD        = CBCommand.DMRActionID.RECORD;
        public static final int DMR_STATUS        = CBCommand.DMRActionID.DMR_STATUS;

        public static final int DMR_AVT_STOPPED          = CBCommand.DMRActionID.STOPPED;
        public static final int DMR_AVT_PLAYING          = CBCommand.DMRActionID.PLAYING;
        public static final int DMR_AVT_TRANSITIONING    = CBCommand.DMRActionID.TRANSITIONING;
        public static final int DMR_AVT_PAUSED_PLAYBACK  = CBCommand.DMRActionID.PAUSED_PLAYBACK;
        public static final int DMR_AVT_RECORDING        = CBCommand.DMRActionID.RECORDING;
        public static final int DMR_AVT_PAUSED_RECORDING = CBCommand.DMRActionID.PAUSED_RECORDING;
        public static final int DMR_AVT_NO_MEDIA_PRESENT = CBCommand.DMRActionID.NO_MEDIA_PRESENT;

        public static final int DMR_STATE_STOPPED          = Dlna.DmrState.DMR_STATE_STOPPED;
        public static final int DMR_STATE_PLAYING          = Dlna.DmrState.DMR_STATE_PLAYING;
        public static final int DMR_STATE_TRANSITIONING    = Dlna.DmrState.DMR_STATE_TRANSITIONING;
        public static final int DMR_STATE_PAUSED_PLAYBACK  = Dlna.DmrState.DMR_STATE_PAUSED_PLAYBACK;
        public static final int DMR_STATE_RECORDING        = Dlna.DmrState.DMR_STATE_RECORDING;
        public static final int DMR_STATE_PAUSED_RECORDING = Dlna.DmrState.DMR_STATE_PAUSED_RECORDING;
        public static final int DMR_STATE_NO_MEDIA_PRESENT = Dlna.DmrState.DMR_STATE_NO_MEDIA_PRESENT;

        /**
         * Defined for DMC error code operation
         */
        public static final int DMR_RESPONSE_OK            = CBCommand.ErrorID.OK;
        public static final int DMR_RESPONSE_NULL          = CBCommand.ErrorID.NULL_POINTER;
        public static final int DMR_RESPONSE_UNINIT        = CBCommand.ErrorID.UNINITIALIZED;
        public static final int DMR_RESPONSE_ARG_ERROR     = CBCommand.ErrorID.ARGUMENT_ERROR;
        public static final int DMR_RESPONSE_DEV_DISAPPEAR = CBCommand.ErrorID.DEVICE_DISAPPEAR;
        public static final int DMR_RESPONSE_PROTOCOL_ERR  = CBCommand.ErrorID.PROTOCOL_ERROR;
        public static final int DMR_RESPONSE_RECORDING     = CBCommand.ErrorID.RECORDING;
        public static final int DMR_RESPONSE_DBERROR       = CBCommand.ErrorID.DB_ERROR;
    }
    /*
     * Define Manufactures whose url has no subfile name 
     */
    public static final String MANUFACTURE_MICROCOFT = "Microsoft Corporation";
    public static final String MANUFACTURE_SKIFTA = "Skifta";
    /*
     * Device ID definition
     */
    public static final int DEVICE_MANUFACTURER_ACER = Dlna.DeviceID.DLNA_DEVICE_ID_ACER;
    public static final int DEVICE_MANUFACTURER_MS = Dlna.DeviceID.DLNA_DEVICE_ID_MS;
    public static final int DEVICE_MANUFACTURER_OTHER = Dlna.DeviceID.DLNA_DEVICE_ID_NO_ACER;
    public static final int DEVICE_MANUFACTURER_CLCLEARFI15 = Dlna.DeviceID.DLNA_DEVICE_ID_CLEARFI15;
    public static final int DEVICE_MANUFACTURER_CLCLEARFI15L = Dlna.DeviceID.DLNA_DEVICE_ID_CLEARFI15L;
    public static final int DEVICE_MANUFACTURER_CLCLEARFI10 = Dlna.DeviceID.DLNA_DEVICE_ID_CLEARFI10;

    /** Object Id for default container or sorting entries */
    public static final class ObjectId {
        private ObjectId() { }

        /** Container that includes everything */
        public static final String ALL_ROOT = "0";
        /* Photo related container */
        public static final String PHOTO_ROOT = "3";
        public static final String PHOTO_ALL = "B";
        public static final String PHOTO_TIMELINE = "C";
        public static final String PHOTO_ALBUM = "D";
        public static final String PHOTO_FOLDER = "16";

        /* Music related container */
        public static final String MUSIC_ROOT = "1";
        public static final String MUSIC_ALL = "4";
        public static final String MUSIC_GENRE = "5";
        public static final String MUSIC_ARTIST = "6";
        public static final String MUSIC_ALBUM = "7";
        /* Video related container */
        public static final String VIDEO_ROOT = "2";
        public static final String VIDEO_ALL = "8";
        public static final String VIDEO_ARTIST = "A";
        public static final String VIDEO_ALBUM = "15";

        /* Default category */
        public static final String DEFAULT_PHOTO = PHOTO_ALBUM;
        public static final String DEFAULT_MUSIC = MUSIC_ALBUM;
        public static final String DEFAULT_VIDEO = VIDEO_ALBUM;

        public static final String DEFAULT_MS_PHOTO = PHOTO_FOLDER;
    }

    /* Sort Dialog related*/
    public static final int SORT_PHOTO_ALBUM = 0;
    public static final int SORT_PHOTO_TIMELINE = 1;
    public static final int SORT_MUSIC_ALBUM = 0;
    public static final int SORT_MUSIC_ARTIST = 1;
    public static final int SORT_MUSIC_GENRE = 2;
    public static final int SORT_MUSIC_PLAYLIST = 3;
    public static final int SORT_VIDEO_ALBUM = 0;

    /**
     * Extra name used for intent
     */
    public static final String EXTRA_MEDIA_DB_IDS = "com.acer.android.clearfi.extra.MEDIA_DB_IDS";
    public static final String EXTRA_MEDIA_ITEM_DB_ID = "com.acer.android.clearfi.extra.ITEM_DB_ID";
    public static final String EXTRA_MEDIA_ITEM_POS = "com.acer.android.clearfi.extra.ITEM_POS";
    /** specify if launched from select mode */
    public static final String EXTRA_MEDIA_SELECT_MODE = "com.acer.android.clearfi.extra.SELECT_MODE";
    /** used for DMP's playlist */
    public static final String EXTRA_MEDIA_SORTING_CRITERION = "com.acer.android.clearfi.extra.SORTING_CRITERION";
    /** identify DB tableID for DMP */
    public static final String EXTRA_MEDIA_DB_TBL_ID = "com.acer.android.clearfi.extra.DB_TBL_ID";
    /** identify playlist's entry type to Music DMP */
    public static final String EXTRA_MUSIC_PLAYLIST_ENTRY_TYPE = "com.acer.android.clearfi.extra.MUSIC_PLAYLIST_ENTRY_TYPE";
    public static final String EXTRA_MUSIC_PLAYLIST_ID = "com.acer.android.clearfi.extra.MUSIC_PLAYLIST_ID";
    public static final String EXTRA_MUSIC_TRACK = "com.acer.android.clearfi.extra.MUSIC_TRACK";
    public static final String EXTRA_MUSIC_ALBUM = "com.acer.android.clearfi.extra.MUSIC_ALBUM";
    public static final String EXTRA_MUSIC_ARTIST = "com.acer.android.clearfi.extra.MUSIC_ARTIST";
    public static final String EXTRA_MUSIC_ISPLAYING = "com.acer.android.clearfi.extra.MUSIC_ISPLAYING";

    // Argument passed to MediaViewActivity
    public static final String EXTRA_DEVICE_NAME = "com.acer.android.clearfi.extra.DeviceName";
    public static final String EXTRA_DEVICE_ID = "com.acer.android.clearfi.extra.DeviceID";
    public static final String EXTRA_DEVICE_MANUFACTURER = "com.acer.android.clearfi.extra.DeviceManufacturer";
    /** which media category, photo / music / video */
    public static final String EXTRA_DEVICE_CATEGORY = "com.acer.android.clearfi.extra.DeviceCategory";

    // Arguments to specify which media types are passed
    public static final String EXTRA_MEDIA_TYPE = "com.acer.android.clearfi.extra.MEDIA_TYPE";
    public static final int TYPE_IMAGE = 0;
    public static final int TYPE_AUDIO = 1;
    public static final int TYPE_VIDEO = 2;

    // Argument passed to PhotoPlayer
    public static final String EXTRA_PHOTO_OPEN_TYPE = "com.acer.android.clearfi.extra.PHOTO_OPEN_TYPE";
    public static final int TYPE_FROM_CONTENTS = 0;
    public static final int TYPE_FROM_SEARCH = 1;
    public static final int TYPE_FROM_SEARCH_ALBUM = 2;
    public static final long MAX_PHOTO_EXTERNAL_CACHE_SIZE = (15 * 1024 * 1024);
    public static final long MAX_PHOTO_INTERNAL_CACHE_SIZE = (5 * 1024 * 1024);

    // Argument passed to Search
    public static final String EXTRA_SEARCH_DEVICE_TYPE = "com.acer.android.clearfi.extra.SEARCH_DEVICE_TYPE";
    public static final String EXTRA_SEARCH_DEVICE_UUID = "com.acer.android.clearfi.extra.DEVICE_UUID";
    public static final String EXTRA_SEARCH_DEVICE_NAME = "com.acer.android.clearfi.extra.DEVICE_NAME";
    public static final int SEARCH_TYPE_ALL_DEVICE = 0;
    public static final int SEARCH_TYPE_ONE_DEVICE = 1;

    // Pass to Controller to specify DMS and DMR uuids
    public static final String EXTRA_DMS_UUID = "com.acer.android.clearfi.extra.DMS_UUID";
    public static final String EXTRA_DMR_UUID = "com.acer.android.clearfi.extra.DMR_UUID";
    public static final String UUID_UNKNOWN = "unknown";
    /*
     * Definition for device type used in DBManager.getDevice()
     */
    public static final int DEVICE_TYPE_DMS = 1;
    public static final int DEVICE_TYPE_DMR = 2;

    /**
     *  The flag is used to identify playlist's entry type to Music DMP
     */
    public static final class PlaylistEntryType {
        private PlaylistEntryType() { }
        public static final int MUSIC_SINGLE_PLAYLIST = 1;
        public static final int MUSIC_MULTIPLE_PLAYLIST = 2;
    }

    public static final String EXTRA_MODE = "com.acer.android.clearfi.extra.MODE";
    // The constant to indicate what action we should take
    public static final int MODE_PLAYTO = 0;
    public static final int MODE_SAVETO = 1;

    public static final String PREFERENCE_NAME = "ClearfiDmpDmc";
    public static final String PREF_REPEAT = "repeat";
    public static final String PREF_SHUFFLE = "shuffle";
    
    public static final String PTAG = "ClearfiPerformance";
    public static final String FTAG = "FilterDebug";

    /**
     * Broadcast Action:  Request the music player service to update now-playing info.
     */
    public static final String ACTION_ACTIVATE_NOWPLAYING = "com.acer.android.clearfi.action.ACTIVATE_NOWPLAYING";
    
    public static final int TIME_NS_TO_MS = 1000000;

    public static final long SEARCH_DEVICE_TIMEOUT_MS = 20*1000;
    public static final long SEARCH_DEVICE_RETRY = 2;
}
