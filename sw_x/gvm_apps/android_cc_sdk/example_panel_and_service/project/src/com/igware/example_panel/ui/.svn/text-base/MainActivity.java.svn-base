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

import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;

import com.igware.example_panel.util.Constants;
import com.igware.example_panel.util.Utils;
import com.igware.android_cc_sdk.example_panel_and_service.R;


public class MainActivity extends CcdActivity {
    
    private static final String LOG_TAG = "MainActivity";
    
    private volatile Boolean mWaitingForResult = Boolean.FALSE;
    
    @Override
    public void onCreate(Bundle savedInstanceState) {

        // May be useful during development:
        // android.os.StrictMode.setThreadPolicy(new
        // StrictMode.ThreadPolicy.Builder().detectAll().build());
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
    }
    
    @Override
    protected void onResume() {

        super.onResume();
        new StartActivityTask().execute((Void[]) null);
    }
    
    private class StartActivityTask extends AsyncTask<Void, Void, Integer> {
        
        @Override
        protected Integer doInBackground(Void... params) {

            Log.i(LOG_TAG, "Begin StartActivityTask!");
            synchronized (mWaitingForResult) {
                if (mWaitingForResult) {
                    // Ignore duplicate request.
                    Log.i(LOG_TAG, "Ignoring duplicate request.");
                    return Constants.ACTIVITY_NONE;
                }
                else {
                    mWaitingForResult = Boolean.TRUE;
                }
            }
            
            Log.i(LOG_TAG, "Determining which activity to start.");
            
            // Look up the user name cached in shared preferences.
            String userName = Utils.getCachedUserName(MainActivity.this);
            if (userName.equals("")) {
                // No user name found in shared preferences.
                Log.i(LOG_TAG, "No cached user name found.");
            }
            else {
                // Attempt to automatically log in.
                Log.i(LOG_TAG, "Attempting to log in with cached user name: "
                        + userName);
                mBoundService.doLogin(userName, "");
            }
            
            boolean isLoggedIn = mBoundService.isLoggedIn();
            
            if (Utils.isFirstTimeUser(MainActivity.this) && !isLoggedIn) {
                return Constants.ACTIVITY_LOGO;
            }
            
            if (!isLoggedIn) {
                return Constants.ACTIVITY_LOGIN;
            }
            
            if (!mBoundService.isDeviceLinked()) {
                return Constants.ACTIVITY_LINK_DEVICE;
            }
            
            return Constants.ACTIVITY_STATUS;
        }
        
        @Override
        protected void onPostExecute(Integer result) {
            
            switch (result) {
            case Constants.ACTIVITY_NONE:
                break;
            
            case Constants.ACTIVITY_LOGO:
                Log.i(LOG_TAG,
                        "Device not registered. Launching logo activity ("
                                + result + ")");
                Intent logoIntent = new Intent(MainActivity.this,
                        LogoActivity.class);
                startActivityForResult(logoIntent, result);
                break;
            
            case Constants.ACTIVITY_LOGIN:
                Log.i(LOG_TAG, "User not logged in. Launching login activity ("
                        + result + ")");
                Intent loginIntent = new Intent(MainActivity.this,
                        LoginActivity.class);
                startActivityForResult(loginIntent, result);
                break;
            
            case Constants.ACTIVITY_LINK_DEVICE:
                Log.i(LOG_TAG,
                        "Device not linked. Launching link device activity ("
                                + result + ")");
                Intent linkDeviceIntent = new Intent(MainActivity.this,
                        LinkDeviceActivity.class);
                startActivityForResult(linkDeviceIntent, result);
                break;
            
            case Constants.ACTIVITY_STATUS:
                Log.i(LOG_TAG,
                        "User is logged in, and device is linked. Launching status activity ("
                                + result + ")");
                Intent statusIntent = new Intent(MainActivity.this,
                        StatusActivity.class);
                startActivityForResult(statusIntent, result);
                break;
            
            default:
                break;
            }
        }
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        
        Log.i(LOG_TAG, "Result from request " + requestCode + ": " + resultCode);      
        
        switch (requestCode) {
        case Constants.ACTIVITY_LOGO:
        case Constants.ACTIVITY_LOGIN:
        case Constants.ACTIVITY_LINK_DEVICE:
        case Constants.ACTIVITY_STATUS:
            if (resultCode == RESULT_CANCELED) {
                // The child activity canceled out. Exit the app.
                setResult(RESULT_CANCELED);
                finish();
            }
            else if (resultCode == Constants.RESULT_EXIT) {
                // The child activity has asked to exit the app.
                setResult(RESULT_OK);
                finish();
            }
            else if (resultCode == Constants.RESULT_RESTART) {
                // The child activity has asked for a forced restart of the app.
                System.exit(2);
            }
            else {
                if (resultCode != RESULT_OK
                        && resultCode != Constants.RESULT_BACK_TO_MAIN) {
                    Log.e(LOG_TAG, "Encountered unknown result code: " + resultCode);
                }
                // The child activity has finished. Start another child activity.           
                mWaitingForResult = Boolean.FALSE;
                new StartActivityTask().execute((Void[]) null);
            }       
            break;
            
        default:
            super.onActivityResult(requestCode, resultCode, data);
        }
    }
}
