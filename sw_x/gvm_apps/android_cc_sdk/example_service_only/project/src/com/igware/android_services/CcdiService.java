//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.igware.android_services;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.LinkedList;
import java.util.Queue;
import java.util.Stack;

import igware.gvm.pb.CcdiRpc.UpdateSystemStateInput;
import igware.gvm.pb.CcdiRpc.UpdateSystemStateOutput;
import igware.gvm.pb.CcdiRpcClient.CCDIServiceClient;
import igware.protobuf.AbstractByteArrayProtoChannel;
import igware.protobuf.ProtoRpcException;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.NetworkInfo.State;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

import com.igware.ccdi_android.ICcdiAidlRpc;

public class CcdiService extends Service {
    
    private static final String LOG_TAG = "CcdiService";
    
    /*
     * Keep the following values the same with the value declared in dx_remote_agent/src/com/igware/dx_remote_agent/StatusActivity.java
     */
    private static final String BROADCAST_FILTER_STATUS_ACTIVITY = "com.igware.dx_remote_agent.StatusActivity";
    private static final String BROADCAST_FILTER_CCDISERVICE = "com.igware.android_services.CcdiService";

    private static final String KEYWORD_COMMAND = "command";
    private static final String COMMAND_CREATEDIR = "create_dir";
    private static final String COMMAND_DELETEDIR = "delete_dir";
    private static final String COMMAND_COPYFILE = "copy_file";
    private static final String COMMAND_RENAMEFILE = "rename_file";
    private static final String COMMAND_DELETEFILE = "delete_file";
    private static final String COMMAND_STATFILE = "stat_file";
    private static final String COMMAND_NOTIFYRESULT = "notify_result";

    private static final String PARAMETER_IDENTIFIER = "identifier";
    private static final String PARAMETER_FILEPATH = "filepath";
    private static final String PARAMETER_TARGETPATH = "target_path";
    private static final String PARAMETER_DIRPATH = "dir_path";
    private static final String PARAMETER_RESULT = "result";
    private static final String PARAMETER_RETURN_VALUE = "return_value";

    private static final String FILESTATE_SIZE = "file_size";
    private static final String FILESTATE_ATIME = "file_atime";
    private static final String FILESTATE_MTIME = "file_mtime";
    private static final String FILESTATE_CTIME = "file_ctime";
    private static final String FILESTATE_IS_FILE = "file_is_file";
    private static final String FILESTATE_IS_HIDDEN = "file_is_hidden";
    private static final String FILESTATE_IS_SYMLINK = "file_is_symlink";

    private CcdBroadcastReceiver myReceiver = new CcdBroadcastReceiver();
    private BroadcastReceiver broadcast_receiver = null;
    // ------------------------------------------------------
    // Android-specific overrides
    // ------------------------------------------------------
    
    @Override
    public void onCreate() {

        Log.i(LOG_TAG, "onCreate()");
        registerReceiver(myReceiver, new IntentFilter("android.net.conn.CONNECTIVITY_CHANGE"));
        super.onCreate();

        broadcast_receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                int rv = -1;
                Log.i(LOG_TAG, "Received one intent.");
                if (intent.hasExtra(KEYWORD_COMMAND)){
                    String command = intent.getStringExtra(KEYWORD_COMMAND);
                    String command_id = intent.getStringExtra(PARAMETER_IDENTIFIER);

                    Log.i(LOG_TAG, "command_id = " + command_id);

                    if (command.equals(COMMAND_CREATEDIR)) {
                        Log.i(LOG_TAG, "COMMAND_CREATEDIR");

                        String dirpath = intent.getStringExtra(PARAMETER_DIRPATH);

                        Log.i(LOG_TAG, "dirpath = " + dirpath);

                        rv = createDir(dirpath);

                    } else if (command.equals(COMMAND_DELETEDIR)) {
                        Log.i(LOG_TAG, "COMMAND_DELETEDIR");

                        String dirpath = intent.getStringExtra(PARAMETER_DIRPATH);

                        Log.i(LOG_TAG, "dirpath = " + dirpath);

                        rv = deleteDir(dirpath);

                    } else if (command.equals(COMMAND_COPYFILE)) {
                        Log.i(LOG_TAG, "COMMAND_COPYFILE");

                        String filepath = intent.getStringExtra(PARAMETER_FILEPATH);
                        String target_path = intent.getStringExtra(PARAMETER_TARGETPATH);

                        Log.i(LOG_TAG, "filepath = " + filepath);
                        Log.i(LOG_TAG, "target_path = " + target_path);

                        rv = copyFile(filepath, target_path);

                    } else if (command.equals(COMMAND_RENAMEFILE)) {
                        Log.i(LOG_TAG, "COMMAND_RENAMEFILE");

                        String filepath = intent.getStringExtra(PARAMETER_FILEPATH);
                        String target_path = intent.getStringExtra(PARAMETER_TARGETPATH);

                        Log.i(LOG_TAG, "filepath = " + filepath);
                        Log.i(LOG_TAG, "target_path = " + target_path);

                        rv = renameFile(filepath, target_path);

                    } else if (command.equals(COMMAND_DELETEFILE)) {
                        Log.i(LOG_TAG, "COMMAND_DELETEFILE");

                        String filepath = intent.getStringExtra(PARAMETER_FILEPATH);
                        Log.i(LOG_TAG, "filepath = " + filepath);

                        rv = deleteSingleFile(filepath);

                    } else if (command.equals(COMMAND_STATFILE)) {
                        Log.i(LOG_TAG, "COMMAND_STATFILE");

                        String filepath = intent.getStringExtra(PARAMETER_FILEPATH);
                        Log.i(LOG_TAG, "filepath = " + filepath);

                        File statFile = new File(filepath);
                        if (!statFile.exists()) {
                            rv = -9024; // VPL_ERR_NOENT
                        }

                        rv = 0;

                        Intent response_intent = new Intent();
                        response_intent.setAction(BROADCAST_FILTER_STATUS_ACTIVITY);
                        Bundle bundle = new Bundle();
                        bundle.putString(COMMAND_NOTIFYRESULT, PARAMETER_RESULT);
                        bundle.putString(PARAMETER_IDENTIFIER, command_id);
                        bundle.putInt(PARAMETER_RETURN_VALUE, rv);
                        bundle.putLong(FILESTATE_SIZE, statFile.length());
                        bundle.putLong(FILESTATE_ATIME, 0);
                        bundle.putLong(FILESTATE_MTIME, statFile.lastModified());
                        bundle.putLong(FILESTATE_CTIME, 0);
                        bundle.putBoolean(FILESTATE_IS_FILE, statFile.isFile());
                        bundle.putBoolean(FILESTATE_IS_HIDDEN, statFile.isHidden());

                        try {
                            bundle.putBoolean(FILESTATE_IS_SYMLINK, !statFile.getCanonicalFile().equals(statFile.getAbsoluteFile()));
                        } catch(IOException e) {
                            e.printStackTrace();
                            Log.w(LOG_TAG, "unexpected exception: " + e);
                        }

                        response_intent.putExtras(bundle);
                        sendBroadcast(response_intent);

                        Log.i(LOG_TAG, "rv = " + rv);
                        return;

                    } else {
                        Log.e(LOG_TAG, "Unsupported command:" + command);
                        rv = -1;
                    }


                    Intent response_intent = new Intent();
                    response_intent.setAction(BROADCAST_FILTER_STATUS_ACTIVITY);
                    Bundle bundle = new Bundle();
                    bundle.putString(COMMAND_NOTIFYRESULT, PARAMETER_RESULT);
                    bundle.putString(PARAMETER_IDENTIFIER, command_id);
                    bundle.putInt(PARAMETER_RETURN_VALUE, rv);

                    response_intent.putExtras(bundle);
                    sendBroadcast(response_intent);
                    Log.i(LOG_TAG, "rv = " + rv);
                }
            }
        };

        this.registerReceiver(broadcast_receiver, new IntentFilter(BROADCAST_FILTER_CCDISERVICE));
    }
    
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        // Note that this is only called in response to startService; binding
        // with BIND_AUTO_CREATE does not cause this to be invoked.
        //
        // Note that the system calls this on your service's main thread.
        // A service's main thread is the same thread where UI operations take place
        // for Activities running in the same process.
        
        Log.i(LOG_TAG, "onStartCommand(), start id " + startId + ": " + intent);
        // We want this service to continue running until it is explicitly
        // stopped, so return sticky.
        return START_STICKY;
    }
    
    @Override
    public void onDestroy() {

        Log.i(LOG_TAG, "onDestroy()");
        unregisterReceiver(myReceiver);
        unregisterReceiver(broadcast_receiver);
        android.os.Process.killProcess(android.os.Process.myPid());
    }
    
    @Override
    public IBinder onBind(Intent intent) {

        Log.i(LOG_TAG, "onBind()");
        return mBinder;
    }
    
    // ------------------------------------------------------
    
    /**
     * Make sure CCDI is loaded.  Getting the ServiceSingleton instance
     * should force ccd-jni.so to be loaded.
     */
    public void ensureCcdiLoaded()
    {
        while (!ServiceSingleton.getInstance(this).isReady()) {
            try {
                Thread.sleep(25);
            } catch (InterruptedException e) {
            }
        }
    }

    protected int createDir(String dirpath) {
        File createDirFile = new File(dirpath);
        if (!createDirFile.mkdir()) {
            if (createDirFile.exists() && createDirFile.isDirectory()) {
                return -9043; // VPL_ERR_EXIST
            } else {
                return -9099; // VPL_ERR_FAIL
            }
        }
        return 0;
    }

    protected int deleteDir(String dirpath) {
        File rmDir = new File(dirpath);
        if (!rmDir.exists()) {
            return -9024; // VPL_ERR_NOENT d
        }

        Queue<String> dirs = new LinkedList<String>();
        Stack<String> emptyDirs = new Stack<String>();

        if(rmDir.isFile()) {
            // Delete the file.
            rmDir.delete();
        } else {
            // prime the deletion process
            dirs.add(dirpath);
        }

        while (!dirs.isEmpty()) {
            String currDir = dirs.element();
            dirs.remove();
            Log.i(LOG_TAG, "Clearing directory \"" + currDir + "\"");
            File currFileDir = new File(currDir);
            String[] subDirs = currFileDir.list();
            for (String element : subDirs) {
                String strSubToDelete = currDir + "/" + element;
                File tempFile = new File(strSubToDelete);
                // add directories to the delete later stack
                // delete files immediately
                if(tempFile.isDirectory()) {
                    dirs.add(strSubToDelete);
                    Log.i(LOG_TAG, "Add dir \"" + strSubToDelete + "\" for delete.");
                } else {
                    boolean del_rc = tempFile.delete();
                    Log.i(LOG_TAG, "Delete \"" + strSubToDelete + "\":" + del_rc);
                }
            }
            emptyDirs.push(currDir);
        }

        while (!emptyDirs.empty()) {
            String currEmptyDir = emptyDirs.pop();
            File currFileEmptyDir = new File(currEmptyDir);
            boolean del_rc = currFileEmptyDir.delete();
            Log.i(LOG_TAG, "Delete empty directory\"" + currEmptyDir + "\":" + del_rc);
        }

        return 0;
    }

    protected int copyFile(String source_filepath, String target_filepath) {
        try {
            File source_file = new File(source_filepath);
            File target_file = new File(target_filepath);
            if (!source_file.exists()) {
                return -1;
            }

            if (target_file.exists()) {
                target_file.delete();
            }
            InputStream in = new FileInputStream(source_file);
            OutputStream out = new FileOutputStream(target_file);
    
            byte[] buf = new byte[2048];
            int len;
            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
            out.flush();
            in.close();
            out.close();
            return 0;

        } catch(IOException e) {
            e.printStackTrace();
            Log.w(LOG_TAG, "unexpected exception: " + e);
            return -1;
        }
    }

    protected int renameFile(String source_filepath, String target_filepath) {
        File srcRenameFile = new File(source_filepath);
        File dstRenameFile = new File(target_filepath);
        if (!srcRenameFile.renameTo(dstRenameFile)) {
            return -9099; // VPL_ERR_FAIL
        }
        return 0;
    }

    protected int deleteSingleFile(String filepath) {
        File file = new File(filepath);
        if (!file.exists()) {
            return -9024; // VOL_ERR_NOENT
        }

        if (!file.delete()) {
            return -9099; // VPL_ERR_FAIL
        }

        return 0;
    }

    private final CcdiAidlRpcImpl mBinder = new CcdiAidlRpcImpl();
    
    /**
     * For the local case, this object will be passed to ServiceConnection.onServiceConnected as
     * the {@link IBinder}.
     * For the remote case, this object's methods will be called by the Android platform.
     */
    // By extending ICcdiAidlRpc.Stub, this class also implements ICcdiAidlRpc.
    public class CcdiAidlRpcImpl extends ICcdiAidlRpc.Stub {
        
        /**
         * Only used for the remote case.
         */
        @Override
        public byte[] protoRpc(byte[] serializedRequest) {
            // This request originated from outside of this process, so we may want to enforce
            // some restrictions on what it can do.  For now, we assume that the Android manifest
            // will take care of securing remote connections to this service.
            Log.i(LOG_TAG, "remote protoRpc() waiting for ccdi");
            ensureCcdiLoaded();
            Log.i(LOG_TAG, "before remote ccdiJniProtoRpc()");
            byte[] result = ServiceSingleton.ccdiJniProtoRpc(serializedRequest, false);
            Log.i(LOG_TAG, "after remote ccdiJniProtoRpc()");
            return result;
        }
        
        /**
         * @see ICcdiAidlRpc#isReady()
         */
        @Override
        public boolean isReady() {

            return ServiceSingleton.getInstance(CcdiService.this).isReady();
        }
        
        /**
         * Only used for the local case.
         */
        private class LocalCcdiProtoChannel extends AbstractByteArrayProtoChannel {
            
            @Override
            protected byte[] perform(byte[] requestBuf) {

                Log.i(LOG_TAG, "local protoRpc() waiting for ccdi");
                ensureCcdiLoaded();
                Log.i(LOG_TAG, "before local ccdiJniProtoRpc()");
                byte[] result = ServiceSingleton.ccdiJniProtoRpc(requestBuf, false);
                Log.i(LOG_TAG, "after local ccdiJniProtoRpc()");
                return result;
            }
        }
        /**
         * Only used for the local case.
         */
        public CCDIServiceClient getLocalServiceClient() {
            return new CCDIServiceClient(new LocalCcdiProtoChannel(), true);
        }
    }
    
    private class CcdBroadcastReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent)
        {
            String action = intent.getAction();
            if (action.equals(ConnectivityManager.CONNECTIVITY_ACTION)) {
                // Matches up with "android.net.conn.CONNECTIVITY_CHANGE".
                Log.i(LOG_TAG, "Got broadcast: ConnectivityManager.CONNECTIVITY_ACTION");
                
                boolean noConnectivity = intent.getBooleanExtra(ConnectivityManager.EXTRA_NO_CONNECTIVITY, false);        
                String info = intent.getStringExtra(ConnectivityManager.EXTRA_EXTRA_INFO);
                NetworkInfo networkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
                NetworkInfo otherNetworkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_OTHER_NETWORK_INFO);

                Log.i(LOG_TAG, "NO_CONNECTIVITY = " + noConnectivity + "\n" + 
                           "EXTRA_INFO = " + info + "\n" + 
                           "NETWORK_INFO = " + networkInfo + "\n" +
                           "OTHER_NETWORK_INFO = " + otherNetworkInfo);
                
                if (!networkInfo.getState().equals(State.CONNECTED))
                    return;

                Log.i(LOG_TAG, "Has connectivity!");
                
                Thread tempThread = new Thread() {
                    @Override
                    public void run()
                    {
                        Log.i(LOG_TAG, "Reporting connectivity.");
                        CcdiAidlRpcImpl client = new CcdiAidlRpcImpl();
                        UpdateSystemStateInput req = UpdateSystemStateInput.newBuilder().setReportNetworkConnected(true).build();
                        UpdateSystemStateOutput.Builder resBuilder = UpdateSystemStateOutput.newBuilder();
                        try {
                            client.getLocalServiceClient().UpdateSystemState(req, resBuilder);
                            Log.i(LOG_TAG, "Success reporting connectivity.");
                        } catch (ProtoRpcException e) {
                            Log.e(LOG_TAG, "UpdateSystemState failed: " + e);
                        }
                    }
                };
                tempThread.start();
            } else {
                Log.e(LOG_TAG, "Ignoring unknown action: " + action);
            }
        }
    }
}
