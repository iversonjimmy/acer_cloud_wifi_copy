//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.acer.ccd.serviceclient;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import igware.cloud.media_metadata.pb.MediaMetadata.MediaServerInfo;
import igware.cloud.media_metadata.pb.MediaMetadata.ContentDirectoryObject;
import igware.cloud.media_metadata.pb.MCAForControlPoint.EnumerateMediaServersInput;
import igware.cloud.media_metadata.pb.MCAForControlPoint.EnumerateMediaServersOutput;
import igware.cloud.media_metadata.pb.MCAForControlPoint.GetContentUrlInput;
import igware.cloud.media_metadata.pb.MCAForControlPoint.GetContentUrlOutput;
import igware.cloud.media_metadata.pb.MCAForControlPoint.GetMetadataInput;
import igware.cloud.media_metadata.pb.MCAForControlPoint.GetMetadataOutput;
import igware.cloud.media_metadata.pb.MCAForControlPoint.InitForControlPointInput;
import igware.cloud.media_metadata.pb.MCAForControlPoint.InitForControlPointOutput;
import igware.cloud.media_metadata.pb.MCAForControlPointClient.MCAForControlPointServiceClient;
import igware.protobuf.AppLayerException;
import igware.protobuf.ProtoRpcException;

import android.app.Activity;
import android.provider.MediaStore;
import android.util.Log;

import com.acer.ccd.provider.CloudMediaColumns;
import com.acer.ccd.serviceclient.McaClientRemoteBinder;

public class McaClient {

    public static final int ERROR_CODE_RPC_LAYER = -100;

    private static final String LOG_TAG = "McaClient";

    private final McaClientRemoteBinder mClient;

    public McaClient(Activity a) {
        mClient = new McaClientRemoteBinder(a);
    }

    private MCAForControlPointServiceClient getMcaRpcClient() throws ProtoRpcException {
        return mClient.getMcaRpcClient();
    }

    public void onCreate() {
        mClient.startMcaService();
    }

    public void onStart() {
        // From Android SDK docs:
        // "If you only need to interact with the service while your activity is visible, you should
        //  bind during onStart() and unbind during onStop().
        //  If you want your activity to receive responses even while it is stopped in the background,
        //  then you can bind during onCreate() and unbind during onDestroy(). Beware that this implies
        //  that your activity needs to use the service the entire time it's running (even in the
        //  background), so if the service is in another process, then you increase the weight of 
        //  the process and it becomes more likely that the system will kill it.
        //  Note: You should usually not bind and unbind during your activity's onResume() and onPause()..."
        mClient.bindService();
    }

    public void onResume() {
    }

    public void onPause() {
    }

    public void onStop() {
        mClient.unbindService();
    }

    public void onDestroy() {
    }

    public int initForControlPoint( String appDataDirectory ) {
        try {
            InitForControlPointInput request = InitForControlPointInput
                    .newBuilder( ).setAppDataDirectory( appDataDirectory )
                    .build( );
            InitForControlPointOutput.Builder responseBuilder = InitForControlPointOutput
                    .newBuilder( );

            int errCode = getMcaRpcClient( ).InitForControlPoint( request, responseBuilder );

            return errCode;
        }
        catch ( AppLayerException e ) {
            return e.getAppStatus( );
        }
        catch ( ProtoRpcException e ) {
            Log.e( LOG_TAG, "initForControlPoint", e );
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int enumerateMediaServers( JSONArray serverList ) {
        try {
        	EnumerateMediaServersInput request = EnumerateMediaServersInput
        	        .newBuilder( ).build();
            EnumerateMediaServersOutput.Builder responseBuilder = EnumerateMediaServersOutput
                    .newBuilder( );

            int errCode = getMcaRpcClient( ).EnumerateMediaServers( request, responseBuilder );

            if ( 0 == errCode ) {
                EnumerateMediaServersOutput response = responseBuilder.build( );
                for ( MediaServerInfo currServer : response.getServersList() ) {
                    JSONObject server = new JSONObject( );
                    try {
                        server.put( "DeviceName", currServer.getDeviceName( ) );
                        server.put( "Uuid", currServer.getUuid( ) );
                        server.put( "CloudDeviceId", currServer.getCloudDeviceId( ) );
                        serverList.put( server );
                        
                        server = null;
                    }
                    catch ( JSONException e ) {
                    	e.printStackTrace( );
                    }
                }
            }

            return errCode;
        }
        catch ( AppLayerException e ) {
            return e.getAppStatus( );
        }
        catch ( ProtoRpcException e ) {
            Log.e( LOG_TAG, "enumerateMediaServers", e );
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int getMetaData( long cloudDeviceId, int mediaFlag, int sortFlag, JSONArray metadataList ) {
        try {
        	GetMetadataInput request =  GetMetadataInput
                    .newBuilder()
                    .setCloudDeviceId( cloudDeviceId )
                    .setMediaFlag( GetMetadataInput.MediaFlags.valueOf( mediaFlag ) )
                    .setSortFlag( GetMetadataInput.SortFlags.valueOf( sortFlag ) )
                    .build();
            GetMetadataOutput.Builder responseBuilder = GetMetadataOutput
                    .newBuilder( );

            int errCode = getMcaRpcClient( ).GetMetadata( request, responseBuilder );

            if ( 0 == errCode ) {
                GetMetadataOutput response = responseBuilder.build( );

                for ( ContentDirectoryObject currObject : response.getObjectsList() ) {
                    JSONObject metadata = new JSONObject( );

                    metadata.put( MediaStore.MediaColumns._ID, currObject.getObjectId( ) );
                    metadata.put( "Source", currObject.getSource( ).getNumber( ) );

                    if ( currObject.hasMusicTrack( ) ) {
                        metadata.put( "Media", "audio" );
                        metadata.put( MediaStore.Audio.Media.TITLE, currObject.getMusicTrack( ).getTitle( ) );
                        metadata.put( MediaStore.Audio.Media.ARTIST, currObject.getMusicTrack( ).getArtist( ) );

                        if ( currObject.getMusicTrack( ).hasTrackNumber( ) ) {
                            metadata.put( MediaStore.Audio.Media.TRACK, currObject.getMusicTrack( ).getTrackNumber( ) );
                        }

                        metadata.put( "Genre", currObject.getMusicTrack( ).getGenre( ) );
                        metadata.put( MediaStore.Audio.Media.DURATION, currObject.getMusicTrack( ).getDurationSec( ) );
                        metadata.put( MediaStore.Audio.Media.ALBUM_ID, currObject.getMusicTrack( ).getAlbumRef( ) );
                    }
                    else if ( currObject.hasVideoItem( ) ) {
                    	metadata.put( "Media", "video" );
                        metadata.put( MediaStore.Video.Media.TITLE, currObject.getVideoItem( ).getTitle( ) );

                        if ( currObject.getVideoItem( ).hasThumbnail( ) ) {
                            metadata.put( CloudMediaColumns.THUMBNAIL_URL, currObject.getVideoItem( ).getThumbnail( ) );
                        }

                        metadata.put( MediaStore.Video.Media.BUCKET_DISPLAY_NAME, currObject.getVideoItem( ).getAlbumName( ) );
                    }
                    else if ( currObject.hasPhotoItem( ) ) {
                        metadata.put( "Media", "photo" );
                        metadata.put( MediaStore.Images.Media.TITLE, currObject.getPhotoItem( ).getTitle( ) );

                        if ( currObject.getPhotoItem( ).hasThumbnail( ) ) {
                            metadata.put( CloudMediaColumns.THUMBNAIL_URL, currObject.getPhotoItem( ).getThumbnail( ) );
                        }

                        metadata.put( MediaStore.Images.Media.DATE_TAKEN, currObject.getPhotoItem( ).getDateTime( ) );
                        metadata.put( MediaStore.Images.Media.BUCKET_DISPLAY_NAME, currObject.getPhotoItem( ).getAlbumName( ) );
                        Log.i(LOG_TAG, "photo album: " + currObject.getPhotoItem( ).getAlbumName( ));
                    }
                    else if ( currObject.hasMusicAlbum( ) ) {
                    	metadata.put( "Media", "album" );
                        metadata.put( MediaStore.Audio.Albums.ALBUM, currObject.getMusicAlbum( ).getAlbumName( ) );

                        if ( currObject.getMusicAlbum( ).hasAlbumArtist( ) ) {
                            metadata.put( MediaStore.Audio.Albums.ARTIST, currObject.getMusicAlbum( ).getAlbumArtist( ) );
                        }

                        if ( currObject.getMusicAlbum( ).hasAlbumThumbnail( ) ) {
                            metadata.put( MediaStore.Audio.Albums.ALBUM_ART, currObject.getMusicAlbum( ).getAlbumThumbnail( ) );
                        }
                    }

                    metadataList.put( metadata );
                    metadata = null;
                }
            }

            return errCode;
        }
        catch ( AppLayerException e ) {
            return e.getAppStatus( );
        }
        catch ( ProtoRpcException e ) {
            Log.e( LOG_TAG, "getMetaData", e );
            return ERROR_CODE_RPC_LAYER;
        }
        catch ( JSONException e ) {
        	e.printStackTrace( );
        	return ERROR_CODE_RPC_LAYER;
        }
    }

    public String getContentUrl( long cloudDeviceId, String objectId, int urlFlag ) {
        String contentUrl = "";

        try {
            GetContentUrlInput request = GetContentUrlInput
                    .newBuilder( )
                    .setCloudDeviceId( cloudDeviceId )
                    .setObjectId( objectId )
                    .setUrlFlag( GetContentUrlInput.UrlFlags.valueOf( urlFlag ) )
                    .setReadCache( false )
                    .build( );
            GetContentUrlOutput.Builder responseBuilder = GetContentUrlOutput
                    .newBuilder( );

            int errCode = getMcaRpcClient( ).GetContentUrl( request, responseBuilder );

            if ( 0 == errCode ) {
                GetContentUrlOutput response = responseBuilder.build( );
                contentUrl = response.getUrl( );
            }
            else {
                Log.e( LOG_TAG, "getContentUrl" );
            }

            return contentUrl;
        }
        catch ( AppLayerException e ) {
            return "";
        }
        catch ( ProtoRpcException e ) {
            Log.e( LOG_TAG, "getContentUrl", e );
            return "";
        }
    }
}
