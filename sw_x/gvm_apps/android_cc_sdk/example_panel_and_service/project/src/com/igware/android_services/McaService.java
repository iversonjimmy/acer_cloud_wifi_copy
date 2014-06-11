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

import com.igware.mca_android.IMcaAidlRpc;
import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class McaService extends Service {

    private static final String LOG_TAG = "MediaMetaClientService";

    private static final int CHUNK_SIZE = 512000;

    private class MetadataClone {
        public int pid;
        public int length;
        public byte[] metadata;
    }

    private List<MetadataClone> mMetadataList;

    // ------------------------------------------------------
    // Android-specific overrides
    // ------------------------------------------------------
    
    @Override
    public void onCreate() {
        Log.i(LOG_TAG, "onCreate()");
        super.onCreate();
        mMetadataList = new ArrayList<MetadataClone>();
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
        public byte[] protoRpc(int processId, byte[] serializedRequest) {
            
            // This request originated from outside of this process, so we may want to enforce
            // some restrictions on what it can do.  For now, we assume that the Android manifest
            // will take care of securing remote connections to this service.
            Log.i(LOG_TAG, "remote protoRpc() waiting for ccdi");
            ensureCcdiLoaded();
            Log.i(LOG_TAG, "before remote mcaJniProtoRpc()");
            byte[] result = mcaJniProtoRpc(serializedRequest);
            Log.i(LOG_TAG, "after remote mcaJniProtoRpc()");

            // If fits in CHUNK_SIZE, return the whole response.
            if ( CHUNK_SIZE >= result.length ) { return result; }

            boolean newPid = true;

            synchronized(mMetadataList) {
                for ( MetadataClone clone : mMetadataList ) {
                    if ( processId == clone.pid ) {
                        clone.length = result.length;
                        clone.metadata = result;
                        newPid = false;
                        break;
                    }
                }
                
                if ( newPid ) {
                    MetadataClone tmpClone = new MetadataClone();
                    tmpClone.pid = processId;
                    tmpClone.length = result.length;
                    tmpClone.metadata = result;
                    
                    mMetadataList.add(tmpClone);
                }
            }

            // Return null to indicate that the response didn't fit in CHUNK_SIZE.
            return null;
        }

        @Override
        public int getLength( int pid ) {
            int length = 0;

            synchronized(mMetadataList) {
                for ( MetadataClone clone : mMetadataList ) {
                    if ( pid == clone.pid ) { length = clone.length; }
                }
            }

            return length;
        }

        @Override
        public byte[] getNext( int pid, int offset, int size ) {
            Log.i(LOG_TAG, "SCSCSC pid = " + pid + ", offset = " + offset + ", size = " + size );
            int eoc = size + offset;

            byte[] mChunkOut = null;

            synchronized(mMetadataList) {
                for (int i = 0; i < mMetadataList.size(); i++) {
                    MetadataClone clone = mMetadataList.get(i);
                    if ( pid == clone.pid ) {
                        if ( eoc <= clone.length ) {
                            mChunkOut = Arrays.copyOfRange(clone.metadata, offset, eoc);
                        } else {
                            Log.w(LOG_TAG, "pid " + pid + " asked for too much data");
                        }
                        if ( eoc >= clone.length ) {
                            clone.length = 0;
                            clone.metadata = null;
                            mMetadataList.remove(i);
                        }
                        break;
                    }
                }
            }
            Log.i(LOG_TAG, "SCSCSC getNext mChunkOut.size = " + mChunkOut.length );
            return mChunkOut;
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
    
    /*
     * This is used to load the jni library on application startup. The library
     * has already been unpacked into /data/data/<package name>/lib/<libname>.so
     * at installation time by the package manager.
     */
    static {
        System.loadLibrary("mca-jni");
    }
}
