/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.dx.util;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.AlarmManager;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Environment;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.widget.Toast;

//import com.acer.ccd.provider.CameraRollStreamProvider;

public class Utility {
    
    private static final String REGULAR_EXPRESSION_EMAIL = "^[\\w-]+(\\.[\\w-]+)*@[\\w-]+(\\.[\\w-]+)+$";//"^[w-]+(.[w-]+)*@[w-]+(.[w-]+)+$";

    static private PendingIntent mCcdPeriodicSyncIntent;

    public static void showMessageDialog(Context context, String title, String message) {
        new AlertDialog.Builder(context).setTitle(title).setMessage(message)
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface arg0, int arg1) {

                    }
                }).show();
    }
    
    public static void showMessageDialog(Context context, int titleId, int messageId) {
        showMessageDialog(context, context.getText(titleId).toString(), context.getText(messageId).toString());
    }
    
    public static ProgressDialog showProgressDialog(Context context, String title, String message, boolean cancelable) {
        ProgressDialog dialog = new ProgressDialog(context);
        dialog.setTitle(title);
        dialog.setMessage(message);
        dialog.setCancelable(cancelable);
        dialog.show();
        return dialog;
    }
    
    public static ProgressDialog showProgressDialog(Context context, int titleId, int messageId, boolean cancelable) {
        return showProgressDialog(context, context.getText(titleId).toString(), context.getText(messageId).toString(), cancelable);
    }

    public static void ToastMessage(Context context, String message) {
        Toast.makeText(context, message, Toast.LENGTH_SHORT).show();
    }
    
    public static void ToastMessage(Context context, int resId) {
        Toast.makeText(context, context.getText(resId), Toast.LENGTH_SHORT).show();
    }
    
    public static void delayTime(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
    
    public static boolean isAcerDevice() {
        return Build.MANUFACTURER.equals(InternalDefines.ACER_MANUFACTURER);
    }
    
    public static boolean checkEmailFormatInvalid(String email) {
        return email.matches(REGULAR_EXPRESSION_EMAIL);
    }
    
    public static String getDeviceName() {
        return String.format("%s %s", Build.MANUFACTURER, Build.MODEL);
    }
    
    public static void setCameraRollSwitch(Context context, boolean value) {
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
        pref.edit().putBoolean(InternalDefines.PREFERENCE_SYNC_CAMERAROLL, value).commit();
    }
    
    public static boolean getCameraRollSwitch(Context context) {
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
        return pref.getBoolean(InternalDefines.PREFERENCE_SYNC_CAMERAROLL, false);
    }
    
    public static void setIsRememberAccount(Context context, boolean value) {
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
        pref.edit().putBoolean(InternalDefines.PREFERENCE_IS_REMEMBER_ACCOUNT, value).commit();
    }
    
    public static boolean getIsRememberAccount(Context context) {
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
        return pref.getBoolean(InternalDefines.PREFERENCE_IS_REMEMBER_ACCOUNT, false);
    }
    
    public static void setRememberedAccount(Context context, String account) {
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
        pref.edit().putString(InternalDefines.PREFERENCE_REMEMBERED_ACCOUNT, account).commit();
    }
    
    public static String getRememberedAccount(Context context) {
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
        return pref.getString(InternalDefines.PREFERENCE_REMEMBERED_ACCOUNT, "");
    }
    
//    public static boolean isCameraRollStreamSync(Context context) {
//        AccountManager accountManager = AccountManager.get(context);
//        Account[] accounts = accountManager.getAccountsByType(CcdSdkDefines.ACCOUNT_TYPE);
//        if (accounts.length == 0)
//            return false;
//        
//        return ContentResolver.getSyncAutomatically(accounts[0], CameraRollStreamProvider.AUTHORITY);
//    }
    
    public static boolean isHoneyCombAbove() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB;
    }
    
    public static boolean isExternalStorageExists() {
        return Environment.getExternalStorageDirectory().equals(android.os.Environment.MEDIA_MOUNTED);
    }
    
    public static String getAccountId(Context context) {
        AccountManager accountManager = AccountManager.get(context);
        Account[] accounts = accountManager.getAccountsByType(CcdSdkDefines.ACCOUNT_TYPE);
        if (accounts.length == 0)
            return "";
        
        return accounts[0].name;
    }

    /** This method is for testing only */
    public static void startCcdPeriodicSync(Context context) {
        AlarmManager alarmManager = (AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
        if (alarmManager == null)
            return;
        Intent intent = new Intent(InternalDefines.BROADCAST_MESSAGE_PERIODIC_SYNC);
        mCcdPeriodicSyncIntent = PendingIntent.getBroadcast(context, 0, intent, 0);
        alarmManager.setRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime(), (InternalDefines.SYNC_PERIOD_TIME * 1000), mCcdPeriodicSyncIntent);
    }

    /** This method is for testing only */    
    public static void stopCcdPeriodicSync(Context context) {
        AlarmManager alarmManager = (AlarmManager)context.getSystemService(Context.ALARM_SERVICE);
        if (alarmManager == null)
            return;
        alarmManager.cancel(mCcdPeriodicSyncIntent);
        mCcdPeriodicSyncIntent = null;
    }

}
