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

public class ContactsSyncAdapterService extends Service {
    // private static final String TAG = "ContactsSyncAdapterService";
    private static ContactsSyncAdapterImpl sSyncAdapter = null;
    // private static ContentResolver mContentResolver = null;

    public ContactsSyncAdapterService() {
        super();
    }

    @Override
    public IBinder onBind(Intent intent) {
        IBinder ret = null;
        ret = getSyncAdapter().getSyncAdapterBinder();
        return ret;
    }

    private ContactsSyncAdapterImpl getSyncAdapter() {
        if (sSyncAdapter == null)
            sSyncAdapter = new ContactsSyncAdapterImpl(this);
        return sSyncAdapter;
    }
    
    public class ContactsSyncAdapterImpl extends AbstractThreadedSyncAdapter {
        
        // private static final String TAG = "ContactsSyncAdapterImpl";
        // private Context mContext;

        public ContactsSyncAdapterImpl(Context context) {
            super(context, true);
            // mContext = context;

        }

        @Override
        public void onPerformSync(Account account, Bundle extras, String authority,
                ContentProviderClient provider, SyncResult syncResult) {

            // mContentResolver = mContext.getContentResolver();

        }

    }

}