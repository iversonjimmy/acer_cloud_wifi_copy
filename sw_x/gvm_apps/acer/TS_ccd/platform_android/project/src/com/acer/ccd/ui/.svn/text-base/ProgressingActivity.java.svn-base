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

import com.acer.ccd.R;
import com.acer.ccd.serviceclient.CcdiClient;
import com.acer.ccd.util.InternalDefines;
import com.acer.ccd.util.Utility;
import com.acer.ccd.util.igware.Constants;
import com.acer.ccd.util.igware.Dataset;
import com.acer.ccd.util.igware.Utils;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.TextView;

public class ProgressingActivity extends Activity {

    private static final String TAG = "ProgressingActivity";

    //private Button mButtonCancel;

    private String mLastname, mFirstname, mEmail, mPassword;
    private TextView mTextMessage;
    private int mProgressType;
    private ProgressingThread mThread = null;
    protected CcdiClient mBoundService = new CcdiClient(ProgressingActivity.this);

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.progressing_activity);

        findViews();
        setEvents();

        mBoundService.onCreate();

        setResult(InternalDefines.PROGRESSING_RESULT_CANCEL);

        mProgressType = getIntent().getIntExtra(InternalDefines.BUNDLE_PROGRESS_TYPE,
                InternalDefines.PROGRESSING_TYPE_INVALID);

        if (mProgressType == InternalDefines.PROGRESSING_TYPE_INVALID) {
            finish();
        }

        if (mProgressType == InternalDefines.PROGRESSING_TYPE_SIGNUP) {
            mTextMessage.setText(R.string.processing_label_processing_signup);
            mFirstname = getIntent().getStringExtra(InternalDefines.ACCOUNT_BUNDLE_FIRSTNAME);
            mLastname = getIntent().getStringExtra(InternalDefines.ACCOUNT_BUNDLE_LASTNAME);
            mEmail = getIntent().getStringExtra(InternalDefines.ACCOUNT_BUNDLE_EMAIL);
            mPassword = getIntent().getStringExtra(InternalDefines.ACCOUNT_BUNDLE_PASSWORD);
        } else if (mProgressType == InternalDefines.PROGRESSING_TYPE_SIGNIN) {
            mTextMessage.setText(R.string.processing_label_processing_signin);
            mEmail = getIntent().getStringExtra(InternalDefines.ACCOUNT_BUNDLE_EMAIL);
            mPassword = getIntent().getStringExtra(InternalDefines.ACCOUNT_BUNDLE_PASSWORD);
        }

        mThread = new ProgressingThread();
        mThread.start();
    }

    @Override
    public void onStart() {
        super.onStart();
        mBoundService.onStart();
    }

    @Override
    public void onResume() {
        super.onResume();
        mBoundService.onResume();
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

    // Catch back key.
    @Override
    public void onBackPressed() {
        // super.onBackPressed();
    }

    private void setEvents() {
        // mButtonCancel.setOnClickListener(buttonCancelListener);
    }

    private void findViews() {
        // mButtonCancel = (Button) findViewById(R.id.progressing_button_cancel);
        mTextMessage = (TextView) findViewById(R.id.progressing_label_message);
    }

    Handler mResultHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case InternalDefines.PROGRESSING_RESULT_CANCEL:
                    break;
                case InternalDefines.PROGRESSING_RESULT_UNKNOWN:
                case InternalDefines.PROGRESSING_RESULT_ACCOUNT_ERROR:
                case InternalDefines.PROGRESSING_RESULT_PW_LENGTH:
                case InternalDefines.PROGRESSING_RESULT_EMAIL_INVALID:
                case InternalDefines.PROGRESSING_RESULT_DUPLICATE:
                case InternalDefines.PROGRESSING_RESULT_ID_LENGTH:
                    setResult(msg.what);
                    finish();
                    break;
                case InternalDefines.PROGRESSING_RESULT_SUCCESS:
                    if (mProgressType == InternalDefines.PROGRESSING_TYPE_SIGNUP) {
                        Intent intent = new Intent(ProgressingActivity.this, FinishActivity.class);
                        startActivity(intent);
                    } else {
                        setResult(msg.what);
                        finish();
                    }
                    break;
                default:

            }
            destroyThread();
        }
    };

    private void destroyThread() {
        if (mThread != null) {
            mThread.interrupt();
            mThread = null;
        }
    }

    private class ProgressingThread extends Thread {

        @Override
        public void run() {

            int processResult = InternalDefines.PROGRESSING_RESULT_CANCEL;
            Log.i(TAG, "mProgressType = " + mProgressType);
            if (mProgressType == InternalDefines.PROGRESSING_TYPE_SIGNUP)
                processResult = doSignUp();
            if (mProgressType == InternalDefines.PROGRESSING_TYPE_SIGNIN)
                processResult = doSignIn();

            mResultHandler.sendEmptyMessage(processResult);
        }

        private int doSignUp() {
            // TODO : correct user name to email
            if (this.isInterrupted())
                return InternalDefines.PROGRESSING_RESULT_CANCEL;

            int result = mBoundService.createAccount(mFirstname, mPassword, mPassword, mEmail, "");
            Log.i(TAG, "createAccount result = " + result);
            if (result < 0) {
                Log.i(TAG, "Failed to create account : "
                                + Utils.errorCodeToMessage(result, ProgressingActivity.this));
                return InternalDefines.PROGRESSING_RESULT_DUPLICATE;
            } else if (result == Constants.ERR_CODE_REGISTER_EMAIL) {
                return InternalDefines.PROGRESSING_RESULT_EMAIL_INVALID;
            } else if (result == 500) {
                return InternalDefines.PROGRESSING_RESULT_DUPLICATE;
            } else if (result == Constants.ERR_CODE_REGISTER_USERNAME) {
                return InternalDefines.PROGRESSING_RESULT_ID_LENGTH;
            } else if (result == Constants.ERR_CODE_REGISTER_PASSWORD) {
                return InternalDefines.PROGRESSING_RESULT_PW_LENGTH;
            }

            return InternalDefines.PROGRESSING_RESULT_SUCCESS;

        }

        private int doSignIn() {
            if (this.isInterrupted())
                return InternalDefines.PROGRESSING_RESULT_CANCEL;
            Long a = 0L, b = 0L, c = 0L, d = 0L, e = 0L;
            a = System.currentTimeMillis();
            if (!mBoundService.isLoggedIn()) {
                b = System.currentTimeMillis();
                int result = mBoundService.doLogin(mEmail, mPassword);
                c = System.currentTimeMillis();
                Log.i(TAG, "doLogin result = " + result);
                if (result < 0) {
                    Log.e(TAG, "Failed to log in : "
                                    + Utils.errorCodeToMessage(result, ProgressingActivity.this));
                    return InternalDefines.PROGRESSING_RESULT_ACCOUNT_ERROR;
                }
            }

            if (this.isInterrupted())
                return InternalDefines.PROGRESSING_RESULT_CANCEL;

            int result = mBoundService.linkDevice(Utility.getDeviceName());
            d = System.currentTimeMillis();
            Log.i(TAG, "linkDevice(" + Utility.getDeviceName() + ") result = " + result);

//            subscribeClearfi();
            e = System.currentTimeMillis();
            Log.e(TAG, "KKKKK isLoggedIn()=" + (b - a) + " doLogin()=" + (c - b) + " linkDevice()=" + (d - c) + " subscribeClearfi()=" + (e - d));
            
            return InternalDefines.PROGRESSING_RESULT_SUCCESS;
        }

        private void subscribeClearfi() {
            Long a = 0L, b = 0L, c = 0L, d = 0L, e = 0L, f = 0L;
            a = System.currentTimeMillis();
            Dataset[] syncFoldersArray = mBoundService.listOwnedDataSets();
            b = System.currentTimeMillis();
            for (Dataset item : syncFoldersArray) {
                Log.i(TAG, "item.getName() = " + item.getName());
                if (item.getName().equals(InternalDefines.SUBSCRIPTION_FOLDER_CLEARFI)) {
                    Log.i(TAG, item.getName() + " subscribe()");
                    c = System.currentTimeMillis();
                    mBoundService.subscribeDataset(item);
                    d = System.currentTimeMillis();
                }
            }
            e = System.currentTimeMillis();
            mBoundService.savechange();
            f = System.currentTimeMillis();
            Log.e(TAG, "KKKKK listOwnedDataSets()=" + (b - a) + " subscribeDataset()=" + (d - c) + " savechange()=" + (f - e));
        }

    }

//    private Button.OnClickListener buttonCancelListener = new Button.OnClickListener() {
//        @Override
//        public void onClick(View arg0) {
//            destroyThread();
//            setResult(Defines.PROGRESSING_RESULT_CANCEL);
//            int result = mBoundService.doLogin(mEmail, mPassword);
//            Log.i(TAG, "doLogin result = " + result);
//            finish();
//        }
//    };
}
