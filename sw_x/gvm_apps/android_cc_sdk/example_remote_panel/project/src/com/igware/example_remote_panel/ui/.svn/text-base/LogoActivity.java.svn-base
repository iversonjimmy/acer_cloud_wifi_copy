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

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import com.igware.android_cc_sdk.example_remote_panel.R;
import com.igware.example_remote_panel.util.Constants;
import com.igware.example_remote_panel.util.Utils;


public class LogoActivity extends CcdActivity {
    
    private static final String LOG_TAG = "LogoActivity";
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        
        super.onCreate(savedInstanceState);
        setContentView(R.layout.logo);
    }
    
    public void onClick_OldUser(View view) {
        
        // This is not a first time user.
        Utils.cacheFirstTimeUser(false, this);
        setResult(Constants.RESULT_BACK_TO_MAIN);
        finish();  
    }
    
    public void onClick_NewUser(View view) {
        
        Log.i(LOG_TAG, "Launching create new account activity ("
                + Constants.ACTIVITY_CREATE_NEW_ACCT + ")");
        Intent createAccountIntent = new Intent(LogoActivity.this,
                CreateAccountActivity.class);
        startActivityForResult(createAccountIntent,
                Constants.ACTIVITY_CREATE_NEW_ACCT);
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        
        Log.i(LOG_TAG, "Result from request " + requestCode + ": " + resultCode);
        
        switch (requestCode) {
        case Constants.ACTIVITY_CREATE_NEW_ACCT:
            if (resultCode == RESULT_CANCELED) {
                // The child activity canceled out. Do nothing.
            }
            else {
                if (resultCode != RESULT_OK
                        && resultCode != Constants.RESULT_RESTART
                        && resultCode != Constants.RESULT_EXIT
                        && resultCode != Constants.RESULT_BACK_TO_MAIN) {
                    Log.e(LOG_TAG, "Encountered unknown result code: " + resultCode);
                }
                // Pass the result back up to the parent activity.
                setResult(resultCode, data);
                finish();
            }
            break;
            
        default:
            super.onActivityResult(requestCode, resultCode, data);
        }
    }
}
