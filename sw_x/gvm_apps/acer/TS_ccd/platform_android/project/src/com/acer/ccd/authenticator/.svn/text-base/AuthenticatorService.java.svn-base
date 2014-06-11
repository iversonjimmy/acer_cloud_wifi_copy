/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.authenticator;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class AuthenticatorService extends Service {

    // private static final String TAG = "AuthenticatorService";
    private AccountAuthenticatorImpl mAuthenticator = null;

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        IBinder ret = null;
        if (intent.getAction().equals(android.accounts.AccountManager.ACTION_AUTHENTICATOR_INTENT))
            ret = getAuthenticator().getIBinder();
        return ret;
    }

    private AccountAuthenticatorImpl getAuthenticator() {
        if (mAuthenticator == null)
            mAuthenticator = new AccountAuthenticatorImpl(this);
        return mAuthenticator;
    }
}
