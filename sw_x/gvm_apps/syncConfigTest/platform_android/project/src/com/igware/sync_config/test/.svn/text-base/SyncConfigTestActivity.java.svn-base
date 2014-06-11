package com.igware.sync_config.test;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import java.util.ArrayList;

public class SyncConfigTestActivity extends Activity {
    static final String TAG = "SyncConfigTest"; 
    static final String END_MSG = "SyncConfig TEST COMPLETED";
    private String account;
    private String password;
    private String domain;
    private String port = "443";
    private boolean doLargeTest = false;
    
    @Override
    protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        
        String extra = intent.getStringExtra("account"); 
        if( extra != null ) {
            account = extra;
        }else{
            Log.e(TAG, "account not found!");
            finish();
        }
        
        extra = intent.getStringExtra("password"); 
        if( extra != null ) {
            password = extra;
        }else{
            Log.e(TAG, "password not found!");
            finish();
        }
        
        extra = intent.getStringExtra("domain");
        if( extra != null ) {
            domain = extra;
        }else{
            Log.e(TAG, "running domain is not set!");
            finish();
        }
        
        extra = intent.getStringExtra("port");
        if( extra != null ) {
            port = extra;
        }

        extra = intent.getStringExtra("do_large_test");
        if( extra != null && extra.equals("true")) {
            doLargeTest = true;
        }
    }

    @Override
    protected void onStart(){
        super.onStart();
        runTest();
        finish();
    }
 
    @Override
    protected void onDestroy(){
        Log.d(TAG, END_MSG);
        super.onDestroy();
    }
    
    private void runTest(){
        // emulate ./syncConfigTest sc_acct_test_SET_YOUR_OWN_ACCT@igware.com password pc-int.igware.net 443
        ArrayList<String> argv = new ArrayList();
        argv.add("syncConfigTest");
        argv.add(account);
        argv.add(password);
        argv.add(domain);
        argv.add(port);
        if( doLargeTest ) {
            argv.add("--do_large_test");
        }
        SyncConfigNative.testSyncConfig(argv.size(),
                                        argv.toArray(new String [argv.size()]));
    }
}
