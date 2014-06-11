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

public class FavoriteSyncAdapterService extends Service {
    // private static final String TAG = "FavoriteSyncAdapterService";
    private static FavoriteSyncAdapterImpl sSyncAdapter = null;
    // private static ContentResolver mContentResolver = null;

    public FavoriteSyncAdapterService() {
        super();
    }

    @Override
    public IBinder onBind(Intent intent) {
        IBinder ret = null;
        ret = getSyncAdapter().getSyncAdapterBinder();
        return ret;
    }

    private FavoriteSyncAdapterImpl getSyncAdapter() {
        if (sSyncAdapter == null)
            sSyncAdapter = new FavoriteSyncAdapterImpl(this);
        return sSyncAdapter;
    }
    
    public class FavoriteSyncAdapterImpl extends AbstractThreadedSyncAdapter {

        // private Context mContext;

        public FavoriteSyncAdapterImpl(Context context) {
            super(context, true);
            // mContext = context;
        }

        @Override
        public void onPerformSync(Account account, Bundle extras, String authority,
                ContentProviderClient provider, SyncResult syncResult) {
            // TODO : Start sync for Favorite.

            // mContentResolver = mContext.getContentResolver();

            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

    }
}
