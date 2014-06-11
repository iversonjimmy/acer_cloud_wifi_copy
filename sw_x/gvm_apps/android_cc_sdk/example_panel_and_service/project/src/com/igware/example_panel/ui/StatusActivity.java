//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.igware.example_panel.ui;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.igware.example_panel.util.Constants;
import com.igware.android_cc_sdk.example_panel_and_service.R;


public class StatusActivity extends CcdActivity {
    
    private static final String LOG_TAG = "StatusActivity";
    
    private TextView mAppVersionField;
    private TextView mUserNameField;
    private TextView mDeviceNameField;
    private TextView mSyncStateField;
    private TextView mDownloadStatusField;
    private TextView mUploadStatusField;
    //private TextView mQuotaStatusField;
    
    //private ProgressBar mDownloadProgressBar;
    //private ProgressBar mUploadProgressBar;
    //private ProgressBar mQuotaProgressBar;
    
    private String mAppVersion = "";
    private volatile String mUserName = "";
    private volatile String mDeviceName = "";
    private volatile int[] mSyncStatus = { 0, 0, 0, 0 };
    //private volatile long[] mQuotaStatus = { 0, 0 };
    
    private volatile Boolean mIsPolling = Boolean.FALSE;
    
    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.status);
        setTitle(R.string.Label_Status);
        
        mAppVersionField = (TextView) findViewById(R.id.TextView_AppVersion);
        mUserNameField = (TextView) findViewById(R.id.TextView_UserName);
        mDeviceNameField = (TextView) findViewById(R.id.TextView_DeviceName);
        mSyncStateField = (TextView) findViewById(R.id.TextView_SyncState);
        mDownloadStatusField = (TextView) findViewById(R.id.TextView_DownloadStatus);
        mUploadStatusField = (TextView) findViewById(R.id.TextView_UploadStatus);
        //mQuotaStatusField = (TextView) findViewById(R.id.TextView_QuotaStatus);
        
        //mDownloadProgressBar = (ProgressBar) findViewById(R.id.ProgressBar_DownloadStatus);
        //mUploadProgressBar = (ProgressBar) findViewById(R.id.ProgressBar_UploadStatus);
        //mQuotaProgressBar = (ProgressBar) findViewById(R.id.ProgressBar_QuotaStatus);
        
        try {
            mAppVersion = this.getPackageManager().getPackageInfo(
                    this.getPackageName(), 0).versionName;
            mAppVersionField.setText(mAppVersion);
        } catch (NameNotFoundException e) {
            Log.e(LOG_TAG, e.getMessage());
        }
    }
    
    @Override
    protected void onResume() {
        
        super.onResume();
        mIsPolling = Boolean.TRUE;
        new PollStatusTask().execute(0);
    }
    
    @Override 
    protected void onPause() {
        
        super.onPause();
        mIsPolling = Boolean.FALSE;
    }
    
    private class PollStatusTask extends AsyncTask<Integer, Void, Void> {
        
        @Override
        protected Void doInBackground(Integer... params) {
            
            if ((params.length > 0) && (params[0] != null)) {
                int sleepTimeMs = params[0].intValue();
                if (sleepTimeMs > 0) {
                    try {
                        Thread.sleep(sleepTimeMs);
                    }
                    catch (InterruptedException e) {
                        Log.i(LOG_TAG, "Spurious wakeup.");
                    }
                }
            }
            
            if (!mBoundService.isLoggedIn()) {
                // No need to poll status if not logged in.
                Log.i(LOG_TAG,
                        "Do not poll for status if not logged in.");
                return null;
            }
            
            if (mIsPolling) {               
                // Get status strings.
                Log.i(LOG_TAG, "Polling for status");
                mUserName = mBoundService.getUserName();
                mDeviceName = mBoundService.getDeviceName();
                mSyncStatus = mBoundService.getSyncStatus();
                //mQuotaStatus = mBoundService.getQuotaStatus();
                
                // Schedule another update two seconds from now.
                new PollStatusTask().execute(2000);
            }
            
            return null;
        }
        
        @Override
        protected void onPostExecute(Void result) {
            
            if (mIsPolling) {
                mUserNameField.setText(mUserName);
                mDeviceNameField.setText(mDeviceName);
                
                Context context = StatusActivity.this;
                
                // Simplified the sync status display for demo purposes.
                
                int numDownloading = mSyncStatus[0];
                int numTotalDownload = mSyncStatus[1];
                int numUploading = mSyncStatus[2];
                int numTotalUpload = mSyncStatus[3];
                
                String syncState;
                if (numTotalDownload <= 0 && numTotalUpload <= 0) {
                    syncState = context.getString(R.string.Msg_StatusNotSyncing);
                }
                else {
                    syncState = context.getString(R.string.Msg_StatusSyncing);
                }
                mSyncStateField.setText(syncState);
                
                //mDownloadProgressBar.setVisibility(View.GONE);
                //int downloadProgress;
                //if (numTotalDownload <= 0) {
                    //mDownloadProgressBar.setVisibility(View.INVISIBLE);
                    //downloadProgress = mDownloadProgressBar.getMax();
                //}
                //else {
                    //mDownloadProgressBar.setVisibility(View.VISIBLE);
                    //downloadProgress = numDownloading
                            //* mDownloadProgressBar.getMax() / numTotalDownload;
                //}
                //mDownloadProgressBar.setProgress(downloadProgress);
                
                //String downloadStatus = 
                //    context.getString(R.string.Label_Downloading)
                //        + " " + numDownloading + " / " + numTotalDownload + " "
                //        + context.getString(R.string.Label_DownloadingFiles);
                String downloadStatus;
                if (numDownloading <= 0) {
                    downloadStatus = "";
                }
                else {
                    downloadStatus = String.format(context
                            .getString(R.string.Msg_DownloadingFiles),
                            numTotalDownload);
                }
                mDownloadStatusField.setText(downloadStatus);
                
                //mUploadProgressBar.setVisibility(View.GONE);
                //int uploadProgress;
                //if (numTotalUpload <= 0) {
                    //mUploadProgressBar.setVisibility(View.INVISIBLE);
                    //uploadProgress = mUploadProgressBar.getMax();
                //}
                //else {
                    //mUploadProgressBar.setVisibility(View.VISIBLE);
                    //uploadProgress = numUploading * mUploadProgressBar.getMax()
                            /// numTotalUpload;
                //}
                //mUploadProgressBar.setProgress(uploadProgress);

                //String uploadStatus = context.getString(R.string.Label_Uploading)
                //        + " " + numUploading + " / " + numTotalUpload + " "
                //        + context.getString(R.string.Label_UploadingFiles);
                String uploadStatus;
                if (numUploading <= 0) {
                    uploadStatus = "";
                }
                else {
                    uploadStatus = String.format(context
                            .getString(R.string.Msg_UploadingFiles),
                            numTotalUpload);
                }
                mUploadStatusField.setText(uploadStatus);
                
                //long freeBytes = mQuotaStatus[0];
                //long totalBytes = mQuotaStatus[1];
                
                //mQuotaProgressBar.setVisibility(View.GONE);
                //int quotaProgress;
                //if (totalBytes <= 0) {
                    //quotaProgress = mQuotaProgressBar.getMax();
                //}
                //else {
                    //quotaProgress = (int) (freeBytes
                            //* mQuotaProgressBar.getMax() / totalBytes);
                //}
                //mQuotaProgressBar.setProgress(quotaProgress);
                
                //mQuotaStatusField.setVisibility(View.GONE);
                //String quotaStatus = "" + freeBytes + " / " + totalBytes + " "
                        //+ context.getString(R.string.Label_AvailableBytes);
                //mQuotaStatusField.setText(quotaStatus);
            }  
        }
    }
    
    public void onClick_SyncFolders(View view) {

        Log.i(LOG_TAG, "Launching sync folders activity ("
                + Constants.ACTIVITY_SYNCFOLDERS + ")");
        Intent syncFoldersIntent = new Intent(StatusActivity.this,
                SyncFoldersActivity.class);
        startActivityForResult(syncFoldersIntent,
                Constants.ACTIVITY_SYNCFOLDERS);
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        
        Log.i(LOG_TAG, "Result from request " + requestCode + ": " + resultCode);
        
        switch (requestCode) {
        case Constants.ACTIVITY_SYNCFOLDERS:
            if (resultCode == RESULT_CANCELED) {
                // The child activity canceled out. Do nothing.
            }
            else {
                if (resultCode != RESULT_OK
                        && resultCode != Constants.RESULT_RESTART
                        && resultCode != Constants.RESULT_EXIT
                        && resultCode != Constants.RESULT_BACK_TO_MAIN) {
                    Log.e(LOG_TAG, "Encountered unknown result code: " + resultCode);
                }
                // Pass the result back up to the parent activity.
                setResult(resultCode, data);
                finish();
            }
            break;
            
        default:
            super.onActivityResult(requestCode, resultCode, data);
        }
    }
}
