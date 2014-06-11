package com.acer.dx.util;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.NetworkInfo.State;
import android.net.wifi.WifiManager;

import com.acer.dx.util.Debug;

/** Network specific utility function */
public final class Network {

    private Network() { }

    /**
     * Get the Local DMS enabled or not
     * @return boolean result
     */
    public static boolean isLocalMediaServerEnabled() {
        // TODO: No way to get Media server status now, return true because it's annoying
        return true;
    }

    /**
     * Check if Wifi is currently enabled
     * @param context Context
     * @return yes or no
     */
    public static boolean isWifiEnabled(Context context) {
        WifiManager wifi = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        if (!wifi.isWifiEnabled())
            return false;

        ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        State st = cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI).getState();
        if (st != NetworkInfo.State.CONNECTED)
            return false;

        return true;
    }

    /**
     * Retrieve the file size of current URL
     * @param url target URL
     * @return size of url, return 0 if IOException happens
     */
    public static long getUrlSize(URL url) {
        long size;
        try {
            size = url.openConnection().getContentLength();
        } catch (IOException e) {
            size = 0;
        }

        return size;
    }

    /**
     * Check url is proper formed.
     * @param url url for test
     * @return return false if url format is incorrect
     */
    public static boolean checkURL(String url) {
        try {
            new URL(url);
            return true;
        } catch (StringIndexOutOfBoundsException e) {
            return false;
        } catch (MalformedURLException e) {
            return false;
        }
    }

    public static boolean startDMS(Context context) {
        if (Debug.NO_DMS)
            return true;

        final String dmsSetContents = "com.acer.AcerDLNAInterface.SET_SHARE_CONTENTS";
        final String dmsValueContents = "com.acer.AcerDLNAInterface.VALUE_SHARE_CONTENTS";

        Intent i = new Intent();
        i.setAction(dmsSetContents);
        i.putExtra(dmsValueContents, true);
        context.sendBroadcast(i);

        return true;
    }
}
