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

import java.util.HashMap;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.ServiceConnection;
import com.acer.ccd.debug.L;
import com.acer.ccd.service.IDlnaService;

public class DlnaServiceWrapper {
    private static final String TAG = "DlnaServiceWrapper";

    private static final String ACTION_SEARCH_CONTENTS = "com.acer.ccd.service.DlnaService";

    public static IDlnaService sService = null;
    private static HashMap<Context, ServiceBinder> sConnectionMap = new HashMap<Context, ServiceBinder>();

    public static ServiceToken bindToService(Activity context, ServiceConnection callback) {
        Activity realActivity = context.getParent();

        if (realActivity == null) {
            realActivity = context;
        }
        ContextWrapper cw = new ContextWrapper(realActivity);
        cw.startService(new Intent(ACTION_SEARCH_CONTENTS));
        ServiceBinder sb = new ServiceBinder(callback);
        if (cw.bindService((new Intent()).setAction(ACTION_SEARCH_CONTENTS), sb, Context.BIND_AUTO_CREATE)) {
            sConnectionMap.put(cw, sb);
            return (new ServiceToken(cw));
        }
        // for testing only, will remove soon
/*
        cw.startService(new Intent(cw, com.acer.clearfi.dlnaservice.IDlnaService.class));
        ServiceBinder sb = new ServiceBinder(callback);        
        if (cw.bindService((new Intent()).setClass(cw, com.acer.clearfi.dlnaservice.IDlnaService.class), sb, 0)) {
            sConnectionMap.put(cw, sb);
            return (new ServiceToken(cw));
        }
*/
        // fail to bind to DlnaService
        L.t(TAG, "bindToService() fail to bind to DlnaService");
        return null;
    }

    public static ServiceToken bindToService(Context context, ServiceConnection callback) {
        ContextWrapper cw = new ContextWrapper(context);
        cw.startService(new Intent(ACTION_SEARCH_CONTENTS));
        ServiceBinder sb = new ServiceBinder(callback);        
        if (cw.bindService((new Intent()).setAction(ACTION_SEARCH_CONTENTS), sb, Context.BIND_AUTO_CREATE)) {
            sConnectionMap.put(cw, sb);
            return (new ServiceToken(cw));
        }
        L.t(TAG, "bindToService() fail to bind to DlnaService");
        return null;
    }
    
    public static void unbindFromService(ServiceToken token) {
        if (token == null) {
            L.t(TAG, "unbindFromService() try to unbind with null token");
            return;
        }
        ContextWrapper cw = token.mWrappedContext;
        ServiceBinder sb = sConnectionMap.remove(cw);
        if (sb == null) {
            L.t(TAG, "try to unbind for unknown Context");
            return;
        }
        cw.unbindService(sb);
        if (sConnectionMap.isEmpty()) {
            // presumably there is nobody interested in the service at this point,
            // so don't hang on to the ServiceConnection
            sService = null;
        }
    }

    /**
     * This class is used to store the client information
     */
    public static class ServiceToken {
        ContextWrapper mWrappedContext;
        ServiceToken(ContextWrapper context) {
            mWrappedContext = context;
        }
    }

    private static class ServiceBinder implements ServiceConnection {
        ServiceConnection mCallback;
        ServiceBinder(ServiceConnection callback) {
            mCallback = callback;
        }

        public void onServiceConnected(ComponentName className, android.os.IBinder service) {
            sService = IDlnaService.Stub.asInterface(service);
            if (mCallback != null) {
                mCallback.onServiceConnected(className, service);
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            if (mCallback != null) {
                mCallback.onServiceDisconnected(className);
            }
            sService = null;
        }
    }
}