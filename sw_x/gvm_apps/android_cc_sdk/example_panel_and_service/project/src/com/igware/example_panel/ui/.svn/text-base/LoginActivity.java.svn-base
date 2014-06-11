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


public class LoginActivity extends CcdActivity {
    
    //private static final String LOG_TAG = "LoginActivity";
    
    private static final String KEY_USER_NAME = "userName";
    private static final String KEY_PASSWORD = "password";
    
    private EditText mUserNameField;
    private EditText mPasswordField;
    
    private ProgressDialog mProgressDialog;
    
    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.login);
        setTitle(R.string.Label_Login);
        
        mUserNameField = (EditText) findViewById(R.id.EditText_UserName);
        mPasswordField = (EditText) findViewById(R.id.EditText_Password);
        
        if (savedInstanceState != null) {
            String userName = savedInstanceState.getString(KEY_USER_NAME);
            if (userName != null) {
                mUserNameField.setText(userName);
            }
            String password = savedInstanceState.getString(KEY_PASSWORD);
            if (password != null) {
                mPasswordField.setText(password);
            }
        }
    }
    
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        
        outState.putString(KEY_USER_NAME, mUserNameField.getText().toString());
        outState.putString(KEY_PASSWORD, mPasswordField.getText().toString());
    }
    
    private class LoginTask extends AsyncTask<String, Void, Integer> {
        
        @Override
        protected Integer doInBackground(String... params) {

            String userName = params[0].trim();
            String password = params[1].trim();
            int result = mBoundService.doLogin(userName, password);
            
            if (result == 0) {
                // Cache the user name in shared preferences.
                Utils.cacheUserName(userName, LoginActivity.this);
            }
            
            return result;
        }
        
        @Override
        protected void onPostExecute(Integer result) {
            
            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(LoginActivity.this, R.string.Msg_LoggedIn,
                        Toast.LENGTH_SHORT).show();
                setResult(Constants.RESULT_BACK_TO_MAIN);
                finish();                  
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                showDialog(Constants.DIALOG_LOG_IN_FAILED, bundle);
            }
        }
    }
    
    public void onClick_Login(View view) {

        String userName = mUserNameField.getText().toString();
        String password = mPasswordField.getText().toString();
        
        // Make sure the user name field is not empty.
        if (userName.equals("")) {
            // Show toast to the user and then ignore this click.
            Toast.makeText(LoginActivity.this, R.string.Msg_NeedUserName,
                    Toast.LENGTH_LONG).show();
            return;
        }
        
        mProgressDialog = ProgressDialog.show(this, "",
                getText(R.string.Msg_LoggingIn), true);
        new LoginTask().execute(userName, password);
    }
    
    public void onClick_NewUser(View view) {
        
        // This is a first time user (or just an user who needs a new account).
        Utils.cacheFirstTimeUser(true, this);
        setResult(Constants.RESULT_BACK_TO_MAIN);
        finish();
    }
    
    @Override
    protected void onPrepareDialog(int id, Dialog dialog, Bundle args) {
        
        switch (id) {
        case Constants.DIALOG_LOG_IN_FAILED:
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
        case Constants.DIALOG_CACHE_CLEARED:            
            builder.setMessage(getText(R.string.Msg_CacheCleared))
                    .setCancelable(false)
                    .setNeutralButton("OK",
                            new DialogInterface.OnClickListener() {
                                
                                public void onClick(DialogInterface dialog,
                                        int ignore) {

                                    // Terminate the process.
                                    setResult(Constants.RESULT_RESTART);
                                    finish();
                                }
                            });
            return builder.create();
        
        case Constants.DIALOG_LOG_IN_FAILED: 
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(getText(R.string.Label_LoginFailed))
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
