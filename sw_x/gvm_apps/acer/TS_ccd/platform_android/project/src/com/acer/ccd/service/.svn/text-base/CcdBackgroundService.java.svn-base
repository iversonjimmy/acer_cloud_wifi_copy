package com.acer.ccd.service;

import com.acer.ccd.serviceclient.CcdiClient;
import com.acer.ccd.ui.ReloginDialogActivity;
import com.acer.ccd.util.InternalDefines;
import com.acer.ccd.util.Utility;
import com.acer.ccd.util.igware.Constants;

import android.app.Activity;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;

public class CcdBackgroundService extends Service {
    
    private static final String TAG = "CcdBackgroundService";
    
    private static final int MESSAGE_START_SERVICE = 0;
    private static final int MESSAGE_BIND_SERVICE = 1;
    private static final int MESSAGE_UNBIND_SERVICE = 2;
    private static final int MESSAGE_LOGOUT = 3;
    private static final int MESSAGE_RELOGIN = 4;
    // Call login, sync, and then release the wake lock.
    private static final int MESSAGE_RELOGIN_AND_SYNC = 5;
    
    private static final long DELAY_TIME = 25; 

    private int mProcessType;
    private CcdiClient mBoundService;
    
    private ArrayList<Thread> threads = new ArrayList<Thread>();
    
    @Override
    public IBinder onBind(Intent arg0) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        
        if (intent == null) {
            // for debug, if intent is null, you can find "mProcessType" from logcat to know what service did the last action.
            Log.e(TAG, "intent is null, it will do stopSelf().");
            stopSelf();
            return super.onStartCommand(intent, flags, startId);
        }

        mProcessType = intent.getIntExtra(InternalDefines.BUNDLE_PROGRESS_TYPE, InternalDefines.PROGRESSING_TYPE_INVALID);
        Log.i(TAG, "mProcessType = " + mProcessType);
        if (mProcessType == InternalDefines.PROGRESSING_TYPE_INVALID) {
            Log.e(TAG, "Can not get type from Intent integer extra, progress type is invalid.");
            return Service.START_NOT_STICKY;
        }
        
        mHandler.sendEmptyMessageDelayed(MESSAGE_START_SERVICE, DELAY_TIME);
        
        return Service.START_STICKY;//super.onStartCommand(intent, flags, startId);
    }
    
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_START_SERVICE:
                    mBoundService = new CcdiClient(CcdBackgroundService.this);
                    mBoundService.onCreate();
                    mHandler.sendEmptyMessageDelayed(MESSAGE_BIND_SERVICE, DELAY_TIME);
                    break;
                case MESSAGE_BIND_SERVICE:
                    mBoundService.onStart();
                    processAction();
                    break;
                case MESSAGE_UNBIND_SERVICE:
                    mBoundService.onStop();
                    destroyThread();
                    break;
                case MESSAGE_LOGOUT: {
                    Thread t = new LogoutThread();
                    t.start();
                    addThread(t);
                    break;
                }
                case MESSAGE_RELOGIN: {
                    Thread t = new ReloginThread();
                    t.start();
                    addThread(t);
                    break;
                }
                case MESSAGE_RELOGIN_AND_SYNC: {
                    Thread t = new ReloginAndSyncThread();
                    t.start();
                    addThread(t);
                    break;
                }
            }
        }
    };
    
    private void processAction() {
        switch (mProcessType) {
            case InternalDefines.ACTION_TYPE_LOGOUT:
                mHandler.sendEmptyMessageDelayed(MESSAGE_LOGOUT, DELAY_TIME);
                break;
            case InternalDefines.ACTION_TYPE_RELOGIN:
                mHandler.sendEmptyMessageDelayed(MESSAGE_RELOGIN, DELAY_TIME);
                break;
            case InternalDefines.ACTION_TYPE_RELOGIN_AND_SYNC:
                mHandler.sendEmptyMessageDelayed(MESSAGE_RELOGIN_AND_SYNC, DELAY_TIME);
                break;
        }
    }
    
    private void addThread(Thread t) {
        synchronized (threads) {
            threads.add(t);
        }
    }
    
    private void destroyThread() {
        synchronized (threads) {
            for (Thread curr : threads) {
                curr.interrupt();
            }
            threads.clear();
        }
    }
    
    private void removeCloudData() {
        Log.i(TAG, "remove all files and folders under /sdcard/acerCloud/");
        File dir = new File(Constants.DEFAULT_FOLDER_PATH);
        if (!dir.exists())
            return;
        
        for (File child : dir.listFiles()) {
            deleteTree(child);
        }
    }
    
    private void deleteTree(File file) {
        Log.i(TAG, "deleteTree : file = " + file.getPath());
        if (file.isDirectory())
            for (File child : file.listFiles())
                deleteTree(child);
        file.delete();
    }
    
    private class LogoutThread extends Thread {
        @Override
        public void run() {
            Log.i(TAG, "LogoutThread : run() start.");
            int result = -1;
            
            // Utility.stopCcdPeriodicSync(CcdBackgroundService.this);
            
            if (mBoundService == null) {
                Log.e(TAG, "CcdiClient encountered some error, mBoundService is null");
                return;
            }
            if (mBoundService.isLoggedIn()) {
                result = mBoundService.unlinkDevice();
                if (result < 0)
                    Log.e(TAG, "unLinkDevice() resuleCode = " + result);
                result = mBoundService.doLogout();
                if (result < 0)
                    Log.e(TAG, "doLogout() resuleCode = " + result);
            }
            removeCloudData();
            // must clean CameraRoll flag in SharedPreference
            Utility.setCameraRollSwitch(CcdBackgroundService.this, false);
            mHandler.sendEmptyMessageDelayed(MESSAGE_UNBIND_SERVICE, DELAY_TIME);
            Log.i(TAG, "LogoutThread : run() end.");
        }
    };
    
    private class ReloginThread extends Thread {
        @Override
        public void run() {
            Log.i(TAG, "Relogin Thread : run() start.");
            int result = -1;
            String id = Utility.getAccountId(CcdBackgroundService.this); 
            if (!id.equals("")) {
                if (mBoundService == null) {
                    Log.e(TAG, "CcdiClient encountered some error, mBoundService is null");
                    return;
                }
                if (!mBoundService.isLoggedIn())
                    result = mBoundService.doLogin(id, null);
                if (result < 0)
                    Log.e(TAG, "do Login() resultCode = " + result);
            }
            Log.i(TAG, "Relogin Thread : run() end.");
        }
    }

    private class ReloginAndSyncThread extends Thread {
        @Override
        public void run() {
            try {
                Log.i(TAG, "ReloginAndSyncThread Thread : run() start.");
                int result = -1;
                String id = Utility.getAccountId(CcdBackgroundService.this); 
                if (!id.equals("")) {
                    if (mBoundService == null) {
                        Log.e(TAG, "CcdiClient encountered some error, mBoundService is null");
                        return;
                    }
                    result = mBoundService.doLogin(id, null);
                    Log.i(TAG, "doLogin() resultCode = " + result);
                    if (result < 0) {
                        // Only notify user to retype password when return CCDI_ERROR_NO_PASSWORD
                        if (result == InternalDefines.CCDI_ERROR_NO_PASSWORD) {
                            // App data may be cleared, call relogin dialog 
                            Intent intent = new Intent(CcdBackgroundService.this, ReloginDialogActivity.class);
                            intent.setFlags(intent.getFlags() | Intent.FLAG_ACTIVITY_NEW_TASK);
                            startActivity(intent);
                        } else {
                            // TODO: check with UI flow
                        }
                    } else {
                        result = mBoundService.ownershipSync();
                        if (result < 0) {
                            Log.e(TAG, "ownershipSync() resultCode = " + result);
                        }
                    }
                }
            } finally {
                releaseWakeLock(CcdBackgroundService.this.getApplicationContext());
            }
        }
    }
    
    synchronized private static PowerManager.WakeLock getLock(Context context) {
        if (mWakeLock == null) {
            PowerManager mgr = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
            mWakeLock = mgr.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "com.acer.ccd.service.CcdBackgroundService.WAKE_LOCK");
            mWakeLock.setReferenceCounted(true);
        }
        return mWakeLock;
    }
    private static PowerManager.WakeLock mWakeLock = null;
    
    public static void acquireWakeLock(Context context) {
        Log.i(TAG, "Acquiring Wake Lock");
        getLock(context).acquire();
    }
    
    public static void releaseWakeLock(Context context) {
        Log.i(TAG, "Releasing Wake Lock");
        getLock(context).release();
    }
}
