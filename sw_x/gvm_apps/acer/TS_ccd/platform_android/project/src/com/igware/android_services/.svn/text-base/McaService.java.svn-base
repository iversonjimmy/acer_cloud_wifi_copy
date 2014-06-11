//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.igware.android_services;

import igware.cloud.media_metadata.pb.MCAForControlPointClient.MCAForControlPointServiceClient;
import igware.protobuf.AbstractByteArrayProtoChannel;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class McaService extends Service {
    
    private static final String LOG_TAG = "MediaMetaClientService";
    
    // ------------------------------------------------------
    // Android-specific overrides
    // ------------------------------------------------------
    
    @Override
    public void onCreate() {
        Log.i(LOG_TAG, "onCreate()");
        super.onCreate();
    }
    
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        
        // Note that this is only called in response to startService; binding
        // with BIND_AUTO_CREATE does not cause this to be invoked.
        //
        // Note that the system calls this on your service's main thread.
        // A service's main thread is the same thread where UI operations take place
        // for Activities running in the same process.
        
        Log.i(LOG_TAG, "onStartCommand(), start id " + startId + ": "
                + intent);
        // We want this service to continue running until it is explicitly
        // stopped, so return sticky.
        return super.onStartCommand(intent, flags, startId);
    }
    
    @Override
    public void onDestroy() {
        super.onDestroy();
    }
    
    @Override
    public IBinder onBind(Intent intent) {

        Log.i(LOG_TAG, "onBind()");
        return mBinder;
    }
    
    // ------------------------------------------------------
    
    /**
     * Make sure CCDI is loaded.  Getting the ServiceSingleton instance
     * should force ccd-jni.so to be loaded.
     */
    public void ensureCcdiLoaded()
    {
        while (!ServiceSingleton.getInstance(this).isReady()) {
            try {
                Thread.sleep(25);
            } catch (InterruptedException e) {
            }
        }
    }

    private final MediaMetadataClientAidlImpl mBinder = new MediaMetadataClientAidlImpl();
    
    /**
     * For the local case, this object will be passed to ServiceConnection.onServiceConnected as
     * the {@link IBinder}.
     * For the remote case, this object's methods will be called by the Android platform.
     */
    // By extending IMediaMetadataClientAidl.Stub, this class also implements IMediaMetadataClientAidl.
    public class MediaMetadataClientAidlImpl extends IMcaAidlRpc.Stub {

        /**
         * Only used for the remote case.
         */
        @Override
        public byte[] protoRpc(byte[] serializedRequest) {
            
            // This request originated from outside of this process, so we may want to enforce
            // some restrictions on what it can do.  For now, we assume that the Android manifest
            // will take care of securing remote connections to this service.
            Log.i(LOG_TAG, "remote protoRpc() waiting for ccdi");
            ensureCcdiLoaded();
            Log.i(LOG_TAG, "before remote mcaJniProtoRpc()");
            byte[] result = mcaJniProtoRpc(serializedRequest);
            Log.i(LOG_TAG, "after remote mcaJniProtoRpc()");
            return result;
        }
        
        /**
         * Only used for the local case.
         */
        private class LocalProtoChannel extends AbstractByteArrayProtoChannel {
            
            @Override
            protected byte[] perform(byte[] requestBuf) {

                Log.i(LOG_TAG, "local protoRpc() waiting for ccdi");
                ensureCcdiLoaded();
                Log.i(LOG_TAG, "before local ccdiJniProtoRpc()");
                byte[] result = mcaJniProtoRpc(requestBuf);
                Log.i(LOG_TAG, "after local ccdiJniProtoRpc()");
                return result;
            }
        }
        /**
         * Only used for the local case.
         */
        public MCAForControlPointServiceClient getLocalServiceClient() {
            return new MCAForControlPointServiceClient(new LocalProtoChannel(), true);
        }
    }

    // ------------------------------------------------------
    // JNI calls
    // ------------------------------------------------------
 
    /**
     * Note that this can be called by multiple threads concurrently. Any thread
     * that calls this expects to block until the operation is complete.
     * @param serializedRequest
     *      The request, serialized according to iGware's proprietary
     *      Protocol Buffer RPC protocol (search for protorpcgen).
     * @return The response, serialized according to iGware's proprietary
     *      Protocol Buffer RPC protocol (search for protorpc/rpc.proto).
     */
    native static byte[] mcaJniProtoRpc(byte[] serializedRequest);
}
