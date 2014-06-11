//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.igware.example_remote_panel.ui;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.Toast;

import com.igware.android_cc_sdk.example_remote_panel.R;
import com.igware.example_remote_panel.CcdiClientForAcpanel;
import com.igware.example_remote_panel.util.Constants;
import com.igware.example_remote_panel.util.Utils;


/**
 * This class handles the common functionality needed by CcdActivity,
 * CcdListActivity, and CcdPreferenceActivity.
 */
public class CcdActivityDelegate {
    
    private static final String LOG_TAG = "CcdActivityDelegate";
    
    private final Activity mActivity;
    private final CcdiClientForAcpanel mBoundService;
    
    private ProgressDialog mProgressDialog;
    
    public CcdActivityDelegate(Activity activity, CcdiClientForAcpanel service) {
        
        mActivity = activity;
        mBoundService = service;
    }
    
    public void onCreate() {
        
        mBoundService.onCreate();
    }
    
    public void onStart() {
        
        mBoundService.onStart();
    }
    
    public void onResume() {
        mBoundService.onResume();
    }
    
    public void onPause() {
        mBoundService.onPause();
    }
    
    public void onStop() {
        
        mBoundService.onStop();
    }
    
    public void onDestroy() {
        
        mBoundService.onDestroy();
    }
   
    public boolean onCreateOptionsMenu(Menu menu) {
        
        MenuInflater inflater = mActivity.getMenuInflater();
        inflater.inflate(R.menu.options_menu, menu);
        return true;
    }
    
    public boolean onCreateOptionsMenuForList(Menu menu) {
        
        MenuInflater inflater = mActivity.getMenuInflater();
        inflater.inflate(R.menu.options_menu_list, menu);
        return true;
    }
    
    // Common options menu items for all activities.
    public boolean onPrepareOptionsMenuHelper(Menu menu) {
        
        // Remove all items.
        menu.removeItem(R.id.Menu_PauseSync);
        menu.removeItem(R.id.Menu_ResumeSync);
        menu.removeItem(R.id.Menu_Exit);
        menu.removeItem(R.id.Menu_Status);
        menu.removeItem(R.id.Menu_Logout);
        menu.removeItem(R.id.Menu_Settings);
        menu.removeItem(R.id.Menu_Trashcan);
        
        // Check whether the sync agent is currently enabled.
        // We really should not call to the back-end outside of an async thread,
        // but I don't see an alternative here. onPrepareOptionsMenu must be
        // synchronous.
        if (mBoundService.isSyncEnabled()) {
            // Add "pause sync" button.
            menu.add(1, R.id.Menu_PauseSync, Menu.FIRST,
                    R.string.Menu_PauseSync).setIcon(R.drawable.pause);
        }
        else {
            // Add "resume sync" button.
            menu.add(1, R.id.Menu_ResumeSync, Menu.FIRST,
                    R.string.Menu_ResumeSync).setIcon(R.drawable.resume);
        }
        
        // Add the rest of the items, so that they show up in the correct order.
        menu.add(1, R.id.Menu_Exit, Menu.FIRST + 1, R.string.Menu_Exit)
                .setIcon(R.drawable.exit);
        menu.add(1, R.id.Menu_Status, Menu.FIRST + 2, R.string.Menu_Status)
                .setIcon(R.drawable.ic_menu_view);
        menu.add(1, R.id.Menu_Logout, Menu.FIRST + 3, R.string.Menu_Logout)
                .setIcon(R.drawable.logout);
        menu.add(1, R.id.Menu_Settings, Menu.FIRST + 4, R.string.Menu_Settings)
                .setIcon(R.drawable.settings);
        menu.add(1, R.id.Menu_Trashcan, Menu.FIRST + 5, R.string.Menu_Trashcan)
                .setIcon(R.drawable.trash);
        
        return true;
    }
    
    // Options menu items for non-list activities.
    public boolean onPrepareOptionsMenu(Menu menu) {
        
        onPrepareOptionsMenuHelper(menu);
        
        // Remove all items.       
        menu.removeItem(R.id.Menu_RenameDevice);
        menu.removeItem(R.id.Menu_UnlinkDevice);
        
        // Add all items, so that they show up in the correct order.
        menu.add(1, R.id.Menu_RenameDevice, Menu.FIRST + 6,
                R.string.Menu_RenameDevice).setIcon(R.drawable.rename);
        menu.add(1, R.id.Menu_UnlinkDevice, Menu.FIRST + 7,
                R.string.Menu_UnlinkDevice).setIcon(R.drawable.unlink);
        
        return true;
    }
    
    // Options menu items for list activities.
    public boolean onPrepareOptionsMenuForList(Menu menu) {
        
        onPrepareOptionsMenuHelper(menu);
        
        // Remove all items.
        menu.removeItem(R.id.Menu_Refresh);
        menu.removeItem(R.id.Menu_RefreshSD);
        
        // Add all items, so that they show up in the correct order.
        menu.add(1, R.id.Menu_Refresh, Menu.FIRST + 6, R.string.Menu_Refresh)
                .setIcon(R.drawable.ic_menu_refresh);
        menu.add(1, R.id.Menu_RefreshSD, Menu.FIRST + 7,
                R.string.Menu_RefreshSD).setIcon(R.drawable.ic_menu_refresh);
        
        return true;
    }
    
    // Common options handled by all activities.
    private boolean onOptionsItemSelectedHelper(MenuItem item) {
        
        // Handle item selection
        switch (item.getItemId()) {
        
        case R.id.Menu_PauseSync:
            onPauseSyncOptionSelected();
            // For Android 3.0:
            //mActivity.invalidateOptionsMenu();
            return true;
            
        case R.id.Menu_ResumeSync:
            onResumeSyncOptionSelected();
            // For Android 3.0:
            //mActivity.invalidateOptionsMenu();
            return true;
            
        case R.id.Menu_Exit:
            onExitOptionSelected();
            return true;
            
        case R.id.Menu_Status:    
            onStatusOptionSelected();   
            return true;
            
        case R.id.Menu_Logout:
            onLogoutOptionSelected();
            return true;
            
        case R.id.Menu_Settings:
            onSettingsOptionSelected();
            return true;
            
        default:
            return false;
        }
    }
    
    // Options handled by non-list activities.
    public boolean onOptionsItemSelected(MenuItem item) {
        
        boolean handled = onOptionsItemSelectedHelper(item);
        if (handled) {
            return true;
        }
        
        // Handle item selection
        switch (item.getItemId()) {
        
        case R.id.Menu_RenameDevice:
            onRenameDeviceOptionSelected();
            return true;
            
        case R.id.Menu_UnlinkDevice:
            onUnlinkDeviceOptionSelected();
            return true;
            
        default:
            return false;
        }
    }
    
    // Options handled by list activities.
    public boolean onOptionsItemSelectedForList(MenuItem item) {
        
        boolean handled = onOptionsItemSelectedHelper(item);
        if (handled) {
            return true;
        }
        
        // Handle item selection
        switch (item.getItemId()) {
        
        case R.id.Menu_Refresh:
            OnRefreshOptionSelected();
            return true;
            
        case R.id.Menu_RefreshSD:
            onRefreshSdOptionSelected();
            return true;
            
        default:
            return false;
        }
    }

    private class RefreshTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... params) {
            
            mActivity.startActivity(mActivity.getIntent()); 
            mActivity.finish();
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            
            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
        }
    }
    
    private void OnRefreshOptionSelected() {

        mProgressDialog = ProgressDialog.show(mActivity, "",
                mActivity.getText(R.string.Msg_Refreshing), true);

        new RefreshTask().execute();
    }
    
    private void onExitOptionSelected() {
        
        mActivity.setResult(Constants.RESULT_EXIT);
        mActivity.finish();
    }
    
    private class LogOutTask extends AsyncTask<Void, Void, Void> {
        
        @Override
        protected Void doInBackground(Void... params) {
            
            mBoundService.doLogout();
            
            // Erase the user name cached in shared preferences.
            Utils.cacheUserName("", mActivity);
            
            return null;
        }
        
        @Override
        protected void onPostExecute(Void result) {
            
            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            mActivity.setResult(Constants.RESULT_BACK_TO_MAIN);
            mActivity.finish();
        }
    }
    
    private void onLogoutOptionSelected() {
        
        mProgressDialog = ProgressDialog.show(mActivity, "", 
                mActivity.getText(R.string.Msg_LoggingOut), true);
        
        new LogOutTask().execute();
    }
    
    private void onRenameDeviceOptionSelected() {
        
        // Launch rename device activity.
        Log.i(LOG_TAG, "Launching rename device activity ("
                + Constants.ACTIVITY_RENAME_DEVICE + ")");
        Intent renameDeviceIntent = new Intent(mActivity,
                RenameDeviceActivity.class);
        mActivity.startActivityForResult(renameDeviceIntent,
                Constants.ACTIVITY_RENAME_DEVICE);
    }
    
    private class UnlinkDeviceTask extends AsyncTask<Void, Void, Integer> {
        
        @Override
        protected Integer doInBackground(Void... params) {

            int result = mBoundService.unlinkDevice();
            return result;
        }
        
        @Override
        protected void onPostExecute(Integer result) {
            
            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(mActivity, R.string.Msg_UnlinkedDevice,
                        Toast.LENGTH_SHORT).show();
                mActivity.setResult(Constants.RESULT_BACK_TO_MAIN);
                mActivity.finish();
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                mActivity.showDialog(Constants.DIALOG_UNLINK_DEVICE_FAILED,
                        bundle);
            }
        }
    }
    
    private void onUnlinkDeviceOptionSelected() {
        
        mProgressDialog = ProgressDialog.show(mActivity, "",
                mActivity.getText(R.string.Msg_UnlinkingDevice), true);
        new UnlinkDeviceTask().execute();
    }
    
    private void onRefreshSdOptionSelected() {
        
        mActivity.sendBroadcast(new Intent(Intent.ACTION_MEDIA_MOUNTED, Uri
                .parse("file://" + Environment.getExternalStorageDirectory())));
    }
    
    private void onSettingsOptionSelected() {
        
        // Launch settings activity.
        Log.i(LOG_TAG, "Launching settings activity ("
                + Constants.ACTIVITY_SETTINGS + ")");
        Intent settingsIntent = new Intent(mActivity,
                SettingsActivity.class);
        mActivity.startActivityForResult(settingsIntent,
                Constants.ACTIVITY_SETTINGS);
    }
    
    private class PauseSyncTask extends AsyncTask<Void, Void, Integer> {
        
        @Override
        protected Integer doInBackground(Void... params) {

            int result = mBoundService.pauseSync();
            return result;
        }
        
        @Override
        protected void onPostExecute(Integer result) {
            
            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(mActivity, R.string.Msg_PausedSync,
                        Toast.LENGTH_SHORT).show();                 
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                mActivity.showDialog(Constants.DIALOG_PAUSE_SYNC_FAILED,
                        bundle);
            }
        }
    }
    
    private void onPauseSyncOptionSelected() {
        
        mProgressDialog = ProgressDialog.show(mActivity, "",
                mActivity.getText(R.string.Msg_PausingSync), true);
        new PauseSyncTask().execute();
    }
    
    private class ResumeSyncTask extends AsyncTask<Void, Void, Integer> {
        
        @Override
        protected Integer doInBackground(Void... params) {

            int result = mBoundService.resumeSync();
            return result;
        }
        
        @Override
        protected void onPostExecute(Integer result) {
            
            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(mActivity, R.string.Msg_ResumedSync,
                        Toast.LENGTH_SHORT).show();                 
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                mActivity.showDialog(Constants.DIALOG_RESUME_SYNC_FAILED,
                        bundle);
            }
        }
    }
    
    private void onResumeSyncOptionSelected() {
        
        mProgressDialog = ProgressDialog.show(mActivity, "",
                mActivity.getText(R.string.Msg_ResumingSync), true);
        new ResumeSyncTask().execute();
    }
    
    private void onStatusOptionSelected() {
        
        // Launch status activity.
        Log.i(LOG_TAG, "Launching status activity ("
                + Constants.ACTIVITY_STATUS + ")");
        Intent statusIntent = new Intent(mActivity, StatusActivity.class);
        mActivity.startActivityForResult(statusIntent,
                Constants.ACTIVITY_STATUS);
    }
    
    public void onPrepareDialog(int id, Dialog dialog, Bundle args) {
        
        switch (id) {
        case Constants.DIALOG_UNLINK_DEVICE_FAILED:
        case Constants.DIALOG_PAUSE_SYNC_FAILED:
        case Constants.DIALOG_RESUME_SYNC_FAILED:
            // The Dialog returned from onCreateDialog gets cached, so we need
            // to modify the existing one each time it is displayed.
            int errorCode = args.getInt(Constants.BUNDLE_ID_ERROR_CODE, 1);
            ((AlertDialog) dialog).setMessage(Utils.errorCodeToMessage(
                    errorCode, mActivity));
            break;
        
        default:
            break;
        }
    }
    
    public Dialog onCreateDialog(int id, Bundle args) {
        
        AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
        
        switch (id) {
        case Constants.DIALOG_UNLINK_DEVICE_FAILED:           
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(mActivity.getText(
                            R.string.Label_UnlinkDeviceFailed))
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
            
        case Constants.DIALOG_PAUSE_SYNC_FAILED:           
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(mActivity.getText(
                            R.string.Label_PauseSyncFailed))
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
            
        case Constants.DIALOG_RESUME_SYNC_FAILED:
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(mActivity.getText(
                            R.string.Label_ResumeSyncFailed))
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
            return null;
        }       
    }
    
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        
        Log.i(LOG_TAG, "Result from request " + requestCode + ": " + resultCode);      
        
        switch (requestCode) {
        case Constants.ACTIVITY_RENAME_DEVICE:
        case Constants.ACTIVITY_SETTINGS:
        case Constants.ACTIVITY_STATUS:
            if (resultCode == Activity.RESULT_CANCELED
                    || resultCode == Activity.RESULT_OK) {
                // Child activity done. Do nothing.
            }
            else {
                if (resultCode != Constants.RESULT_RESTART
                        && resultCode != Constants.RESULT_EXIT
                        && resultCode != Constants.RESULT_BACK_TO_MAIN) {
                    Log.e(LOG_TAG, "Encountered unknown result code: " + resultCode);
                }
                // Pass the result code back up to the parent activity.
                mActivity.setResult(resultCode, data);
                mActivity.finish();
            }       
            break;
            
        default:
            break;
        }
    }
}
