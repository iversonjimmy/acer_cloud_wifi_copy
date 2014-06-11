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

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.Toast;

import com.igware.android_cc_sdk.example_panel_and_service.R;
import com.igware.example_panel.util.Constants;

public class SettingsActivity extends CcdPreferenceActivity {
    
    private static final String LOG_TAG = "SettingsActivity";
    
    private ProgressDialog mProgressDialog;
    
    // Sync settings
    
    private boolean mCameraSyncUpload;
    private boolean mCameraSyncDownload;
    private int mCameraSyncDownloadMaxSize;
    private int mCameraSyncDownloadMaxFiles;
    private boolean mNotification;
    
    private CheckBoxPreference mCameraSyncUploadPreference;
    private CheckBoxPreference mCameraSyncDownloadPreference;
    private EditTextPreference mCameraSyncDownloadMaxSizePreference;
    private EditTextPreference mCameraSyncDownloadMaxFilesPreference;
    private CheckBoxPreference mNotificationPreference;
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        
        super.onCreate(savedInstanceState);
        setTitle(R.string.Label_Settings);
        
        // Load the preferences from XML resource.
        addPreferencesFromResource(R.xml.settings);
            
        SettingChangedListener listener = new SettingChangedListener();
        
        mCameraSyncUploadPreference = (CheckBoxPreference) findPreference(Constants.SETTING_CAMERA_SYNC_UPLOAD);
        mCameraSyncUploadPreference.setOnPreferenceClickListener(listener);
        
        mCameraSyncDownloadPreference = (CheckBoxPreference) findPreference(Constants.SETTING_CAMERA_SYNC_DOWNLOAD);
        mCameraSyncDownloadPreference.setOnPreferenceClickListener(listener);
        
        mCameraSyncDownloadMaxSizePreference = (EditTextPreference) findPreference(Constants.SETTING_CAMERA_SYNC_DOWNLOAD_MAX_SIZE);
        mCameraSyncDownloadMaxSizePreference.setOnPreferenceChangeListener(listener);
        
        mCameraSyncDownloadMaxFilesPreference = (EditTextPreference) findPreference(Constants.SETTING_CAMERA_SYNC_DOWNLOAD_MAX_FILES);
        mCameraSyncDownloadMaxFilesPreference.setOnPreferenceChangeListener(listener);
        
        mNotificationPreference = (CheckBoxPreference) findPreference(Constants.SETTING_NOTIFICATION);
        mNotificationPreference.setOnPreferenceClickListener(listener);
    }
    
    @Override
    public void onStart() {
        
        super.onStart();
        
        getPrefs();
    }
    
    @Override
    protected void onResume() {
        
        super.onResume();
        
        new UpdateCameraSyncUploadFolderSubscribeStateTask().execute();
        new UpdateCameraSyncDownloadFolderSubscribeStateTask().execute();
        new UpdateCameraSyncDownloadFolderMaxSizeTask().execute();
        new UpdateCameraSyncDownloadFolderMaxFilesTask().execute();
    }
    
    private class UpdateCameraSyncUploadFolderSubscribeStateTask extends AsyncTask<Void, Void, Boolean> {
        
        @Override
        protected Boolean doInBackground(Void... params) {
            
            // Whether the "camera sync" check boxes should be checked or not
            // depends on the service state.
            boolean cameraSyncUploadSubscribed = mBoundService
                    .isCameraSyncUploadFolderSubscribed();
            Log.i(LOG_TAG, "Is the camera sync upload folder subscribed? "
                    + cameraSyncUploadSubscribed);
            return cameraSyncUploadSubscribed;
        }
        
        @Override
        protected void onPostExecute(Boolean result) {
            
            mCameraSyncUploadPreference.setChecked(result);
        }
    }
    
    private class UpdateCameraSyncDownloadFolderSubscribeStateTask extends AsyncTask<Void, Void, Boolean> {
        
        @Override
        protected Boolean doInBackground(Void... params) {
            
            // Whether the "camera sync" check boxes should be checked or not
            // depends on the service state.
            boolean cameraSyncDownloadSubscribed = mBoundService
                    .isCameraSyncDownloadFolderSubscribed();
            Log.i(LOG_TAG, "Is the camera sync download folder subscribed? "
                    + cameraSyncDownloadSubscribed);
            return cameraSyncDownloadSubscribed;
        }
        
        @Override
        protected void onPostExecute(Boolean result) {
            
            mCameraSyncDownloadPreference.setChecked(result);
            mCameraSyncDownloadMaxSizePreference.setEnabled(result);
            mCameraSyncDownloadMaxFilesPreference.setEnabled(result);
        }
    }
    
    private class UpdateCameraSyncDownloadFolderMaxSizeTask extends AsyncTask<Void, Void, Integer> {

        @Override
        protected Integer doInBackground(Void... params) {

            // Get the current max size value from the server.
            int maxSize = mBoundService.getCameraSyncDownloadMaxSize();
            Log.i(LOG_TAG, "Current max size for camera sync download folder: "
                    + maxSize);
            return maxSize;
        }

        @Override
        protected void onPostExecute(Integer result) {

            mCameraSyncDownloadMaxSizePreference.setText("" + result);
        }
    }
    
    private class UpdateCameraSyncDownloadFolderMaxFilesTask extends AsyncTask<Void, Void, Integer> {

        @Override
        protected Integer doInBackground(Void... params) {

            // Get the current max files value from the server.
            int maxFiles = mBoundService.getCameraSyncDownloadMaxFiles();
            Log.i(LOG_TAG, "Current max files for camera sync download folder: "
                    + maxFiles);
            return maxFiles;
        }

        @Override
        protected void onPostExecute(Integer result) {

            mCameraSyncDownloadMaxFilesPreference.setText("" + result);
        }
    }
    
    private class SubscribeCameraSyncUploadFolderTask extends AsyncTask<Void, Void, Integer> {
        
        @Override
        protected Integer doInBackground(Void... params) {
            
            return mBoundService.addCameraSyncUploadSubscription();
        }
        
        @Override
        protected void onPostExecute(Integer result) {

            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(SettingsActivity.this,
                        R.string.Msg_SubscribedCameraSyncUploadFolder,
                        Toast.LENGTH_SHORT).show();
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                showDialog(Constants.DIALOG_SUBSCRIBE_CAMERA_SYNC_UPLOAD_FOLDER_FAILED,
                        bundle);
            }
        }
    }
    
    private class SubscribeCameraSyncDownloadFolderTask extends AsyncTask<Void, Void, Integer> {
        
        @Override
        protected Integer doInBackground(Void... params) {
            
            return mBoundService.addCameraSyncDownloadSubscription(
                    mCameraSyncDownloadMaxSize, mCameraSyncDownloadMaxFiles);
        }
        
        @Override
        protected void onPostExecute(Integer result) {

            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(SettingsActivity.this,
                        R.string.Msg_SubscribedCameraSyncDownloadFolder,
                        Toast.LENGTH_SHORT).show();
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                showDialog(Constants.DIALOG_SUBSCRIBE_CAMERA_SYNC_DOWNLOAD_FOLDER_FAILED,
                        bundle);
            }
        }
    }
    
    private class UnsubscribeCameraSyncUploadFolderTask extends AsyncTask<Void, Void, Integer> {
        
        @Override
        protected Integer doInBackground(Void... params) {
            
            int result;
            long cameraSyncUploadFolderId = mBoundService.getCameraSyncUploadFolderId();
            if (cameraSyncUploadFolderId < 0) {
                // Camera sync upload folder does not exist yet. No need to unsubscribe.
                Log.e(LOG_TAG, "Cannot find camera sync upload folder.");
                return 0;
            }
            result = mBoundService.removeCameraSyncSubscription(cameraSyncUploadFolderId);
            return result;
        }
        
        @Override
        protected void onPostExecute(Integer result) {

            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(SettingsActivity.this,
                        R.string.Msg_UnsubscribedCameraSyncUploadFolder,
                        Toast.LENGTH_SHORT).show();
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                showDialog(Constants.DIALOG_UNSUBSCRIBE_CAMERA_SYNC_UPLOAD_FOLDER_FAILED,
                        bundle);
            }
        }
    }
    
    private class UnsubscribeCameraSyncDownloadFolderTask extends AsyncTask<Void, Void, Integer> {
        
        @Override
        protected Integer doInBackground(Void... params) {
            
            int result;
            long cameraSyncDownloadFolderId = mBoundService.getCameraSyncDownloadFolderId();
            if (cameraSyncDownloadFolderId < 0) {
                // Camera sync download folder does not exist yet. No need to unsubscribe.
                Log.e(LOG_TAG, "Cannot find camera sync download folder.");
                return 0;
            }
            result = mBoundService.removeCameraSyncSubscription(cameraSyncDownloadFolderId);
            return result;
        }
        
        @Override
        protected void onPostExecute(Integer result) {

            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(SettingsActivity.this,
                        R.string.Msg_UnsubscribedCameraSyncDownloadFolder,
                        Toast.LENGTH_SHORT).show();
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                showDialog(Constants.DIALOG_UNSUBSCRIBE_CAMERA_SYNC_DOWNLOAD_FOLDER_FAILED,
                        bundle);
            }
        }
    }
    
    private class UpdateCameraSyncDownloadFolderSubscriptionTask extends AsyncTask<Void, Void, Integer> {
        
        @Override
        protected Integer doInBackground(Void... params) {
            
            return mBoundService.updateCameraSyncDownloadSubscription(
                    mCameraSyncDownloadMaxSize, mCameraSyncDownloadMaxFiles);
        }
        
        @Override
        protected void onPostExecute(Integer result) {

            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(SettingsActivity.this,
                        R.string.Msg_UpdatedCameraSyncDownloadFolder,
                        Toast.LENGTH_SHORT).show();
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                showDialog(Constants.DIALOG_UPDATE_CAMERA_SYNC_DOWNLOAD_FAILED,
                        bundle);
            }
        }
    }
    
    private class SettingChangedListener implements OnPreferenceClickListener,
            OnPreferenceChangeListener {
        
        @Override
        public boolean onPreferenceClick(Preference preference) {
            
            getPrefs();
            
            String key = preference.getKey();
            
            if (key.equals(Constants.SETTING_CAMERA_SYNC_UPLOAD)) {
                Log.i(LOG_TAG, "Preference change: " + key + " = "
                        + mCameraSyncUpload);
                if (mCameraSyncUpload) {
                    // User wants camera photos to be uploaded.
                    mProgressDialog = ProgressDialog.show(
                            SettingsActivity.this,
                            "",
                            getText(R.string.Msg_SubscribingCameraSyncUploadFolder),
                            true);
                    new SubscribeCameraSyncUploadFolderTask().execute();
                }
                else {
                    // User wants to stop uploading camera photos.
                    mProgressDialog = ProgressDialog.show(
                            SettingsActivity.this,
                            "",
                            getText(R.string.Msg_UnsubscribingCameraSyncUploadFolder),
                            true);
                    new UnsubscribeCameraSyncUploadFolderTask().execute();
                }                
                return true;
            }
            else if (key.equals(Constants.SETTING_CAMERA_SYNC_DOWNLOAD)) {
                Log.i(LOG_TAG, "Preference change: " + key + " = "
                        + mCameraSyncDownload);
                if (mCameraSyncDownload) {
                    // User wants camera photos to be downloaded.
                    mCameraSyncDownloadMaxSizePreference.setEnabled(true);
                    mCameraSyncDownloadMaxFilesPreference.setEnabled(true);
                    mProgressDialog = ProgressDialog.show(
                            SettingsActivity.this,
                            "",
                            getText(R.string.Msg_SubscribingCameraSyncDownloadFolder),
                            true);
                    new SubscribeCameraSyncDownloadFolderTask().execute();
                }
                else {
                    // User wants to stop downloading camera photos.
                    mCameraSyncDownloadMaxSizePreference.setEnabled(false);
                    mCameraSyncDownloadMaxFilesPreference.setEnabled(false);
                    mProgressDialog = ProgressDialog.show(
                            SettingsActivity.this,
                            "",
                            getText(R.string.Msg_UnsubscribingCameraSyncDownloadFolder),
                            true);
                    new UnsubscribeCameraSyncDownloadFolderTask().execute();
                }                
                return true;
            }
            else if (key.equals(Constants.SETTING_NOTIFICATION)) {
                Log.i(LOG_TAG, "Preference change: " + key + " = "
                        + mNotification);
                if (mNotification) {
                    Toast.makeText(SettingsActivity.this,
                            R.string.Msg_NotificationOn, Toast.LENGTH_SHORT)
                            .show();
                }
                else {
                    Toast.makeText(SettingsActivity.this,
                            R.string.Msg_NotificationOff, Toast.LENGTH_SHORT)
                            .show();
                }
                return true;
            }
            
            return false;
        }
        
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            
            String key = preference.getKey();
            
            if (key.equals(Constants.SETTING_CAMERA_SYNC_DOWNLOAD_MAX_SIZE)) {
                
                String maxSize = (String)newValue;
                Log.i(LOG_TAG, "Preference change: " + key + ", string = "
                        + maxSize);
                try {
                    mCameraSyncDownloadMaxSize = Integer.parseInt(maxSize);
                } catch (NumberFormatException e) {
                    mCameraSyncDownloadMaxSizePreference.setText("0");
                    mCameraSyncDownloadMaxSize = 0;
                }
                
                Log.i(LOG_TAG, "Preference change: " + key + " = "
                        + mCameraSyncDownloadMaxSize);
                updateCameraSyncDownloadFolder();
                return true;
            }
            else if (key.equals(Constants.SETTING_CAMERA_SYNC_DOWNLOAD_MAX_FILES)) {
                
                String maxFiles = (String)newValue;
                Log.i(LOG_TAG, "Preference change: " + key + ", string = "
                        + maxFiles);
                try {
                    mCameraSyncDownloadMaxFiles = Integer.parseInt(maxFiles);
                } catch (NumberFormatException e) {
                    mCameraSyncDownloadMaxFilesPreference.setText("0");
                    mCameraSyncDownloadMaxFiles = 0;
                }
                
                Log.i(LOG_TAG, "Preference change: " + key + " = "
                        + mCameraSyncDownloadMaxFiles);
                updateCameraSyncDownloadFolder();
                return true;
            }
            
            return false;
        }
        
        private void updateCameraSyncDownloadFolder() {
            
            mProgressDialog = ProgressDialog.show(
                    SettingsActivity.this,
                    "",
                    getText(R.string.Msg_UpdatingCameraSyncDownloadFolder),
                    true);
            new UpdateCameraSyncDownloadFolderSubscriptionTask().execute();
        }
    }
    
    private void getPrefs() {
        
        // Get the shared preferences object.
        SharedPreferences prefs = PreferenceManager
                .getDefaultSharedPreferences(this);
        
        mCameraSyncUpload = prefs.getBoolean(
                Constants.SETTING_CAMERA_SYNC_UPLOAD, false);
        
        mCameraSyncDownload = prefs.getBoolean(
                Constants.SETTING_CAMERA_SYNC_DOWNLOAD, false);
        
        mNotification = prefs.getBoolean(Constants.SETTING_NOTIFICATION, true);
    }
    
    @Override
    protected void onPrepareDialog(int id, Dialog dialog, Bundle args) {
        
        switch (id) {
        case Constants.DIALOG_SUBSCRIBE_CAMERA_SYNC_UPLOAD_FOLDER_FAILED:
        case Constants.DIALOG_SUBSCRIBE_CAMERA_SYNC_DOWNLOAD_FOLDER_FAILED:
        case Constants.DIALOG_UNSUBSCRIBE_CAMERA_SYNC_UPLOAD_FOLDER_FAILED:
        case Constants.DIALOG_UNSUBSCRIBE_CAMERA_SYNC_DOWNLOAD_FOLDER_FAILED:
        case Constants.DIALOG_UPDATE_CAMERA_SYNC_DOWNLOAD_FAILED:
            // The Dialog returned from onCreateDialog gets cached, so we need
            // to modify the existing one each time it is displayed.
            int errorCode = args.getInt(Constants.BUNDLE_ID_ERROR_CODE, 1);
            ((AlertDialog) dialog).setMessage(errorCodeToMessage(errorCode));
            break;
            
        default:
            break;
        }
        
        // Need to call through to super.
        super.onPrepareDialog(id, dialog, args);
    }
    
    @Override
    protected Dialog onCreateDialog(int id, Bundle args) {
        
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        
        switch (id) {           
        case Constants.DIALOG_SUBSCRIBE_CAMERA_SYNC_UPLOAD_FOLDER_FAILED:
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(getText(R.string.Label_SubscribeCameraSyncUploadFolderFailed))
                    .setCancelable(false)
                    .setNeutralButton("OK",
                            new DialogInterface.OnClickListener() {
                                
                                public void onClick(DialogInterface dialog,
                                        int ignore) {

                                    // Nothing to do; the dialog will be
                                    // dismissed now.
                                }
                            });
            return builder.create();
            
        case Constants.DIALOG_SUBSCRIBE_CAMERA_SYNC_DOWNLOAD_FOLDER_FAILED:
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(getText(R.string.Label_SubscribeCameraSyncDownloadFolderFailed))
                    .setCancelable(false)
                    .setNeutralButton("OK",
                            new DialogInterface.OnClickListener() {
                                
                                public void onClick(DialogInterface dialog,
                                        int ignore) {

                                    // Nothing to do; the dialog will be
                                    // dismissed now.
                                }
                            });
            return builder.create();
            
        case Constants.DIALOG_UNSUBSCRIBE_CAMERA_SYNC_UPLOAD_FOLDER_FAILED:
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(getText(R.string.Label_UnsubscribeCameraSyncUploadFolderFailed))
                    .setCancelable(false)
                    .setNeutralButton("OK",
                            new DialogInterface.OnClickListener() {
                                
                                public void onClick(DialogInterface dialog,
                                        int ignore) {

                                    // Nothing to do; the dialog will be
                                    // dismissed now.
                                }
                            });
            return builder.create();
            
        case Constants.DIALOG_UNSUBSCRIBE_CAMERA_SYNC_DOWNLOAD_FOLDER_FAILED:
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(getText(R.string.Label_UnsubscribeCameraSyncDownloadFolderFailed))
                    .setCancelable(false)
                    .setNeutralButton("OK",
                            new DialogInterface.OnClickListener() {
                                
                                public void onClick(DialogInterface dialog,
                                        int ignore) {

                                    // Nothing to do; the dialog will be
                                    // dismissed now.
                                }
                            });
            return builder.create();
            
        case Constants.DIALOG_UPDATE_CAMERA_SYNC_DOWNLOAD_FAILED:
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(getText(R.string.Label_UpdateCameraSyncDownloadFolderFailed))
                    .setCancelable(false)
                    .setNeutralButton("OK",
                            new DialogInterface.OnClickListener() {
                                
                                public void onClick(DialogInterface dialog,
                                        int ignore) {

                                    // Nothing to do; the dialog will be
                                    // dismissed now.
                                }
                            });
            return builder.create();
            
        default:
            return super.onCreateDialog(id, args);
        }
    }
}
