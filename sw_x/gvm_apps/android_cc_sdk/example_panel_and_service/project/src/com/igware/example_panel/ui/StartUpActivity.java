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

import java.util.List;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;

// Source: https://github.com/cleverua/android_startup_activity

public class StartUpActivity extends Activity {
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        
        if (needToStartApp()) {
            Intent intent = new Intent(StartUpActivity.this, MainActivity.class);
            startActivity(intent);
        }
        
        finish();
    }
    
    @Override
    public void onConfigurationChanged(Configuration newConfig) {

        // This prevents StartUpActivity recreation on Configuration changes
        // (device orientation changes or hardware keyboard open/close).
        // Just do nothing on these changes.
        super.onConfigurationChanged(null);
    }
    
    private boolean needToStartApp() {

        final ActivityManager am = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        final List<RunningTaskInfo> tasksInfo = am.getRunningTasks(1024);
        
        if (!tasksInfo.isEmpty()) {
            final String ourAppPackageName = getPackageName();
            RunningTaskInfo taskInfo;
            final int size = tasksInfo.size();
            for (int i = 0; i < size; i++) {
                taskInfo = tasksInfo.get(i);
                if (ourAppPackageName.equals(taskInfo.baseActivity
                        .getPackageName())) {
                    // Proceed with starting the app only if there is only one
                    // Activity (the StartUpActivity) in the task.
                    return taskInfo.numActivities == 1;
                }
            }
        }
        
        return true;
    }
}
