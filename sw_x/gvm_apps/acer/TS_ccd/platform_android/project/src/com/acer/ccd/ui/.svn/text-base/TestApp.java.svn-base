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
import android.accounts.AccountManager;
import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.acer.ccd.R;
import com.acer.ccd.provider.CameraRollStreamProvider;
import com.acer.ccd.provider.FavoriteProvider;
import com.acer.ccd.provider.NotebookProvider;
import com.acer.ccd.serviceclient.CcdiClient;
import com.acer.ccd.util.CcdSdkDefines;
import com.acer.ccd.util.InternalDefines;

public class TestApp extends Activity {

    private TextView textFirstname, textLastname, textUsername, textCountry;
    private TextView textSyncNotebook, textSyncFavorite, textSyncContact, textSyncCalendar, textSyncCameraRollStream;
    private TextView textSyncStatus;
    private Button btnGetSyncStatus, btnResumeSync;

    private final static String TAG = "TestApp";
    
    protected CcdiClient mBoundService = new CcdiClient(TestApp.this);

    private String mAccessToken;
    private String mUsername;
    private String mPassword;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.main);
        findViews();

        loadAccountInfo();
        loadSyncSetting();
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

    private void loadSyncSetting() {
        AccountManager accountManager = AccountManager.get(this);
        Account[] accounts = accountManager.getAccountsByType(CcdSdkDefines.ACCOUNT_TYPE);
        if (accounts.length == 0) {
            textFirstname.setText("no account exists!");
            return;
        }
        
        Account account = accounts[0];
        
        textSyncNotebook.setText("Notebook sync is off");
        if (ContentResolver.getSyncAutomatically(account, NotebookProvider.AUTHORITY))
            textSyncNotebook.setText("Notebook sync is on");
        
        textSyncFavorite.setText("Favorite sync is off");
        if (ContentResolver.getSyncAutomatically(account, FavoriteProvider.AUTHORITY))
            textSyncFavorite.setText("Favorite sync is on");
        
        textSyncContact.setText("Contacts sync is off");
        if (ContentResolver.getSyncAutomatically(account, "com.android.contacts"))
            textSyncContact.setText("Contacts sync is on");
        
        textSyncCalendar.setText("Calendar sync is off");
        if (ContentResolver.getSyncAutomatically(account, "com.android.calendar"))
            textSyncCalendar.setText("Calendar sync is on");
        
        textSyncCameraRollStream.setText("Camera Roll Stream sync is off");
        if (ContentResolver.getSyncAutomatically(account, CameraRollStreamProvider.AUTHORITY))
            textSyncCameraRollStream.setText("Camera Roll Stream sync is on");
    }

    private void loadAccountInfo() {
        // TODO Auto-generated method stub
        AccountManager am = AccountManager.get(this);
        Account[] accounts = am.getAccountsByType(CcdSdkDefines.ACCOUNT_TYPE);
        if (accounts.length == 0)
            return;

        String firstname = am.getUserData(accounts[0], InternalDefines.ACCOUNT_USERDATA_FIRSTNAME);
        String lastname = am.getUserData(accounts[0], InternalDefines.ACCOUNT_USERDATA_LASTNAME);
        String country = am.getUserData(accounts[0], InternalDefines.ACCOUNT_USERDATA_COUNTRY);
        Log.i("TestApp", "firstname = " + firstname);
        Log.i("TestApp", "textFirstname = " + textFirstname);

        if (firstname != null)
            textFirstname.setText(firstname);
        if (lastname != null)
            textLastname.setText(lastname);
        textUsername.setText(accounts[0].name);
        if (country != null)
            textCountry.setText(country);
        
        
//        AccountManagerCallback<Bundle> callback = new AccountManagerCallback<Bundle>() {
//
//            @Override
//            public void run(AccountManagerFuture<Bundle> accountManagerFuture) {
//                Log.d(TAG, "getAuthToken complete:" + accountManagerFuture);
//                try {
//                    Bundle result = accountManagerFuture.getResult();
//                    Log.d(TAG, "account name:" + result.getString(AccountManager.KEY_ACCOUNT_NAME));
//                    Log.d(TAG, "account type:" + result.getString(AccountManager.KEY_ACCOUNT_TYPE));
//                    Log.d(TAG, "account password:" + result.getString(AccountManager.KEY_PASSWORD));
//                    Log.d(TAG, "auth token:" + result.getString(AccountManager.KEY_AUTHTOKEN));
//                    mAccessToken = result.getString(AccountManager.KEY_AUTHTOKEN);
//                    mPassword = result.getString(AccountManager.KEY_AUTHTOKEN);
//                } catch (OperationCanceledException e) {
//                    e.printStackTrace();
//                } catch (AuthenticatorException e) {
//                    e.printStackTrace();
//                } catch (IOException e) {
//                    e.printStackTrace();
//                }
//            }
//        };
//        
//        am.getAuthToken(accounts[0], "com.acer.TS.authToken", null, this, callback, null);
        
//        try {
//            Bundle result = am.getAuthToken(accounts[0], "com.acer.TS.authToken", null, this, null, null).getResult();
//            Log.d(TAG, "account name:" + result.getString(AccountManager.KEY_ACCOUNT_NAME));
//            Log.d(TAG, "account type:" + result.getString(AccountManager.KEY_ACCOUNT_TYPE));
//            Log.d(TAG, "account password:" + result.getString(AccountManager.KEY_PASSWORD));
//            Log.d(TAG, "auth token:" + result.getString(AccountManager.KEY_AUTHTOKEN));
//        } catch (OperationCanceledException e) {
//            // TODO Auto-generated catch block
//            e.printStackTrace();
//        } catch (AuthenticatorException e) {
//            // TODO Auto-generated catch block
//            e.printStackTrace();
//        } catch (IOException e) {
//            // TODO Auto-generated catch block
//            e.printStackTrace();
//        }

    }

    private void findViews() {
        // TODO Auto-generated method stub
        textFirstname = (TextView) findViewById(R.id.textview1);
        textLastname = (TextView) findViewById(R.id.textview2);
        textUsername = (TextView) findViewById(R.id.textview3);
        textCountry = (TextView) findViewById(R.id.textview4);
        textSyncNotebook = (TextView) findViewById(R.id.textview5);
        textSyncFavorite = (TextView) findViewById(R.id.textview6);
        textSyncContact = (TextView) findViewById(R.id.textview7);
        textSyncCalendar = (TextView) findViewById(R.id.textview8);
        textSyncCameraRollStream = (TextView) findViewById(R.id.textview9);
        textFirstname.setOnClickListener(clickListener);
        textLastname.setOnClickListener(click2Listener);
        textSyncNotebook.setOnClickListener(logoutListener);
        
        
        textSyncStatus = (TextView) findViewById(R.id.textSyncStatus);
        btnGetSyncStatus = (Button) findViewById(R.id.btnGetSyncStatus);
        btnResumeSync = (Button) findViewById(R.id.btnResumeSync);
        btnGetSyncStatus.setOnClickListener(getStatusListener);
        btnResumeSync.setOnClickListener(resumeSyncClickListener);
    }

    private TextView.OnClickListener clickListener = new TextView.OnClickListener() {

        @Override
        public void onClick(View arg0) {
            // TODO Auto-generated method stub
            startActivity(new Intent(Settings.ACTION_SYNC_SETTINGS));
        }
        
    };
    
    private TextView.OnClickListener click2Listener = new TextView.OnClickListener() {

        @Override
        public void onClick(View arg0) {
            // TODO Auto-generated method stub
//            long userid = mBoundService.getUserId();
//            Log.i(TAG, "mBoundService.getUserId( = " + userid);
//
//            long id = mBoundService.getCameraSyncUploadFolderId();
//            Log.i(TAG, "mBoundService.getCameraSyncUploadFolderId() = " + id);
//            
//            int result = mBoundService.addCameraSyncUploadSubscription();
//            Log.i(TAG, "addCameraSyncUploadSubscription() result = " + result);
//            int result = mBoundService.addCameraSyncUploadSubscription();
//            Log.i(TAG, "addCameraSyncUploadSubscription result = " + result);
            
            mBoundService.isLoggedIn();
            mBoundService.isDeviceLinked();
            mBoundService.resumeSync();
            
            String userName = mBoundService.getUserName();
            String deviceName = mBoundService.getDeviceName();
            int[] syncStatus = mBoundService.getSyncStatus();
            long[] quotaStatus = mBoundService.getQuotaStatus();
            Log.i(TAG, "userName = " + userName);
            Log.i(TAG, "deviceName = " + deviceName);
            Log.i(TAG, "syncStatus = " + syncStatus);
            for (int state: syncStatus)
                Log.i(TAG, "syncStatus => " + state);
            Log.i(TAG, "quotaStatus" + quotaStatus);
            for (long state: quotaStatus)
                Log.i(TAG, "quotaState" + state);
        }
        
    };
                    
    private TextView.OnClickListener logoutListener = new TextView.OnClickListener() {

        @Override
        public void onClick(View arg0) {
            if (mBoundService.isLoggedIn()) {
                Log.i(TAG, "mBoundService.unlinkDevice()");
                mBoundService.unlinkDevice();
                Log.i(TAG, "mBoundService.isLoggedIn()");
                mBoundService.doLogout();
            }
        }
        
    };
    
    private Button.OnClickListener getStatusListener = new Button.OnClickListener() {
        @Override
        public void onClick(View arg0) {
            if (!mBoundService.isLoggedIn()) {
                textSyncStatus.setText("isLoggedIn() = false");
                return;
            }
            String msg = "Status = ";
            int[] status = mBoundService.getSyncStatus();
            for (int i: status)
                msg += String.format(", %d", i);
            
            textSyncStatus.setText(msg);
                
        }
    };
    
    private Button.OnClickListener resumeSyncClickListener = new Button.OnClickListener() {
        @Override
        public void onClick(View arg0) {
            if (!mBoundService.isLoggedIn()) {
                textSyncStatus.setText("isLoggedIn() = false");
                return;
            }
            mBoundService.resumeSync();
        }
    };
    
}
