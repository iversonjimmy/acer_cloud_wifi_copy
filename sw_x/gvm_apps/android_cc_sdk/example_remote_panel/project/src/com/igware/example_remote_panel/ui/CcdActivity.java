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
import android.app.Dialog;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

import com.igware.example_remote_panel.CcdiClientForAcpanel;
import com.igware.example_remote_panel.util.Utils;

abstract public class CcdActivity extends Activity {
    
    protected CcdiClientForAcpanel mBoundService = new CcdiClientForAcpanel(this);
    protected CcdActivityDelegate mDelegate = new CcdActivityDelegate(this, mBoundService);
    
    private static final String LOG_TAG = "CcdActivity";
    
    // ------------------------------------------------------
    // Android Activity life-cycle.
    // ------------------------------------------------------
    
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        Log.i(LOG_TAG, "onCreate: " + this);
        mDelegate.onCreate();
    }
    
    /** The activity is about to become visible. */
    @Override
    protected void onStart() {

        super.onStart();
        Log.i(LOG_TAG, "onStart: " + this);
        mDelegate.onStart();
    }
    
    /** The activity has become visible (it is now "resumed"). */
    @Override
    protected void onResume() {

        super.onResume();
        Log.i(LOG_TAG, "onResume: " + this);
        mDelegate.onResume();
    }
    
    /**
     * Another activity is taking focus (this activity is about to be "paused").
     */
    @Override
    protected void onPause() {

        super.onPause();
        Log.i(LOG_TAG, "onPause: " + this);
        mDelegate.onPause();
    }
    
    /** The activity is no longer visible (it is now "stopped"). */
    @Override
    protected void onStop() {

        super.onStop();
        Log.i(LOG_TAG, "onStop: " + this);
        mDelegate.onStop();
    }
    
    /** The activity is about to be destroyed. */
    @Override
    protected void onDestroy() {

        super.onDestroy();
        Log.i(LOG_TAG, "onDestroy: " + this);
        mDelegate.onDestroy();
    }
    
    // ------------------------------------------------------
    // Options menu.
    // ------------------------------------------------------
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        
        mDelegate.onCreateOptionsMenu(menu);
        
        // MUST call through to super.
        return super.onCreateOptionsMenu(menu);
    }
    
    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        
        mDelegate.onPrepareOptionsMenu(menu);
        
        // MUST call through to super.
        return super.onPrepareOptionsMenu(menu);
    }
    
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        
        boolean handled = mDelegate.onOptionsItemSelected(item);
        if (!handled) {
            handled = super.onOptionsItemSelected(item);
        }
        return handled;
    }
       
    @Override
    protected void onPrepareDialog(int id, Dialog dialog, Bundle args) {
        
        mDelegate.onPrepareDialog(id, dialog, args);
        
        // MUST call through to super.
        super.onPrepareDialog(id, dialog, args);
    }
    
    @Override
    protected Dialog onCreateDialog(int id, Bundle args) {
        
        Dialog result = mDelegate.onCreateDialog(id, args);
        if (result == null) {
            result = super.onCreateDialog(id, args);
        }
        return result;
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        
        mDelegate.onActivityResult(requestCode, resultCode, data);
        
        super.onActivityResult(requestCode, resultCode, data);
    }
    
    // ------------------------------------------------------
    // Convert error code to error message.
    // ------------------------------------------------------
    
    protected String errorCodeToMessage(int errorCode) {

        return Utils.errorCodeToMessage(errorCode, this);
    }
}
