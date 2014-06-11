/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.ui;

import java.io.IOException;
import java.util.ArrayList;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerCallback;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.Activity;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ListView;

import com.acer.ccd.R;
import com.acer.ccd.ui.cmp.DeviceItem;
import com.acer.ccd.ui.cmp.DeviceListAdapter;
import com.acer.ccd.util.CcdSdkDefines;
public class DeviceManagementActivity extends Activity {
    

    private static final String TAG = "DeviceManagementActivity";

    private static final int DIALOG_SHOW_PROGRESS = 0;
    private static final int ACTIVITY_RESULT_SYNCSETTINGS = 22;

    private static final int MESSAGE_SEND_DEVICE_ITEMS = 0x10;
    private static final int MESSAGE_GET_DEVICES_DONE = (MESSAGE_SEND_DEVICE_ITEMS + 1);

    private ListView mDeviceListView;
    private DeviceListAdapter mAdapter;
    private GetDeviceInfoThread mThread;

    private Button mButtonSettings;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.device_management_activity);

        findViews();
        setEvents();
        
        // doTestDevice();
        mAdapter = new DeviceListAdapter(this);
        mDeviceListView.setAdapter(mAdapter);

        if (!hasTSAccount()) {
            AccountManager accountManager = AccountManager.get(this);
            accountManager.addAccount(  CcdSdkDefines.ACCOUNT_TYPE,
                                        null,
                                        null,
                                        null,
                                        this,
                                        new AccountManagerCallback<Bundle>() {
                                            @Override
                                            public void run(AccountManagerFuture<Bundle> future) {
                                                try {
                                                    Log.i(TAG, "result.getResult() = " + future.getResult().toString());
                                                    // Login success, start to get devices list.
                                                    startGetDevices();
                                                    return;
                                                } catch (OperationCanceledException e) {
                                                    e.printStackTrace();
                                                } catch (AuthenticatorException e) {
                                                    e.printStackTrace();
                                                } catch (IOException e) {
                                                    e.printStackTrace();
                                                }
                                                finish();
                                            }
                                            
                                        },
                                        null
                                      );
            return;
        }
        startGetDevices();
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        switch (id) {
            case DIALOG_SHOW_PROGRESS:
                ProgressDialog dialog = new ProgressDialog(this);
                dialog.setTitle("Get devices info");
                dialog.setMessage("Please wait a moment");
                dialog.setCancelable(false);
                return dialog;
        }
        return super.onCreateDialog(id);
    }

    @Override
    protected void onPrepareDialog(int id, Dialog dialog) {
        super.onPrepareDialog(id, dialog);
        switch (id) {
            case DIALOG_SHOW_PROGRESS:
                break;
        }
    }
    
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Log.i(TAG, "onActivityResult");
        if (requestCode == ACTIVITY_RESULT_SYNCSETTINGS) {
            boolean result = hasTSAccount();
            Log.i(TAG, "result = " + result);
            if (!result)
                finish();
        }
    }
    
    private void startGetDevices() {
        showProgressBar();
        mThread = new GetDeviceInfoThread();
        mThread.start();
    }
    
    private boolean hasTSAccount() {
        AccountManager accountManager = AccountManager.get(this);
        Account[] accounts = accountManager.getAccountsByType(CcdSdkDefines.ACCOUNT_TYPE);
        return accounts.length != 0;
    }
    
    private Account getTSAccount() {
        AccountManager accountManager = AccountManager.get(this);
        Account[] accounts = accountManager.getAccountsByType(CcdSdkDefines.ACCOUNT_TYPE);
        if (accounts.length == 0)
            return null;
        
        return accounts[0];
    }

    private void showProgressBar() {
        showDialog(DIALOG_SHOW_PROGRESS);
    }

    private void hideProgressBar() {
        dismissDialog(DIALOG_SHOW_PROGRESS);
    }

    private void findViews() {
        mDeviceListView = (ListView) findViewById(R.id.devce_mg_list_device);
        mButtonSettings = (Button) findViewById(R.id.device_mg_button_settings);
    }

    private void setEvents() {
        mButtonSettings.setOnClickListener(mSettingsClickListener);
    }

    private Button.OnClickListener mSettingsClickListener = new Button.OnClickListener() {

        @Override
        public void onClick(View arg0) {
            //startActivity(new Intent(Settings.ACTION_SYNC_SETTINGS));
            Account account = getTSAccount();
            if (account == null)
                return;
            Intent intent = new Intent("android.settings.ACCOUNT_SYNC_SETTINGS");
            intent.putExtra("account", account);
            //intent.setFlags(intent.getFlags() | Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivityForResult(intent, ACTIVITY_RESULT_SYNCSETTINGS);
        }

    };

    private Handler mUpdateUIHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_SEND_DEVICE_ITEMS: {
                    @SuppressWarnings("unchecked")
					ArrayList<DeviceItem> items = (ArrayList<DeviceItem>) msg.obj;
                    if (items == null)
                        break;
                    mAdapter.addItems(items);
                    mAdapter.notifyDataSetChanged();
                    // runOnUiThread(doFreshListView);
                    break;
                }
                case MESSAGE_GET_DEVICES_DONE:
                    hideProgressBar();
                    break;
            }
        }
    };

    private class GetDeviceInfoThread extends Thread {

        public GetDeviceInfoThread() {

        }

        @Override
        public void run() {
            ArrayList<DeviceItem> items;
            DeviceItem item;
            Message msg;
            
            // TODO : Get all device Info that linked from TS Service
            //        fill DeviceItem data and add item to list.

            items = new ArrayList<DeviceItem>();
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_ACER);
            item.setIsLink(true);
            item.setIsOnline(false);
            item.setDeviceName("Acer LiquidMT");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_PHONE);
            delayTime(100);
            items.add(item);
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_ACER);
            item.setIsLink(true);
            item.setIsOnline(false);
            item.setDeviceName("Acer Liquid");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_PHONE);
            delayTime(100);
            items.add(item);
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_ACER);
            item.setIsLink(true);
            item.setIsOnline(true);
            item.setDeviceName("Acer Smart");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_PHONE);
            delayTime(200);
            items.add(item);
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_ACER);
            item.setIsLink(true);
            item.setIsOnline(true);
            item.setDeviceName("Acer Mini");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_PHONE);
            delayTime(200);
            items.add(item);
            mUpdateUIHandler.obtainMessage(MESSAGE_SEND_DEVICE_ITEMS, items);

            items = new ArrayList<DeviceItem>();
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_ACER);
            item.setIsLink(true);
            item.setIsOnline(true);
            item.setDeviceName("Acer A500");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_TABLET);
            delayTime(500);
            items.add(item);
            mUpdateUIHandler.obtainMessage(MESSAGE_SEND_DEVICE_ITEMS, items);
            
            items = new ArrayList<DeviceItem>();
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_OTHERS);
            item.setIsLink(true);
            item.setIsOnline(true);
            item.setDeviceName("HTC Pucci");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_TABLET);
            delayTime(300);
            items.add(item);
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_OTHERS);
            item.setIsLink(true);
            item.setIsOnline(false);
            item.setDeviceName("HTC Salsa");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_PHONE);
            delayTime(300);
            items.add(item);
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_OTHERS);
            item.setIsLink(true);
            item.setIsOnline(false);
            item.setDeviceName("HTC Magic");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_PHONE);
            delayTime(300);
            items.add(item);
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_OTHERS);
            item.setIsLink(true);
            item.setIsOnline(false);
            item.setDeviceName("Samsung Galaxy S");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_PHONE);
            delayTime(300);
            items.add(item);
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_OTHERS);
            item.setIsLink(true);
            item.setIsOnline(false);
            item.setDeviceName("Samsung Tab 10.1");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_TABLET);
            delayTime(300);
            items.add(item);
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_OTHERS);
            item.setIsLink(true);
            item.setIsOnline(false);
            item.setDeviceName("SonyEricsson X10i");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_PHONE);
            delayTime(300);
            items.add(item);
            item = new DeviceItem();
            item.setDeviceId(DeviceItem.DEVICE_ID_OTHERS);
            item.setIsLink(true);
            item.setIsOnline(false);
            item.setDeviceName("LG P500");
            item.setDeviceType(DeviceItem.DEVICE_TYPE_PHONE);
            delayTime(300);
            items.add(item);
            mUpdateUIHandler.obtainMessage(MESSAGE_SEND_DEVICE_ITEMS, items);

            mUpdateUIHandler.sendEmptyMessage(MESSAGE_GET_DEVICES_DONE);
        }

        private void delayTime(long ms) {
            try {
                Thread.sleep(ms);
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

        }
    }

}
