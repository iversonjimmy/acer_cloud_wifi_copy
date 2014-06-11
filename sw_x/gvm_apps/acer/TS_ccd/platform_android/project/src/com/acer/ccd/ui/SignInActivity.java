/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.ui;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import com.acer.ccd.R;
import com.acer.ccd.provider.CameraRollStreamProvider;
import com.acer.ccd.serviceclient.CcdiClient;
import com.acer.ccd.ui.cmp.ResetAccountDialog;
import com.acer.ccd.util.CcdSdkDefines;
import com.acer.ccd.util.InternalDefines;
import com.acer.ccd.util.Utility;

public class SignInActivity extends AccountAuthenticatorActivity {
    private static final String TAG = "SignInActivity";

    private static final int DIALOG_SHOW_PROGRESS = 0;
    
    private final static int RESET_REQUEST_RESULT_OK = 0x24;
    private final static int RESET_REQUEST_RESULT_NO_ACCOUNT = (RESET_REQUEST_RESULT_OK + 1);

    private EditText mEditEmail;
    private EditText mEditPassword;
    // private CheckBox mCheckboxRemember;
    private TextView mTextForgotPassword;
    private TextView mTextCreateAccount;
    private Button mButtonSignIn;
    protected CcdiClient mBoundService = new CcdiClient(SignInActivity.this);
    
    private SendResetPasswordRequestThread mSendResetPasswordRequestThread;

    @Override
    public void onCreate(Bundle stateInstance) {
        super.onCreate(stateInstance);

        setResult(RESULT_CANCELED);
        if (checkAccountTypeExists()) {
            Utility.ToastMessage(this, R.string.message_cannot_add_account);
            finish();
            return;
        }

        this.requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.sign_in_activity);
        findView();
        setEvents();
        mBoundService.onCreate();
    }
    
    @Override
    public void onStart() {
        super.onStart();
        mBoundService.onStart();
    }
    
    @Override
    public void onResume() {
        Log.i(TAG, "onResume");
        super.onResume();
        mBoundService.onResume();
        // getSavedAccount();
    }
    
    @Override
    public void onPause() {
        super.onPause();
        mBoundService.onPause();
    }
    
    @Override
    public void onStop() {
        super.onStop();
        mBoundService.onStop();
    }
    
    @Override
    public void onDestroy() {
        super.onDestroy();
        mBoundService.onDestroy();
    }
    
    @Override
    protected Dialog onCreateDialog(int id) {
        switch (id) {
            case DIALOG_SHOW_PROGRESS:
                ProgressDialog dialog = new ProgressDialog(this);
                dialog.setTitle("Get devices info");
                dialog.setMessage("Please wait a moment");
                dialog.setCancelable(false);
                return dialog;
        }
        return super.onCreateDialog(id);
    }

    private void showProgressBar() {
        showDialog(DIALOG_SHOW_PROGRESS);
    }

    private void hideProgressBar() {
        dismissDialog(DIALOG_SHOW_PROGRESS);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Log.i(TAG, "resultCode = " + resultCode);
        switch (resultCode) {
            case InternalDefines.PROGRESSING_RESULT_ACCOUNT_ERROR:
                Utility.showMessageDialog(this, R.string.signin_dialog_title_incorrect,
                        R.string.signin_dialog_content_incorrent);
                break;
            case InternalDefines.PROGRESSING_RESULT_SUCCESS:
                addAccount();
                break;
            default:
        }
    }
    
    private void setEvents() {
        if (mTextCreateAccount != null)
            mTextCreateAccount.setOnClickListener(mCreateAccountClickListener);
        mTextForgotPassword.setOnClickListener(forgotPasswordClickListener);
        mButtonSignIn.setOnClickListener(signInClickListener);
    }

    private void findView() {
        mEditEmail = (EditText) findViewById(R.id.signin_edit_username);
        mEditPassword = (EditText) findViewById(R.id.signin_edit_password);
        // mCheckboxRemember = (CheckBox) findViewById(R.id.checkbox_remember_account);
        mTextForgotPassword = (TextView) findViewById(R.id.text_link_forgot_password);
        mTextCreateAccount = (TextView) findViewById(R.id.text_link_sign_up);
        mButtonSignIn = (Button) findViewById(R.id.button_sign_in);
    }
    
//    private void getSavedAccount() {
//        boolean checked = Utility.getIsRememberAccount(this);
//        mCheckboxRemember.setChecked(checked);
//        if (checked) {
//            String account = Utility.getRememberedAccount(this);
//            mEditEmail.setText(account);
//        
//        } else
//            mEditEmail.setText("");
//    }
    
//    private void saveAccount () {
//        boolean checked = mCheckboxRemember.isChecked();
//        Utility.setIsRememberAccount(this, checked);
//        if (checked)
//            Utility.setRememberedAccount(this, mEditEmail.getText().toString());
//    }

    private boolean checkAccountTypeExists() {
        AccountManager accountManager = AccountManager.get(this);
        Account[] accounts = accountManager.getAccountsByType(CcdSdkDefines.ACCOUNT_TYPE);
        return accounts.length > 0;
    }

    private void addAccount() {
        Account account = new Account(mEditEmail.getText().toString(), CcdSdkDefines.ACCOUNT_TYPE);
        AccountManager am = AccountManager.get(SignInActivity.this);
        boolean accountCreated = am.addAccountExplicitly(account, mEditPassword.getText()
                .toString(), null);

        Bundle extras = getIntent().getExtras();
        Log.i(TAG, "extras = " + extras );
        if (extras != null) {
            if (accountCreated) { // Pass the new account back to the
                                  // account manager
//                AccountAuthenticatorResponse response = extras
//                        .getParcelable(AccountManager.KEY_ACCOUNT_AUTHENTICATOR_RESPONSE);
//                Bundle result = new Bundle();
//                result.putString(AccountManager.KEY_ACCOUNT_NAME, mEditEmail.getText().toString());
//                result.putString(AccountManager.KEY_ACCOUNT_TYPE, Defines.ACCOUNT_TYPE);
//                result.putString(AccountManager.KEY_PASSWORD, mEditPassword.getText().toString());
//                // TODO : put your AuthToken
//                result.putString(AccountManager.KEY_AUTHTOKEN, "testtoken faj3vjDfjakKj27GadfASdkj2j5");
//
//                response.onResult(result);
            }
            Bundle bundle = new Bundle();
            ContentResolver.addPeriodicSync(account, CameraRollStreamProvider.AUTHORITY, bundle, InternalDefines.SYNC_PERIOD_TIME);
            ContentResolver.setSyncAutomatically(account, CameraRollStreamProvider.AUTHORITY, true);
            Log.i(TAG, "Result = RESULT_OK, finish()");
            finish();
            setResult(RESULT_OK);
        }
    }

    private boolean checkFieldFilled() {
        
        if ("".equals(mEditEmail.getText().toString())) {
            Utility.ToastMessage(this, R.string.message_email_empty);
            return false;
        }
//        if (!Utility.checkEmailFormatInvalid(mEditEmail.getText().toString())) {
//            Utility.ToastMessage(this, R.string.message_email_invalid);
//            return false;
//        }
        if ("".equals(mEditPassword.getText().toString())) {
            Utility.ToastMessage(this, R.string.message_password_empty);
            return false;
        }
        return true;
    }
    
    private void destroyThread() {
        if (mSendResetPasswordRequestThread != null) {
            mSendResetPasswordRequestThread.interrupt();
            mSendResetPasswordRequestThread = null;
        }
    }

    private TextView.OnClickListener forgotPasswordClickListener = new TextView.OnClickListener() {

        @Override
        public void onClick(View arg0) {
            ResetAccountDialog dialog = new ResetAccountDialog(SignInActivity.this);
            dialog.setTitle(R.string.signin_dialog_title_reset_account);
            dialog.setButton(DialogInterface.BUTTON_POSITIVE, getText(R.string.signin_dialog_button_submit).toString(),
                    mDialogSubmitClickListener);
            dialog.show();
        }

    };

    DialogInterface.OnClickListener mDialogSubmitClickListener = new DialogInterface.OnClickListener() {
        public void onClick(DialogInterface dialog, int whichButton) {
            showProgressBar();
            mSendResetPasswordRequestThread = new SendResetPasswordRequestThread(((ResetAccountDialog)dialog).getEditEmail().getText().toString());
            mSendResetPasswordRequestThread.start();
        }
    };

    private TextView.OnClickListener mCreateAccountClickListener = new TextView.OnClickListener() {

        @Override
        public void onClick(View arg0) {
            if (!Utility.isAcerDevice()) {
                Utility.showMessageDialog(SignInActivity.this, R.string.signin_dialog_title_nonacer_limitation, R.string.signin_dialog_content_nonacer_limitation);
                return;
            }
            Intent intent = new Intent(SignInActivity.this, SignUpEntryActivity.class);
            startActivityForResult(intent, 0);
        }

    };

    private Button.OnClickListener signInClickListener = new Button.OnClickListener() {

        @Override
        public void onClick(View arg0) {
            if (!checkFieldFilled()) {
                return;
            }
            
            // saveAccount();

            Intent intent = new Intent(SignInActivity.this, ProgressingActivity.class);
            intent.putExtra(InternalDefines.BUNDLE_PROGRESS_TYPE, InternalDefines.PROGRESSING_TYPE_SIGNIN);
            intent.putExtra(InternalDefines.ACCOUNT_BUNDLE_EMAIL, mEditEmail.getText().toString());
            intent.putExtra(InternalDefines.ACCOUNT_BUNDLE_PASSWORD, mEditPassword.getText().toString());
            startActivityForResult(intent, 0);
        }
    };
    
    private Handler mResultHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case RESET_REQUEST_RESULT_OK:
                    String email = (String)msg.obj;
                    String title = getText(R.string.signin_dialog_title_reset_account).toString();
                    String content = String.format(getText(R.string.signin_dialog_content_reset_account).toString(), email);
                    Utility.showMessageDialog(SignInActivity.this, title, content);
                    break;
                case RESET_REQUEST_RESULT_NO_ACCOUNT:
                    Utility.showMessageDialog(SignInActivity.this, R.string.signin_dialog_title_reset_account, R.string.signin_dialog_content_no_account);
                    break;
            }
            destroyThread();
            hideProgressBar();
        }
    };
    
    private class SendResetPasswordRequestThread extends Thread {
        
        private String mEmail;
        
        public SendResetPasswordRequestThread (String email) {
            mEmail = email;
        }
        
        @Override
        public void run() {
            super.run();
            
            // TODO : Implement Reset your password.
            
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            
            // Simulate result
            mResultHandler.obtainMessage(RESET_REQUEST_RESULT_OK, mEmail);
        }
    }

}
