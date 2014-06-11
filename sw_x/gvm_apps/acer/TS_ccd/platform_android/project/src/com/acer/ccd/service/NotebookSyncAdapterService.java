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

public class NotebookSyncAdapterService extends Service {
    // private static final String TAG = "NotebookSyncAdapterService";
    private static NotebookSyncAdapterImpl sSyncAdapter = null;
    // private static ContentResolver mContentResolver = null;
    // private AccountManager mAccountManager;

    public NotebookSyncAdapterService() {
        super();

    }

    @Override
    public IBinder onBind(Intent intent) {
        IBinder ret = null;
        ret = getSyncAdapter().getSyncAdapterBinder();
        return ret;
    }

    private NotebookSyncAdapterImpl getSyncAdapter() {
        if (sSyncAdapter == null)
            sSyncAdapter = new NotebookSyncAdapterImpl(this);
        return sSyncAdapter;
    }
    
    public class NotebookSyncAdapterImpl extends AbstractThreadedSyncAdapter {

        // private Context mContext;

        public NotebookSyncAdapterImpl(Context context) {
            super(context, true);
            // mContext = context;
        }

        @Override
        public void onPerformSync(Account account, Bundle extras, String authority,
                ContentProviderClient provider, SyncResult syncResult) {
            
            // TODO : Start sync for Notebook.

            // mContentResolver = mContext.getContentResolver();

            // This is where the magic will happen!
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }

        @Override
        public void onSyncCanceled() {
            // Log.i(TAG, "trigger : onSyncCancled!");
            super.onSyncCanceled();
        }
    }

}