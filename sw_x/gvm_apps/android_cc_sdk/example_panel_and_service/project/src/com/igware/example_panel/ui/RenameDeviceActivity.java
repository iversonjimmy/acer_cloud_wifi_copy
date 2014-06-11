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
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;

import com.igware.example_panel.util.Constants;
import com.igware.example_panel.util.Utils;
import com.igware.android_cc_sdk.example_panel_and_service.R;


public class RenameDeviceActivity extends CcdActivity {
    
    // private static final String LOG_TAG = "RenameDeviceActivity";
    
    private static final String KEY_DEVICE_NAME = "deviceName";
    
    private EditText mDeviceNameField;
    
    private ProgressDialog mProgressDialog;
    
    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.rename_device);
        setTitle(R.string.Label_RenameDevice);
        
        mDeviceNameField = (EditText) findViewById(R.id.EditText_DeviceName);
        
        if (savedInstanceState != null) {
            String deviceName = savedInstanceState.getString(KEY_DEVICE_NAME);
            if (deviceName != null) {
                mDeviceNameField.setText(deviceName);
            }
        }
    }
    
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        
        outState.putString(KEY_DEVICE_NAME, mDeviceNameField.getText()
                .toString());
    }
    
    @Override
    protected void onResume() {
        
        super.onResume();
        // Auto-fill the device name field with the current device name.
        new GetDeviceNameTask().execute();
    }
    
    private class GetDeviceNameTask extends AsyncTask<Void, Void, String> {
        
        @Override
        protected String doInBackground(Void... params) {
            
            return mBoundService.getDeviceName();
        }
        
        @Override
        protected void onPostExecute(String result) {
            
            mDeviceNameField.setText(result);
        }
    }
    
    private class RenameDeviceTask extends AsyncTask<String, Void, Integer> {
        
        @Override
        protected Integer doInBackground(String... params) {

            String deviceName = params[0];
            int result = mBoundService.renameDevice(deviceName);
            return result;
        }
        
        @Override
        protected void onPostExecute(Integer result) {

            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(RenameDeviceActivity.this,
                        R.string.Msg_RenamedDevice, Toast.LENGTH_SHORT).show();
                setResult(RESULT_OK);
                finish();
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                showDialog(Constants.DIALOG_RENAME_DEVICE_FAILED, bundle);
            }
        }
    }
    
    public void onClick_RenameDevice(View view) {

        String deviceName = mDeviceNameField.getText().toString();
        
        // Make sure that we do not have disallowed chars in the device name.
        if (Utils.containsChars(Constants.DISALLOWED_CHARS, deviceName)) {
            // Show toast to the user and then ignore this click.
            Toast.makeText(
                    RenameDeviceActivity.this,
                    String.format(this
                            .getString(R.string.Msg_DeviceNameHasDisallowedChars),
                            Constants.DISALLOWED_CHARS_DISPLAYABLE),
                    Toast.LENGTH_LONG).show();
            return;
        }
        
        mProgressDialog = ProgressDialog.show(this, "",
                getText(R.string.Msg_RenamingDevice), true);
        new RenameDeviceTask().execute(deviceName);
    }
    
    @Override
    protected void onPrepareDialog(int id, Dialog dialog, Bundle args) {
        
        switch (id) {
        case Constants.DIALOG_RENAME_DEVICE_FAILED:
            // The Dialog returned from onCreateDialog gets cached, so we need
            // to modify the existing one each time it is displayed.
            int errorCode = args.getInt(Constants.BUNDLE_ID_ERROR_CODE, 1);
            ((AlertDialog) dialog).setMessage(errorCodeToMessage(errorCode));
            break;
            
        default:
            break;
        }
        
        super.onPrepareDialog(id, dialog, args);
    }
    
    @Override
    protected Dialog onCreateDialog(int id, Bundle args) {
        
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        
        switch (id) {           
        case Constants.DIALOG_RENAME_DEVICE_FAILED: 
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(getText(R.string.Label_RenameDeviceFailed))
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
