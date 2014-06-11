package com.igware.vplex.test;

import java.util.HashMap;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class VplexTestActivity extends Activity {
    static final String TAG = "VplexTest"; 
    static final String END_MSG = "VPLEX_TEST_COMPLETED";
    private String url;
    private String branch;
    private String product = "android";
    
    @Override
    protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        String extra = intent.getStringExtra("URL"); 
        if( extra != null ){
            url = extra;
        }else{
            Log.e(TAG, "URL not found!");
            finish();
        }
        
        extra = intent.getStringExtra("branch");
        if( extra != null ) {
            branch = extra;
        } else { 
            Log.e(TAG, "branch not found!");
            finish();
        }
        
        extra = intent.getStringExtra("product");
        if( extra != null )
            product = extra;
        
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
        String[] argv = {   "testVPLex",
                            "--test-server-url",
                            url,
                            "--branch",
                            branch,
                            "--product",
                            product        };        
        int argc = argv.length;
        
        VplexNative.testVPLex(argc, argv);
    }
    

}