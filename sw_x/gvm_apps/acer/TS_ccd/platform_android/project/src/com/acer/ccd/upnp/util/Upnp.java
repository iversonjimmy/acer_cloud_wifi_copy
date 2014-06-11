/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: Upnp.java
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
 * The Class Upnp.
 * 
 * @author chaozhong li
 */
public class Upnp {

    /**
     * The Interface common.
     */
    public interface Common {

        /** The Constant Uuid. */
        public final static String COMMON_UUID = "uuid";

        /** The Constant dmsUuid. */
        public final static String COMMON_DMS_UUID = "dmsUuid";

        /** The Constant dmrUuid. */
        public final static String COMMON_DMR_UUID = "dmrUuid";

        /** The Constant tableId. */
        public final static String COMMON_TABLE_ID = "tableId";

        /** The Constant updateAlbum. */
        public final static String COMMON_UPDATE_ALBUM = "updateAlbum";

        /** The Constant command. */
        public final static String COMMON_COMMAND = "command";

        /** The Constant url. */
        public final static String COMMON_URL = "url";

        /** The Constant recordingLoopTime. */
        public final static long COMMON_RECORDING_LOOP_TIME = 2000; // 2seconds

        /** The Constant acerDeviceFlag. */
        public final static String COMMON_ACER_DEVICE_FLAG = "ACER";

        /** The Constant msDeviceFlag. */
        public final static String COMMON_MS_DEVICE_FLAG = "MICROSOFT";

        public static final String COMMON_ACER_MODLE_NAME = "AcerDLNA2.0";

        /** The Constant clearfi15DeviceFlag */
        public final static String COMMON_CLEARFI15_DEVICE_FLAG = "Acer clear.fi";
        
        /** The Constant clearfi15lDeviceFlag */
        public final static String COMMON_CLEARFI15L_DEVICE_FLAG = "CyberLink SoftDMA Lite";

        /** The Constant clearfi10DeviceFlag */
        public final static String COMMON_CLEARFI10_DEVICE_FLAG = "CyberLink SoftDMA";
    }

    /**
     * The Interface Service.
     */
    public interface Service {

        /** The Constant CDS1. */
        public final static String SERVICE_CDS1 = "urn:schemas-upnp-org:service:ContentDirectory:1";

        /** The Constant CDS2. */
        public final static String SERVICE_CDS2 = "urn:schemas-upnp-org:service:ContentDirectory:2";

        /** The Constant CDS3. */
        public final static String SERVICE_CDS3 = "urn:schemas-upnp-org:service:ContentDirectory:3";

        /** The Constant CMS1. */
        public final static String SERVICE_CMS1 = "urn:schemas-upnp-org:service:ConnectionManager:1";

        /** The Constant CMS2. */
        public final static String SERVICE_CMS2 = "urn:schemas-upnp-org:service:ConnectionManager:2";

        /** The Constant AVTS1. */
        public final static String SERVICE_AVTS1 = "urn:schemas-upnp-org:service:AVTransport:1";

        /** The Constant AVTS2. */
        public final static String SERVICE_AVTS2 = "urn:schemas-upnp-org:service:AVTransport:2";

        /** The Constant RCS1. */
        public final static String SERVICE_RCS1 = "urn:schemas-upnp-org:service:RenderingControl:1";

        /** The Constant RCS2. */
        public final static String SERVICE_RCS2 = "urn:schemas-upnp-org:service:RenderingControl:2";
    }

    /**
     * The Interface CDSAction.
     */
    public interface CDSAction {

        /** The Constant GetSearchCapabilities. */
        public final static String CDS_ACTION_GET_SEARCH_CAPABILITIES = "GetSearchCapabilities";

        /** The Constant GetSortCapabilities. */
        public final static String CDS_ACTION_GET_SORT_CAPABILITIES = "GetSortCapabilities";

        /** The Constant GetSortExtensionCapabilities. */
        public final static String CDS_ACTION_GET_SORT_EXTENSION_CAPABILITIES = "GetSortExtensionCapabilities";

        /** The Constant GetFeatureList. */
        public final static String CDS_ACTION_GET_FEATURE_LIST = "GetFeatureList";

        /** The Constant GetSystemUpdateID. */
        public final static String CDS_ACTION_GET_SYSTEM_UPDATE_ID = "GetSystemUpdateID";

        /** The Constant Browse. */
        public final static String CDS_ACTION_BROWSE = "Browse";

        /** The Constant Search. */
        public final static String CDS_ACTION_SEARCH = "Search";

        /** The Constant CreateObject. */
        public final static String CDS_ACTION_CREATE_OBJECT = "CreateObject";

        /** The Constant DestroyObject. */
        public final static String CDS_ACTION_DESTROY_OBJECT = "DestroyObject";

        /** The Constant UpdateObject. */
        public final static String CDS_ACTION_UPDATE_OBJECT = "UpdateObject";

        /** The Constant MoveObject. */
        public final static String CDS_ACTION_MOVE_OBJECT = "MoveObject";

        /** The Constant ImportResource. */
        public final static String CDS_ACTION_IMPORT_RESOURCE = "ImportResource";

        /** The Constant ExportResource. */
        public final static String CDS_ACTION_EXPORT_RESOURCE = "ExportResource";

        /** The Constant DeleteResource. */
        public final static String CDS_ACTION_DELETE_RESOURCE = "DeleteResource";

        /** The Constant StopTransferResource. */
        public final static String CDS_ACTION_STOP_TRANSFER_RESOURCE = "StopTransferResource";

        /** The Constant GetTransferProgress. */
        public final static String CDS_ACTION_GET_TRANSFER_PROGRESS = "GetTransferProgress";

        /** The Constant CreateReference. */
        public final static String CDS_ACTION_CREATE_REFERENCE = "CreateReference";
    }

    /**
     * The Interface CDSArgVariable.
     */
    public interface CDSArgVariable {

        /**
         * The Interface GetSearchCapabilities.
         */
        public interface GetSearchCapabilities {

            /** The Constant OutSearchCaps. */
            public final static String CDS_VARIABLE_OUT_SEARCH_CAPS = "SearchCaps";
        }

        /**
         * The Interface GetSortCapabilities.
         */
        public interface GetSortCapabilities {

            /** The Constant OutSortCaps. */
            public final static String CDS_VARIABLE_OUT_SORT_CAPS = "SortCaps";
        }

        /**
         * The Interface GetSortExtensionCapabilities.
         */
        public interface GetSortExtensionCapabilities {

            /** The Constant OutSortExtensionCaps. */
            public final static String CDS_VARIABLE_OUT_SORT_EXTENSION_CAPS = "SortExtensionCaps";
        }

        /**
         * The Interface GetFeatureList.
         */
        public interface GetFeatureList {

            /** The Constant OutFeatureList. */
            public final static String CDS_VARIABLE_OUT_FEATURE_LIST = "FeatureList";
        }

        /**
         * The Interface GetSystemUpdateID.
         */
        public interface GetSystemUpdateID {

            /** The Constant OutId. */
            public final static String CDS_VARIABLE_OUT_ID = "Id";
        }

        /**
         * The Interface Browse.
         */
        public interface Browse {

            /** The Constant InObjectID. */
            public final static String CDS_VARIABLE_IN_OBJECT_ID = "ObjectID";

            /** The Constant InBrowseFlag. */
            public final static String CDS_VARIABLE_IN_BROWSE_FLAG = "BrowseFlag";

            /** The Constant BrowseMetadata. */
            public final static String CDS_VARIABLE_BROWSE_META_DATA = "BrowseMetadata";

            /** The Constant BrowseDirectChildren. */
            public final static String CDS_VARIABLE_BROWSE_DIRECT_CHILDREN = "BrowseDirectChildren";

            /** The Constant InFilter. */
            public final static String CDS_VARIABLE_IN_FILTER = "Filter";

            /** The Constant InStartingIndex. */
            public final static String CDS_VARIABLE_IN_STARTING_INDEX = "StartingIndex";

            /** The Constant InRequestedCount. */
            public final static String CDS_VARIABLE_IN_REQUESTED_COUNT = "RequestedCount";

            /** The Constant InSortCriteria. */
            public final static String CDS_VARIABLE_IN_SORT_CRITERIA = "SortCriteria";

            /** The Constant OutResult. */
            public final static String CDS_VARIABLE_OUT_RESULT = "Result";

            /** The Constant OutNumberReturned. */
            public final static String CDS_VARIABLE_OUT_NUMBER_RETURNED = "NumberReturned";

            /** The Constant OutTotalMatches. */
            public final static String CDS_VARIABLE_OUT_TOTAL_MATCHES = "TotalMatches";

            /** The Constant OutUpdateID. */
            public final static String CDS_VARIABLE_OUT_UPDATE_ID = "UpdateID";
        }

        /**
         * The Interface Search.
         */
        public interface Search {

            /** The Constant InContainerID. */
            public final static String CDS_VARIABLE_IN_CONTAINER_ID = "ContainerID";

            /** The Constant InSearchCriteria. */
            public final static String CDS_VARIABLE_IN_SEARCH_CRITERIA = "SearchCriteria";

            /** The Constant InFilter. */
            public final static String CDS_VARIABLE_IN_FILTER = "Filter";

            /** The Constant InStartingIndex. */
            public final static String CDS_VARIABLE_IN_STARTING_INDEX = "StartingIndex";

            /** The Constant InRequestedCount. */
            public final static String CDS_VARIABLE_IN_REQUESTED_COUNT = "RequestedCount";

            /** The Constant InSortCriteria. */
            public final static String CDS_VARIABLE_IN_SORT_CRITERIA = "SortCriteria";

            /** The Constant OutResult. */
            public final static String CDS_VARIABLE_OUT_RESULT = "Result";

            /** The Constant OutNumberReturned. */
            public final static String CDS_VARIABLE__OUT_NUMBER_RETURNED = "NumberReturned";

            /** The Constant OutTotalMatches. */
            public final static String CDS_VARIABLE_OUT_TOTAL_MATCHED = "TotalMatches";

            /** The Constant OutUpdateID. */
            public final static String CDS_VARIABLE_OUT_UPDATE_ID = "UpdateID";
        }

        /**
         * The Interface CreateObject.
         */
        public interface CreateObject {

            /** The Constant InContainerID. */
            public final static String CDS_VARIABLE_IN_CONTAINER_ID = "ContainerID";

            /** The Constant InElements. */
            public final static String CDS_VARIABLE_IN_ELEMENTS = "Elements";

            /** The Constant OutObjectID. */
            public final static String CDS_VARIABLE_OUT_OBJECT_ID = "ObjectID";

            /** The Constant OutResult. */
            public final static String CDS_VARIABLE_OUT_RESULT = "Result";
        }

        /**
         * The Interface DestroyObject.
         */
        public interface DestroyObject {

            /** The Constant InObjectID. */
            public final static String CDS_VARIABLE_IN_OBJECT_ID = "ObjectID";
        }

        /**
         * The Interface UpdateObject.
         */
        public interface UpdateObject {

            /** The Constant InObjectID. */
            public final static String CDS_VARIABLE_IN_OBJECT_ID = "ObjectID";

            /** The Constant InCurrentTagValue. */
            public final static String CDS_VARIABLE_IN_CURRENT_TAG_VALUE = "CurrentTagValue";

            /** The Constant InNewTagValue. */
            public final static String CDS_VARIABLE_IN_NEW_TAG_VALUE = "NewTagValue";
        }

        /**
         * The Interface MoveObject.
         */
        public interface MoveObject {

            /** The Constant InObjectID. */
            public final static String CDS_VARIABLE_IN_OBJECT_ID = "ObjectID";

            /** The Constant InNewParentID. */
            public final static String CDS_VARIABLE_IN_PARENT_ID = "NewParentID";

            /** The Constant OutNewObjectID. */
            public final static String CDS_VARIABLE_OUT_NEW_OBJECT_ID = "NewObjectID";
        }

        /**
         * The Interface ImportResource.
         */
        public interface ImportResource {

            /** The Constant InSourceURI. */
            public final static String CDS_VARIABLE_IN_SOURCE_URI = "SourceURI";

            /** The Constant InDestinationURI. */
            public final static String CDS_VARIABLE_IN_DESTINATION_URI = "DestinationURI";

            /** The Constant OutTransferID. */
            public final static String CDS_VARIABLE_OUT_TRANSFER_ID = "TransferID";
        }

        /**
         * The Interface ExportResource.
         */
        public interface ExportResource {

            /** The Constant InSourceURI. */
            public final static String CDS_VARIABLE_IN_SOURCE_URI = "SourceURI";

            /** The Constant InDestinationURI. */
            public final static String CDS_VARIABLE_IN_DESTINATION_URI = "DestinationURI";

            /** The Constant OutTransferID. */
            public final static String CDS_VARIABLE_OUT_TRANSFER_ID = "TransferID";
        }

        /**
         * The Interface DeleteResource.
         */
        public interface DeleteResource {

            /** The Constant InResourceURI. */
            public final static String CDS_VARIABLE_IN_RESOURCE_URI = "ResourceURI";
        }

        /**
         * The Interface StopTransferResource.
         */
        public interface StopTransferResource {

            /** The Constant InTransferID. */
            public final static String CDS_VARIABLE_IN_TRANSFER_ID = "TransferID";
        }

        /**
         * The Interface GetTransferProgress.
         */
        public interface GetTransferProgress {

            /** The Constant InTransferID. */
            public final static String CDS_VARIABLE_IN_TRANSFER_ID = "TransferID";

            /** The Constant OutTransferStatus. */
            public final static String CDS_VARIABLE_OUT_TRANSFER_STATUS = "TransferStatus";

            /** The Constant OutTransferLength. */
            public final static String CDS_VARIABLE_OUT_TRANSFER_LENGTH = "TransferLength";

            /** The Constant OutTransferTotal. */
            public final static String CDS_VARIABLE_OUT_TRANSFER_TOTAL = "TransferTotal";
        }

        /**
         * The Interface CreateReference.
         */
        public interface CreateReference {

            /** The Constant InContainerID. */
            public final static String CDS_VARIABLE_IN_CONTAINER_ID = "ContainerID";

            /** The Constant InObjectID. */
            public final static String CDS_VARIABLE_IN_OBJECT_ID = "ObjectID";

            /** The Constant OutNewID. */
            public final static String CDS_VARIABLE_OUT_NEW_ID = "NewID";
        }
    }

    /**
     * The Interface CMSAction.
     */
    public interface CMSAction {

        /** The Constant GetProtocolInfo. */
        public final static String CMS_ACTION_GET_PROTOCOL_INFO = "GetProtocolInfo";

        /** The Constant PrepareForConnection. */
        public final static String CMS_ACTION_PREPARE_FOR_CONNECTION = "PrepareForConnection";

        /** The Constant ConnectionComplete. */
        public final static String CMS_ACTION_CONNECTION_COMPLETE = "ConnectionComplete";

        /** The Constant GetCurrentConnectionIDs. */
        public final static String CMS_ACTION_GET_CURRENT_CONNECTION_IDS = "GetCurrentConnectionIDs";

        /** The Constant GetCurrentConnectionInfo. */
        public final static String CMS_ACTION_GET_CURRENT_CONNECTION_INFO = "GetCurrentConnectionInfo";
    }

    /**
     * The Interface CMSArgVariable.
     */
    public interface CMSArgVariable {

        /**
         * The Interface stateVariable.
         */
        public interface stateVariable {

            /** The Constant LastChange. */
            public final static String CMS_VARIABLE_LAST_CHANGE = "LastChange";

            /** The Constant PresetNameList. */
            public final static String CMS_VARIABLE_PRESET_NAME_LIST = "PresetNameList";

            /** The Constant Brightness. */
            public final static String CMS_VARIABLE_BRIGHTNESS = "Brightness";

            /** The Constant Contrast. */
            public final static String CMS_VARIABLE_CONTRAST = "Contrast";

            /** The Constant Sharpness. */
            public final static String CMS_VARIABLE_SHARPNESS = "Sharpness";

            /** The Constant RedVideoGain. */
            public final static String CMS_VARIABLE_RED_VIDEO_GAIN = "RedVideoGain";

            /** The Constant GreenVideoGain. */
            public final static String CMS_VARIABLE_GREEN_VIDEO_GAIN = "GreenVideoGain";

            /** The Constant BlueVideoGain. */
            public final static String CMS_VARIABLE_BLUE_VIDEO_GAIN = "BlueVideoGain";

            /** The Constant RedVideoBlackLevel. */
            public final static String CMS_VARIABLE_RED_VIDEO_BLACK_LEVEL = "RedVideoBlackLevel";

            /** The Constant GreenVideoBlackLevel. */
            public final static String CMS_VARIABLE_GREEN_VIDEO_BALCK_LEVEL = "GreenVideoBlackLevel";

            /** The Constant BlueVideoBlackLevel. */
            public final static String CMS_VARIABLE_BLUE_VIDEO_BLACK_LEVEL = "BlueVideoBlackLevel";

            /** The Constant ColorTemperature. */
            public final static String CMS_VARIABLE_COLOR_TEMPERATURE = "ColorTemperature";

            /** The Constant HorizontalKeystone. */
            public final static String CMS_VARIABLE_HORIZONTAL_KEYSTONE = "HorizontalKeystone";

            /** The Constant VerticalKeystone. */
            public final static String CMS_VARIABLE_VERTICAL_KEYSTONE = "VerticalKeystone";

            /** The Constant Mute. */
            public final static String CMS_VARIABLE_MUTE = "Mute";

            /** The Constant Volume. */
            public final static String CMS_VARIABLE_VOLUME = "Volume";

            /** The Constant VolumeDB. */
            public final static String CMS_VARIABLE_VOLUME_DB = "VolumeDB";

            /** The Constant Loudness. */
            public final static String CMS_VARIABLE_LOUDNESS = "Loudness";

            /** The Constant A_ARG_TYPE_Channel. */
            public final static String CMS_VARIABLE_A_ARG_TYPE_CHANNEL = "A_ARG_TYPE_Channel";

            /** The Constant A_ARG_TYPE_InstanceID. */
            public final static String CMS_VARIABLE_A_ARG_TYPE_INSTANCE_ID = "A_ARG_TYPE_InstanceID";

            /** The Constant A_ARG_TYPE_PresetName. */
            public final static String CMS_VARIABLE_A_ARG_TYPE_PRESET_NAME = "A_ARG_TYPE_PresetName";

            /** The Constant A_ARG_TYPE_DeviceUDN. */
            public final static String CMS_VARIABLE_A_ARG_TYPE_DEVICE_UDN = "A_ARG_TYPE_DeviceUDN";

            /** The Constant A_ARG_TYPE_ServiceType. */
            public final static String CMS_VARIABLE_A_ARG_TYPE_SERVICE_TYPE = "A_ARG_TYPE_ServiceType";

            /** The Constant A_ARG_TYPE_ServiceID. */
            public final static String CMS_VARIABLE_A_ARG_TYPE_SERVICE_ID = "A_ARG_TYPE_ServiceID";

            /** The Constant A_ARG_TYPE_StateVariableValuePairs. */
            public final static String CMS_VARIABLE_A_ARG_TYPE_STATE_VALUE_PAIRS = "A_ARG_TYPE_StateVariableValuePairs";

            /** The Constant A_ARG_TYPE_StateVariableList. */
            public final static String CMS_VARIABLE_A_ARG_TYPE_STATE_LIST = "A_ARG_TYPE_StateVariableList";
        }

        /**
         * The Interface GetProtocolInfo.
         */
        public interface GetProtocolInfo {

            /** The Constant SourceProtocolInfo. */
            public final static String CMS_VARIABLE_OUT_SOURCE = "Source";

            /** The Constant SinkProtocolInfo. */
            public final static String CMS_VARIABLE_OUT_SINK = "Sink";
        }

        /**
         * The Interface PrepareForConnection.
         */
        public interface PrepareForConnection {

            /** The Constant RemoteProtocolInfo. */
            public final static String CMS_VARIABLE_IN_REMOTE_PROTOCOL_INFO = "RemoteProtocolInfo";

            /** The Constant PeerConnectionManager. */
            public final static String CMS_VARIABLE_IN_PEER_CONNECTION_MANAGER = "PeerConnectionManager";

            /** The Constant PeerConnectionID. */
            public final static String CMS_VARIABLE_IN_PEER_CONNECTION_ID = "PeerConnectionID";

            /** The Constant Direction. */
            public final static String CMS_VARIABLE_IN_DIRECTION = "Direction";

            /** The Constant ConnectionID. */
            public final static String CMS_VARIABLE_OUT_CONNECTION_ID = "ConnectionID";

            /** The Constant AVTransportID. */
            public final static String CMS_VARIABLE_OUT_AVTRANSPORT_ID = "AVTransportID";

            /** The Constant RcsID. */
            public final static String CMS_VARIABLE_OUT_RCS_ID = "RcsID";

        }

        /**
         * The Interface ConnectionComplete.
         */
        public interface ConnectionComplete {

            /** The Constant ConnectionID. */
            public final static String CMS_VARIABLE_IN_CONNECTION_ID = "ConnectionID";

        }

        /**
         * The Interface GetCurrentConnectionIDs.
         */
        public interface GetCurrentConnectionIDs {

            /** The Constant CurrentConnectionIDs. */
            public final static String CMS_VARIABLE_OUT_CONNECTION_IDS = "ConnectionIDs";
        }

        /**
         * The Interface GetCurrentConnectionInfo.
         */
        public interface GetCurrentConnectionInfo {

            /** The Constant RcsID. */
            public final static String CMS_VARIABLE_OUT_RCS_ID = "RcsID";

            /** The Constant AVTransportID. */
            public final static String CMS_VARIABLE_OUT_AVTRANSPORT_ID = "AVTransportID";

            /** The Constant ProtocolInfo. */
            public final static String CMS_VARIABLE_OUT_PROTOCOL_INFO = "ProtocolInfo";

            /** The Constant PeerConnectionManager. */
            public final static String CMS_VARIABLE_OUT_PEER_CONNECTION_MANAGER = "PeerConnectionManager";

            /** The Constant PeerConnectionID. */
            public final static String CMS_VARIABLE_OUT_PEER_CONNECTION_ID = "PeerConnectionID";

            /** The Constant Direction. */
            public final static String CMS_VARIABLE_OUT_DIRECTION = "Direction";

            /** The Constant ConnectionID. */
            public final static String CMS_VARIABLE_IN_CONNECTION_ID = "ConnectionID";

            /** The Constant Status. */
            public final static String CMS_VARIABLE_OUT_STATUS = "Status";
        }
    }

    /**
     * The Interface AVTSAction.
     */
    public interface AVTSAction {

        /** The Constant SetAVTransportURI. */
        public final static String AVTS_ACTION_SET_AVTRANSPORT_URI = "SetAVTransportURI";

        /** The Constant SetNextAVTransportURI. */
        public final static String AVTS_ACTION_SET_NEXT_AVTRANSPROT_URI = "SetNextAVTransportURI";

        /** The Constant GetMediaInfo. */
        public final static String AVTS_ACTION_GET_MEDIA_INFO = "GetMediaInfo";

        /** The Constant GetMediaInfo_Ext. */
        public final static String AVTS_ACTION_GET_MEDIA_INFO_EXT = "GetMediaInfo_Ext";

        /** The Constant GetTransportInfo. */
        public final static String AVTS_ACTION_GET_TRANSPORT_INFO = "GetTransportInfo";

        /** The Constant GetPositionInfo. */
        public final static String AVTS_ACTION_GET_POSITION_INFO = "GetPositionInfo";

        /** The Constant GetDeviceCapabilities. */
        public final static String AVTS_ACTION_GET_DEVICE_CAPABILITIES = "GetDeviceCapabilities";

        /** The Constant GetTransportSettings. */
        public final static String AVTS_ACTION_GET_TRANSPORT_SETTINGS = "GetTransportSettings";

        /** The Constant Stop. */
        public final static String AVTS_ACTION_STOP = "Stop";

        /** The Constant Play. */
        public final static String AVTS_ACTION_PLAY = "Play";

        /** The Constant Pause. */
        public final static String AVTS_ACTION_PAUSE = "Pause";

        /** The Constant Record. */
        public final static String AVTS_ACTION_RECORD = "Record";

        /** The Constant Seek. */
        public final static String AVTS_ACTION_SEEK = "Seek";

        /** The Constant Next. */
        public final static String AVTS_ACTION_NEXT = "Next";

        /** The Constant Previous. */
        public final static String AVTS_ACTION_PREVIOUS = "Previous";

        /** The Constant SetPlayMode. */
        public final static String AVTS_ACTION_SET_PLAY_MODE = "SetPlayMode";

        /** The Constant SetRecordQualityMode. */
        public final static String AVTS_ACTION_SET_RECORD_QUALITY_MODE = "SetRecordQualityMode";

        /** The Constant GetCurrentTransportActions. */
        public final static String AVTS_ACTION_GET_CURRENT_TRANSPORT_ACTIONS = "GetCurrentTransportActions";

        /** The Constant GetDRMState. */
        public final static String AVTS_ACTION_GET_DRM_STATE = "GetDRMState";

        /** The Constant GetStateVariables. */
        public final static String AVTS_ACTION_GET_STATE_VARIABLES = "GetStateVariables";

        /** The Constant SetStateVariables. */
        public final static String AVTS_ACTION_SET_STATE_VARIABLES = "SetStateVariables";
    }

    /**
     * The Interface AVTSArgVariable.
     */
    public interface AVTSArgVariable {

        /**
         * The Interface stateVariable.
         */
        public interface stateVariable {
            /**
             * The Interface TransportState.
             */
            public interface TransportState {

                /** The Constant TransportState. */
                public final static String AVTS_VARIABLE_TRANSPORT_STATE = "TransportState";

                /** The Constant STOPPED. */
                public final static String AVTS_VARIABLE_STOPPED = "STOPPED";

                /** The Constant PLAYING. */
                public final static String AVTS_VARIABLE_PLAYING = "PLAYING";

                /** The Constant TRANSITIONING. */
                public final static String AVTS_VARIABLE_TRANSITIONING = "TRANSITIONING";

                /** The Constant PAUSED_PLAYBACK. */
                public final static String AVTS_VARIABLE_PAUSED_PLAYBACK = "PAUSED_PLAYBACK";

                /** The Constant PAUSED_RECORDING. */
                public final static String AVTS_VARIABLE_PAUSED_RECORDING = "PAUSED_RECORDING";

                /** The Constant RECORDING. */
                public final static String AVTS_VARIABLE_RECORDING = "RECORDING";

                /** The Constant NO_MEDIA_PRESENT. */
                public final static String AVTS_VARIABLE_NO_MEDIA_PRESENT = "NO_MEDIA_PRESENT";

            }
        }

        /**
         * The Interface SetAVTransportURI.
         */
        public interface SetAVTransportURI {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentURI. */
            public final static String AVTS_VARIABLE_IN_CURRENT_URI = "CurrentURI";

            /** The Constant CurrentURIMetaData. */
            public final static String AVTS_VARIABLE_IN_CURRENT_URI_META_DATA = "CurrentURIMetaData";

        }

        /**
         * The Interface SetNextAVTransportURI.
         */
        public interface SetNextAVTransportURI {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant NextURI. */
            public final static String AVTS_VARIABLE_IN_NEXT_URI = "NextURI";

            /** The Constant NextURIMetaData. */
            public final static String AVTS_VARIABLE_IN_NEXT_URI_META_DATA = "NextURIMetaData";
        }

        /**
         * The Interface GetMediaInfo.
         */
        public interface GetMediaInfo {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant NrTracks. */
            public final static String AVTS_VARIABLE_OUT_NR_TRACKS = "NrTracks";

            /** The Constant MediaDuration. */
            public final static String AVTS_VARIABLE_OUT_MEDIA_DURATION = "MediaDuration";

            /** The Constant CurrentURI. */
            public final static String AVTS_VARIABLE_OUT_CURRENT_URI = "CurrentURI";

            /** The Constant CurrentURIMetaData. */
            public final static String AVTS_VARIABLE_OUT_CURRENT_URI_META_DATA = "CurrentURIMetaData";

            /** The Constant NextURI. */
            public final static String AVTS_VARIABLE_OUT_NEXT_URI = "NextURI";

            /** The Constant NextURIMetaData. */
            public final static String AVTS_VARIABLE_OUT_NEXT_URI_META_DATA = "NextURIMetaData";

            /** The Constant PlayMedium. */
            public final static String AVTS_VARIABLE_OUT_PLAY_MEDIUM = "PlayMedium";

            /** The Constant RecordMedium. */
            public final static String AVTS_VARIABLE_OUT_RECORD_MEDIUM = "RecordMedium";

            /** The Constant WriteStatus. */
            public final static String AVTS_VARIABLE_OUT_WRITE_STATUS = "WriteStatus";
        }

        /**
         * The Interface GetMediaInfo_Ext.
         */
        public interface GetMediaInfo_Ext {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentType. */
            public final static String AVTS_VARIABLE_OUT_CURRENT_TYPE = "CurrentType";

            /** The Constant NrTracks. */
            public final static String AVTS_VARIABLE_OUT_NR_TRACKS = "NrTracks";

            /** The Constant MediaDuration. */
            public final static String AVTS_VARIABLE_OUT_MEDIA_DURATION = "MediaDuration";

            /** The Constant CurrentURI. */
            public final static String AVTS_VARIABLE_OUT_CURRENT_URI = "CurrentURI";

            /** The Constant CurrentURIMetaData. */
            public final static String AVTS_VARIABLE_OUT_CURRENT_URI_META_DATA = "CurrentURIMetaData";

            /** The Constant NextURI. */
            public final static String AVTS_VARIABLE_OUT_NEXT_URI = "NextURI";

            /** The Constant NextURIMetaData. */
            public final static String AVTS_VARIABLE_OUT_NEXT_URI_META_DATA = "NextURIMetaData";

            /** The Constant PlayMedium. */
            public final static String AVTS_VARIABLE_OUT_PLAY_MEDIUM = "PlayMedium";

            /** The Constant RecordMedium. */
            public final static String AVTS_VARIABLE_OUT_RECORD_MEDIUM = "RecordMedium";

            /** The Constant WriteStatus. */
            public final static String AVTS_VARIABLE_OUT_WRITE_STATUS = "WriteStatus";
        }

        /**
         * The Interface GetTransportInfo.
         */
        public interface GetTransportInfo {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentTransportState. */
            public final static String AVTS_VARIABLE_OUT_CURRENT_TRANSPORT_STATE = "CurrentTransportState";

            /** The Constant CurrentTransportStatus. */
            public final static String AVTS_VARIABLE_OUT_CURRENT_TRANSPORT_STATUS = "CurrentTransportStatus";

            /** The Constant CurrentSpeed. */
            public final static String AVTS_VARIABLE_OUT_CURRENT_SPEED = "CurrentSpeed";
        }

        /**
         * The Interface GetPositionInfo.
         */
        public interface GetPositionInfo {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Track. */
            public final static String AVTS_VARIABLE_OUT_TRACK = "Track";

            /** The Constant TrackDuration. */
            public final static String AVTS_VARIABLE_OUT_TRACK_DURATION = "TrackDuration";

            /** The Constant TrackMetaData. */
            public final static String AVTS_VARIABLE_OUT_TRACK_META_DATA = "TrackMetaData";

            /** The Constant TrackURI. */
            public final static String AVTS_VARIABLE_OUT_TRACK_URI = "TrackURI";

            /** The Constant RelTime. */
            public final static String AVTS_VARIABLE_OUT_REL_TIME = "RelTime";

            /** The Constant AbsTime. */
            public final static String AVTS_VARIABLE_OUT_ABS_TIME = "AbsTime";

            /** The Constant RelCount. */
            public final static String AVTS_VARIABLE_OUT_REL_COUNT = "RelCount";

            /** The Constant AbsCount. */
            public final static String AVTS_VARIABLE_OUT_ABS_COUNT = "AbsCount";
        }

        /**
         * The Interface GetDeviceCapabilities.
         */
        public interface GetDeviceCapabilities {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant PlayMedia. */
            public final static String AVTS_VARIABLE_OUT_PLAY_MEDIA = "PlayMedia";

            /** The Constant RecMedia. */
            public final static String AVTS_VARIABLE_OUT_REC_MEDIA = "RecMedia";

            /** The Constant RecQualityModes. */
            public final static String AVTS_VARIABLE_OUT_REC_QUALITY_MODES = "RecQualityModes";
        }

        /**
         * The Interface GetTransportSettings.
         */
        public interface GetTransportSettings {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant PlayMode. */
            public final static String AVTS_VARIABLE_OUT_PLAY_MODE = "PlayMode";

            /** The Constant RecQualityMode. */
            public final static String AVTS_VARIABLE_OUT_REC_QUALITY_MODE = "RecQualityMode";
        }

        /**
         * The Interface Stop.
         */
        public interface Stop {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";
        }

        /**
         * The Interface Play.
         */
        public interface Play {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Speed. */
            public final static String AVTS_VARIABLE_IN_SPEED = "Speed";
        }

        /**
         * The Interface Pause.
         */
        public interface Pause {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";
        }

        /**
         * The Interface Record.
         */
        public interface Record {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";
        }

        /**
         * The Interface Seek.
         */
        public interface Seek {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Unit. */
            public final static String AVTS_VARIABLE_IN_UNIT = "Unit";

            /** The Constant Target. */
            public final static String AVTS_VARIABLE_IN_TARGET = "Target";
            
            /** The Constant REL_TIME **/
            public final static String AVTS_VARIABLE_REL_TIME = "REL_TIME";

            /** The Constant TRACK_NR **/
            public final static String AVTS_VARIABLE_TRACK_NR = "TRACK_NR";
        }

        /**
         * The Interface Next.
         */
        public interface Next {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";
        }

        /**
         * The Interface Previous.
         */
        public interface Previous {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";
        }

        /**
         * The Interface SetPlayMode.
         */
        public interface SetPlayMode {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant NewPlayMode. */
            public final static String AVTS_VARIABLE_IN_NEW_PLAY_MODE = "NewPlayMode";
        }

        /**
         * The Interface SetRecordQualityMode.
         */
        public interface SetRecordQualityMode {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant NewRecordQualityMode. */
            public final static String AVTS_VARIABLE_IN_NEW_RECORD_QUALITY_MODE = "NewRecordQualityMode";
        }

        /**
         * The Interface GetCurrentTransportActions.
         */
        public interface GetCurrentTransportActions {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Actions. */
            public final static String AVTS_VARIABLE_OUT_ACTIONS = "Actions";
        }

        /**
         * The Interface GetDRMState.
         */
        public interface GetDRMState {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentDRMState. */
            public final static String AVTS_VARIABLE_OUT_CURRENT_DRM_STATE = "CurrentDRMState";
        }

        /**
         * The Interface GetStateVariables.
         */
        public interface GetStateVariables {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant StateVariableList. */
            public final static String AVTS_VARIABLE_IN_STATE_LIST = "StateVariableList";

            /** The Constant StateVariableValuePairs. */
            public final static String AVTS_VARIABLE_OUT_STATE_VALUE_PAIRS = "StateVariableValuePairs";
        }

        /**
         * The Interface SetStateVariables.
         */
        public interface SetStateVariables {

            /** The Constant InstanceID. */
            public final static String AVTS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant AVTransportUDN. */
            public final static String AVTS_VARIABLE_IN_AVTRANSPORT_UDN = "AVTransportUDN";

            /** The Constant ServiceType. */
            public final static String AVTS_VARIABLE_IN_SERVICE_TYPE = "ServiceType";

            /** The Constant ServiceId. */
            public final static String AVTS_VARIABLE_IN_SERVICE_ID = "ServiceId";

            /** The Constant StateVariableValuePairs. */
            public final static String AVTS_VARIABLE_IN_STATE_VALUE_PAIRS = "StateVariableValuePairs";

            /** The Constant StateVariableList. */
            public final static String AVTS_VARIABLE_OUT_STATE_LIST = "StateVariableList";
        }
    }

    /**
     * The Interface RCSAction.
     */
    public interface RCSAction {

        /** The Constant ListPresets. */
        public final static String RCS_ACTION_LIST_PRESETS = "ListPresets";

        /** The Constant SelectPreset. */
        public final static String RCS_ACTION_SELECT_PRESET = "SelectPreset";

        /** The Constant GetBrightness. */
        public final static String RCS_ACTION_GET_BRIGHTNESS = "GetBrightness";

        /** The Constant SetBrightness. */
        public final static String RCS_ACTION_SET_BRIGHTNESS = "SetBrightness";

        /** The Constant GetContrast. */
        public final static String RCS_ACTION_GET_CONTRAST = "GetContrast";

        /** The Constant SetContrast. */
        public final static String RCS_ACTION_SET_CONTRAST = "SetContrast";

        /** The Constant GetSharpness. */
        public final static String RCS_ACTION_GET_SHARPNESS = "GetSharpness";

        /** The Constant SetSharpness. */
        public final static String RCS_ACTION_SET_SHARPNESS = "SetSharpness";

        /** The Constant GetRedVideoGain. */
        public final static String RCS_ACTION_GET_RED_VIDEO_GAIN = "GetRedVideoGain";

        /** The Constant SetRedVideoGain. */
        public final static String RCS_ACTION_SET_RED_VIDEO_GAIN = "SetRedVideoGain";

        /** The Constant GetGreenVideoGain. */
        public final static String RCS_ACTION_GET_GREEN_VIDEO_GAIN = "GetGreenVideoGain";

        /** The Constant SetGreenVideoGain. */
        public final static String RCS_ACTION_SET_GREEN_VIDEO_GAIN = "SetGreenVideoGain";

        /** The Constant GetBlueVideoGain. */
        public final static String RCS_ACTION_GET_BLUE_VIDEO_GAIN = "GetBlueVideoGain";

        /** The Constant SetBlueVideoGain. */
        public final static String RCS_ACTION_SET_BLUE_VIDEO_GAIN = "SetBlueVideoGain";

        /** The Constant GetRedVideoBlackLevel. */
        public final static String RCS_ACTION_GET_RED_VIDEO_BLACK_LEVEL = "GetRedVideoBlackLevel";

        /** The Constant SetRedVideoBlackLevel. */
        public final static String RCS_ACTION_SET_RED_VIDEO_BLACK_LEVEL = "SetRedVideoBlackLevel";

        /** The Constant GetGreenVideoBlackLevel. */
        public final static String RCS_ACTION_GET_GREEN_VIDEO_BLACK_LEVEL = "GetGreenVideoBlackLevel";

        /** The Constant SetGreenVideoBlackLevel. */
        public final static String RCS_ACTION_SET_GREEN_VIDEO_BLACK_LEVEL = "SetGreenVideoBlackLevel";

        /** The Constant GetBlueVideoBlackLevel. */
        public final static String RCS_ACTION_GET_BLUE_VIDEO_BLACK_LEVEL = "GetBlueVideoBlackLevel";

        /** The Constant SetBlueVideoBlackLevel. */
        public final static String RCS_ACTION_SET_BLUE_VIDEO_BLACK_LEVEL = "SetBlueVideoBlackLevel";

        /** The Constant GetColorTemperature. */
        public final static String RCS_ACTION_GET_COLOR_TEMPERATURE = "GetColorTemperature";

        /** The Constant SetColorTemperature. */
        public final static String RCS_ACTION_SET_COLOR_TEMPERATURE = "SetColorTemperature";

        /** The Constant GetHorizontalKeystone. */
        public final static String RCS_ACTION_GET_HORIZONTAL_KEYSTONE = "GetHorizontalKeystone";

        /** The Constant SetHorizontalKeystone. */
        public final static String RCS_ACTION_SET_HORIZONTAL_KEYSTONE = "SetHorizontalKeystone";

        /** The Constant GetVerticalKeystone. */
        public final static String RCS_ACTION_GET_VERTICAL_KEYSTONE = "GetVerticalKeystone";

        /** The Constant SetVerticalKeystone. */
        public final static String RCS_ACTION_SET_VERTICAL_KEYSTONE = "SetVerticalKeystone";

        /** The Constant GetMute. */
        public final static String RCS_ACTION_GET_MUTE = "GetMute";

        /** The Constant SetMute. */
        public final static String RCS_ACTION_SET_MUTE = "SetMute";

        /** The Constant GetVolume. */
        public final static String RCS_ACTION_GET_VOLUME = "GetVolume";

        /** The Constant SetVolume. */
        public final static String RCS_ACTION_SET_VOLUME = "SetVolume";

        /** The Constant GetVolumeDB. */
        public final static String RCS_ACTION_GET_VOLUME_DB = "GetVolumeDB";

        /** The Constant SetVolumeDB. */
        public final static String RCS_ACTION_SET_VOLUME_DB = "SetVolumeDB";

        /** The Constant GetVolumeDBRange. */
        public final static String RCS_ACTION_GET_VOLUME_DB_RANGE = "GetVolumeDBRange";

        /** The Constant GetLoudness. */
        public final static String RCS_ACTION_GET_LOUDNESS = "GetLoudness";

        /** The Constant SetLoudness. */
        public final static String RCS_ACTION_SET_LOUDNESS = "SetLoudness";

        /** The Constant GetStateVariables. */
        public final static String RCS_ACTION_GET_STATE_VARIABLES = "GetStateVariables";

        /** The Constant SetStateVariables. */
        public final static String RCS_ACTION_SET_STATE_VARIABLES = "SetStateVariables";
    }

    /**
     * The Interface RCSArgVariable.
     */
    public interface RCSArgVariable {

        /**
         * The Interface ListPresets.
         */
        public interface ListPresets {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentPresetNameList. */
            public final static String RCS_VARIABLE_OUT_CURRENT_PRESET_NAME_LIST = "CurrentPresetNameList";
        }

        /**
         * The Interface SelectPreset.
         */
        public interface SelectPreset {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant PresetName. */
            public final static String RCS_VARIABLE_IN_PRESET_NAME = "PresetName";
        }

        /**
         * The Interface GetBrightness.
         */
        public interface GetBrightness {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentBrightness. */
            public final static String RCS_VARIABLE_OUT_CURRENT_BRIGHTNESS = "CurrentBrightness";
        }

        /**
         * The Interface SetBrightness.
         */
        public interface SetBrightness {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredBrightness. */
            public final static String RCS_VARIABLE_IN_DESIRED_BRIGHTNESS = "DesiredBrightness";
        }

        /**
         * The Interface GetContrast.
         */
        public interface GetContrast {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentContrast. */
            public final static String RCS_VARIABLE_OUT_CURRENT_CONTRAST = "CurrentContrast";
        }

        /**
         * The Interface SetContrast.
         */
        public interface SetContrast {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredContrast. */
            public final static String RCS_VARIABLE_IN_DESIRED_CONTRAST = "DesiredContrast";
        }

        /**
         * The Interface GetSharpness.
         */
        public interface GetSharpness {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentSharpness. */
            public final static String RCS_VARIABLE_OUT_CURRENT_SHARPNESS = "CurrentSharpness";
        }

        /**
         * The Interface SetSharpness.
         */
        public interface SetSharpness {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredSharpness. */
            public final static String RCS_VARIABLE_IN_DESIRED_SHARPNESS = "DesiredSharpness";
        }

        /**
         * The Interface GetRedVideoGain.
         */
        public interface GetRedVideoGain {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentRedVideoGain. */
            public final static String RCS_VARIABLE_OUT_CURRENT_RED_VIDEO_GAIN = "CurrentRedVideoGain";
        }

        /**
         * The Interface SetRedVideoGain.
         */
        public interface SetRedVideoGain {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredRedVideoGain. */
            public final static String RCS_VARIABLE_IN_DESIRED_RED_VIDEO_GAIN = "DesiredRedVideoGain";
        }

        /**
         * The Interface GetGreenVideoGain.
         */
        public interface GetGreenVideoGain {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentGreenVideoGain. */
            public final static String RCS_VARIABLE_OUT_CURRENT_GREEN_VIDEO_GAIN = "CurrentGreenVideoGain";
        }

        /**
         * The Interface SetGreenVideoGain.
         */
        public interface SetGreenVideoGain {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredGreenVideoGain. */
            public final static String RCS_VARIABLE_IN_DESIRED_RED_GREEN_VIDEO_GAIN = "DesiredGreenVideoGain";
        }

        /**
         * The Interface GetBlueVideoGain.
         */
        public interface GetBlueVideoGain {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentBlueVideoGain. */
            public final static String RCS_VARIABLE_OUT_CURRENT_BLUE_VIDEO_GAIN = "CurrentBlueVideoGain";
        }

        /**
         * The Interface SetBlueVideoGain.
         */
        public interface SetBlueVideoGain {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredBlueVideoGain. */
            public final static String RCS_VARIABLE_IN_DESIRED_BLUE_VIDEO_GAIN = "DesiredBlueVideoGain";
        }

        /**
         * The Interface GetRedVideoBlackLevel.
         */
        public interface GetRedVideoBlackLevel {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentRedVideoBlackLevel. */
            public final static String RCS_VARIABLE_OUT_CURRENT_RED_VIDEO_BLACK_LEVEL = "CurrentRedVideoBlackLevel";
        }

        /**
         * The Interface SetRedVideoBlackLevel.
         */
        public interface SetRedVideoBlackLevel {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredRedVideoBlackLevel. */
            public final static String RCS_VARIABLE_IN_DESIRED_RED_VIDEO_BLACK_LEVEL = "DesiredRedVideoBlackLevel";
        }

        /**
         * The Interface GetGreenVideoBlackLevel.
         */
        public interface GetGreenVideoBlackLevel {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentGreenVideoBlackLevel. */
            public final static String RCS_VARIABLE_OUT_CURRENT_GREEN_VIDEO_BLACK_LEVEL = "CurrentGreenVideoBlackLevel";
        }

        /**
         * The Interface SetGreenVideoBlackLevel.
         */
        public interface SetGreenVideoBlackLevel {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredGreenVideoBlackLevel. */
            public final static String RCS_VARIABLE_IN_DESIRED_GREEN_VIDEO_BLACK_LEVEL = "DesiredGreenVideoBlackLevel";
        }

        /**
         * The Interface GetBlueVideoBlackLevel.
         */
        public interface GetBlueVideoBlackLevel {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentBlueVideoBlackLevel. */
            public final static String RCS_VARIABLE_OUT_CURRENT_BLUE_VIDEO_BLACK_LEVEL = "CurrentBlueVideoBlackLevel";
        }

        /**
         * The Interface SetBlueVideoBlackLevel.
         */
        public interface SetBlueVideoBlackLevel {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredBlueVideoBlackLevel. */
            public final static String RCS_VARIABLE_IN_DESIRED_BLUE_VIDEO_BLACK_LEVEL = "DesiredBlueVideoBlackLevel";
        }

        /**
         * The Interface GetColorTemperature.
         */
        public interface GetColorTemperature {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentColorTemperature. */
            public final static String RCS_VARIABLE_OUT_CURRENT_COLOR_TEMPERATURE = "CurrentColorTemperature";
        }

        /**
         * The Interface SetColorTemperature.
         */
        public interface SetColorTemperature {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredColorTemperature. */
            public final static String RCS_VARIABLE_IN_DESIRED_COLOR_TEMPERATURE = "DesiredColorTemperature";
        }

        /**
         * The Interface GetHorizontalKeystone.
         */
        public interface GetHorizontalKeystone {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentHorizontalKeystone. */
            public final static String RCS_VARIABLE_OUT_CURRENT_HORIZONTAL_KEYSTONE = "CurrentHorizontalKeystone";
        }

        /**
         * The Interface SetHorizontalKeystone.
         */
        public interface SetHorizontalKeystone {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTACNE_ID = "InstanceID";

            /** The Constant DesiredHorizontalKeystone. */
            public final static String RCS_VARIABLE_IN_DESIRED_HORIZONTAL_KEYSTONE = "DesiredHorizontalKeystone";
        }

        /**
         * The Interface GetVerticalKeystone.
         */
        public interface GetVerticalKeystone {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentVerticalKeystone. */
            public final static String RCS_VARIABLE_OUT_CURRENT_VERTICAL_KEYSTONE = "CurrentVerticalKeystone";
        }

        /**
         * The Interface SetVerticalKeystone.
         */
        public interface SetVerticalKeystone {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredVerticalKeystone. */
            public final static String RCS_VARIABLE_IN_DESIRED_VERTICAL_KEYSTONE = "DesiredVerticalKeystone";
        }

        /**
         * The Interface GetMute.
         */
        public interface GetMute {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Channel. */
            public final static String RCS_VARIABLE_IN_CHANNEL = "Channel";

            /** The Constant CurrentMute. */
            public final static String RCS_VARIABLE_OUT_CURRENT_MUTE = "CurrentMute";
        }

        /**
         * The Interface SetMute.
         */
        public interface SetMute {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Channel. */
            public final static String RCS_VARIABLE_IN_CHANNEL = "Channel";

            /** The Constant DesiredMute. */
            public final static String RCS_VARIABLE_IN_DESIRED_MUTE = "DesiredMute";
        }

        /**
         * The Interface GetVolume.
         */
        public interface GetVolume {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Channel. */
            public final static String RCS_VARIABLE_IN_CHANNEL = "Channel";

            /** The Constant CurrentVolume. */
            public final static String RCS_VARIABLE_OUT_CURRENT_VOLUME = "CurrentVolume";
        }

        /**
         * The Interface SetVolume.
         */
        public interface SetVolume {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Channel. */
            public final static String RCS_VARIABLE_IN_CHANNEL = "Channel";

            /** The Constant CurrentVolume. */
            public final static String RCS_VARIABLE_IN_DESIRED_VOLUME = "DesiredVolume";
        }

        /**
         * The Interface GetVolumeDB.
         */
        public interface GetVolumeDB {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Channel. */
            public final static String RCS_VARIABLE_IN_CHANNEL = "Channel";

            /** The Constant CurrentVolume. */
            public final static String RCS_VARIABLE_OUT_CURRENT_VOLUME = "CurrentVolume";
        }

        /**
         * The Interface SetVolumeDB.
         */
        public interface SetVolumeDB {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Channel. */
            public final static String RCS_VARIABLE_IN_CHANNEL = "Channel";

            /** The Constant DesiredVolume. */
            public final static String RCS_VARIABLE_IN_DESIRED_VOLUME = "DesiredVolume";
        }

        /**
         * The Interface GetVolumeDBRange.
         */
        public interface GetVolumeDBRange {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Channel. */
            public final static String RCS_VARIABLE_IN_CHANNEL = "Channel";

            /** The Constant MinValue. */
            public final static String RCS_VARIABLE_OUT_MIN_VALUE = "MinValue";

            /** The Constant MaxValue. */
            public final static String RCS_VARIABLE_OUT_MAX_VALUE = "MaxValue";
        }

        /**
         * The Interface GetLoudness.
         */
        public interface GetLoudness {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Channel. */
            public final static String RCS_VARIABLE_IN_CHANNEL = "Channel";

            /** The Constant CurrentLoudness. */
            public final static String RCS_VARIABLE_OUT_CURRENT_LOUDNESS = "CurrentLoudness";
        }

        /**
         * The Interface SetLoudness.
         */
        public interface SetLoudness {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant Channel. */
            public final static String RCS_VARIABLE_IN_CHANNEL = "Channel";

            /** The Constant DesiredLoudness. */
            public final static String RCS_VARIABLE_IN_DESIRED_LOUDNESS = "DesiredLoudness";
        }

        /**
         * The Interface GetStateVariables.
         */
        public interface GetStateVariables {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant StateVariableList. */
            public final static String RCS_VARIABLE_IN_STATE_LIST = "StateVariableList";

            /** The Constant StateVariableValuePairs. */
            public final static String RCS_VARIABLE_OUT_STATE_VALUE_PAIRS = "StateVariableValuePairs";
        }

        /**
         * The Interface SetStateVariables.
         */
        public interface SetStateVariables {

            /** The Constant InstanceID. */
            public final static String RCS_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant RenderingControlUDN. */
            public final static String RCS_VARIABLE_IN_RENDERING_CONTROL_UDN = "RenderingControlUDN";

            /** The Constant ServiceType. */
            public final static String RCS_VARIABLE_IN_SERVICE_TYPE = "ServiceType";

            /** The Constant ServiceId. */
            public final static String RCS_VARIABLE_IN_SERVICE_ID = "ServiceId";

            /** The Constant StateVariableValuePairs. */
            public final static String RCS_VARIABLE_IN_STATE_VALUE_PAIRS = "StateVariableValuePairs";

            /** The Constant StateVariableList. */
            public final static String RCS_VARIABLE_OUT_STATE_LIST = "StateVariableList";
        }
    }

    /**
     * The Interface RCSAcerAction.
     */
    public interface RCSAcerAction {
        /** The Constant ListPresets. */
        public final static String RCS_ACER_ACTION_GET_ZOOM = "GetZoom";

        /** The Constant SelectPreset. */
        public final static String RCS_ACER_ACTION_SET_ZOOM = "SetZoom";

        /** The Constant ListPresets. */
        public final static String RCS_ACER_ACTION_GET_ROTATE = "GetRotate";

        /** The Constant SelectPreset. */
        public final static String RCS_ACER_ACTION_SET_ROTATE = "SetRotate";

        /** The Constant ListPresets. */
        public final static String RCS_ACER_ACTION_GET_DOLBY = "GetDolby";

        /** The Constant SelectPreset. */
        public final static String RCS_ACER_ACTION_SET_DOLBY = "SetDolby";
    }

    /**
     * The Interface RCSAcerArgVariable.
     */
    public interface RCSAcerArgVariable {
        /**
         * The Interface ListPresets.
         */
        public interface GetZoom {

            /** The Constant InstanceID. */
            public final static String RCS_ACER_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentZoom. */
            public final static String RCS_ACER_VARIABLE_OUT_CURRENT_ZOOM = "CurrentZoom";
        }

        /**
         * The Interface SetZoom.
         */
        public interface SetZoom {

            /** The Constant InstanceID. */
            public final static String RCS_ACER_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredZoom. */
            public final static String RCS_ACER_VARIABLE_IN_DESIRED_ZOOM = "DesiredZoom";
        }

        /**
         * The Interface ListPresets.
         */
        public interface GetRotate {

            /** The Constant InstanceID. */
            public final static String RCS_ACER_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentRotate. */
            public final static String RCS_ACER_VARIABLE_OUT_CURRENT_ROTATE = "CurrentRotate";
        }

        /**
         * The Interface SetRotate.
         */
        public interface SetRotate {

            /** The Constant InstanceID. */
            public final static String RCS_ACER_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredRotate. */
            public final static String RCS_ACER_VARIABLE_IN_DESIRED_ROTATE = "DesiredRotate";
        }

        /**
         * The Interface ListPresets.
         */
        public interface GetDolby {

            /** The Constant InstanceID. */
            public final static String RCS_ACER_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant CurrentDolby. */
            public final static String RCS_ACER_VARIABLE_OUT_CURRENT_DOLBY = "CurrentDolby";
        }

        /**
         * The Interface SetDolby.
         */
        public interface SetDolby {

            /** The Constant InstanceID. */
            public final static String RCS_ACER_VARIABLE_IN_INSTANCE_ID = "InstanceID";

            /** The Constant DesiredDolby. */
            public final static String RCS_ACER_VARIABLE_IN_DESIRED_DOLBY = "DesiredDolby";
        }
    }

    /**
     * The Interface ContentProperties.
     */
    public interface ContentProp {

        /**
         * The Interface object.
         */
        public interface Object {

            /** The Constant object. */
            public final static String CONTENT_PROP_OBJECT = "object";

            /**
             * The Interface item.
             */
            public interface item {

                /** The Constant item. */
                public final static String CONTENT_PROP_ITEM = "item";

                /** The Constant imageItem. */
                public final static String CONTENT_PROP_IMAGE_ITEM = "imageItem";

                /** The Constant audioItem. */
                public final static String CONTENT_PROP_AUDIO_ITEM = "audioItem";

                /** The Constant videoItem. */
                public final static String CONTENT_PROP_VIDEO_ITEM = "videoItem";

                /** The Constant playlistItem. */
                public final static String CONTENT_PROP_PLAY_LIST_ITEM = "playlistItem";

                /** The Constant textItem. */
                public final static String CONTENT_PROP_TEXT_ITEM = "textItem";

                /** The Constant bookmarkItem. */
                public final static String CONTENT_PROP_BOOKMARK_ITEM = "bookmarkItem";

                /** The Constant epgItem. */
                public final static String CONTENT_PROP_EPG_ITEM = "epgItem";
            }

            /**
             * The Interface container.
             */
            public interface container {

                /** The Constant container. */
                public final static String CONTENT_PROP_CONTAINER = "container";

                /** The Constant person. */
                public final static String CONTENT_PROP_PERSON = "person";

                /** The Constant playlistContainer. */
                public final static String CONTENT_PROP_PLAYLIST_CONTAINER = "playlistContainer";

                /** The Constant album. */
                public final static String CONTENT_PROP_ALBUM = "album";

                /** The Constant genre. */
                public final static String CONTENT_PROP_GENRE = "genre";

                /** The Constant channelGroup. */
                public final static String CONTENT_PROP_CHANNEL_GROUP = "channelGroup";

                /** The Constant epgContainer. */
                public final static String CONTENT_PROP_EPG_CONTAINER = "epgContainer";

                /** The Constant storageSystem. */
                public final static String CONTENT_PROP_STORAGE_SYSTEM = "storageSystem";

                /** The Constant storageVolume. */
                public final static String CONTENT_PROP_STORAGE_VOLUME = "storageVolume";

                /** The Constant storageFolder. */
                public final static String CONTENT_PROP_STORAGE_FOLDER = "storageFolder";

                /** The Constant bookmarkFolder. */
                public final static String CONTENT_PROP_BOOKMARK_FOLDER = "bookmarkFolder";
            }
        }

        /**
         * The Interface DIDLLite.
         */
        public interface didllite {

            /** The Constant id. */
            public final static String CONTENT_PROP_ID = "id";

            /** The Constant parentID. */
            public final static String CONTENT_PROP_PARENT_ID = "parentID";

            /** The Constant refID. */
            public final static String CONTENT_PROP_REF_ID = "refID";

            /** The Constant restricted. */
            public final static String CONTENT_PROP_RESTRICTED = "restricted";

            /** The Constant searchable. */
            public final static String CONTENT_PROP_SEARCHABLE = "searchable";

            /** The Constant childCount. */
            public final static String CONTENT_PROP_CHILD_COUNT = "childCount";

            /** The Constant neverPlayable. */
            public final static String CONTENT_PROP_NEVER_PLAYABLE = "neverPlayable";

            /**
             * The Interface res.
             */
            public interface res {
                /** The Constant res. */
                public final static String CONTENT_PROP_RES = "res";

                /** The Constant protocolInfo. */
                public final static String CONTENT_PROP_PROTOCOL_INFO = "protocolInfo";

                /** The Constant importUri. */
                public final static String CONTENT_PROP_IMPORT_URI = "importUri";

                /** The Constant size. */
                public final static String CONTENT_PROP_SIZE = "size";

                /** The Constant duration. */
                public final static String CONTENT_PROP_DURATION = "duration";

                /** The Constant protection. */
                public final static String CONTENT_PROP_PROTECTION = "protection";

                /** The Constant bitrate. */
                public final static String CONTENT_PROP_BITRATE = "bitrate";

                /** The Constant bitsPerSample. */
                public final static String CONTENT_PROP_BITS_PER_SAMPLE = "bitsPerSample";

                /** The Constant sampleFrequency. */
                public final static String CONTENT_PROP_SAMPLE_FREQUENCY = "sampleFrequency";

                /** The Constant nrAudioChannels. */
                public final static String CONTENT_PROP_NR_AUDIO_CHANNELS = "nrAudioChannels";

                /** The Constant resolution. */
                public final static String CONTENT_PROP_RESOLUTION = "resolution";

                /** The Constant colorDepth. */
                public final static String CONTENT_PROP_COLOR_DEPTH = "colorDepth";

                /** The Constant tspec. */
                public final static String CONTENT_PROP_TSPEC = "tspec";

                /** The Constant allowedUse. */
                public final static String CONTENT_PROP_ALLOWED_USE = "allowedUse";

                /** The Constant validityStart. */
                public final static String CONTENT_PROP_VALIDITY_START = "validityStart";

                /** The Constant validityEnd. */
                public final static String CONTENT_PROP_VALIDITY_END = "validityEnd";

                /** The Constant remainingTime. */
                public final static String CONTENT_PROP_REMAINING_TIME = "remainingTime";

                /** The Constant usageInfo. */
                public final static String CONTENT_PROP_USAGE_INFO = "usageInfo";

                /** The Constant rightsInfoURI. */
                public final static String CONTENT_PROP_RIGHTS_INFO_URI = "rightsInfoURI";

                /** The Constant contentInfoURI. */
                public final static String CONTENT_PROP_CONTENT_INFO_URI = "contentInfoURI";

                /** The Constant recordQuality. */
                public final static String CONTENT_PROP_RECORD_QUALITY = "recordQuality";
            }
        }

        /**
         * The Interface DC.
         */
        public interface dc {

            /** The Constant title. */
            public final static String CONTENT_PROP_TITLE = "dc:title";

            /** The Constant creator. */
            public final static String CONTENT_PROP_CREATOR = "dc:creator";

            /** The Constant publisher. */
            public final static String CONTENT_PROP_PUBLISHER = "dc:publisher";

            /** The Constant contributor. */
            public final static String CONTENT_PROP_CONTRIBUTOR = "dc:contributor";

            /** The Constant relation. */
            public final static String CONTENT_PROP_RELATION = "dc:relation";

            /** The Constant description. */
            public final static String CONTENT_PROP_DESCRIPTION = "dc:description";

            /** The Constant rights. */
            public final static String CONTENT_PROP_RIGHTS = "dc:rights";

            /** The Constant date. */
            public final static String CONTENT_PROP_DATE = "dc:date";

            /** The Constant language. */
            public final static String CONTENT_PROP_LANGUAGE = "dc:language";
        }

        /**
         * The Interface upnp.
         */
        public interface upnp {

            /** The Constant classtag. */
            public final static String CONTENT_PROP_CLASS_TAG = "upnp:class";

            /**
             * The Interface classProperty.
             */
            public interface classProperty {

                /** The Constant object_item. */
                public final static String OBJECT_ITEM = "object.item";

                /** The Constant object_item_imageItem. */
                public final static String OBJECT_ITEM_IMAGEITEM = "object.item.imageItem";

                /** The Constant object_item_imageItem_photo. */
                public final static String OBJECT_ITEM_IMAGEITEM_PHOTO = "object.item.imageItem.photo";

                /** The Constant object_item_audioItem. */
                public final static String OBJECT_ITEM_AUDIOITEM = "object.item.audioItem";

                /** The Constant object_item_audioItem_musicTrack. */
                public final static String OBJECT_ITEM_AUDIOITEM_MUSICTRACK = "object.item.audioItem.musicTrack";

                /** The Constant object_item_audioItem_audioBroadcast. */
                public final static String OBJECT_ITEM_AUDIOITEM_AUDIOBROADCAST = "object.item.audioItem.audioBroadcast";

                /** The Constant object_item_audioItem_audioBook. */
                public final static String OBJECT_ITEM_AUDIOITEM_AUDIOBOOK = "object.item.audioItem.audioBook";

                /** The Constant object_item_videoItem. */
                public final static String OBJECT_ITEM_VIDEOITEM = "object.item.videoItem";

                /** The Constant object_item_videoItem_movie. */
                public final static String OBJECT_ITEM_VIDEOITEM_MOVIE = "object.item.videoItem.movie";

                /** The Constant object_item_videoItem_videoBroadcast. */
                public final static String OBJECT_ITEM_VIDEOITEM_VIDEOBROADCAST = "object.item.videoItem.videoBroadcast";

                /** The Constant object_item_videoItem_musicVideoClip. */
                public final static String OBJECT_ITEM_VIDEOITEM_MUSICVIDEOCLIP = "object.item.videoItem.musicVideoClip";

                /** The Constant object_item_playlistItem. */
                public final static String OBJECT_ITEM_PLAYLISTITEM = "object.item.playlistItem";

                /** The Constant object_item_textItem. */
                public final static String OBJECT_ITEM_TEXTITEM = "object.item.textItem";

                /** The Constant object_item_bookmarkItem. */
                public final static String OBJECT_ITEM_BOOKMARKITEM = "object.item.bookmarkItem";

                /** The Constant object_item_epgItem. */
                public final static String OBJECT_ITEM_EPGITEM = "object.item.epgItem";

                /** The Constant object_item_epgItem_audioProgram. */
                public final static String OBJECT_ITEM_EPGITEM_AUDIOPROGRAM = "object.item.epgItem.audioProgram";

                /** The Constant object_item_epgItem_videoProgram. */
                public final static String OBJECT_ITEM_EPGITEM_VIDEOPROGRAM = "object.item.epgItem.videoProgram";

                /** The Constant object_container. */
                public final static String OBJECT_CONTAINER = "object.container";

                /** The Constant object_container_person. */
                public final static String OBJECT_CONTAINER_PERSON = "object.container.person";

                /** The Constant object_container_person_musicArtist. */
                public final static String OBJECT_CONTAINER_PERSON_MUSICARTIST = "object.container.person.musicArtist";

                /** The Constant object_container_playlistContainer. */
                public final static String OBJECT_CONTAINER_PLAYLISTCONTAINER = "object.container.playlistContainer";

                /** The Constant object_container_album. */
                public final static String OBJECT_CONTAINER_ALBUM = "object.container.album";

                /** The Constant object_container_album_musicAlbum. */
                public final static String OBJECT_CONTAINER_ALBUM_MUSICALBUM = "object.container.album.musicAlbum";

                /** The Constant object_container_album_photoAlbum. */
                public final static String OBJECT_CONTAINER_ALBUM_PHOTOALBUM = "object.container.album.photoAlbum";

                /** The Constant object_container_genre. */
                public final static String OBJECT_CONTAINER_GENRE = "object.container.genre";

                /** The Constant object_container_genre_musicGenre. */
                public final static String OBJECT_CONTAINER_GENRE_MUSICGENRE = "object.container.genre.musicGenre";

                /** The Constant object_container_genre_movieGenre. */
                public final static String OBJECT_CONTAINER_GENRE_MOVIEGENRE = "object.container.genre.movieGenre";

                /** The Constant object_container_channelGroup. */
                public final static String OBJECT_CONTAINER_CHANNELGROUP = "object.container.channelGroup";

                /**
                 * The Constant object_container_channelGroup_audioChannelGroup.
                 */
                public final static String OBJECT_CONTAINER_CHANNELGROUP_AUDIOCHANNELGROUP = "object.container.channelGroup.audioChannelGroup";

                /**
                 * The Constant object_container_channelGroup_videoChannelGroup.
                 */
                public final static String OBJECT_CONTAINER_CHANNELGROUP_VIDEOCHANNELGROUP = "object.container.channelGroup.videoChannelGroup";

                /** The Constant object_container_epgContainer. */
                public final static String OBJECT_CONTAINER_EPGCONTAINER = "object.container.epgContainer";

                /** The Constant object_container_storageSystem. */
                public final static String OBJECT_CONTAINER_STORAGESYSTEM = "object.container.storageSystem";

                /** The Constant object_container_storageVolume. */
                public final static String OBJECT_CONTAINER_STORAGEVOLUME = "object.container.storageVolume";

                /** The Constant object_container_storageFolder. */
                public final static String OBJECT_CONTAINER_STORAGE_FOLDER = "object.container.storageFolder";

                /** The Constant object_container_bookmarkFolder. */
                public final static String OBJECT_CONTAINER_BOOKMARKFOLDER = "object.container.bookmarkFolder";
            }

            /** The Constant classname. */
            public final static String CONTENT_PROP_CLASS_NAME = "upnp:class@name";

            /** The Constant searchClass. */
            public final static String CONTENT_PROP_SEARCH_CLASS = "upnp:searchClass";

            /** The Constant searchClassname. */
            public final static String CONTENT_PROP_SEARCH_CLASS_NAME = "upnp:searchClass@name";

            /** The Constant searchClassincludeDerived. */
            public final static String CONTENT_PROP_SEARCH_CLASS_INCLUDE_DERIVED = "upnp:searchClass@includeDerived";

            /** The Constant createClass. */
            public final static String CONTENT_PROP_CREATE_CLASS = "upnp:createClass";

            /** The Constant createClassname. */
            public final static String CONTENT_PROP_CREATE_CLASS_NAME = "upnp:createClass@name";

            /** The Constant createClassincludeDerived. */
            public final static String CONTENT_PROP_CREATE_CLASS_INCLUDE_DERIVED = "upnp:createClass@includeDerived";

            /** The Constant writeStatus. */
            public final static String CONTENT_PROP_WRITE_STATUS = "upnp:writeStatus";

            /** The Constant artist. */
            public final static String CONTENT_PROP_ARTIST = "upnp:artist";

            /** The Constant artistrole. */
            public final static String CONTENT_PROP_ARTIST_ROLE = "upnp:artist@role";

            /** The Constant actor. */
            public final static String CONTENT_PROP_ACTOR = "upnp:actor";

            /** The Constant actorrole. */
            public final static String CONTENT_PROP_ACTOR_ROLE = "upnp:actor@role";

            /** The Constant author. */
            public final static String CONTENT_PROP_AUTHOR = "upnp:author";

            /** The Constant authorrole. */
            public final static String CONTENT_PROP_AUTHOR_ROLE = "upnp:author@role";

            /** The Constant producer. */
            public final static String CONTENT_PROP_PRODUCER = "upnp:producer";

            /** The Constant director. */
            public final static String CONTENT_PROP_DIRECTOR = "upnp:director";

            /** The Constant genre. */
            public final static String CONTENT_PROP_GENRE = "upnp:genre";

            /** The Constant genreid. */
            public final static String CONTENT_PROP_GENRE_ID = "upnp:genre@id";

            /** The Constant genreextended. */
            public final static String CONTENT_PROP_GENRE_EXTENDED = "upnp:genre@extended";

            /** The Constant album. */
            public final static String CONTENT_PROP_ALBUM = "upnp:album";

            /** The Constant playlist. */
            public final static String CONTENT_PROP_PLAY_LIST = "upnp:playlist";

            /** The Constant albumArtURI. */
            public final static String CONTENT_PROP_ALBUM_ART_URI = "upnp:albumArtURI";

            /** The Constant artistDiscographyURI. */
            public final static String CONTENT_PROP_ARTIST_DISCOGRAPHY_URI = "upnp:artistDiscographyURI";

            /** The Constant lyricsURI. */
            public final static String CONTENT_PROP_LYRICS_URI = "upnp:lyricsURI";

            /** The Constant storageTotal. */
            public final static String CONTENT_PROP_STORAGE_TOTAL = "upnp:storageTotal";

            /** The Constant storageUsed. */
            public final static String CONTENT_PROP_STORAGE_USED = "upnp:storageUsed";

            /** The Constant storageFree. */
            public final static String CONTENT_PROP_STORAGE_FREE = "upnp:storageFree";

            /** The Constant storageMaxPartition. */
            public final static String CONTENT_PROP_STORAGE_MAX_PARTITION = "upnp:storageMaxPartition";

            /** The Constant storageMedium. */
            public final static String CONTENT_PROP_STORAGE_MEDIUM = "upnp:storageMedium";

            /** The Constant longDescription. */
            public final static String CONTENT_PROP_LONG_DESCRIPTION = "upnp:longDescription";

            /** The Constant icon. */
            public final static String CONTENT_PROP_ICON = "upnp:icon";

            /** The Constant region. */
            public final static String CONTENT_PROP_REGION = "upnp:region";

            /** The Constant playbackCount. */
            public final static String CONTENT_PROP_PLAYBACK_COUNT = "upnp:playbackCount";

            /** The Constant lastPlaybackTime. */
            public final static String CONTENT_PROP_LAST_PLAYBACK_TIME = "upnp:lastPlaybackTime";

            /** The Constant lastPlaybackPosition. */
            public final static String CONTENT_PROP_LAST_PLAYBACK_POSITION = "upnp:lastPlaybackPosition";

            /** The Constant recordedStartDateTime. */
            public final static String CONTENT_PROP_RECORDED_START_DATE_TIME = "upnp:recordedStartDateTime";

            /** The Constant recordedDuration. */
            public final static String CONTENT_PROP_RECORDED_DURATION = "upnp:recordedDuration";

            /** The Constant recordedDayOfWeek. */
            public final static String CONTENT_PROP_RECORDED_DAY_OF_WEEK = "upnp:recordedDayOfWeek";

            /** The Constant srsRecordScheduleID. */
            public final static String CONTENT_PROP_SRS_RECORD_SCHEDULE_ID = "upnp:srsRecordScheduleID";

            /** The Constant srsRecordTaskID. */
            public final static String CONTENT_PROP_SRS_RECORD_TASK_ID = "upnp:srsRecordTaskID";

            /** The Constant recordable. */
            public final static String CONTENT_PROP_RECORDABLE = "upnp:recordable";

            /** The Constant programTitle. */
            public final static String CONTENT_PROP_PROGRAM_TITLE = "upnp:programTitle";

            /** The Constant seriesTitle. */
            public final static String CONTENT_PROP_SERIES_TITLE = "upnp:seriesTitle";

            /** The Constant programID. */
            public final static String CONTENT_PROP_PROGRAM_ID = "upnp:programID";

            /** The Constant programIDtype. */
            public final static String CONTENT_PROP_PROGRAM_ID_TYPE = "upnp:programID@type";

            /** The Constant seriesID. */
            public final static String CONTENT_PROP_SERIES_ID = "upnp:seriesID";

            /** The Constant seriesIDtype. */
            public final static String CONTENT_PROP_SERIES_ID_TYPE = "upnp:seriesID@type";

            /** The Constant channelID. */
            public final static String CONTENT_PROP_CHANNEL_ID = "upnp:channelID";

            /** The Constant channelIDtype. */
            public final static String CONTENT_PROP_CHANNEL_ID_TYPE = "upnp:channelID@type";

            /** The Constant episodeCount. */
            public final static String CONTENT_PROP_EPISODE_COUNT = "upnp:episodeCount";

            /** The Constant episodeNumber. */
            public final static String CONTENT_PROP_EPISODE_NUMBER = "upnp:episodeNumber";

            /** The Constant programCode. */
            public final static String PROGRAM_CODE = "upnp:programCode";

            /** The Constant programCodetype. */
            public final static String CONTENT_PROP_PROGRAM_CODE_TYPE = "upnp:programCode@type";

            /** The Constant rating. */
            public final static String CONTENT_PROP_RATING = "upnp:rating";

            /** The Constant ratingtype. */
            public final static String CONTENT_PROP_RATING_TYPE = "upnp:rating@type";

            /** The Constant episodeType. */
            public final static String CONTENT_PROP_EPISODE_TYPE = "upnp:episodeType";

            /** The Constant channelGroupName. */
            public final static String CONTENT_PROP_CHANNEL_GROUP_NAME = "upnp:channelGroupName";

            /** The Constant channelGroupNameid. */
            public final static String CONTENT_PROP_CHANNEL_GROUP_NAME_ID = "upnp:channelGroupName@id";

            /** The Constant callSign. */
            public final static String CONTENT_PROP_CALL_SIGN = "upnp:callSign";

            /** The Constant networkAffiliation. */
            public final static String CONTENT_PROP_NETWORK_AFFILIATION = "upnp:networkAffiliation";

            /** The Constant serviceProvider. */
            public final static String CONTENT_PROP_SERVICE_PROVIDER = "upnp:serviceProvider";

            /** The Constant price. */
            public final static String CONTENT_PROP_PRICE = "upnp:price";

            /** The Constant pricecurrency. */
            public final static String CONTENT_PROP_PRICE_CURRENCY = "upnp:price@currency";

            /** The Constant payPerView. */
            public final static String CONTENT_PROP_PAY_PER_VIEW = "upnp:payPerView";

            /** The Constant epgProviderName. */
            public final static String CONTENT_PROP_EPG_PROVIDER_NAME = "upnp:epgProviderName";

            /** The Constant dateTimeRange. */
            public final static String CONTENT_PROP_DATE_TIME_RANGE = "upnp:dateTimeRange";

            /** The Constant radioCallSign. */
            public final static String CONTENT_PROP_RADIO_CALL_SIGN = "upnp:radioCallSign";

            /** The Constant radioStationID. */
            public final static String CONTENT_PROP_RADIO_STATION_ID = "upnp:radioStationID";

            /** The Constant radioBand. */
            public final static String CONTENT_PROP_RADIO_BAND = "upnp:radioBand";

            /** The Constant channelNr. */
            public final static String CONTENT_PROP_CHANNEL_NR = "upnp:channelNr";

            /** The Constant channelName. */
            public final static String CONTENT_PROP_CHANNEL_NAME = "upnp:channelName";

            /** The Constant scheduledStartTime. */
            public final static String CONTENT_PROP_SCHEDULE_START_TIME = "upnp:scheduledStartTime";

            /** The Constant scheduledEndTime. */
            public final static String CONTENT_PROP_SCHEDULED_END_TIME = "upnp:scheduledEndTime";

            /** The Constant signalStrength. */
            public final static String CONTENT_PROP_SIGNAL_STRENGTH = "upnp:signalStrength";

            /** The Constant signalLocked. */
            public final static String CONTENT_PROP_SIGNAL_LOCKED = "upnp:signalLocked";

            /** The Constant tuned. */
            public final static String CONTENT_PROP_TUNED = "upnp:tuned";

            /** The Constant bookmarkID. */
            public final static String CONTENT_PROP_BOOKMARK_ID = "upnp:bookmarkID";

            /** The Constant bookmarkedObjectID. */
            public final static String CONTENT_PROP_BOOKMARKED_OBJECT_ID = "upnp:bookmarkedObjectID";

            /** The Constant deviceUDN. */
            public final static String CONTENT_PROP_DEVICE_UDN = "upnp:deviceUDN";

            /** The Constant deviceUDNserviceType. */
            public final static String CONTENT_PROP_DEVICE_UDN_SERVICE_TYPE = "upnp:deviceUDN@serviceType";

            /** The Constant deviceUDNserviceId. */
            public final static String CONTENT_PROP_DEVICE_UDN_SERVICE_ID = "upnp:deviceUDN@serviceId";

            /** The Constant stateVariableCollection. */
            public final static String CONTENT_PROP_STATE_VARIABLE_COLLECTION = "upnp:stateVariableCollection";

            /** The Constant stateVariableCollectionserviceName. */
            public final static String CONTENT_PROP_STATE_VARIABLE_COLLECTION_SERVICE_NAME = "upnp:stateVariableCollection@serviceName";

            /** The Constant stateVariableCollectionrcsInstanceType. */
            public final static String CONTENT_PROP_STATE_VARIABLE_COLLECTION_RCS_INSTANCE_TYPE = "upnp:stateVariableCollection@rcsInstanceType";

            /** The Constant DVDRegionCode. */
            public final static String CONTENT_PROP_DVD_REGION_CODE = "upnp:DVDRegionCode";

            /** The Constant originalTrackNumber. */
            public final static String CONTENT_PROP_ORIGINAL_TRACK_NUMBER = "upnp:originalTrackNumber";

            /** The Constant toc. */
            public final static String CONTENT_PROP_TOC = "upnp:toc";

            /** The Constant userAnnotation. */
            public final static String CONTENT_PROP_USER_ANNOTATION = "upnp:userAnnotation";
        }
    }
}
