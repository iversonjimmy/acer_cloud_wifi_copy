/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

/*
 *  LogoutActivity.java is not being used currently.
 */

package com.acer.ccd.ui;

import java.io.File;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.WindowManager;
import android.widget.TextView;

import com.acer.ccd.R;
import com.acer.ccd.serviceclient.CcdiClient;
import com.acer.ccd.util.InternalDefines;
import com.acer.ccd.util.Utility;
import com.acer.ccd.util.igware.Constants;

public class LogoutActivity extends Activity {
    
    private static final String TAG = "LogoutActivity";
    private static final int MESSAGE_FINISH_ACTIVITY = 1;
    
    private int mProcessType;
    
    private TextView mTextMessage;
    
    protected CcdiClient mBoundService = new CcdiClient(this);
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND,
                WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
        
        setContentView(R.layout.logout_activity);
        
        mTextMessage = (TextView) findViewById(R.id.logout_text_message);
        
        mProcessType = getIntent().getIntExtra(InternalDefines.BUNDLE_PROGRESS_TYPE, InternalDefines.PROGRESSING_TYPE_INVALID);
        if (mProcessType == InternalDefines.PROGRESSING_TYPE_INVALID)
            finish();
        
        switch (mProcessType) {
        case InternalDefines.ACTION_TYPE_LOGOUT:
        	mTextMessage.setText(R.string.logout_label_logout);
        	break;
        case InternalDefines.ACTION_TYPE_RELOGIN:
        	mTextMessage.setText(R.string.logout_label_login);
        }
        
        mBoundService.onCreate();
    }
    
    @Override
    public void onStart() {
        super.onStart();
        Log.i(TAG, "onStart : mBoundService.onStart()");
        mBoundService.onStart();
    }
    
    @Override
    public void onStop() {
        mBoundService.onStop();
        super.onStop();
    }
    
    @Override
    public void onResume() {
        super.onResume();
        mBoundService.onResume();
        new LogoutThread().start();
    }
    
    @Override
    public void onPause() {
        mBoundService.onPause();
        super.onPause();
    }
    
    @Override
    public void onDestroy() {
        mBoundService.onDestroy();
        super.onDestroy();
    }
    
    private Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_FINISH_ACTIVITY:
                    finish();
                    break;
            }
        }
        
    };
    
    private class LogoutThread extends Thread {

        @Override
        public void run() {
            super.run();
            if (mProcessType == InternalDefines.ACTION_TYPE_LOGOUT) {
                if (mBoundService.isLoggedIn()) {
                    mBoundService.unlinkDevice();
                    mBoundService.doLogout();
                }
                removeTSFiles();
                Log.i(TAG, "doLogout complete.");
            } else if (mProcessType == InternalDefines.ACTION_TYPE_RELOGIN) {
                // 1. check Cached ID
                // 2. check isLoggedIn()
                // 3. doLogin(ID, "")
                String id = Utility.getAccountId(LogoutActivity.this); 
                if (!id.equals(""))
                    if (!mBoundService.isLoggedIn())
                        mBoundService.doLogin(id, "");
            }
            mHandler.sendEmptyMessage(MESSAGE_FINISH_ACTIVITY);
        }
        
        private void removeTSFiles() {
            Log.i(TAG, "remove CameraRoll files and folders");
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
    }
}
