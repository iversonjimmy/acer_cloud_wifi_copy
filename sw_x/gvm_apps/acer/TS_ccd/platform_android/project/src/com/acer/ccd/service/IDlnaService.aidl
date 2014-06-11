package com.acer.ccd.service;

/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

import com.acer.ccd.service.IDlnaServiceCallback;

interface IDlnaService {
    /**
     * Search all DMS/DMR/DMSDMR in the same LAN networking
     * @param reset: reset DMS/Device/DMR resource
     */
    void searchDevices(boolean reset);
    
    /**
     * Stop searching all DMS/DMR/DMSDMR in the same LAN networking. DLNA Service should stop writing data by DBManager
     */
    void stopSearchDevices();
    
    /**
     * Browse containers/items under specific container
     * Uuid: Device uuid
     * ContainerID: Unique identifier of the container in which to begin browsing.
     *     A ContainerID value of zero corresponds to the root object of the Content Directory.
     * StartingIndex: specify an offset into an arbitrary list of objects.
     *     0 represents the first object in the list.
     * RequestedCount: specify an ordinal number of arbitrary objects
     * SortCriteria: TBD
     * tableId: Unique id to specify the target table in DBManager
     */
    void browserContent(String Uuid, String ContainerID, int StartingIndex, int RequestedCount, String SortCriteria, int tableId);

    /** update specific container album url
     * @param uuid: Device uuid
     * @param containerID Container ID
     * @param table Table Id
     */
    void updateContainerAlbum(String uuid, String containerID, int table);

    /**
     * Stop browsing containers/items under specific container. DLNA Service should stop writing data by DBManager
     * @token specific token to service
     */
    void stopBrowserContent(int token);

    /**
     * Search containers/items under specific container/UUID
     * Uuid: Device uuid
     * ContainerID: Unique identifier of the container in which to begin browsing.
     *     A ContainerID value of zero corresponds to the root object of the Content Directory.
     * searchCriteria: to be used for querying the Content Directory.
     * StartingIndex: specify an offset into an arbitrary list of objects.
     *     0 represents the first object in the list.
     * RequestedCount: specify an ordinal number of arbitrary objects
     */
    void searchContent(String Uuid, String ContainerID, String searchCriteria, int StartingIndex, int RequestedCount, String SortCriteria);
    
    /**
     * Stop searching containers/items under specific container/UUID. DLNA Service should stop writing data by DBManager
     */
    void stopSearchContent(int token);

    /**
     * Register callback functions from DLNA Service. Synchronized function call
     * cb: Callback functions
     */
    void registerCallback(IDlnaServiceCallback cb);
    
    /**
     * Unregister callback functions from DLNA Service. Synchronized function call
     * cb: Callback functions
     */
    void unregisterCallback(IDlnaServiceCallback cb);
    
    /**
     * Attach selected DMR. Asynchronized function call
     * dmsUuid: DMS's UUID 
     * dmrUuid: DMR's UUID
     */
    void attachDMRAsync(String dmsUuid, String dmrUuid);
    
    /**
     * Detach selected DMR. Asynchronized function call
     */
    void detachDMRAsync(String dmrUuid);

    /**
     * AIDL function required by photo, music and video DMC
     */
    
    /**
     * Get audio/video max volume. Synchronized function call
     * @ return percentage
     */
    int getAVMaxVolume(String dmrUuid);
    
    /**
     * Get audio/video min volume. Synchronized function call
     * @ return percentage
     */
    int getAVMinVolume(String dmrUuid);
    
    /**
     * Get current audio/video volume. Synchronized function call
     * @ return percentage (from 0 to 100)
     */
    int getAVCurrentVolume(String dmrUuid);

    /**
     * Get current audio/video is mute. Synchronized function call
     * @ return true: mute, false: no mute
     */
    boolean getAVMute(String dmrUuid);
    
    /**
     * Get photo max zoom level. Synchronized function call
     * @ return zoom value
     */
    int getPhotoMaxZoomLevel(String dmrUuid);
    
    /**
     * Get photo current zoom level. Synchronized function call
     * @ return current zoom value
     */
    int getPhotoZoomLevel(String dmrUuid);
    
    /**
     * Set audio/video current volume level. Asynchronized function call
     * volume: volume value
     * @ return true: succeed, false: failed
     */
    void setAVVolumeAsync(int volume, String dmrUuid);
    
    /**
     * Set audio/video current mute. Asynchronized function call
     * state: mute value
     * @ return true: succeed, false: failed
     */
    void setAVMuteAsync(boolean state, String dmrUuid);
    
    /**
     * Set photo max Zoom. Asynchronized function call
     * maxzoom: Zoom value
     * @ return true: succeed, false: failed
     */
    void setPhotoMaxZoomLevelAsync(int maxzoom, String dmrUuid);
    
    /**
     * Set photo current Zoom. Asynchronized function call
     * zoom: Zoom value
     * @ return true: succeed, false: failed
     */
    void setPhotoZoomLevelAsync(int zoom, String dmrUuid);
    
    /**
     * Rotate current photo to left. Asynchronized function call
     * @ return true: succeed, false: failed
     */
    void photoRotateLeftAsync(String dmrUuid);
    
    /**
     * Rotate current photo to right. Asynchronized function call
     * @ return true: succeed, false: failed
     */
    void photoRotateRightAsync(String dmrUuid);
    
   /**
     * Check playback status of current streaming media file. Synchronized function call
     * @ return if playing
     */
    boolean isAvPlaying(String dmrUuid);

    /**
     * Get duration of current streaming media file. Synchronized function call
     * @ return duration (in msec)
     */
    long avDuration(String dmrUuid);

    /**
     * Get current position of streaming media file. Asynchronized function call
     */
    void avPositionAsync(String dmrUuid);

    /**
     * Seek to specific position. Asynchronized function call
     * @ param command callback command
     * @ param track track of media
     * @ return true: succeed, false: failed
     */
    void SeekToTrackAsync(int command, int track, String dmrUuid);

    /**
     * Seek to specific position. Asynchronized function call
     * @ param pos destination position (in msec)
     * @ return true: succeed, false: failed
     */
    void avSeekToAsync(int pos, String dmrUuid);

    /**
     * Open streaming media file with sepcific url. Asynchronized function call
     * @ param itemIdList Each item's id of Playlist 
     * @ param tbid ID of Table
     * @ param dmrUuid UUID of DMR
     * @ return true: succeed, false: failed
     */
    void openPhotoPlaylistAsync(in long[] itemIdList, int tbid, String dmrUuid);
    
    /**
     * Open streaming media file with sepcific url. Asynchronized function call
     * @ param itemIdList Each item's id of Playlist 
     * @ param tbid ID of Table
     * @ param mediaType Media type of Playlist
     * @ param playlistId local playlist's id
     * @ param dmrUuid UUID of DMR
     * @ return true: succeed, false: failed
     */
    void openAvPlaylistAsync(in long[] itemIdList, int tbid, String mediaType, long playlistId, String dmrUuid);

    /**
     * Open streaming media file with sepcific url. Asynchronized function call
     * @ param command callback command
     * @ param url url of media
     * @ param protocolInfo of meida
     * @ return true: succeed, false: failed
     */
    void openAsync(int command, String songUrl, String title, String protocolInfo, String dmrUuid);

    /**
     * add next streaming media file with sepcific url. Asynchronized function call
     * @ param command callback command
     * @ param url url of media
     * @ param protocolInfo of media
     * @ return true: succeed, false: failed
     */
    void addNextAsync(int command, String songUrl, String protocolInfo, String dmrUuid);
  
    /**
     * Play next streaming media file on playlist. Asynchronized function call
     * @ return true: succeed, false: failed
     */
    void nextAsync(int command, String dmrUuid);
  
    /**
     * Play previous streaming media file on playlist. Asynchronized function call
     * @ return true: succeed, false: failed
     */
    void prevAsync(int command, String dmrUuid);

    /**
     * Play or resume current streaming media file. Asynchronized function call
     * @ return true: succeed, false: failed
     */
    void playAsync(int command, String dmrUuid);
    
    /**
     * Pause current streaming media file. Asynchronized function call
     * @ return true: succeed, false: failed
     */
    void pauseAsync(int command, String dmrUuid);

    /**
     * Stop current streaming media file. Asynchronized function call
     * @ return true: succeed, false: failed
     */
    void stopAsync(int command, String dmrUuid);

    /**
     * Save current streaming media file to dmr device. Asynchronized function call
     *
     */
    void recordAsync(String dmrUuid);
    
    /**
      * Retrieve DMR status, return by callback
      * @param uuid Device uuid
      */
    void getDMRStatus(String uuid);
}
