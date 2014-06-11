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


public class CreateAccountActivity extends CcdActivity {
    
    // private static final String LOG_TAG = "CreateAccountActivity";
    
    private static final String KEY_USER_NAME = "userName";
    private static final String KEY_PASSWORD = "password";
    private static final String KEY_CONFIRM_PASSWORD = "confirmPassword";
    private static final String KEY_EMAIL = "email";
    private static final String KEY_REG_KEY = "regKey";
    
    private EditText mUserNameField;
    private EditText mPasswordField;
    private EditText mConfirmPasswordField;
    private EditText mEmailField;
    private EditText mRegKeyField;
    
    private ProgressDialog mProgressDialog;
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        
        super.onCreate(savedInstanceState);
        setContentView(R.layout.create_account);
        setTitle(R.string.Label_CreateAccount);
        
        mUserNameField = (EditText) findViewById(R.id.EditText_UserName);
        mPasswordField = (EditText) findViewById(R.id.EditText_Password);
        mConfirmPasswordField = (EditText) findViewById(R.id.EditText_ConfirmPassword);
        mEmailField = (EditText) findViewById(R.id.EditText_Email);
        mRegKeyField = (EditText) findViewById(R.id.EditText_RegKey);
        
        if (savedInstanceState != null) {
            String userName = savedInstanceState.getString(KEY_USER_NAME);
            if (userName != null) {
                mUserNameField.setText(userName);
            }
            String password = savedInstanceState.getString(KEY_PASSWORD);
            if (password != null) {
                mPasswordField.setText(password);
            }
            String confirmPassword = savedInstanceState
                    .getString(KEY_CONFIRM_PASSWORD);
            if (confirmPassword != null) {
                mConfirmPasswordField.setText(confirmPassword);
            }
            String email = savedInstanceState.getString(KEY_EMAIL);
            if (email != null) {
                mEmailField.setText(email);
            }
            String regKey = savedInstanceState.getString(KEY_REG_KEY);
            if (regKey != null) {
                mRegKeyField.setText(regKey);
            }
        }
    }
    
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        
        outState.putString(KEY_USER_NAME, mUserNameField.getText().toString());
        outState.putString(KEY_PASSWORD, mPasswordField.getText().toString());
        outState.putString(KEY_CONFIRM_PASSWORD, mConfirmPasswordField
                .getText().toString());
        outState.putString(KEY_EMAIL, mEmailField.getText().toString());
        outState.putString(KEY_REG_KEY, mRegKeyField.getText().toString());
    }
    
    private class CreateAccountTask extends AsyncTask<String, Void, Integer> {
        
        @Override
        protected Integer doInBackground(String... params) {

            String userName = params[0].trim();
            String password = params[1].trim();
            String confirmpassword = params[2].trim();
            String email = params[3].trim();
            String regKey = params[4].trim();
            
            int result = mBoundService.createAccount(userName, password, confirmpassword, email,
                    regKey);
            
            if (result == 0) {
                // This is no longer a first time user.
                Utils.cacheFirstTimeUser(false, CreateAccountActivity.this);
            }
            
            return result;
        }
        
        @Override
        protected void onPostExecute(Integer result) {

            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(CreateAccountActivity.this, R.string.Msg_CreatedAccount,
                        Toast.LENGTH_SHORT).show();

                // Continue on to the login task.
                String username = mUserNameField.getText().toString();
                String password = mPasswordField.getText().toString();
                mProgressDialog = ProgressDialog.show(
                        CreateAccountActivity.this, "",
                        getText(R.string.Msg_LoggingIn), true);
                new LoginTask().execute(username, password);
            }
            else {
                Bundle bundle = new Bundle(1);
                bundle.putInt(Constants.BUNDLE_ID_ERROR_CODE, result);
                showDialog(Constants.DIALOG_CREATE_ACCOUNT_FAILED, bundle);
            }
        }
    }
    
    private class LoginTask extends AsyncTask<String, Void, Integer> {
        
        @Override
        protected Integer doInBackground(String... params) {

            String userName = params[0];
            String password = params[1];
            int result = mBoundService.doLogin(userName, password);
            
            if (result == 0) {
                // Cache the user name in shared preferences.
                Utils.cacheUserName(userName, CreateAccountActivity.this);
            }
            
            return result;
        }
        
        @Override
        protected void onPostExecute(Integer result) {
            
            if (mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            if (result == 0) {
                Toast.makeText(CreateAccountActivity.this,
                        R.string.Msg_LoggedIn, Toast.LENGTH_SHORT).show();
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
    
    public void onClick_CreateAccount(View view) {
        
        String userName = mUserNameField.getText().toString();
        String password = mPasswordField.getText().toString();
        String confirmPassword = mConfirmPasswordField.getText().toString();
        String email = mEmailField.getText().toString();
        String regKey = mRegKeyField.getText().toString();
        
        // Make sure the user name field is not empty.
        if (userName.equals("")) {
            // Show toast to the user and then ignore this click.
            Toast.makeText(CreateAccountActivity.this,
                    R.string.Msg_NeedUserName, Toast.LENGTH_LONG).show();
            return;
        }
        
        // Make sure that the user name has the correct length.
        if (userName.length() > Constants.MAX_USER_NAME_LENGTH) {
            // Show toast to the user and then ignore this click.
            Toast.makeText(
                    CreateAccountActivity.this,
                    String.format(this
                            .getString(R.string.Msg_UserNameWrongLength),
                            Constants.MAX_USER_NAME_LENGTH),
                    Toast.LENGTH_LONG).show();
            return;
        }
        
        // Make sure that the password field is not empty.
        if (password.equals("")) {
            // Show toast to the user and then ignore this click.
            Toast.makeText(CreateAccountActivity.this,
                    R.string.Msg_NeedPassword, Toast.LENGTH_LONG).show();
            return;
        }
        
        // Make sure that the password has the correct length.
        if (password.length() < Constants.MIN_PASSWORD_LENGTH
                || password.length() > Constants.MAX_PASSWORD_LENGTH) {
            // Show toast to the user and then ignore this click.
            Toast.makeText(
                    CreateAccountActivity.this,
                    String.format(this
                            .getString(R.string.Msg_PasswordWrongLength),
                            Constants.MIN_PASSWORD_LENGTH,
                            Constants.MAX_PASSWORD_LENGTH),
                    Toast.LENGTH_LONG).show();
            return;
        }
        
        // Check to see if the two password fields match.
        if (!password.equals(confirmPassword)) {
            // Show toast to the user and then ignore this click.
            Toast.makeText(CreateAccountActivity.this, R.string.Msg_PasswordMismatch,
                    Toast.LENGTH_LONG).show();
            return;
        }
        
        // Make sure that the email field is not empty.
        if (email.equals("")) {
            // Show toast to the user and then ignore this click.
            Toast.makeText(CreateAccountActivity.this,
                    R.string.Msg_NeedEmail, Toast.LENGTH_LONG).show();
            return;
        }
        
        mProgressDialog = ProgressDialog.show(this, "",
                getText(R.string.Msg_CreatingAccount), true);
        new CreateAccountTask().execute(userName, password, confirmPassword, email, regKey);
    }
    
    @Override
    protected void onPrepareDialog(int id, Dialog dialog, Bundle args) {
        
        switch (id) {
        case Constants.DIALOG_CREATE_ACCOUNT_FAILED:
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
        case Constants.DIALOG_CREATE_ACCOUNT_FAILED: 
            // This seems to be required to be able to set a message later. The
            // real message will be set each time onPrepareDialog() is called.
            builder.setMessage("")
                    .setTitle(getText(R.string.Label_CreateAccountFailed))
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
