/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.receiver;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.NetworkInfo.State;
import android.util.Log;

import com.acer.ccd.service.CcdBackgroundService;
import com.acer.ccd.util.CcdSdkDefines;
import com.acer.ccd.util.InternalDefines;

public class TSReceiver extends BroadcastReceiver {
    
    private static final String TAG = "TSReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (action.equals(AccountManager.LOGIN_ACCOUNTS_CHANGED_ACTION)) {
            Log.i(TAG, "receive AccountManager.LOGIN_ACCOUNCHANGED_ACTION");
            processLogout(context);

        } else if (action.equals(ConnectivityManager.CONNECTIVITY_ACTION)) {
            Log.i(TAG, "receive ConnectivityManager.CONNECTIVITY_ACTION");
            processRelogin(context, intent);
        } else if (action.equals(InternalDefines.BROADCAST_MESSAGE_PERIODIC_SYNC)) {
            Log.i(TAG, "receive " + InternalDefines.BROADCAST_MESSAGE_PERIODIC_SYNC);
            processCcdPeriodicSync(context);
        } else {
            Log.e(TAG, "Ignoring unknown action: " + action);
        }
    }
    
    private void processLogout(Context context) {
        AccountManager accountManager = AccountManager.get(context);
        Account[] accounts = accountManager.getAccountsByType(CcdSdkDefines.ACCOUNT_TYPE);
        if (accounts.length == 0) {
            Intent serviceIntent = new Intent(context, CcdBackgroundService.class);
            serviceIntent.putExtra(InternalDefines.BUNDLE_PROGRESS_TYPE, InternalDefines.ACTION_TYPE_LOGOUT);
            context.startService(serviceIntent);
        }
    }
    
    private void processRelogin(Context context, Intent intent) {
        boolean noConnectivity = intent.getBooleanExtra(ConnectivityManager.EXTRA_NO_CONNECTIVITY, true);        
        String info = intent.getStringExtra(ConnectivityManager.EXTRA_EXTRA_INFO);
        NetworkInfo networkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
        NetworkInfo otherNetworkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_OTHER_NETWORK_INFO);

        Log.i(TAG, "noConnectivity = " + noConnectivity + "\n" + 
                   "EXTRA_EXTRA_INFO = " + info + "\n" + 
                   "EXTRA_NETWORK_INFO = " + networkInfo + "\n" +
                   "EXTRA_OTHER_NETWORK_INFO = " + otherNetworkInfo);
        
        if (!networkInfo.getState().equals(State.CONNECTED))
            return;

        Log.i(TAG, "Has connectivity!");
        
        AccountManager accountManager = AccountManager.get(context);
        Account[] accounts = accountManager.getAccountsByType(CcdSdkDefines.ACCOUNT_TYPE);
        if (accounts.length > 0) {
            Intent serviceIntent = new Intent(context, CcdBackgroundService.class);
            serviceIntent.putExtra(InternalDefines.BUNDLE_PROGRESS_TYPE, InternalDefines.ACTION_TYPE_RELOGIN);
            context.startService(serviceIntent);
        }   
    }
    
    private void processCcdPeriodicSync(Context context) {
        AccountManager accountManager = AccountManager.get(context);
        Account[] accounts = accountManager.getAccountsByType(CcdSdkDefines.ACCOUNT_TYPE);
        if (accounts.length != 0) {
            Intent serviceIntent = new Intent(context, CcdBackgroundService.class);
            serviceIntent.putExtra(InternalDefines.BUNDLE_PROGRESS_TYPE, InternalDefines.ACTION_TYPE_RELOGIN_AND_SYNC);
            CcdBackgroundService.acquireWakeLock(context);
            // TODO: call CcdBackgroundService.releaseWakeLock(context) if unable to deliver Intent for any reason.
            //   It could return null or throw a SecurityException.  Anything else?
            context.startService(serviceIntent);
        }
    }
}
