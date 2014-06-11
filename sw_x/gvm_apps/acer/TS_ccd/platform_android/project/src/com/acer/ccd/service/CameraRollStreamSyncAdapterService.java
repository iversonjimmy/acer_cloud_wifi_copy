/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.service;

import com.acer.ccd.serviceclient.CcdiClient;
import com.acer.ccd.util.InternalDefines;
import com.acer.ccd.util.Utility;
import com.acer.ccd.util.igware.Constants;

import android.accounts.Account;
import android.app.Service;
import android.content.AbstractThreadedSyncAdapter;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SyncResult;
import android.content.SyncStatusObserver;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

import java.io.File;
import java.io.IOException;

public class CameraRollStreamSyncAdapterService extends Service {
    private static final String TAG = "CameraRollStreamSyncAdapterService";
    private static CameraRollStreamSyncAdapterImpl sSyncAdapter = null;

    // private static ContentResolver mContentResolver = null;

    public CameraRollStreamSyncAdapterService() {
        super();
    }

    @Override
    public IBinder onBind(Intent intent) {
        IBinder ret = null;
        ret = getSyncAdapter().getSyncAdapterBinder();
        return ret;
    }

    private CameraRollStreamSyncAdapterImpl getSyncAdapter() {
        if (sSyncAdapter == null)
            sSyncAdapter = new CameraRollStreamSyncAdapterImpl(this);
        return sSyncAdapter;
    }

    public class CameraRollStreamSyncAdapterImpl extends AbstractThreadedSyncAdapter {

        private static final long DELAY_TIME = 25;
        
        protected CcdiClient mBoundService;
        private Context mContext;

        public CameraRollStreamSyncAdapterImpl(Context context) {
            super(context, true);
            mContext = context;
            ContentResolver.addStatusChangeListener(ContentResolver.SYNC_OBSERVER_TYPE_PENDING
                    | ContentResolver.SYNC_OBSERVER_TYPE_SETTINGS
                    | ContentResolver.SYNC_OBSERVER_TYPE_ACTIVE, syncObserver);
            mBoundService = new CcdiClient(context);
            mBoundService.onCreate();
        }

        private SyncStatusObserver syncObserver = new SyncStatusObserver() {
            @Override
            public void onStatusChanged(int which) {
                Log.i(TAG, "Trigger SyncStatusObserver onStatusChanged(which = " + which + ").");
                switch (which) {
                    case ContentResolver.SYNC_OBSERVER_TYPE_SETTINGS: {
                        if (Utility.getCameraRollSwitch(mContext)
                                && !Utility.isCameraRollStreamSync(mContext)) {
                            Utility.setCameraRollSwitch(mContext, false);
                            disableCameraRoll();
                        }
                    }
                    break;
                }
            }
        };

        @Override
        public void onPerformSync(Account account, Bundle extras, String authority,
                ContentProviderClient provider, SyncResult syncResult) {

            int cameraSyncDownloadMaxSize = 0;
            int cameraSyncDownloadMaxFiles = 0;

            mBoundService.onStart();
            Utility.delayTime(DELAY_TIME);
            
            try {
                if (Utility.getCameraRollSwitch(mContext)) {
                    Intent intent = new Intent(InternalDefines.BROADCAST_MESSAGE_PERIODIC_SYNC);
                    sendBroadcast(intent);
                    return;
                }
                Utility.setCameraRollSwitch(mContext, true);
                // mContentResolver = mContext.getContentResolver();
                Long a, b, c;
                a = System.currentTimeMillis();
                int result = mBoundService.addCameraSyncUploadSubscription();
                b = System.currentTimeMillis();
                Log.i(TAG, "addCameraSyncUploadSubscription() result = " + result);
                if (result != 0) {
                    Log.e(TAG, "addCameraSyncUploadSubscription encounter some error, result = "
                            + result);
                    syncResult.databaseError = true;
                    return;
                }

                result = mBoundService.addCameraSyncDownloadSubscription(cameraSyncDownloadMaxSize,
                        cameraSyncDownloadMaxFiles);
                c = System.currentTimeMillis();
                if (result != 0) {
                    Log.e(TAG, "addCameraSyncDownloadSubscription encounter some error, result = "
                            + result);
                    syncResult.databaseError = true;
                    return;
                }
                Log.e(TAG, "KKKKK addCameraSyncUploadSubscription()=" + (b - a) + " addCameraSyncDownloadSubscription()=" + (c - b));
                // create .nomedia in /sdcard/CameraRoll/
                String path = (Constants.DEFAULT_FOLDER_PATH + InternalDefines.DEFAULT_CAMERAROLL_PATH + InternalDefines.NO_MEDIA_FILE_NAME);
                File file = new File(path);
                if (!file.exists()) {
                    file.createNewFile();
                }
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                mBoundService.onStop();
            }
        }

        private void disableCameraRoll() {
            Log.i(TAG, "enter disablCameraRoll");

            mBoundService.onStart();
            Utility.delayTime(DELAY_TIME);

            long cameraSyncUploadFolderId = mBoundService.getCameraSyncUploadFolderId();
            if (cameraSyncUploadFolderId < 0) {
                // Camera sync upload folder does not exist yet. No need to
                // unsubscribe.
                Log.e(TAG, "Cannot find camera sync upload folder.");
                return;
            }
            mBoundService.removeCameraSyncSubscription(cameraSyncUploadFolderId);

            long cameraSyncDownloadFolderId = mBoundService.getCameraSyncDownloadFolderId();
            if (cameraSyncDownloadFolderId < 0) {
                // Camera sync download folder does not exist yet. No need to
                // unsubscribe.
                Log.e(TAG, "Cannot find camera sync download folder.");
                return;
            }
            mBoundService.removeCameraSyncSubscription(cameraSyncDownloadFolderId);
            mBoundService.onStop();

            Log.i(TAG, "exit disablCameraRoll");
        }

    }
}
