/******************************************************************
 *
 *	Clear.fi for Android
 *
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 *
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

package com.acer.ccd.service;

/**
 * @author chaozhong li
 * 
 */
import org.cybergarage.util.Mutex;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteCallbackList;
import android.os.RemoteException;

import com.acer.ccd.service.IDlnaService;
import com.acer.ccd.service.IDlnaServiceCallback;

import com.acer.ccd.cache.DBManager;
import com.acer.ccd.cache.data.DlnaAudio;
import com.acer.ccd.cache.data.DlnaImage;
import com.acer.ccd.cache.data.DlnaVideo;
import com.acer.ccd.upnp.device.DeviceAction;
import com.acer.ccd.upnp.device.DeviceListener;
import com.acer.ccd.upnp.dmr.DMRAction;
import com.acer.ccd.upnp.dmr.DMRActionListener;
import com.acer.ccd.upnp.dmr.DMRTool;
import com.acer.ccd.upnp.dmr.DmrActionList;
import com.acer.ccd.upnp.dms.DMSAction;
import com.acer.ccd.upnp.dms.DMSActionListener;
import com.acer.ccd.upnp.util.CBCommand;
import com.acer.ccd.upnp.util.DBManagerUtil;
import com.acer.ccd.upnp.util.Logger;
import com.acer.ccd.upnp.util.Upnp;
import com.acer.ccd.util.TimeFormatter;

/**
 * The Class DlnaService.
 */
public class DlnaService extends Service implements DMRActionListener, DMSActionListener,
        DeviceListener {

    /** The mutex. */
    private Mutex mutex = new Mutex();

    /** The TAG. */
    private final String TAG = "DlnaService";

    /** The dev action. */
    private DeviceAction devAction = null;

    /** The dmr action. */
    private DMRAction mDmrAction = null;

    /** The dms action. */
    private DMSAction dmsAction = null;
    private DMSAction dmsAlbumAction = null;

    /** The m service. */
    private DlnaService mService = null;
    private boolean mIsBind = false;
    private Handler mStopHandler;
    private StopThread mStopThread = null;
    private IntentFilter mWifiStateFilter;
    private static final int STOP_NOW = 0;
    private static final int STOP_LATER = 1;
    private DmrActionList mDmrActionList = null;
    
    class StopThread extends Thread{
        public StopThread(String name){
            super(name);
        }
    	public void run() {
            Looper.prepare();
            mStopHandler = new Handler() {
                @Override
                public void handleMessage(Message msg) {
                    super.handleMessage(msg);
                    
                    switch(msg.what){
                    case STOP_LATER:
                    	
                    	try{
                        	Thread.sleep(1000);
                        }
                        catch(Exception e){
                        	e.printStackTrace();
                        }
                        
                        if(!mIsBind){
                        	Logger.i(TAG, "stop later");
                        	stopSelf();
                        }
                    	break;
                    
                    case STOP_NOW:
                    	Logger.i(TAG, "stop now");
                    	stopSelf();
                    	break;
                    
                    default:
                    	break;

                    
                    }
                    
                    Logger.i(TAG, "quit 1");
                    Looper.myLooper().quit();
                    Logger.i(TAG, "quit 2");
                    
                    
                    
                }
            };
            Looper.loop();
        }
    }
    
    private BroadcastReceiver mWifiStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if(intent == null || intent.getAction() == null){
                return;
            }
            if (intent.getAction().equals(WifiManager.WIFI_STATE_CHANGED_ACTION)) {

                int state = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE, WifiManager.WIFI_STATE_UNKNOWN);
                if (state == WifiManager.WIFI_STATE_DISABLED) {
                }
            }
            else if (intent.getAction().equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)) {

                NetworkInfo netInfo = (NetworkInfo)intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
                if (netInfo != null) {
                    if (netInfo.isConnected()) {
                    	Logger.i(TAG, "wifi enable BriadcastReceiver");
                    	if(null == devAction){
                    	    initAction();
                    	}
                    }
                    else{
                    	Logger.i(TAG, "wifi disable BriadcastReceiver");
                    	new Thread("wifi disable release action"){
                            public void run(){
                                releaseAction();
                            }
                        }.start();
                    }
                }
            }
        }
    };

    /** The m binder. */
    private final IDlnaService.Stub mBinder = new IDlnaService.Stub() {

        @Override
        public void addNextAsync(int command, String url, String protocolInfo, String dmrUuid)
            throws RemoteException {
            Logger.v(TAG, "addNextAsync(" + url + ", " + protocolInfo + ")");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(command, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
            }
            if (url == null || url.trim().equals("") || url.trim().equals("")) {
                mService.dmrNotify(command, dmrUuid,
                        CBCommand.ErrorID.ARGUMENT_ERROR);
            }
            dmrAction.addNextURI(command, url, protocolInfo);
        }

        @Override
        public void attachDMRAsync(String dmsUuid, String dmrUuid) throws RemoteException {
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " IDlnaService.attachDMR()");
            
            if (mDmrActionList == null) {
                Logger.e(TAG, "attachDMRAsync() error, mDmrActionList should not be null, reallocate again");
                mDmrActionList = new DmrActionList();
            }
            mDmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if(null == mDmrAction){
                mDmrAction = new DMRAction(DlnaService.this);
            }
            
            
            Logger.i(TAG, "dmsUuid = " + dmsUuid);
            Logger.i(TAG, "dmrUuid = " + dmrUuid);
            
            
            if (null == mDmrAction) {
                mService.dmrNotify(CBCommand.DMRActionID.ATTACH, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            if (null == dmsUuid || null == dmrUuid) {
                mService.dmrNotify(CBCommand.DMRActionID.ATTACH, dmrUuid,
                        CBCommand.ErrorID.ARGUMENT_ERROR);
                return;
            }
      
            mDmrAction.attachDMR(CBCommand.DMRActionID.ATTACH, dmsUuid, dmrUuid);
            
            
            
        }

        @Override
        public long avDuration(String dmrUuid) throws RemoteException {
            Logger.v(TAG, "avDuration()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                return -1;
            }
            return dmrAction.duration();
        }

        @Override
        public void avPositionAsync(String dmrUuid) throws RemoteException {
            Logger.v(TAG, "avPosition()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(CBCommand.DMRActionID.AV_POSITION, dmrUuid, -1);
                return;
            }
            dmrAction.position(CBCommand.DMRActionID.AV_POSITION);
        }

        @Override
        public void SeekToTrackAsync(int command, int track, String dmrUuid) 
                throws RemoteException {
            Logger.v(TAG, "SeekToTrackAsync()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(command, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.seekToTrack(command, track);
        }

        @Override
        public void avSeekToAsync(int pos, String dmrUuid) throws RemoteException {
            Logger.v(TAG, "avSeekToAsync()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(CBCommand.DMRActionID.SEEK_TO, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.seekTo(CBCommand.DMRActionID.SEEK_TO, pos);
        }

        @Override
        public void browserContent(String uuid, String ContainerID, int StartingIndex,
                int RequestedCount, String SortCriteria, int tableId) throws RemoteException {
            Logger.v(TAG, "browserContent()");
            if (null == dmsAction) {
                mService.dmsNotify(CBCommand.DMSActionID.BROWSE_ACTION, -1, -1, uuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmsAction.browse(uuid, ContainerID, Upnp.CDSArgVariable.Browse.CDS_VARIABLE_BROWSE_DIRECT_CHILDREN,
                    "*", String.valueOf(StartingIndex), String.valueOf(RequestedCount),
                    SortCriteria, tableId, DMRTool.defaultFalse);
        }

        @Override
        public void detachDMRAsync(String dmrUuid) throws RemoteException {
            Logger.v(TAG, "detachDMR()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(CBCommand.DMRActionID.DETACH, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.detachDMR(CBCommand.DMRActionID.DETACH);
        }

        @Override
        public int getAVCurrentVolume(String dmrUuid) throws RemoteException {
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                return -1;
            }
            return dmrAction.getCurVolume();
        }

        @Override
        public int getAVMaxVolume(String dmrUuid) throws RemoteException {
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                return -1;
            }
            return dmrAction.getMaxVolume();
        }

        @Override
        public int getAVMinVolume(String dmrUuid) throws RemoteException {
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                return -1;
            }
            return dmrAction.getMinVolume();
        }

        @Override
        public boolean getAVMute(String dmrUuid) throws RemoteException {
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                return false;
            }
            return dmrAction.getMute();
        }

        @Override
        public void updateContainerAlbum(String uuid, String containerID, int table)
                throws RemoteException {
            Logger.v(TAG, "getContainerAlbum(" + uuid + ", " + containerID + ", " + table + ")");
            if (null == dmsAlbumAction) {
                mService.dmsNotify(CBCommand.DMSActionID.UPDATE_CONTAINER_ALBUM, -1, -1, uuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }

            dmsAlbumAction.browse(uuid, containerID, Upnp.CDSArgVariable.Browse.CDS_VARIABLE_BROWSE_DIRECT_CHILDREN,
                    "*", String.valueOf(0), String.valueOf(1), null, table, DMRTool.defaultTrue);
        }

        @Override
        public void getDMRStatus(String uuid) throws RemoteException {
            Logger.v(TAG, "getDMRStatus(" + uuid + ")");
            DMRAction dmrAction = new DMRAction(DlnaService.this);
            if (null == dmrAction) {
                mService.dmrNotify(CBCommand.DMRActionID.DMR_STATUS, uuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.getDMRStatus(CBCommand.DMRActionID.DMR_STATUS, uuid);
        }

        @Override
        public int getPhotoMaxZoomLevel(String dmrUuid) throws RemoteException {
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                return -1;
            }
            return 0; // TBD
        }

        @Override
        public int getPhotoZoomLevel(String dmrUuid) throws RemoteException {
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                return -1;
            }
            return 0; // TBD
        }

        @Override
        public boolean isAvPlaying(String dmrUuid) throws RemoteException {
            Logger.v(TAG, "isMusicPlaying()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                return false;
            }
            return dmrAction.isPlaying();
        }

        @Override
        public void nextAsync(int command, String dmrUuid) throws RemoteException {
            Logger.v(TAG, "nextAvAsync()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(command, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.next(command);
        }

        @Override
        public void openAvPlaylistAsync(long[] itemIdList, int tbid, String mediaType, long playlistId, String dmrUuid)
            throws RemoteException {
        	DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
        	Logger.e(TAG, "Thread-" + Thread.currentThread().getId() + " openAvPlaylistAsync(" + itemIdList.length + ", " + tbid + ", " + mediaType + ")");
            if (null == dmrAction) {
                mService.dmrNotify(CBCommand.DMRActionID.OPEN_AV, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }

            String firstUrl = "";
            String uriMetaData = 
            	"<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
            	+ "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
            	+ "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">";

            if (mediaType.equals("MediaType_Audio")) {
                DlnaAudio audios[];

                if (tbid == DBManager.CntntTbl.TBL_PL) {
                    if (playlistId != DBManager.INVALID_DBID) {
                         audios = DBManagerUtil.getDBManager().getPlaylistMusic(itemIdList, playlistId);
                    } else {
                         audios = DBManagerUtil.getDBManager().getPlaylistMusic(itemIdList);
                    }
                } else { 
                    audios = DBManagerUtil.getDBManager().getAudio(itemIdList, tbid);
                }

                firstUrl = audios[0].getUrl();

                for(DlnaAudio audio: audios) {
                    uriMetaData +=
                    	"<item><dc:title>"
                    	+ audio.getTitle()
                    	+ "</dc:title><res size=\""
                    	+ ((0 == audio.getFileSize()) ? "" : audio.getFileSize())
                    	+ "\" duration=\""
                    	+ TimeFormatter.makeTimeString(audio.getDuration()*1000)
                    	+ "\" protocolInfo=\""
                    	+ audio.getProtocolName()
                    	+ "\">"
                	    + audio.getUrl()
                	   	+ "</res><upnp:artist role=\"AlbumArtist\">"
                	   	+ audio.getArtist()
                	    + "</upnp:artist><upnp:album>"
                	    + audio.getAlbum()
                	    + "</upnp:album><upnp:albumArtURI dlna:profileID=\"JPEG_SM\">"
                	    + audio.getAlbumUrl()
                	    + "</upnp:albumArtURI></item>";
                }
            }
            else {
            	firstUrl = DBManagerUtil.getDBManager().getVideo(itemIdList[0], tbid).getUrl();

                for(DlnaVideo video: DBManagerUtil.getDBManager().getVideo(itemIdList, tbid)) {
                    uriMetaData +=
                    	"<item><dc:title>"
                    	+ video.getTitle()
                    	+ "</dc:title><res size=\""
                    	+ ((0 == video.getFileSize()) ? "" : video.getFileSize())
                    	+ "\" duration=\""
                    	+ TimeFormatter.makeTimeString(video.getDuration()*1000)
                    	+ "\" protocolInfo=\""
                    	+ video.getProtocolName()
                    	+ "\">"
                	    + video.getUrl()
                	   	+ "</res><upnp:artist role=\"AlbumArtist\">"
                	   	+ video.getArtist()
                	    + "</upnp:artist><upnp:album>"
                	    + video.getAlbum()
                	    + "</upnp:album><upnp:albumArtURI dlna:profileID=\"JPEG_SM\">"
                	    + video.getAlbumUrl()
                	    + "</upnp:albumArtURI></item>";
                }
            }

            uriMetaData +=  "</DIDL-Lite>";
            Logger.e(TAG, "Metadata: " + uriMetaData);
        	dmrAction.open(CBCommand.DMRActionID.OPEN_AV, firstUrl, uriMetaData);
        }

        @Override
        public void openAsync( int command, String url, String title, String protocolInfo, String dmrUuid )
            throws RemoteException {
            Logger.v(TAG, "Thread-" + Thread.currentThread().getId() + " openAsync(" + url + ", "
                    + title + ", " + protocolInfo + ")");

            String uriMetaData = 
                "<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
                + "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
                + "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">"
                + "<item><dc:title>"
                + title
                + "</dc:title><res protocolInfo=\""
                + protocolInfo
                + "\"></res></item></DIDL-Lite>";

            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);

            if (null == dmrAction) {
                mService.dmrNotify(command, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            if (url == null || url.trim().equals("")) {
                mService.dmrNotify(command, dmrUuid,
                        CBCommand.ErrorID.ARGUMENT_ERROR);
                return;
            }

            dmrAction.open(command, url, uriMetaData);
        }

        @Override
        public void openPhotoPlaylistAsync(long[] itemIdList, int tbid, String dmrUuid) 
            throws RemoteException {
        	Logger.e(TAG, "Thread-" + Thread.currentThread().getId() + " openPhotoPlaylistAsync(" + itemIdList.length + ", " + tbid + ")");

            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            
            if (null == dmrAction) {
                mService.dmrNotify(CBCommand.DMRActionID.OPEN_PHOTO, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }

            String firstUrl = "";
            String uriMetaData = 
            	"<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
            	+ "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
            	+ "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">";

            firstUrl = DBManagerUtil.getDBManager().getImage(itemIdList[0], tbid).getUrl();

            for(DlnaImage image: DBManagerUtil.getDBManager().getImages(itemIdList, tbid)) {
                uriMetaData +=
                	"<item><dc:title>"
                	+ image.getTitle()
                	+ "</dc:title><res size=\""
                	+ ((0 == image.getFileSize()) ? "" : image.getFileSize())
                	+ "\" resolution=\""
                	+ image.getResolution()
                	+ "\" protocolInfo=\""
                	+ image.getProtocolName()
                	+ "\">"
            	    + image.getUrl()
            	   	+ "</res><upnp:albumArtURI dlna:profileID=\"JPEG_SM\">"
            	    + image.getThumbnailUrl()
            	    + "</upnp:albumArtURI></item>";
            }
            
            uriMetaData += "</DIDL-Lite>";

            if (firstUrl == null || firstUrl.trim().equals("")) {
                mService.dmrNotify(CBCommand.DMRActionID.OPEN_PHOTO, dmrUuid,
                        CBCommand.ErrorID.ARGUMENT_ERROR);
                return;
            }
            dmrAction.open(CBCommand.DMRActionID.OPEN_PHOTO, firstUrl, uriMetaData);
        }

        @Override
        public void pauseAsync(int command, String dmrUuid) throws RemoteException {
            Logger.v(TAG, "pauseAsync()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(command, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.pause(command);
        }

        @Override
        public void photoRotateLeftAsync(String dmrUuid) throws RemoteException {
            Logger.v(TAG, "photoRotateLeftAsync()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                return;
            }
            // TBD
        }

        @Override
        public void photoRotateRightAsync(String dmrUuid) throws RemoteException {
            Logger.v(TAG, "photoRotateRightAsync()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                return;
            }
            // TBD
        }

        @Override
        public void playAsync(int command, String dmrUuid) throws RemoteException {
            Logger.v(TAG,"Thread-" + Thread.currentThread().getId() + " playAsync()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);

            if (null == dmrAction) {
                mService.dmrNotify(command, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.play(command);
        }

        @Override
        public void prevAsync(int command, String dmrUuid) throws RemoteException {
            Logger.v(TAG, "prevMusic()");
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(command, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.prev(command);
        }

        @Override
        public void recordAsync(String dmrUuid) throws RemoteException {
            Logger.v(TAG, "recordAsync()");
            
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            
            if (null == dmrAction) {
                mService.dmrNotify(CBCommand.DMRActionID.RECORD, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.record(CBCommand.DMRActionID.RECORD);
        }

        @Override
        public void registerCallback(IDlnaServiceCallback cb) throws RemoteException {
            Logger.v(TAG, "IDlnaService.registerCallback()");
            if (cb != null) {
                mCallbacks.register(cb);
            }
        }

        @Override
        public void searchContent(String Uuid, String ContainerID, String searchCriteria,
                int StartingIndex, int RequestedCount, String SortCriteria) throws RemoteException {

            Logger.v(TAG, "IDlnaService.searchContent(" + Uuid + ", " + ContainerID + ", "
                    + searchCriteria + ", " + StartingIndex + ", " + RequestedCount + ", "
                    + SortCriteria + ")");
            if (null != dmsAction) {
                dmsAction.search(Uuid, ContainerID, searchCriteria, "*", String
                        .valueOf(StartingIndex), String.valueOf(RequestedCount), SortCriteria);
            } else {
                mService.dmsNotify(CBCommand.DMSActionID.SEARCH_ACTION, -1, -1, Uuid,
                        CBCommand.ErrorID.UNINITIALIZED);
            }
        }

        @Override
        public void searchDevices(boolean reset) throws RemoteException {
            Logger.v(TAG, "IDlnaService.searchDevices() reset=" + reset);
            if (true == reset) {
                releaseAction();
                initAction();
            }
            if (null == devAction) {
                mService.devNotifyReceived(CBCommand.DevNotifyID.SEARCH_DEVICE, -1,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            devAction.refreshDevices();
        }

        @Override
        public void setAVMuteAsync(boolean state, String dmrUuid) throws RemoteException {
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(CBCommand.DMRActionID.SET_AV_MUTE, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.setMute(CBCommand.DMRActionID.SET_AV_MUTE, state);
        }

        @Override
        public void setAVVolumeAsync(int volume, String dmrUuid) throws RemoteException {
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(CBCommand.DMRActionID.SET_AV_VOLUME, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.setVolume(CBCommand.DMRActionID.SET_AV_VOLUME, volume);
        }

        @Override
        public void setPhotoMaxZoomLevelAsync(int maxzoom, String dmrUuid) 
            throws RemoteException {
            // TBD
        }

        @Override
        public void setPhotoZoomLevelAsync(int zoom, String dmrUuid) throws RemoteException {
            // TBD
        }

        @Override
        public void stopAsync(int command, String dmrUuid) throws RemoteException {
            Logger.v(TAG, "stopAsync()");
            
            DMRAction dmrAction = mDmrActionList.getDmrAction(dmrUuid);
            if (null == dmrAction) {
                mService.dmrNotify(command, dmrUuid,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmrAction.stop(command);
        }

        @Override
        public void stopBrowserContent(int token) throws RemoteException {
            Logger.v(TAG, "stopBrowserContent(" + token + ")");
            if (null == dmsAction || null == dmsAlbumAction) {
                mService.dmsNotify(CBCommand.DMSActionID.STOP_BROWSE_ACTION, token, -1, null,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmsAction.stopBrowserContent(CBCommand.DMSActionID.STOP_BROWSE_ACTION, token);
            dmsAlbumAction.stopBrowserContent(CBCommand.DMSActionID.STOP_BROWSE_ACTION, token);
        }

        @Override
        public void stopSearchContent(int token) throws RemoteException {
            Logger.v(TAG, "stopSearchContent()");
            if (null == dmsAction) {
                mService.dmsNotify(CBCommand.DMSActionID.STOP_SEARCH_ACTION, token, -1, null,
                        CBCommand.ErrorID.UNINITIALIZED);
                return;
            }
            dmsAction.stopSearchContent(CBCommand.DMSActionID.STOP_SEARCH_ACTION, token);
        }

        @Override
        public void stopSearchDevices() throws RemoteException {
            // TBD
        }

        @Override
        public void unregisterCallback(IDlnaServiceCallback cb) throws RemoteException {
            Logger.v(TAG, "IDlnaService.unregisterCallback()");
            if (cb != null) {
                mCallbacks.unregister(cb);
            }
        }

    };

    /** The m callbacks. */
    final RemoteCallbackList<IDlnaServiceCallback> mCallbacks = new RemoteCallbackList<IDlnaServiceCallback>();

    /*
     * (non-Javadoc)
     * @see com.acer.clearfi.device.DeviceListener#devNotifyReceived(int, int)
     */
    @Override
    public void devNotifyReceived(int callbackid, int total, int errCode) {
        lock();
        int N = mCallbacks.beginBroadcast();
        Logger.v(TAG, "devNotifyReceived(" + callbackid + "," + total + ")");
        Logger.v(TAG, "beginBroadcast:" + N);
        for (int i = 0; i < N; i++) {
            try {
                mCallbacks.getBroadcastItem(i).devNotifyReceived(callbackid, total, errCode);
            } catch (RemoteException e) {
            } catch (NullPointerException e) {
                e.printStackTrace();
            }
        }
        mCallbacks.finishBroadcast();
        unlock();
    }

    /**
     * Dmr notify.
     * 
     * @param command the command
     * @param errCode the err code
     */
    public void dmrNotify(int command, String uuid, int errCode) {
        lock();
        
        if(command == CBCommand.DMRActionID.ATTACH && errCode == CBCommand.ErrorID.OK){
            mDmrActionList.add(mDmrAction);
        }
        
        int N = mCallbacks.beginBroadcast();
        //Logger.v(TAG, "Thread-ID-" + Thread.currentThread().getId() + "  dmrNotify(" + command
        //        + "," + uuid + "," + errCode + ")");
        for (int i = 0; i < N; i++) {
            try {
                mCallbacks.getBroadcastItem(i).dmrActionPerformed(command, uuid, errCode);
            } catch (RemoteException e) {
            } catch (NullPointerException e) {
                e.printStackTrace();
            }
        }
        mCallbacks.finishBroadcast();
        unlock();
    }

    /*
     * (non-Javadoc)
     * @see com.acer.clearfi.dms.DMSActionListener#dmsNotify(int, int)
     */
    @Override
    public void dmsNotify(int command, int total, int tableId, String Uuid, int errCode) {
        lock();
        int N = mCallbacks.beginBroadcast();
        //Logger.v(TAG, "Thread-ID-" + Thread.currentThread().getId() + "  dmsNotify(" + command
        //        + "," + total + "," + tableId + "," + Uuid + "," + errCode + ")");
        for (int i = 0; i < N; i++) {
            try {
                mCallbacks.getBroadcastItem(i).dmsActionPerformed(command, total, tableId, Uuid,
                        errCode);
            } catch (RemoteException e) {
            } catch (NullPointerException e) {
                e.printStackTrace();
            }
        }
        mCallbacks.finishBroadcast();
        unlock();
    }

    /**
     * Inits the service.
     */
    private void initService() {

        Logger.v(TAG, "initService started");
        
        long time = System.currentTimeMillis();
        
        new Thread(){
            public void run(){
                if(null == devAction){
                    // Make sure all resource devAction, mDmrActionList, dmsAction are released
                    releaseAction();
                    initAction();
                }
            }
        }.start();

        DBManagerUtil.setContext(this);
        
        if(mStopThread == null){
        	mStopThread = new StopThread("stop thread");
        	mStopThread.start();
        }
        
        mWifiStateFilter = new IntentFilter(WifiManager.WIFI_STATE_CHANGED_ACTION);
        mWifiStateFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        registerReceiver(mWifiStateReceiver, mWifiStateFilter);

        Logger.i(TAG, "time duration = " + (System.currentTimeMillis() - time));
        Logger.v(TAG, "initService end");
    }
    
    private void initAction(){
        try {
            devAction = new DeviceAction();
            if(devAction != null){
                devAction.init();
                devAction.addListener(this);
            }

            mDmrActionList = new DmrActionList();

            dmsAction = new DMSAction();
            dmsAlbumAction = new DMSAction();
            if(dmsAction != null){
                dmsAction.init();
                dmsAction.addListener(this);
            }
            if(dmsAlbumAction != null){
                dmsAlbumAction.init();
                dmsAlbumAction.addListener(this);
            }
        }
        catch (NullPointerException ex) {
            ex.printStackTrace();
        }
    }

    /**
     * Lock.
     */
    private void lock() {
        mutex.lock();
    }

    /*
     * (non-Javadoc)
     * @see android.app.Service#onBind(android.content.Intent)
     */
    @Override
    public IBinder onBind(Intent arg0) {
        Logger.v(TAG, "onBind");
        mIsBind = true;
        return mBinder;
    }

    /*
     * (non-Javadoc)
     * @see android.app.Service#onCreate()
     */
    @Override
    public void onCreate() {

        Logger.v(TAG, "onCreate");
        super.onCreate();

        initService();
        mService = this;
    }

    /*
     * (non-Javadoc)
     * @see android.app.Service#onDestroy()
     */
    @Override
    public void onDestroy() {

        Logger.v(TAG, "onDestroy");
        super.onDestroy();

        releaseSerivce();
        mService = null;
    }

    /*
     * (non-Javadoc)
     * @see android.app.Service#onRebind(android.content.Intent)
     */
    @Override
    public void onRebind(Intent intent) {

        Logger.v(TAG, "onRebind");
        super.onRebind(intent);
    }

    /*
     * (non-Javadoc)
     * @see android.app.Service#onStart(android.content.Intent, int)
     */
    @Override
    public void onStart(Intent intent, int startId) {

        Logger.v(TAG, "onStart");
        super.onStart(intent, startId);
    }

    /*
     * (non-Javadoc)
     * @see android.app.Service#onStartCommand(android.content.Intent, int, int)
     */
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Logger.v(TAG, "onStartCommand");
        mIsBind = true;
        return super.onStartCommand(intent, flags, startId);
    }

    /*
     * (non-Javadoc)
     * @see android.app.Service#onUnbind(android.content.Intent)
     */
    @Override
    public boolean onUnbind(Intent intent) {

        Logger.v(TAG, "onUnbind");
        mIsBind = false;
        if(mStopHandler != null){
            mStopHandler.sendEmptyMessage(STOP_LATER);
        }

        return super.onUnbind(intent);
    }

    /**
     * Release serivce.
     */
    private void releaseSerivce() {

        releaseAction();
        
        unregisterReceiver(mWifiStateReceiver);
    }

    /**
     * Unlock.
     */
    private void unlock() {
        mutex.unlock();
    }
    
    private void releaseAction(){
        if (null != devAction) {
            devAction.release();
            devAction = null;
        }
        if (null != mDmrActionList) {
            mDmrActionList.release();
            mDmrActionList.clear();
            mDmrActionList = null;
        }
        if (null != dmsAction) {
            dmsAction.release();
            dmsAction = null;
        }
        if (null != dmsAlbumAction) {
            dmsAlbumAction.release();
            dmsAlbumAction = null;
        }
    }
}
