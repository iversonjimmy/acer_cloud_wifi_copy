/******************************************************************
 *
 *	Clear.fi for Android
 *
 *	Copyright (C) Acer
 *
 * 	R&D Chaozhong Li
 * 
 *	File: DMSAction.java
 *
 *	Revision:
 *
 *	2011-3-8
 *		- first revision.
 *	
 ******************************************************************/

package com.acer.ccd.upnp.dms;

import java.util.List;
import org.cybergarage.upnp.ArgumentList;
import org.cybergarage.upnp.Device;
import org.cybergarage.util.Mutex;
import com.acer.ccd.cache.data.DlnaAudio;
import com.acer.ccd.cache.data.DlnaContainer;
import com.acer.ccd.cache.data.DlnaImage;
import com.acer.ccd.cache.data.DlnaSearchResult;
import com.acer.ccd.cache.data.DlnaVideo;
import com.acer.ccd.upnp.action.CDSAction;
import com.acer.ccd.upnp.common.ActionThreadCore;
import com.acer.ccd.upnp.common.DlnaAction;
import com.acer.ccd.upnp.common.Item;
import com.acer.ccd.upnp.device.DeviceAction;
import com.acer.ccd.upnp.dmr.DMRTool;
import com.acer.ccd.upnp.util.CBCommand;
import com.acer.ccd.upnp.util.DBManagerUtil;
import com.acer.ccd.upnp.util.Dlna;
import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;
import com.acer.ccd.upnp.util.UpnpTool;

/**
 * The Class DMSAction.
 * 
 * @author chaozhong li
 */
public class DMSAction extends DlnaAction {

    /**
     * The Class BrowseActionThreadCore.
     */
    class BrowseActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":BrowseActionThreadCore");
            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-11)");
                    stop();
                    browseMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_BROWSE_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                } else {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-12)");
                    stop();
                    browseMutex.unlock();
                    notifyService(CBCommand.DMSActionID.BROWSE_ACTION, -1, -1, null,
                            CBCommand.ErrorID.NULL_POINTER);
                    return;
                }
            }

            int tableId = Integer.parseInt(inArgumentList.getArgument(Upnp.Common.COMMON_TABLE_ID)
                    .getValue());

            String uuid = inArgumentList.getArgument(Upnp.Common.COMMON_UUID).getValue();
            if (null == uuid) {
                Logger.w(TAG, "uuid is null pointer!");
                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-21)");
                    stop();
                    browseMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_BROWSE_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                } else {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-22)");
                    stop();
                    browseMutex.unlock();
                    notifyService(CBCommand.DMSActionID.BROWSE_ACTION, -1, -1, null,
                            CBCommand.ErrorID.ARGUMENT_ERROR);
                    return;
                }
            }

            Device dev = DeviceAction.getDMSDevice(uuid);
            if (null == dev) {
                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-31)");
                    stop();
                    browseMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_BROWSE_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                } else {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-32)");
                    stop();
                    browseMutex.unlock();
                    notifyService(CBCommand.DMSActionID.BROWSE_ACTION, -1, -1, uuid,
                            CBCommand.ErrorID.DEVICE_DISAPPEAR);
                    return;
                }
            }

            CDSAction cdsAction = new CDSAction(dev);

            ArgumentList outArgList = cdsAction.browse(inArgumentList.getArgument(
                    Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_OBJECT_ID).getValue(), inArgumentList.getArgument(
                    Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_BROWSE_FLAG).getValue(), inArgumentList
                    .getArgument(Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_FILTER).getValue(), inArgumentList
                    .getArgument(Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_STARTING_INDEX).getValue(),
                    inArgumentList.getArgument(Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_REQUESTED_COUNT)
                            .getValue(), inArgumentList.getArgument(
                            Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_SORT_CRITERIA).getValue());
            if (null != outArgList) {
                List<Item> contentlist = DMSTool.parseContentResult(dev, outArgList
                        .getArgument(Upnp.CDSArgVariable.Browse.CDS_VARIABLE_OUT_RESULT));
                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-41)");
                    stop();
                    browseMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_BROWSE_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                }
                String updateAlbum = inArgumentList.getArgument(Upnp.Common.COMMON_UPDATE_ALBUM).getValue();

                if (updateAlbum.equalsIgnoreCase(DMRTool.defaultTrue)) {
                    // update container album url
                    boolean rst = updateContainerAlbum(inArgumentList.getArgument(
                            Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_OBJECT_ID).getValue(), contentlist, tableId);

                    int total = 0;
                    try {
                        total = Integer.parseInt(outArgList.getArgument(
                                Upnp.CDSArgVariable.Browse.CDS_VARIABLE_OUT_NUMBER_RETURNED).getValue());
                    } catch (NumberFormatException e) {
                        e.printStackTrace();
                    }
                    if (Thread.interrupted()) {
                        Logger
                                .v(TAG, "Thread-" + Thread.currentThread().getId()
                                        + ":(position-51)");
                        stop();
                        browseMutex.unlock();
                        notifyService(CBCommand.DMSActionID.STOP_BROWSE_ACTION, getToken(), -1, null,
                                CBCommand.ErrorID.OK);
                        return;
                    } else {
                        stop();
                        if (rst) {
                            Logger.v(TAG, "Thread-" + Thread.currentThread().getId()
                                    + ":(position-52)");
                            browseMutex.unlock();
                            notifyService(CBCommand.DMSActionID.UPDATE_CONTAINER_ALBUM, total,
                                    tableId, uuid, CBCommand.ErrorID.OK);
                        } else {
                            Logger.v(TAG, "Thread-" + Thread.currentThread().getId()
                                    + ":(position-53)");
                            browseMutex.unlock();
                            notifyService(CBCommand.DMSActionID.UPDATE_CONTAINER_ALBUM, total,
                                    tableId, uuid, CBCommand.ErrorID.DB_ERROR);
                        }
                        return;
                    }
                } else {
                    addBrowseRstToDB(contentlist, tableId);

                    int total = 0;
                    try {
                        total = Integer.parseInt(outArgList.getArgument(
                                Upnp.CDSArgVariable.Browse.CDS_VARIABLE_OUT_NUMBER_RETURNED).getValue());
                    } catch (NumberFormatException e) {
                        e.printStackTrace();
                    }
                    if (Thread.interrupted()) {
                        Logger
                                .v(TAG, "Thread-" + Thread.currentThread().getId()
                                        + ":(position-61)");
                        stop();
                        browseMutex.unlock();
                        notifyService(CBCommand.DMSActionID.STOP_BROWSE_ACTION, getToken(), -1, null,
                                CBCommand.ErrorID.OK);
                        return;
                    } else {
                        Logger
                                .v(TAG, "Thread-" + Thread.currentThread().getId()
                                        + ":(position-62)");
                        stop();
                        browseMutex.unlock();
                        notifyService(CBCommand.DMSActionID.BROWSE_ACTION, total, tableId, uuid,
                                CBCommand.ErrorID.OK);
                        return;
                    }
                }
            } else {
                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-71)");
                    stop();
                    browseMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_BROWSE_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                } else {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-72)");
                    stop();
                    browseMutex.unlock();
                    notifyService(CBCommand.DMSActionID.BROWSE_ACTION, -1, -1, uuid,
                            CBCommand.ErrorID.PROTOCOL_ERROR);
                    return;
                }
            }
        }
    }

    /**
     * The Class searchActionThreadCore.
     */
    class SearchActionThreadCore extends ActionThreadCore {

        /*
         * (non-Javadoc)
         * @see com.acer.clearfi.common.ActionThreadCore#run()
         */
        @Override
        public void run() {
            if (DBG) {
                Logger.v(TAG, "Thread-" + Thread.currentThread().getId()
                        + ":SearchActionThreadCore");
            }
            ArgumentList inArgumentList = getmArgumentList();
            if (null == inArgumentList) {
                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-11)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_SEARCH_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                } else {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-12)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.SEARCH_ACTION, -1, -1, null,
                            CBCommand.ErrorID.NULL_POINTER);
                    return;
                }
            }

            String uuid = inArgumentList.getArgument(Upnp.Common.COMMON_UUID).getValue();
            if (null == uuid) {
                Logger.w(TAG, "uuid is null pointer!");
                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-21)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_SEARCH_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                } else {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-22)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.SEARCH_ACTION, -1, -1, null,
                            CBCommand.ErrorID.ARGUMENT_ERROR);
                    return;
                }
            }

            Device dev = DeviceAction.getDMSDevice(uuid);

            if (null == dev) {
                Logger.w(TAG, "device(uuid) is not a dms device!");
                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-31)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_SEARCH_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                } else {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-32)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.SEARCH_ACTION, -1, -1, null,
                            CBCommand.ErrorID.DEVICE_DISAPPEAR);
                    return;
                }
            }

            CDSAction cdsAction = new CDSAction(dev);
            String searchCriteria = inArgumentList.getArgument(
                    Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_SEARCH_CRITERIA).getValue();
            ArgumentList outArgList = cdsAction.Search(inArgumentList.getArgument(
                    Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_CONTAINER_ID).getValue(), SEARCHTITLE
                    + SEARCHQUOTES + searchCriteria + SEARCHQUOTES + SEARCHOR + SEARCHARTIST
                    + SEARCHQUOTES + searchCriteria + SEARCHQUOTES + SEARCHOR + SEARCHALBUM
                    + SEARCHQUOTES + searchCriteria + SEARCHQUOTES, inArgumentList.getArgument(
                    Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_FILTER).getValue(), inArgumentList.getArgument(
                    Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_STARTING_INDEX).getValue(), inArgumentList
                    .getArgument(Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_REQUESTED_COUNT).getValue(),
                    inArgumentList.getArgument(Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_SORT_CRITERIA)
                            .getValue());
            if (null != outArgList) {
                List<Item> contentlist = DMSTool.parseContentResult(dev, outArgList
                        .getArgument(Upnp.CDSArgVariable.Search.CDS_VARIABLE_OUT_RESULT));

                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-41)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_SEARCH_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                }
                addSearchRstToDB(contentlist, searchCriteria);

                int total = 0;
                try {
                    total = Integer.parseInt(outArgList.getArgument(
                            Upnp.CDSArgVariable.Search.CDS_VARIABLE__OUT_NUMBER_RETURNED).getValue());
                } catch (NumberFormatException e) {
                    e.printStackTrace();
                }
                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-51)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_SEARCH_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                } else {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-52)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.SEARCH_ACTION, total, 0, uuid,
                            CBCommand.ErrorID.OK);
                    return;
                }
            } else {
                if (Thread.interrupted()) {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-61)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.STOP_SEARCH_ACTION, getToken(), -1, null,
                            CBCommand.ErrorID.OK);
                    return;
                } else {
                    Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + ":(position-62)");
                    stop();
                    searchMutex.unlock();
                    notifyService(CBCommand.DMSActionID.SEARCH_ACTION, -1, -1, null,
                            CBCommand.ErrorID.PROTOCOL_ERROR);
                    return;
                }
            }
        }
    }

    /** The Constant tag. */
    private final static String TAG = "DMSAction";

    /** The Constant DBG. */
    private final static boolean DBG = true;

    /** The Constant SearchTitle. */
    private final static String SEARCHTITLE = "dc:title contains ";

    /** The Constant SEARCHARTIST. */
    private final static String SEARCHARTIST = "upnp:artist contains ";

    /** The Constant SEARCHALBUM. */
    private final static String SEARCHALBUM = "upnp:album contains ";

    /** The Constant SEARCHOR. */
    private final static String SEARCHOR = " or ";

    /** The Constant SEARCHQUOTES. */
    private final static String SEARCHQUOTES = "\"";

    /** The search mutex. */
    private Mutex searchMutex = new Mutex();

    /** The browse mutex. */
    private Mutex browseMutex = new Mutex();

    /** The m browse thread. */
    BrowseActionThreadCore mBrowseThread = new BrowseActionThreadCore();

    /** The m search thread. */
    SearchActionThreadCore mSearchThread = new SearchActionThreadCore();

    /**
     * Adds the content to db.
     * 
     * @param contentlist the contentlist
     * @param tableId the table id
     */
    private void addBrowseRstToDB(List<Item> contentlist, int tableId) {
        if (null == contentlist) {
            return;
        }

        // print all content
        int size = contentlist.size();
        for (int idx = 0; idx < size; ++idx) {
            Item item = contentlist.get(idx);
            if (null == item) {
                continue;
            }
            Logger.v(TAG, "index:" + idx);
            DMSTool.printItem(TAG, item);

            String objectclass = item.getObjectClass();
            if (null == objectclass) {
                continue;
            }

            if (objectclass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_AUDIOITEM)) {
                DlnaAudio dlnaAudio = new DlnaAudio();
                dlnaAudio.setAlbum(item.getAlbum());
                dlnaAudio.setArtist(item.getArtist());
                dlnaAudio.setCreator(item.getCreator());
                dlnaAudio.setDateTaken(item.getDateTaken());
                dlnaAudio.setDescription(item.getDescription());
                dlnaAudio.setDeviceName(item.getDeviceName());
                dlnaAudio.setDeviceUuid(item.getDeviceUuid());
                dlnaAudio.setDuration(item.getDuration());
                try {
                    dlnaAudio.setFileSize(Long.parseLong(item.getFileSize()));
                } catch (NumberFormatException e) {
                    e.printStackTrace();
                    dlnaAudio.setFileSize(0);
                }
                dlnaAudio.setFormat(item.getFormat());
                dlnaAudio.setGenre(item.getGenre());
                dlnaAudio.setAlbumUrl(item.getAlbumUrl());
                dlnaAudio.setPublisher(item.getPublisher());
                dlnaAudio.setTitle(item.getTitle());
                if(null == item.getTrackNo()){
                    dlnaAudio.setTrackNo(0);
                }
                else{
                    dlnaAudio.setTrackNo(Integer.parseInt(item.getTrackNo()));
                }
                dlnaAudio.setUrl(item.getUrl());
                dlnaAudio.setProtocolName(item.getProtocolInfo());

                dlnaAudio.setParentContainerId(item.getParentid());

                long idx1 = DBManagerUtil.getDBManager().addAudio(dlnaAudio, tableId);
                DlnaAudio dlnaAudioTmp = DBManagerUtil.getDBManager().getAudio(idx1, tableId);

                DMSTool.printDBAudioItem(TAG, dlnaAudioTmp);
            } else if (objectclass
                    .contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_VIDEOITEM)) {
                DlnaVideo dlnaVideo = new DlnaVideo();
                dlnaVideo.setActor(item.getActor());
                dlnaVideo.setAlbum(item.getAlbum());
                dlnaVideo.setArtist(item.getArtist());
                dlnaVideo.setBitRate(item.getBitRate());
                dlnaVideo.setCreator(item.getCreator());
                dlnaVideo.setDateTaken(item.getDateTaken());
                dlnaVideo.setDescription(item.getDescription());
                dlnaVideo.setDeviceName(item.getDeviceName());
                dlnaVideo.setDuration(item.getDuration());
                try {
                    dlnaVideo.setFileSize(Long.parseLong(item.getFileSize()));
                } catch (NumberFormatException e) {
                    e.printStackTrace();
                    dlnaVideo.setFileSize(0);
                }
                dlnaVideo.setFormat(item.getFormat());
                dlnaVideo.setGenre(item.getGenre());
                dlnaVideo.setAlbumUrl(item.getAlbumUrl());
                dlnaVideo.setPublisher(item.getPublisher());
                dlnaVideo.setResolution(item.getResolution());
                dlnaVideo.setTitle(item.getTitle());
                dlnaVideo.setUrl(item.getUrl());
                dlnaVideo.setProtocolName(item.getProtocolInfo());

                dlnaVideo.setParentContainerId(item.getParentid());
                long idx1 = DBManagerUtil.getDBManager().addVideo(dlnaVideo, tableId);
                DlnaVideo dlnaVideoTmp = DBManagerUtil.getDBManager().getVideo(idx1, tableId);

                DMSTool.printDBVideoItem(TAG, dlnaVideoTmp);
            } else if (objectclass
                    .contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_IMAGEITEM)) {
                DlnaImage dlnaImage = new DlnaImage();
                dlnaImage.setAlbum(item.getAlbum());
                dlnaImage.setCreator(item.getCreator());
                dlnaImage.setDateTaken(item.getDateTaken());
                dlnaImage.setDescription(item.getDescription());
                dlnaImage.setDeviceName(item.getDeviceName());
                try {
                    dlnaImage.setFileSize(Long.parseLong(item.getFileSize()));
                } catch (NumberFormatException e) {
                    e.printStackTrace();
                    dlnaImage.setFileSize(0);
                }
                dlnaImage.setFormat(item.getFormat());
                dlnaImage.setAlbumUrl(item.getAlbumUrl());
                dlnaImage.setPublisher(item.getPublisher());
                dlnaImage.setResolution(item.getResolution());
                dlnaImage.setTitle(item.getTitle());
                dlnaImage.setUrl(item.getUrl());
                dlnaImage.setThumbnailUrl(item.getThumbnailUrl());
                dlnaImage.setProtocolName(item.getProtocolInfo());

                dlnaImage.setParentContainerId(item.getParentid());

                long idx1 = DBManagerUtil.getDBManager().addImage(dlnaImage, tableId);
                DlnaImage dlnaImageTmp = DBManagerUtil.getDBManager().getImage(idx1, tableId);

                DMSTool.printDBImageItem(TAG, dlnaImageTmp);

            } else if (objectclass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_CONTAINER)) {
                DlnaContainer dlnaContainer = new DlnaContainer();
                dlnaContainer.setContainerId(item.getId());
                dlnaContainer.setTitle(item.getTitle());

                dlnaContainer.setParentContainerId(item.getParentid());
                DBManagerUtil.getDBManager().addContainer(dlnaContainer, tableId);

                DMSTool.printDBContainer(TAG, dlnaContainer);
            } else if (objectclass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM)) {
            } else {

            }
        }
    }

    /**
     * Adds the search rst to db.
     * 
     * @param contentlist the contentlist
     * @param keyword the keyword
     */
    private void addSearchRstToDB(List<Item> contentlist, String keyword) {
        if (null == contentlist) {
            return;
        }
        // print all content
        int size = contentlist.size();
        for (int idx = 0; idx < size; ++idx) {
            Item item = contentlist.get(idx);
            if (null == item) {
                continue;
            }
            String objectClass = item.getObjectClass();
            if (null == objectClass) {
                continue;
            }

            Logger.v(TAG, "index:" + idx);
            DMSTool.printItem(TAG, item);
            DlnaSearchResult dlnaSearchResult = new DlnaSearchResult();

            if (objectClass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_AUDIOITEM)) {
                dlnaSearchResult.setArtist(item.getArtist());
                dlnaSearchResult.setDeviceName(item.getDeviceName());
                dlnaSearchResult.setMusicAlbum(item.getAlbum());
                dlnaSearchResult.setMusic(item.getTitle());
                dlnaSearchResult.setMusicDuration(item.getDuration());
                dlnaSearchResult.setUrl(item.getUrl());
                dlnaSearchResult.setAlbumUrl(item.getAlbumUrl());

                dlnaSearchResult.setParentContainerId(item.getParentid());
            } else if (objectClass
                    .contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_VIDEOITEM)) {
                dlnaSearchResult.setArtist(item.getArtist());
                dlnaSearchResult.setDeviceName(item.getDeviceName());
                dlnaSearchResult.setVideo(item.getTitle());
                dlnaSearchResult.setVideoAlbum(item.getAlbum());
                dlnaSearchResult.setMusicDuration(item.getDuration()); 
                dlnaSearchResult.setUrl(item.getUrl());
                dlnaSearchResult.setAlbumUrl(item.getAlbumUrl());

                dlnaSearchResult.setParentContainerId(item.getParentid());
            } else if (objectClass
                    .contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_IMAGEITEM)) {
                dlnaSearchResult.setArtist(item.getArtist());
                dlnaSearchResult.setDeviceName(item.getDeviceName());
                dlnaSearchResult.setPhoto(item.getTitle());
                dlnaSearchResult.setPhotoAlbum(item.getAlbum());
                dlnaSearchResult.setUrl(item.getUrl());
                dlnaSearchResult.setAlbumUrl(item.getAlbumUrl());

                dlnaSearchResult.setParentContainerId(item.getParentid());
            } else if (objectClass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM)) {
                dlnaSearchResult.setArtist(item.getArtist());
                dlnaSearchResult.setDeviceName(item.getDeviceName());
                dlnaSearchResult.setItemName(item.getTitle());
                dlnaSearchResult.setUrl(item.getUrl());
                dlnaSearchResult.setAlbumUrl(item.getAlbumUrl());

                dlnaSearchResult.setParentContainerId(item.getParentid());
            } else if (objectClass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_CONTAINER)) {
                dlnaSearchResult.setArtist(item.getArtist());
                dlnaSearchResult.setDeviceName(item.getDeviceName());
                dlnaSearchResult.setContainerName(item.getTitle());
                dlnaSearchResult.setUrl(item.getUrl());
                dlnaSearchResult.setAlbumUrl(item.getAlbumUrl());

                dlnaSearchResult.setParentContainerId(item.getParentid());
            } else {
            }
            DMSTool.printSearchResult(TAG, "Server", dlnaSearchResult);
            long dbIdx = DBManagerUtil.getDBManager().addSearchResult(dlnaSearchResult, keyword);

            DlnaSearchResult dbSearchResult = new DlnaSearchResult();
            DBManagerUtil.getDBManager().getSearchResult(dbIdx, dbSearchResult);
            DMSTool.printSearchResult(TAG, "DB", dbSearchResult);

        }
    }

    /**
     * Browse.
     * 
     * @param uuid the uuid
     * @param ObjectID the object id
     * @param BrowseFlag the browse flag
     * @param Filter the filter
     * @param StartingIndex the starting index
     * @param RequestedCount the requested count
     * @param SortCriteria the sort criteria
     * @param tableId the table id
     * @param updateAlbum the update album
     */
    public void browse(String uuid, String ObjectID, String BrowseFlag, String Filter,
            String StartingIndex, String RequestedCount, String SortCriteria, int tableId,
            String updateAlbum) {

        browseMutex.lock();
        try {
            if (DBG) {
                Logger.v(TAG, "uuid: " + uuid);
                Logger.v(TAG, "ObjectID: " + ObjectID);
                Logger.v(TAG, "BrowseFlag: " + BrowseFlag);
                Logger.v(TAG, "Filter: " + Filter);
                Logger.v(TAG, "StartingIndex: " + StartingIndex);
                Logger.v(TAG, "RequestedCount: " + RequestedCount);
                Logger.v(TAG, "SortCriteria: " + SortCriteria);
                Logger.v(TAG, "bUpdateAlbum: " + updateAlbum);
            }
            ArgumentList argumentList = new ArgumentList();
            UpnpTool.addArg(argumentList, Upnp.Common.COMMON_UUID, uuid);
            UpnpTool.addArg(argumentList, Upnp.Common.COMMON_TABLE_ID, String.valueOf(tableId));
            UpnpTool.addArg(argumentList, Upnp.Common.COMMON_UPDATE_ALBUM, updateAlbum);
            UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_OBJECT_ID, ObjectID);
            UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_BROWSE_FLAG, BrowseFlag);
            UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_FILTER, Filter);
            UpnpTool
                    .addArg(argumentList, Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_STARTING_INDEX, StartingIndex);
            UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_REQUESTED_COUNT,
                    RequestedCount);
            UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Browse.CDS_VARIABLE_IN_SORT_CRITERIA, SortCriteria);

            if (mBrowseThread.isRunnable()) {
                mBrowseThread.restart(argumentList);
            } else {
                mBrowseThread.start(argumentList);
            }

            Thread.sleep(50);
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            browseMutex.unlock();
        }
    }

    /*
     * (non-Javadoc)
     * @see com.acer.clearfi.common.DlnaAction#init()
     */
    @Override
    public void init() {

    }

    /**
     * Notify service.
     * 
     * @param command the command
     * @param total the total
     * @param tableId the table id
     * @param Uuid the uuid
     * @param errCode the err code
     */
    private void notifyService(int command, int total, int tableId, String Uuid, int errCode) {
        int listenerSize = getListenerlist().size();
        for (int n = 0; n < listenerSize; n++) {
            DMSActionListener listener = (DMSActionListener)getListenerlist().get(n);
            listener.dmsNotify(command, total, tableId, Uuid, errCode);
        }
    }

    /*
     * (non-Javadoc)
     * @see com.acer.clearfi.common.DlnaAction#release()
     */
    @Override
    public void release() {

    }

    /**
     * Removes the all brows content.
     * 
     * @param tableId the table id
     */
    @SuppressWarnings("unused")
    private void removeAllBrowsRstFromDB(int tableId) {
        DBManagerUtil.getDBManager().deleteAllAudio(tableId);
        DBManagerUtil.getDBManager().deleteAllVideo(tableId);
        DBManagerUtil.getDBManager().deleteAllImages(tableId);
        DBManagerUtil.getDBManager().deleteAllContainers(tableId);
    }

    /**
     * Removes the search result.
     */
    @SuppressWarnings("unused")
    private void removeSearchRstFromDB() {
        DBManagerUtil.getDBManager().deleteAllSearchResults();
    }

    /**
     * Search.
     * 
     * @param uuid the uuid
     * @param ContainerID the container id
     * @param SearchCriteria the search criteria
     * @param Filter the filter
     * @param StartingIndex the starting index
     * @param RequestedCount the requested count
     * @param SortCriteria the sort criteria
     */
    public void search(String uuid, String ContainerID, String SearchCriteria, String Filter,
            String StartingIndex, String RequestedCount, String SortCriteria) {
        if (DBG) {
            Logger.v(TAG, "search(" + uuid + ", " + ContainerID + ", " + SearchCriteria + ", "
                    + Filter + ", " + StartingIndex + ", " + RequestedCount + ", " + SortCriteria
                    + ")");
        }
        ArgumentList argumentList = new ArgumentList();
        UpnpTool.addArg(argumentList, Upnp.Common.COMMON_UUID, uuid);
        UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_CONTAINER_ID, ContainerID);
        UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_SEARCH_CRITERIA, SearchCriteria);
        UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_FILTER, Filter);
        UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_STARTING_INDEX, StartingIndex);
        UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_REQUESTED_COUNT, RequestedCount);
        UpnpTool.addArg(argumentList, Upnp.CDSArgVariable.Search.CDS_VARIABLE_IN_SORT_CRITERIA, SortCriteria);

        mSearchThread.restart(argumentList);

    }

    /**
     * Stop browser content.
     * 
     * @param command the command
     * @param token the token
     */
    public void stopBrowserContent(int command, int token) {
        Logger.i(TAG, "stopBrowserContent mutex before");
        browseMutex.lock();
        Logger.i(TAG, "stopBrowserContent ing");
        try {
            mBrowseThread.setToken(token);
            if (mBrowseThread.isRunnable()) {
                Logger.v(TAG, "Thread-" + Thread.currentThread().getId()
                        + "stopBrowserContent111(ui invoke)");
                mBrowseThread.stop();
            } else {
                Logger.v(TAG, "Thread-" + Thread.currentThread().getId()
                        + "stopBrowserContent222(ui invoke)");
                browseMutex.unlock();
                notifyService(command, token, -1, null, CBCommand.ErrorID.OK);
            }

            Thread.sleep(50);
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            browseMutex.unlock();
        }

    }

    /**
     * Stop search content.
     * 
     * @param command the command
     * @param token the token
     */
    public void stopSearchContent(int command, int token) {
        searchMutex.lock();
        try {
            mSearchThread.setToken(token);
            if (mSearchThread.isRunnable()) {
                Logger.v(TAG, "Thread-" + Thread.currentThread().getId()
                        + "stopSearchContent111(ui invoke)");
                mSearchThread.stop();
            } else {
                Logger.v(TAG, "Thread-" + Thread.currentThread().getId()
                        + "stopSearchContent222(ui invoke)");
                searchMutex.unlock();
                notifyService(command, token, -1, null, CBCommand.ErrorID.OK);
            }
        } finally {
            searchMutex.unlock();
        }
    }

    /**
     * Update container album.
     * 
     * @param containerId the container id
     * @param contentlist the contentlist
     * @param tableId the table id
     * @return true, if successful
     */
    private boolean updateContainerAlbum(String containerId, List<Item> contentlist, int tableId) {
        if (null == contentlist) {
            return false;
        }

        // print all content
        int size = contentlist.size();
        if (size < 1) {
            return false;
        }
// B Kyle Org
        Item item = contentlist.get(0);
        if (null == item) {
            return false;
        }
        DMSTool.printItem(TAG, item);

        String objectclass = item.getObjectClass();
        if (null == objectclass) {
            return false;
        }

        String url = Dlna.common.UNKNOWN;
        if (objectclass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_AUDIOITEM)) {
            String tmp = item.getAlbumUrl();
            if (null != tmp) {
                url = tmp;
            }
        } else if (objectclass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_VIDEOITEM)) {
            String tmp = item.getAlbumUrl();
            if (null != tmp) {
                url = tmp;
            }
        } else if (objectclass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_IMAGEITEM)) {
            String tmpThumbnail = item.getThumbnailUrl();
            String tmpUrl = item.getUrl();
            if (null != tmpThumbnail) {
                url = tmpThumbnail;
            } else if (null != tmpUrl) {
                url = tmpUrl;
            }
        } else {
        }

        return DBManagerUtil.getDBManager().updateContainerAlbumUrl(containerId, url, tableId);
// E Kyle Org
/*
        String url = Dlna.common.UNKNOWN;
        int count = size > 4 ? 4 : size;
        Logger.i(TAG, "count = " + count);
        for(int i = 0; i < count; i ++){
            Item item = contentlist.get(i);
            if (null == item && 0 == i) {
                return false;
            }
            else if(null == item){
                continue;
            }
            DMSTool.printItem(TAG, item);

            String objectclass = item.getObjectClass();
            if (null == objectclass) {
                return false;
            }
            
            if (objectclass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_AUDIOITEM)) {
                String tmp = item.getAlbumUrl();
                if (null != tmp) {
                    url = tmp;
                }
            } else if (objectclass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_VIDEOITEM)) {
                String tmp = item.getAlbumUrl();
                if (null != tmp) {
                    url = tmp;
                }
            } else if (objectclass.contains(Upnp.ContentProp.upnp.classProperty.OBJECT_ITEM_IMAGEITEM)) {
                String tmpThumbnail = item.getThumbnailUrl();
                String tmpUrl = item.getUrl();
                if (null != tmpThumbnail) {
                    url += tmpThumbnail + "|";
                } else if (null != tmpUrl) {
                    url += tmpUrl + "|";
                }
            } else {
            }

            Logger.i(TAG, "url = " + url);
        }

        if ( url.endsWith( "|" ) ) {
            url = url.replaceFirst( "\\|$", "" );
        }

        return DBManagerUtil.getDBManager().updateContainerAlbumUrl(containerId, url, tableId);
*/
    }
}
