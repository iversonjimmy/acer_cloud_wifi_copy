/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.service;

import android.accounts.Account;
import android.app.Service;
import android.content.AbstractThreadedSyncAdapter;
import android.content.ContentProviderClient;
import android.content.Context;
import android.content.Intent;
import android.content.SyncResult;
import android.os.Bundle;
import android.os.IBinder;

public class CalendarSyncAdapterService extends Service {
    // private static final String TAG = "CalendarSyncAdapterService";
    private static CalendarSyncAdapterImpl sSyncAdapter = null;
    // private static ContentResolver mContentResolver = null;

    public CalendarSyncAdapterService() {
        super();
    }

    @Override
    public IBinder onBind(Intent intent) {
        IBinder ret = null;
        ret = getSyncAdapter().getSyncAdapterBinder();
        return ret;
    }

    private CalendarSyncAdapterImpl getSyncAdapter() {
        if (sSyncAdapter == null)
            sSyncAdapter = new CalendarSyncAdapterImpl(this);
        return sSyncAdapter;
    }
    
    public class CalendarSyncAdapterImpl extends AbstractThreadedSyncAdapter {

        // private Context mContext;

        public CalendarSyncAdapterImpl(Context context) {
            super(context, true);
            // mContext = context;
        }

        @Override
        public void onPerformSync(Account account, Bundle extras, String authority,
                ContentProviderClient provider, SyncResult syncResult) {
            // TODO : Start sync for Calendar. 
            // mContentResolver = mContext.getContentResolver();

            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            
            syncResult.databaseError = true;
        }

    }
}
