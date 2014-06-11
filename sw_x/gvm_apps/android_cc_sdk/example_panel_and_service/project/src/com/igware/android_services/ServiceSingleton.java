package com.igware.android_services;

import android.content.Context;
import android.util.Log;

class ServiceSingleton {
    
    private static final String TAG = "SrvcSngltn";
    
    private static final String PIM_SHARED_PREF = "PIM_SHARED_PREF";

    private static final String IS_SERVICE_RUNNING = "IS_SERVICE_RUNNING";
    
    private static final String LOG_TAG = "SrvcSngltn";
    
    public static ServiceSingleton mInstance = null;
    
    private static Context mApplicationContext = null;
    
    private boolean mIsReady = false;

    // Private constructor prevents instantiation from other classes.
    private ServiceSingleton() {
        // We may be on either the UI thread or a service handler thread.  Either way,
        // spawn a separate thread to start the native service.
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run()
            {
                String filesDir = getPrimaryFileDir();
                Log.i(TAG, "before startNativeService(" + filesDir + ", \"0\" " + ", AcerCloud" + ", cc" +")");
                boolean result = startNativeService(filesDir, "0", "AcerCloud", "cc");
                if (!result) {
                    Log.e(TAG, "startNativeService() failed!");
                    mIsReady = true;
                } else {
                    Log.i(TAG, "**************** after startNativeService() - started successfully");
                    mIsReady = true;
                }
            }
        });
        thread.start();
    }

    synchronized static ServiceSingleton getInstance(Context context) {
        if (context == null) {
            return null;
        }
        if (mInstance == null) {
            mApplicationContext = context.getApplicationContext();
            mInstance = new ServiceSingleton();
        }
        
        return mInstance;
    }
    
    String getPrimaryFileDir() {
        return mApplicationContext.getFilesDir().getAbsolutePath();
    }
    
    
    boolean isReady() {
        return mIsReady;
    }
    
    @Override
    protected void finalize() throws Throwable {
        try {
            Log.i(TAG, "************************ about to stopNativeService()");
            stopNativeService();
            Log.i(TAG, "**************** after stopNativeService()");
        } finally {
            super.finalize();
        }
    }

    // ------------------------------------------------------
    // JNI calls
    // ------------------------------------------------------
    
    native static boolean startNativeService(String filesDir, String titleId, String brandName, String appName);
    
    native static boolean stopNativeService();
    
    /**
     * Note that this can be called by multiple threads concurrently. Any thread
     * that calls this expects to block until the operation is complete.
     * 
     * @param serializedRequest
     *            The request, serialized according to iGware's proprietary
     *            Protocol Buffer RPC protocol (search for protorpcgen).
     * @param fromOtherApp
     *            Indicates if the request came from an outside app.
     *            Outside apps might not have permission to do everything
     *            that our modules do.
     * @return The response, serialized according to iGware's proprietary
     *         Protocol Buffer RPC protocol (see for protorpc/rpc.proto).
     */
    native static byte[] ccdiJniProtoRpc(byte[] serializedRequest,
            boolean fromOtherApp);

    /*
     * This is used to load the jni library on application startup. The library
     * has already been unpacked into /data/data/<package name>/lib/<libname>.so
     * at installation time by the package manager.
     */
    static {
        System.loadLibrary("ccd-jni");
    }
}
