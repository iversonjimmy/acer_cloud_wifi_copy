//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.igware.ccdi_android;

import igware.gvm.pb.CcdiRpcClient.CCDIServiceClient;
import igware.protobuf.AbstractByteArrayProtoChannel;
import igware.protobuf.ProtoRpcException;
import igware.protobuf.RpcLayerException;
import igware.protobuf.pb.Rpc.RpcStatus.Status;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

public class CcdiClientRemoteBinder extends AbstractCcdiClient {

    private static final String LOG_TAG = "CcdiClientRemoteBinder";
    private static final String ACTION_CCD_SERVICE = "com.igware.ccdi_android.ICcdiAidlRpc";
    private final Context mContext;

    public CcdiClientRemoteBinder(Context c) {
        mContext = c;
    }
    
    /**
     * Returns a one-time use {@link CCDIServiceClient}. You should use it
     * immediately, and not hold a reference to it.
     * 
     * For example: {@code int errCode = getCcdiRpcClient().<RpcMethod>(request,
     * responseBuilder);}
     */
    @Override
    public CCDIServiceClient getCcdiRpcClient() throws ProtoRpcException {
        waitUntilReady();
        return new CCDIServiceClient(new RemoteCcdiProtoChannel(), true);
    }
    private class RemoteCcdiProtoChannel extends AbstractByteArrayProtoChannel {

        @Override
        protected byte[] perform(byte[] requestBuf) throws RemoteException {
            return mBoundService.protoRpc(requestBuf);
        }
    }
    
    public ICcdiAidlRpc getRawAidl() {
        return mBoundService;
    }
    
    @Override
    public boolean isReady() throws ProtoRpcException {

        if (mBoundService == null) {
            return false;
        }
        // TODO: Once it's true, we could probably cache this until the connection breaks.
        try {
            return mBoundService.isReady();
        }
        catch (RemoteException e) {
            throw new RpcLayerException(Status.IO_ERROR, e);
        }
    }

    // ------------------------------------------------------
    // Service binding
    // ------------------------------------------------------

    /** Flag indicating if we have called bind on the service. */
    private boolean mIsBound;
    
    private ICcdiAidlRpc mBoundService;

    private ServiceConnection mConnection = new ServiceConnection() {

        // This is called when the connection with the service has been
        // established, giving us the service object we can use to
        // interact with the service.
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {

            // The activity is in a different process:
            mBoundService = ICcdiAidlRpc.Stub.asInterface(service);
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
        boolean result = mContext.bindService(new Intent(ACTION_CCD_SERVICE), mConnection, Context.BIND_AUTO_CREATE);
        if (!result) {
            Log.e(LOG_TAG, "Bind failed.  Perhaps the cloud client service app is not installed?");
        }
    }

    public void unbindService() {

        if (mIsBound) {
            // Detach our existing connection.
            mContext.unbindService(mConnection);
            mIsBound = false;
        }
    }

    /**
     * Request that the service start now.
     * This can be used to ensure that background sync occurs.
     */
    public void startCcdiService() {
        // Make sure the service is started. It will continue running
        // until someone calls stopService().
        ComponentName name = mContext.startService(new Intent(ACTION_CCD_SERVICE));
        if (name == null) {
            Log.e(LOG_TAG, "Failed to start service.  Perhaps the cloud client service app is not installed?");
        }
    }

    /**
     * Request that the service stop now.
     * It won't actually stop until all clients unbind.
     */
    public void stopCcdiService() {
        boolean result = mContext.stopService(new Intent(ACTION_CCD_SERVICE));
        if (!result) {
            Log.w(LOG_TAG, "Failed to stop service.");
        }
    }
}
