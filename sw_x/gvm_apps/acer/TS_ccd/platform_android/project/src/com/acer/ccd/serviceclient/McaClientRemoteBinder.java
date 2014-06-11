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

import igware.cloud.media_metadata.pb.MCAForControlPointClient.MCAForControlPointServiceClient;
import igware.protobuf.AbstractByteArrayProtoChannel;
import igware.protobuf.ProtoRpcException;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import com.igware.android_services.IMcaAidlRpc;

public class McaClientRemoteBinder extends AbstractMcaClient {

    private static final String LOG_TAG = "McaClientRemoteBinder";
    private static final String ACTION_MCA_SERVICE = "com.igware.android_services.McaService";
    private final Activity mActivity;

    public McaClientRemoteBinder(Activity a) {
        mActivity = a;
    }
    
    /**
     * Returns a one-time use {@link MCAForControlPointServiceClient}. You should use it
     * immediately, and not hold a reference to it.
     * 
     * For example: {@code int errCode = getMcaRpcClient().<RpcMethod>(request,
     * responseBuilder);}
     */
    @Override
    public MCAForControlPointServiceClient getMcaRpcClient() throws ProtoRpcException {
        waitUntilReady();
        return new MCAForControlPointServiceClient(new RemoteMcaProtoChannel(), true);
    }
    private class RemoteMcaProtoChannel extends AbstractByteArrayProtoChannel {

        @Override
        protected byte[] perform(byte[] requestBuf) throws RemoteException {
            return mBoundService.protoRpc(requestBuf);
        }
    }
    
    /**
     * @throws ProtoRpcException 
     */
    @Override
    public boolean isReady() throws ProtoRpcException {

        return (mBoundService != null);
    }

    // ------------------------------------------------------
    // Service binding
    // ------------------------------------------------------

    /** Flag indicating if we have called bind on the service. */
    private boolean mIsBound;
    
    private IMcaAidlRpc mBoundService;

    private ServiceConnection mConnection = new ServiceConnection() {

        // This is called when the connection with the service has been
        // established, giving us the service object we can use to
        // interact with the service.
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {

            // The activity is in a different process:
            mBoundService = IMcaAidlRpc.Stub.asInterface(service);
            mIsBound = true;
        }

        // This is called when the connection with the service has been
        // unexpectedly disconnected -- that is, its process crashed.
        public void onServiceDisconnected(ComponentName name) {

            Log.e(LOG_TAG, "onServiceDisconnected");
            mBoundService = null;
            mIsBound = false;
        }
    };
   
    public void bindService() {

        // Establish a connection with the service.
        // Context.BIND_AUTO_CREATE also indicates to the system that the
        // service is as important as the calling process.
        boolean result = mActivity.bindService(new Intent(ACTION_MCA_SERVICE),
            mConnection, Context.BIND_AUTO_CREATE);
        if (!result) {
            Log.e(LOG_TAG, "Bind failed.  Perhaps the cloud client service app is not installed?");
        }
    }

    public void unbindService() {

        if (mIsBound) {
            // Detach our existing connection.
            mActivity.unbindService(mConnection);
            mIsBound = false;
        }
    }

    public void startMcaService() {
        // Make sure the service is started. It will continue running
        // until someone calls stopService().
        ComponentName name = mActivity.startService(new Intent(ACTION_MCA_SERVICE));
        if (name == null) {
            Log.e(LOG_TAG, "Failed to start service.  Perhaps the cloud client service app is not installed?");
        }
    }
    
    /**
     * Request that the service stop now.
     * It won't actually stop until all clients unbind.
     */
    public void stopMcaService() {
        boolean result = mActivity.stopService(new Intent(ACTION_MCA_SERVICE));
        if (!result) {
            Log.w(LOG_TAG, "Failed to stop service.");
        }
    }
}
