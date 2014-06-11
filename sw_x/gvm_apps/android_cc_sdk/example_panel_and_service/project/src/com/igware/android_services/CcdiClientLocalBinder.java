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

import igware.gvm.pb.CcdiRpcClient.CCDIServiceClient;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.util.Log;

import com.igware.ccdi_android.AbstractCcdiClient;

public class CcdiClientLocalBinder extends AbstractCcdiClient {

    private static final String LOG_TAG = "CcdiClientLocalBinder";

    private final Context mContext;

    public CcdiClientLocalBinder(Context c) {
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
    public CCDIServiceClient getCcdiRpcClient() {
        waitUntilReady();
        return mLocalServiceBinder.getLocalServiceClient();
    }

    @Override
    public void waitUntilReady() {

        while (!isReady()) {
            try {
                Thread.sleep(25);
            } catch (InterruptedException e) {
            }
        }
    }
    
    @Override
    public boolean isReady() {

        if (mLocalServiceBinder == null) {
            return false;
        }
        return mLocalServiceBinder.isReady();
    }

    // ------------------------------------------------------
    // Service binding
    // ------------------------------------------------------

    /** Flag indicating if we have called bind on the service. */
    private boolean mIsBound;

    private CcdiService.CcdiAidlRpcImpl mLocalServiceBinder;

    private ServiceConnection mConnection = new ServiceConnection() {

        // This is called when the connection with the service has been
        // established, giving us the service object we can use to
        // interact with the service.
        public void onServiceConnected(ComponentName name, IBinder service) {

            // Because we have bound to an explicit service
            // that we know is running in our own process, we can
            // cast its IBinder to a concrete class and directly access it.
            mLocalServiceBinder = (CcdiService.CcdiAidlRpcImpl)service;
            mIsBound = true;
        }

        // This is called when the connection with the service has been
        // unexpectedly disconnected -- that is, its process crashed.
        // Since the service is running in our same process, we should never
        // see this happen.
        public void onServiceDisconnected(ComponentName name) {

            Log.e(LOG_TAG, "onServiceDisconnected");
            mLocalServiceBinder = null;
            mIsBound = false;
        }
    };

    public void bindService() {

        // Establish a connection with the service. We use an explicit
        // class name because we want a specific service implementation that
        // we know will be running in our own process (and thus won't be
        // supporting component replacement by other applications).
        // Context.BIND_AUTO_CREATE also indicates to the system that the
        // service is as important as the calling process.
        boolean result = mContext.bindService(new Intent(mContext,
                CcdiService.class), mConnection, Context.BIND_AUTO_CREATE);
        if (!result) {
            Log.e(LOG_TAG, "bind failed");
        }
    }

    public void unbindService() {

        if (mIsBound) {
            // Detach our existing connection.
            mContext.unbindService(mConnection);
            mIsBound = false;
        }
    }

    public void startCcdiService() {
        // Make sure the service is started. It will continue running
        // until someone calls stopService(). The Intent we use to find
        // the service explicitly specifies our service component, because
        // we want it running in our own process and don't want other
        // applications to replace it.
        mContext.startService(new Intent(mContext, CcdiService.class));
    }

    public void stopCcdiService() {
        mContext.stopService(new Intent(mContext, CcdiService.class));
    }
}
