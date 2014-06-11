//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.igware.dx_remote_agent;

import igware.gvm.pb.CcdiRpc.GetSyncStateInput;
import igware.gvm.pb.CcdiRpc.GetSyncStateOutput;
import igware.protobuf.ProtoRpcException;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URL;
import java.net.MalformedURLException;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.Stack;
import java.lang.IllegalArgumentException;
import java.lang.InterruptedException;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;
import android.os.Bundle;
import android.os.RemoteException;
import android.util.Log;
import android.widget.TextView;
import android.media.MediaPlayer;
import android.media.AudioManager;

import com.google.protobuf.ByteString;
import com.igware.ccdi_android.CcdiClientRemoteBinder;
import com.igware.ccdi_android.ICcdiAidlRpc;

import com.igware.dxshell.pb.DxRemoteAgent.ErrorCode;
import com.igware.dxshell.pb.DxRemoteAgent.RequestType;
import com.igware.dxshell.pb.DxRemoteAgent.QueryDeviceOutput;
import com.igware.dxshell.pb.DxRemoteAgent.DxRemoteMessage;
import com.igware.dxshell.pb.DxRemoteAgent.DxRemoteMessage.Command;
import com.igware.dxshell.pb.DxRemoteAgent.DxRemoteMessage.ArgumentName;
import com.igware.dxshell.pb.DxRemoteAgent.DxRemoteAgentFileTransfer_Type;
import com.igware.dxshell.pb.DxRemoteAgent.DxRemoteFileTransfer;
import com.igware.dxshell.pb.DxRemoteAgent.DxRemoteAgentPacket;
import com.igware.dxshell.pb.DxRemoteAgent.HttpAgentCommandType;
import com.igware.dxshell.pb.DxRemoteAgent.HttpGetInput;
import com.igware.dxshell.pb.DxRemoteAgent.HttpGetOutput;
import com.igware.dxshell.pb.DxRemoteAgent.DxRemoteMessage.DxRemote_VPLFS_file_type_t;

import org.apache.http.HttpResponse;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.client.methods.HttpDelete;
import org.apache.http.client.methods.HttpRequestBase;
import org.apache.http.client.methods.HttpEntityEnclosingRequestBase;
import org.apache.http.util.EntityUtils;
import org.apache.http.entity.ByteArrayEntity;
import org.apache.http.entity.InputStreamEntity;
import org.apache.http.client.ClientProtocolException;

public class StatusActivity extends Activity {

    private static final String LOG_TAG = "StatusActivity";

    /*
     * Keep the following values the same with the value declared in example_service_only/src/com/igware/android_service/CcdiService.java
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

    private TextView mAppVersionField;
    private TextView mNumConnectionsField;
    private TextView mNumRequestsField;
    private TextView mNumResponsesField;
    private TextView mStatusField;

    private String mAppVersion = "";
    private int mNumConnections = 0;
    private int mNumRequests = 0;
    private int mNumResponses = 0;
    private String mStatusMsg = "Starting...";

    private MediaPlayer mp = null;
    private Object mp_lock = new Object();

    private Thread listenerThread = null;
    private SocketListener listener = null;

    protected java.util.Map<String, Queue<String> > dirMap;
    
    private BroadcastReceiver broadcast_receiver = null;
    private Map<String, Result> commandId_and_result_map = new HashMap<String, Result>();

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Log.i(LOG_TAG, "onCreate: " + this);

        CcdiClientRemoteBinder mClient = new CcdiClientRemoteBinder(StatusActivity.this);
        Log.i(LOG_TAG, "Starting CCD");
        mClient.startCcdiService();
        Log.i(LOG_TAG, "done starting CCD");
        mClient.unbindService();

        listener = new SocketListener();
        listenerThread = new Thread(listener);
        listenerThread.start();

        setContentView(R.layout.status);
        setTitle(R.string.Label_Status);

        mAppVersionField = (TextView) findViewById(R.id.TextView_AppVersion);
        mNumConnectionsField = (TextView) findViewById(R.id.TextView_Connections);
        mNumRequestsField = (TextView) findViewById(R.id.TextView_Requests);
        mNumResponsesField = (TextView) findViewById(R.id.TextView_Responses);
        mStatusField = (TextView) findViewById(R.id.TextView_SyncState);

        mStatusField.setText(mStatusMsg);

        try {
            mAppVersion = this.getPackageManager().getPackageInfo(
                    this.getPackageName(), 0).versionName;
            mAppVersionField.setText(mAppVersion);
        } catch (NameNotFoundException e) {
            Log.e(LOG_TAG, e.getMessage());
        }

        dirMap = new java.util.TreeMap<String, Queue<String> >();

        broadcast_receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                Log.i(LOG_TAG, "Received one intent.");
                if (intent.hasExtra(COMMAND_NOTIFYRESULT) && intent.getStringExtra(COMMAND_NOTIFYRESULT).equals(PARAMETER_RESULT)) {
                    Log.i(LOG_TAG, "COMMAND_NOTIFYRESULT");
                    synchronized (commandId_and_result_map) {
                        int result = intent.getIntExtra(PARAMETER_RETURN_VALUE, -1);
                        String command_id = intent.getStringExtra(PARAMETER_IDENTIFIER);
                        if (commandId_and_result_map.containsKey(command_id)) {
                            Log.i(LOG_TAG, "command_id: " + command_id);
                            commandId_and_result_map.get(command_id).setReturnCode(result);
                            commandId_and_result_map.get(command_id).setIntent(intent);
                            commandId_and_result_map.notifyAll();

                        } else {
                            Log.e(LOG_TAG, "Invalid command_id: " + command_id);
                        }
                    }
                }
            }
        };

        this.registerReceiver(broadcast_receiver, new IntentFilter(BROADCAST_FILTER_STATUS_ACTIVITY));
    }

    @Override
    protected void onStart()
    {
        super.onStart();
        Log.i(LOG_TAG, "onStart: " + this);
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        Log.i(LOG_TAG, "onResume: " + this);
        updateUi();
    }

    @Override
    protected void onPause()
    {
        super.onPause();

        Log.i(LOG_TAG, "onPause: " + this);

        synchronized(mp_lock) {
            if (mp != null) {
                mp.release();
                mp = null;
            }
        }
    }

    @Override
    protected void onStop()
    {
        super.onStop();

        Log.i(LOG_TAG, "onStop: " + this);

        synchronized(mp_lock) {
            if (mp != null) {
                mp.release();
                mp = null;
            }
        }
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();

        Log.i(LOG_TAG, "onDestroy: " + this);

        synchronized(mp_lock) {
            if (mp != null) {
                mp.release();
                mp = null;
            }
        }

        if(listenerThread != null) {
            try {
                Log.i(LOG_TAG, "Close listener's socket to force exit.");
                listener.closeSocket();
                Log.i(LOG_TAG, "Join SocketListener thread.");
                listenerThread.join();
                listenerThread = null;
                listener = null;
            } catch (InterruptedException e) {
                // Thread was interrupted. Pass it along.
                Thread.currentThread().interrupt();
            }
        }
        else {
            Log.i(LOG_TAG, "SocketListener thread is null.");
        }
        unregisterReceiver(broadcast_receiver);
    }

    void setStatusMessage(final String msg)
    {
        Log.i(LOG_TAG, "setting status message to \"" + msg + "\"");
        synchronized (this) {
            mStatusMsg = msg;
        }
        updateUi();
    }

    void updateUi()
    {
        final String connString;
        final String reqString;
        final String respString;
        final String statusString;
        synchronized (this) {
            connString = "" + mNumConnections;
            reqString = "" + mNumRequests;
            respString = "" + mNumResponses;
            statusString = mStatusMsg;
        }
        runOnUiThread(new Runnable() {
            public void run() {
                mNumConnectionsField.setText(connString);
                mNumRequestsField.setText(reqString);
                mNumResponsesField.setText(respString);
                mStatusField.setText(statusString);
            }
        });
    }

    protected Result sendIntentAndWait(String action, Map<String, String> parameters) {
        Result result;
        String command_id = String.format("handle_%d", System.currentTimeMillis());
        Intent intent = new Intent();
        intent.setAction(BROADCAST_FILTER_CCDISERVICE);
        Bundle bundle = new Bundle();

        bundle.putString(PARAMETER_IDENTIFIER, command_id);
        for (String key : parameters.keySet()) {
            bundle.putString(key, parameters.get(key));
        }

        synchronized (commandId_and_result_map) {
            result = new Result();
            result.setReturnCode(Integer.MAX_VALUE);
            commandId_and_result_map.put(command_id, result); // Integer.MAX_VALUE means didn't receive response, yet.
        }

        intent.putExtras(bundle);
        sendBroadcast(intent);

        synchronized (commandId_and_result_map) {
            try {
                while (true) {
                    Log.i(LOG_TAG, String.format("Waiting for the response. (command_id: %s)", command_id));
                    commandId_and_result_map.wait();
                    if (commandId_and_result_map.get(command_id).getReturnCode() != Integer.MAX_VALUE) {
                        result = commandId_and_result_map.get(command_id); 
                        Log.i(LOG_TAG, String.format("Received the result. (command_id: %s, rv: %d)", command_id, result.getReturnCode()));
                        commandId_and_result_map.remove(command_id);
                        return result;
                    }
                }
            } catch(InterruptedException e) {
                e.printStackTrace();
                Log.w(LOG_TAG, "unexpected exception: " + e);
            }
        }
        result.setReturnCode(-1);
        return result;
    }

    public int requestCcdiServiceCreateDirAndWait(String dirpath)
    {
        Map<String, String> parameters = new HashMap<String, String>();
        parameters.put(KEYWORD_COMMAND, COMMAND_CREATEDIR);
        parameters.put(PARAMETER_DIRPATH, dirpath);
        return sendIntentAndWait(BROADCAST_FILTER_CCDISERVICE, parameters).getReturnCode();
    }

    public int requestCcdiServiceDeleteDirAndWait(String dirpath)
    {
        Map<String, String> parameters = new HashMap<String, String>();
        parameters.put(KEYWORD_COMMAND, COMMAND_DELETEDIR);
        parameters.put(PARAMETER_DIRPATH, dirpath);
        return sendIntentAndWait(BROADCAST_FILTER_CCDISERVICE, parameters).getReturnCode();
    }

    public int requestCcdiServiceCopyFileAndWait(String filepath, String target_path)
    {
        Map<String, String> parameters = new HashMap<String, String>();
        parameters.put(KEYWORD_COMMAND, COMMAND_COPYFILE);
        parameters.put(PARAMETER_FILEPATH, filepath);
        parameters.put(PARAMETER_TARGETPATH, target_path);
        return sendIntentAndWait(BROADCAST_FILTER_CCDISERVICE, parameters).getReturnCode();
    }

    public int requestCcdiServiceRenameFileAndWait(String filepath, String target_path)
    {
        Map<String, String> parameters = new HashMap<String, String>();
        parameters.put(KEYWORD_COMMAND, COMMAND_RENAMEFILE);
        parameters.put(PARAMETER_FILEPATH, filepath);
        parameters.put(PARAMETER_TARGETPATH, target_path);
        return sendIntentAndWait(BROADCAST_FILTER_CCDISERVICE, parameters).getReturnCode();
    }

    public int requestCcdiServiceDeleteFileAndWait(String filepath)
    {
        Map<String, String> parameters = new HashMap<String, String>();
        parameters.put(KEYWORD_COMMAND, COMMAND_DELETEFILE);
        parameters.put(PARAMETER_FILEPATH, filepath);
        return sendIntentAndWait(BROADCAST_FILTER_CCDISERVICE, parameters).getReturnCode();
    }

    public int requestCcdiServiceStatFileAndWait(String filepath, DxRemoteMessage.DxRemote_VPLFS_stat_t.Builder file_state_builder)
    {
        Map<String, String> parameters = new HashMap<String, String>();
        parameters.put(KEYWORD_COMMAND, COMMAND_STATFILE);
        parameters.put(PARAMETER_FILEPATH, filepath);
        Result result = sendIntentAndWait(BROADCAST_FILTER_CCDISERVICE, parameters);
        file_state_builder.setSize(result.getIntent().getLongExtra(FILESTATE_SIZE, -1));
        file_state_builder.setAtime(result.getIntent().getLongExtra(FILESTATE_ATIME, 0));
        file_state_builder.setMtime(result.getIntent().getLongExtra(FILESTATE_MTIME, 0));
        file_state_builder.setCtime(result.getIntent().getLongExtra(FILESTATE_CTIME, 0));
        file_state_builder.setType(result.getIntent().getBooleanExtra(FILESTATE_IS_FILE, false) ?
                                                                      DxRemote_VPLFS_file_type_t.DxRemote_VPLFS_TYPE_FILE :
                                                                      DxRemote_VPLFS_file_type_t.DxRemote_VPLFS_TYPE_DIR);
        file_state_builder.setIsHidden(result.getIntent().getBooleanExtra(FILESTATE_IS_HIDDEN, false) ? 1 : 0);
        file_state_builder.setIsSymLink(result.getIntent().getBooleanExtra(FILESTATE_IS_SYMLINK, false) ? 1 : 0);
        return result.getReturnCode();
    }

    public String getMediaPlaylistPath() {
        try {
            int rv;
            CcdiClientRemoteBinder mClient;
            GetSyncStateInput input;
            GetSyncStateOutput output;
            GetSyncStateOutput.Builder output_builder;

            mClient = new CcdiClientRemoteBinder(StatusActivity.this);
            mClient.bindService();
            Log.i(LOG_TAG, "Waiting for CCD");

            mClient.waitUntilReady();
            Log.i(LOG_TAG, "CCD is ready");
            input = GetSyncStateInput.newBuilder().setGetMediaPlaylistPath(true).build();
            output_builder = GetSyncStateOutput.newBuilder();
            rv = mClient.getCcdiRpcClient().GetSyncState(input, output_builder);
            if (rv < 0) {
                return null;
            }

            output = output_builder.build();
            if (!output.hasMediaPlaylistPath()) {
                return null;
            }

            mClient.unbindService();
            return output.getMediaPlaylistPath();

        } catch(ProtoRpcException e) {
            e.printStackTrace();
            Log.w(LOG_TAG, "unexpected exception: " + e);
        }
        return null;
    }

    public String getCcdiServicePath() {
        // TODO: Find out a way to get the value from ccd or CcdiService.
        return "/data/data/com.igware.android_cc_sdk.example_service_only";
    }

    static void getInterfaceInformation(NetworkInterface netint, StringBuilder statusMsg)
    {
        Log.i(LOG_TAG, "Name: " + netint.getName());
        Log.i(LOG_TAG, "  Display name: " + netint.getDisplayName());
        Enumeration<InetAddress> inetAddresses = netint.getInetAddresses();
        for (InetAddress inetAddress : Collections.list(inetAddresses)) {
            Log.i(LOG_TAG, "  InetAddress: " + inetAddress);
            statusMsg.append(inetAddress + "\n");
        }
    }

    static int computeProtobufSize(BufferedInputStream bis) throws IOException
    {
        int bytesRead = 1;
        byte tmp = (byte)bis.read();
        if (tmp >= 0) {
            return bytesRead + tmp;
        } else {
            int protoSize = tmp & 0x7f;
            bytesRead++;
            if ((tmp = (byte)bis.read()) >= 0) {
              protoSize |= tmp << 7;
            } else {
              protoSize |= (tmp & 0x7f) << 7;
              bytesRead++;
              if ((tmp = (byte)bis.read()) >= 0) {
                protoSize |= tmp << 14;
              } else {
                protoSize |= (tmp & 0x7f) << 14;
                bytesRead++;
                if ((tmp = (byte)bis.read()) >= 0) {
                  protoSize |= tmp << 21;
                } else {
                  protoSize |= (tmp & 0x7f) << 21;
                  bytesRead++;
                  protoSize |= (tmp = (byte)bis.read()) << 28;
                  if (tmp < 0) {
                    // Discard upper 32 bits.
                    for (int i = 0; i < 5; i++) {
                      bytesRead++;
                      if ((byte)bis.read() >= 0) {
                        return bytesRead + protoSize;
                      }
                    }
                    throw new IOException("malformed varint32");
                  }
                }
              }
            }
            return bytesRead + protoSize;
        }
    }

    /**
     * Read a raw Varint from the stream.  If larger than 32 bits, discard the
     * upper bits.  Returns the sum of the size of the Varint and the value of the Varint.
     * Also resets the stream back to the original position.
     */
    static int computeProtoRpcSize(BufferedInputStream bis) throws IOException
    {
        bis.mark(256 - 10);
        int headerSize = computeProtobufSize(bis);
        bis.reset();
        Log.i(LOG_TAG, "Header size: " + headerSize);
        if (headerSize > (256 - 10)) {
            throw new IOException("Header size is too big (" + headerSize + ")");
        }
        bis.mark(256);
        bis.skip(headerSize);
        int payloadSize = computeProtobufSize(bis);
        bis.reset();
        Log.i(LOG_TAG, "Payload size: " + payloadSize);
        return headerSize + payloadSize;
    }

    static void setDxHttpGetResponse(DataOutputStream out, HttpGetOutput.Builder res_http_builder)
        throws IOException
    {
        Log.i(LOG_TAG, "Start Serialize following field");
        Log.i(LOG_TAG, "ResErrorCode: " + res_http_builder.getErrorCode());
        Log.i(LOG_TAG, "ResTotalBytes: " + res_http_builder.getTotalBytes());
        Log.i(LOG_TAG, "ResHttpagentResponse: " + res_http_builder.getHttpagentResponse());
        Log.i(LOG_TAG, "ResResponse: " + res_http_builder.getResponse());
        byte[] bytes = res_http_builder.build().toByteArray();
        out.writeInt(bytes.length);
        out.write(bytes, 0, bytes.length);
    }

    private class ServiceThread implements Runnable
    {
        private Socket connectedSocket;
        private int id;
        private BufferedInputStream bufferedInputStream;
        private DataInputStream sockInput;
        private DataOutputStream sockOutput;

        public ServiceThread(Socket connectedSocket, int id)
        {
            this.connectedSocket = connectedSocket;
            this.id = id;
        }

        private void handleProtoRpc() throws Exception
        {
            // Need to look ahead to extract the length of the protobuf message.
            int size = computeProtoRpcSize(bufferedInputStream);
            byte[] requestBuf = new byte[size];
            sockInput.readFully(requestBuf, 0, size);
            byte[] responseBuf = null;

            CcdiClientRemoteBinder mClient = new CcdiClientRemoteBinder(StatusActivity.this);
            mClient.bindService();
            Log.i(LOG_TAG, "Waiting for CCD");
            mClient.waitUntilReady();
            Log.i(LOG_TAG, "CCD is ready");
           
            for (int attempt = 1; attempt <= 3; attempt++) {
                ICcdiAidlRpc aidl = mClient.getRawAidl();
                if (aidl == null) {
                    Log.w(LOG_TAG, "Attempt " + attempt + " failed: service is not bound");
                } else {
                    try {
                        responseBuf = aidl.protoRpc(requestBuf);
                        sockOutput.write(responseBuf);
                        // Success; exit the retry loop.
                        break;
                    } catch (RemoteException e) {
                        Log.w(LOG_TAG, "Attempt " + attempt + " failed: " + e);
                    }
                }
                Log.i(LOG_TAG, "Attempting to rebind");
                mClient.bindService();
                // TODO: bindService should return bool
                Log.i(LOG_TAG, "Waiting for CCD");
                mClient.waitUntilReady(); // TODO: waitUntilReady should take a timeout
                Log.i(LOG_TAG, "CCD is ready");
            }
            mClient.unbindService();
        }

        private void handleHttpGet() throws Exception
        {
            int maxbytes = 0, totalread = 0;
            boolean useMediaPlayer = false;
            int dxhttp_protocol_size = sockInput.readInt();
            HttpGetOutput.Builder res_http_builder = HttpGetOutput.newBuilder();
            res_http_builder.setErrorCode(ErrorCode.DX_ERR_UNEXPECTED_ERROR.getNumber());
            res_http_builder.setTotalBytes(0);
            res_http_builder.setHttpagentResponse(-1);
            byte[] reqHttpProtocolbuf = new byte[dxhttp_protocol_size];
            sockInput.readFully(reqHttpProtocolbuf, 0, dxhttp_protocol_size);
            HttpGetInput httpReq = HttpGetInput.parseFrom(reqHttpProtocolbuf);
            String urlStr;
            int rv;

            switch(httpReq.getCommandType().getNumber()) {
            case HttpAgentCommandType.DX_HTTP_GET_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() Start");
                urlStr = httpReq.getUrl();
                maxbytes = httpReq.getMaxBytes();
                useMediaPlayer = httpReq.getUseMediaPlayer();
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() urlStr: " + urlStr + " maxbytes: " + maxbytes);
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() useMediaPlayer: " + useMediaPlayer );
                if (useMediaPlayer) {
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() Attempting to play " + urlStr);
                    try {
                        synchronized(mp_lock) {
                            if (mp == null)
                                mp = new MediaPlayer();
                            else
                                mp.reset();
                            mp.setAudioStreamType(AudioManager.STREAM_MUSIC);
                            mp.setDataSource(urlStr);
                            mp.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
                                    public void onPrepared(MediaPlayer _mp) {
                                        synchronized(mp_lock) {
                                            if (_mp == mp) {
                                                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() Calling start()");
                                                _mp.start();
                                            }
                                        }
                                    }
                                });
                            Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() Calling prepareAsync()");
                            mp.prepareAsync();
                        }
                        res_http_builder.setHttpagentStatuscode(200);
                        res_http_builder.setErrorCode(ErrorCode.DX_SUCCESS.getNumber());
                        res_http_builder.setHttpagentResponse(0);
                        Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() success");
                    }
                    catch (Exception e) {
                        Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() MediaPlayer failed to play " + urlStr, e);
                    }
                }
                else {
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() Attempting to get " + urlStr);
                    InputStream in = null;
                    try {
                        URL url = new URL(urlStr);
                        in = url.openStream();
                        byte[] buffer = new byte[4096];
                        while (true) {
                            int nread = in.read(buffer);
                            if (nread < 0)  // EOF
                                break;
                            totalread += nread;
                            if (maxbytes > 0 && totalread >= maxbytes)
                                break;
                        }
                        res_http_builder.setHttpagentStatuscode(200);
                        res_http_builder.setErrorCode(ErrorCode.DX_SUCCESS.getNumber());
                        res_http_builder.setHttpagentResponse(0);
                        res_http_builder.setTotalBytes(totalread);
                        Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() success");
                    } catch (MalformedURLException e) {
                        res_http_builder.setHttpagentStatuscode(400);
                        res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_URL.getNumber());
                        Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() DX_ERR_BAD_URL " + urlStr, e);
                    } catch (IOException e) {
                        res_http_builder.setHttpagentStatuscode(404);
                        res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_ERROR.getNumber());
                        res_http_builder.setTotalBytes(totalread);
                        Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET() DX_ERR_IO_ERROR " + urlStr, e);
                    } finally {
                        if (in != null)
                            in.close();
                    }
                }
                break;

            case HttpAgentCommandType.DX_HTTP_GET_EXTENDED_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() Start");
                InputStream inExtended = null;
                File extendFile = new File(httpReq.getFileSaveResponse());
                OutputStream outExtended = new FileOutputStream(extendFile);
                urlStr = httpReq.getUrl();
                maxbytes = httpReq.getMaxBytes();
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() getFileSaveResponse: " + httpReq.getFileSaveResponse());
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() urlStr: " + urlStr + " maxbytes: " + maxbytes);

                rv = 0;
                urlStr = httpReq.getUrl();
                try {
                    DefaultHttpClient httpGetSaveClient = new DefaultHttpClient();
                    HttpRequestBase method = new HttpGet(urlStr);

                    for (int deleteHeaderIdx = 0; deleteHeaderIdx < httpReq.getHeadersCount(); ++deleteHeaderIdx) {
                        String tmpHeader = httpReq.getHeaders(deleteHeaderIdx);
                        if (tmpHeader.indexOf(':') == -1) {
                            continue;
                        }

                        String tmpHeaderName = tmpHeader.substring(0, tmpHeader.indexOf(':'));
                        String tmpHeaderValue = tmpHeader.substring(tmpHeader.indexOf(':') + 2, tmpHeader.length());
                        method.setHeader(tmpHeaderName, tmpHeaderValue);
                    }

                    HttpResponse getSaveResp = httpGetSaveClient.execute(method);
                    res_http_builder.setResponse(ByteString.EMPTY);
                    res_http_builder.setHttpagentStatuscode(getSaveResp.getStatusLine().getStatusCode());
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() Status Code " + getSaveResp.getStatusLine().getStatusCode());

                    if (getSaveResp.getStatusLine().getStatusCode() < 200 || getSaveResp.getStatusLine().getStatusCode() >= 300) {
                        rv = -1;
                        res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_HTTPAGENT_ERROR.getNumber());
                        res_http_builder.setTotalBytes(totalread);
                        res_http_builder.setHttpagentResponse(-5);
                        Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() DX_ERR_HTTPAGENT_ERROR " + urlStr);
                    }

                    inExtended = getSaveResp.getEntity().getContent();
                    while (true) {
                        byte[] bufferExtended = new byte[4096];
                        int nreadExtended = inExtended.read(bufferExtended);
                        if (nreadExtended < 0)  // EOF
                            break;
                        totalread += nreadExtended;
                        outExtended.write(bufferExtended, 0, nreadExtended);
                        if (maxbytes > 0 && totalread >= maxbytes)
                            break;
                    }
                }
                catch (IllegalArgumentException iaex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_URL.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-4);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() DX_ERR_BAD_URL " + urlStr, iaex);
                }
                catch (ClientProtocolException cpex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_REQUEST.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-3);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() DX_ERR_BAD_REQUEST " + urlStr, cpex);
                    break;
                }
                catch (IOException iex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_ERROR.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-2);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() DX_ERR_IO_ERROR " + urlStr, iex);
                    break;
                }
                catch (Exception ex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_UNEXPECTED_ERROR.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-1);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() DX_ERR_UNEXPECTED_ERROR " + urlStr, ex);
                    break;
                }
                finally {
                    if (outExtended != null) {
                        outExtended.close();
                    }
                    if (inExtended != null)
                        inExtended.close();
                }

                if (rv == 0) {
                    res_http_builder.setErrorCode(ErrorCode.DX_SUCCESS.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(0);
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_EXTENDED() DX_SUCCESS " + urlStr);
                }
                break;

            case HttpAgentCommandType.DX_HTTP_GET_RESPONSE_BACK_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_REPONSE_BACK() Start ");
                rv = 0;
                urlStr = httpReq.getUrl();
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_REPONSE_BACK() urlStr: " + urlStr);

                try {

                    DefaultHttpClient httpGetBackClient = new DefaultHttpClient();
                    HttpRequestBase method = new HttpGet(urlStr);

                    for (int deleteHeaderIdx = 0; deleteHeaderIdx < httpReq.getHeadersCount(); ++deleteHeaderIdx) {
                        String tmpHeader = httpReq.getHeaders(deleteHeaderIdx);
                        if (tmpHeader.indexOf(':') == -1) {
                            continue;
                        }

                        String tmpHeaderName = tmpHeader.substring(0, tmpHeader.indexOf(':'));
                        String tmpHeaderValue = tmpHeader.substring(tmpHeader.indexOf(':') + 2, tmpHeader.length());
                        method.setHeader(tmpHeaderName, tmpHeaderValue);
                    }

                    HttpResponse getBackResp = httpGetBackClient.execute(method);
                    res_http_builder.setResponse(ByteString.copyFrom(EntityUtils.toByteArray(getBackResp.getEntity())));
                    res_http_builder.setHttpagentStatuscode(getBackResp.getStatusLine().getStatusCode());
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_REPONSE_BACK() Status Code " + getBackResp.getStatusLine().getStatusCode());

                    if (getBackResp.getStatusLine().getStatusCode() < 200 || getBackResp.getStatusLine().getStatusCode() >= 300) {
                        rv = -1;
                        res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_HTTPAGENT_ERROR.getNumber());
                        res_http_builder.setTotalBytes(totalread);
                        res_http_builder.setHttpagentResponse(-5);
                        Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_REPONSE_BACK() DX_ERR_HTTPAGENT_ERROR " + urlStr);
                        break;
                    }

                }
                catch (IllegalArgumentException iaex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_URL.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-4);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_REPONSE_BACK() DX_ERR_BAD_URL " + urlStr, iaex);
                }
                catch (ClientProtocolException cpex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_REQUEST.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-3);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_REPONSE_BACK() DX_ERR_BAD_REQUEST " + urlStr, cpex);
                    break;
                }
                catch (IOException iex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_ERROR.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-2);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_REPONSE_BACK() DX_ERR_IO_ERROR " + urlStr, iex);
                    break;
                }
                catch (Exception ex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_UNEXPECTED_ERROR.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-1);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_REPONSE_BACK() DX_ERR_UNEXPECTED_ERROR " + urlStr, ex);
                    break;
                }

                if (rv == 0) {
                    res_http_builder.setErrorCode(ErrorCode.DX_SUCCESS.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(0);
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_GET_REPONSE_BACK() DX_SUCCESS " + urlStr);
                }
                break;

            case HttpAgentCommandType.DX_HTTP_PUT_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_PUT() Start");
                rv = 0;
                urlStr = httpReq.getUrl();
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_PUT() urlStr: " + urlStr);
                try {
                    String strPutFile = httpReq.getFile();
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_PUT() strPutFile: " + strPutFile);

                    DefaultHttpClient httpPutClient = new DefaultHttpClient();
                    HttpEntityEnclosingRequestBase method = new HttpPut(urlStr);

                    for (int putHeaderIdx = 0; putHeaderIdx < httpReq.getHeadersCount(); ++putHeaderIdx) {
                        String tmpHeader = httpReq.getHeaders(putHeaderIdx);
                        if (tmpHeader.indexOf(':') == -1) {
                            continue;
                        }
                        else if (tmpHeader.indexOf("Content-Length") != -1) {
                            continue;
                        }

                        String tmpHeaderName = tmpHeader.substring(0, tmpHeader.indexOf(':'));
                        String tmpHeaderValue = tmpHeader.substring(tmpHeader.indexOf(':') + 2, tmpHeader.length());
                        method.setHeader(tmpHeaderName, tmpHeaderValue);
                    }

                    if (strPutFile.length() == 0) {
                        if (httpReq.getPayload().size() != 0) {
                            method.setEntity(new ByteArrayEntity(httpReq.getPayload().toByteArray()));
                        }
                    }
                    else {
                        File fHttpPutFile = new File(strPutFile);
                        if (!fHttpPutFile.exists()) {
                            res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_ERROR.getNumber());
                            res_http_builder.setTotalBytes(totalread);
                            res_http_builder.setHttpagentResponse(-9024);   // VPL_ERR_NOENT
                            break;
                        }

                        InputStreamEntity reqEntity = new InputStreamEntity(new FileInputStream(fHttpPutFile), fHttpPutFile.length());
                        reqEntity.setContentType("binary/octet-stream");
                        reqEntity.setChunked(false);
                        method.setEntity(reqEntity);
                    }

                    HttpResponse putResp = httpPutClient.execute(method);
                    res_http_builder.setResponse(ByteString.copyFrom(EntityUtils.toByteArray(putResp.getEntity())));
                    res_http_builder.setHttpagentStatuscode(putResp.getStatusLine().getStatusCode());
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_PUT() Status Code" + putResp.getStatusLine().getStatusCode());

                    if (putResp.getStatusLine().getStatusCode() < 200 || putResp.getStatusLine().getStatusCode() >= 300) {
                        rv = -1;
                        res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_HTTPAGENT_ERROR.getNumber());
                        res_http_builder.setTotalBytes(totalread);
                        res_http_builder.setHttpagentResponse(-5);
                        Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_PUT() DX_ERR_IO_HTTPAGENT_ERROR " + urlStr);
                        break;
                    }

                }
                catch (IllegalArgumentException iaex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_URL.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-4);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_PUT() DX_ERR_BAD_URL " + urlStr, iaex);
                }
                catch (ClientProtocolException cpex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_REQUEST.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-3);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_PUT() DX_ERR_BAD_REQUEST " + urlStr, cpex);
                    break;
                }
                catch (IOException iex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_ERROR.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-2);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_PUT() DX_ERR_IO_ERROR " + urlStr, iex);
                    break;
                }
                catch (Exception ex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_UNEXPECTED_ERROR.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-1);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_PUT() DX_ERR_UNEXPECTED_ERROR " + urlStr, ex);
                    break;
                }
                if (rv == 0) {
                    res_http_builder.setErrorCode(ErrorCode.DX_SUCCESS.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(0);
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_PUT() DX_SUCCESS" + urlStr);
                }
                break;

            case HttpAgentCommandType.DX_HTTP_POST_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_POST() Start ");
                rv = 0;
                urlStr = httpReq.getUrl();
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_POST() urlStr: " + urlStr);
                try {
                    String strPostFile = httpReq.getFile();
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_POST() strPostFile: " + strPostFile);

                    DefaultHttpClient httpPostClient = new DefaultHttpClient();
                    HttpEntityEnclosingRequestBase method = new HttpPost(urlStr);

                    for (int postHeaderIdx = 0; postHeaderIdx < httpReq.getHeadersCount(); ++postHeaderIdx) {
                        String tmpHeader = httpReq.getHeaders(postHeaderIdx);
                        if (tmpHeader.indexOf(':') == -1) {
                            continue;
                        }
                        else if (tmpHeader.indexOf("Content-Length") != -1) {
                            continue;
                        }

                        String tmpHeaderName = tmpHeader.substring(0, tmpHeader.indexOf(':'));
                        String tmpHeaderValue = tmpHeader.substring(tmpHeader.indexOf(':') + 2, tmpHeader.length());
                        method.setHeader(tmpHeaderName, tmpHeaderValue);
                    }

                    if (strPostFile.length() == 0) {
                        if (httpReq.getPayload().size() != 0) {
                            method.setEntity(new ByteArrayEntity(httpReq.getPayload().toByteArray()));
                        }
                    }
                    else {
                        File fHttpPostFile = new File(strPostFile);
                        if (!fHttpPostFile.exists()) {
                            res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_ERROR.getNumber());
                            res_http_builder.setTotalBytes(totalread);
                            res_http_builder.setHttpagentResponse(-9024);   // VPL_ERR_NOENT
                            break;
                        }

                        InputStreamEntity reqEntity = new InputStreamEntity(new FileInputStream(fHttpPostFile), fHttpPostFile.length());
                        reqEntity.setContentType("binary/octet-stream");
                        reqEntity.setChunked(false);
                        method.setEntity(reqEntity);
                    }

                    HttpResponse postResp = httpPostClient.execute(method);
                    res_http_builder.setResponse(ByteString.copyFrom(EntityUtils.toByteArray(postResp.getEntity())));
                    res_http_builder.setHttpagentStatuscode(postResp.getStatusLine().getStatusCode());
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_POST() Status Code" + postResp.getStatusLine().getStatusCode());

                    if (postResp.getStatusLine().getStatusCode() < 200 || postResp.getStatusLine().getStatusCode() >= 300) {
                        rv = -1;
                        res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_HTTPAGENT_ERROR.getNumber());
                        res_http_builder.setTotalBytes(totalread);
                        res_http_builder.setHttpagentResponse(-5);
                        Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_POST() DX_ERR_IO_HTTPAGENT_ERROR " + urlStr);
                        break;
                    }

                }
                catch (IllegalArgumentException iaex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_URL.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-4);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_POST() DX_ERR_BAD_URL" + urlStr, iaex);
                }
                catch (ClientProtocolException cpex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_REQUEST.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-3);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_POST() DX_ERR_BAD_REQUEST " + urlStr, cpex);
                    break;
                }
                catch (IOException iex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_ERROR.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-2);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_POST() DX_ERR_IO_ERROR " + urlStr, iex);
                    break;
                }
                catch (Exception ex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_UNEXPECTED_ERROR.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-1);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_POST() DX_ERR_UNEXPECTED_ERROR " + urlStr, ex);
                    break;
                }
                if (rv ==0) {
                    res_http_builder.setErrorCode(ErrorCode.DX_SUCCESS.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(0);
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_POST() DX_SUCCESS " + urlStr);
                }
                break;

            case HttpAgentCommandType.DX_HTTP_DELETE_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_DELETE() Start");
                rv = 0;
                urlStr = httpReq.getUrl();
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_DELETE() urlStr: " + urlStr);

                try {

                    DefaultHttpClient httpDeleteClient = new DefaultHttpClient();
                    HttpRequestBase method = new HttpDelete(urlStr);

                    for (int deleteHeaderIdx = 0; deleteHeaderIdx < httpReq.getHeadersCount(); ++deleteHeaderIdx) {
                        String tmpHeader = httpReq.getHeaders(deleteHeaderIdx);
                        if (tmpHeader.indexOf(':') == -1) {
                            continue;
                        }

                        String tmpHeaderName = tmpHeader.substring(0, tmpHeader.indexOf(':'));
                        String tmpHeaderValue = tmpHeader.substring(tmpHeader.indexOf(':') + 2, tmpHeader.length());
                        method.setHeader(tmpHeaderName, tmpHeaderValue);
                    }

                    HttpResponse deleteResp = httpDeleteClient.execute(method);
                    res_http_builder.setResponse(ByteString.copyFrom(EntityUtils.toByteArray(deleteResp.getEntity())));
                    res_http_builder.setHttpagentStatuscode(deleteResp.getStatusLine().getStatusCode());
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_DELETE() Status Code " + deleteResp.getStatusLine().getStatusCode());

                    if (deleteResp.getStatusLine().getStatusCode() < 200 || deleteResp.getStatusLine().getStatusCode() >= 300) {
                        rv = -1;
                        res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_HTTPAGENT_ERROR.getNumber());
                        res_http_builder.setTotalBytes(totalread);
                        res_http_builder.setHttpagentResponse(-5);
                        Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_DELETE() DX_ERR_IO_HTTPAGENT_ERROR " + urlStr);
                        break;
                    }


                }
                catch (IllegalArgumentException iaex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_URL.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-4);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_DELETE() DX_ERR_BAD_URL " + urlStr, iaex);
                }
                catch (ClientProtocolException cpex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_BAD_REQUEST.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-3);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_DELETE() DX_ERR_BAD_REQUEST " + urlStr, cpex);
                    break;
                }
                catch (IOException iex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_IO_ERROR.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-2);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_DELETE() DX_ERR_IO_ERROR " + urlStr, iex);
                    break;
                }
                catch (Exception ex) {
                    res_http_builder.setErrorCode(ErrorCode.DX_ERR_UNEXPECTED_ERROR.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(-1);
                    Log.e(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_DELETE() DX_ERR_UNEXPECTED_ERROR " + urlStr, ex);
                    break;
                }
                if (rv == 0) {
                    res_http_builder.setErrorCode(ErrorCode.DX_SUCCESS.getNumber());
                    res_http_builder.setTotalBytes(totalread);
                    res_http_builder.setHttpagentResponse(0);
                    Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_HTTP_DELETE() DX_SUCCESS " + urlStr);
                }
                break;

            default:
                res_http_builder.setErrorCode(ErrorCode.DX_ERR_UNKNOWN_REQUEST_TYPE.getNumber());
                Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE::DX_ERR_UNKNOWN_REQUEST_TYPE");
                break;
            }

            setDxHttpGetResponse(sockOutput, res_http_builder);
        }

        private void handleQueryDevice() throws Exception
        {
            // Accept query length to clear stream. Should be zero.
            int request_size = sockInput.readInt();
            if(request_size != 0) {
                Log.w(LOG_TAG, "DX_REQUEST_QUERY_DEVICE_VALUE has request size " +
                      request_size + ", not 0 as expected.");
                byte[] reqQueryProtocolbuf = new byte[request_size];
                sockInput.readFully(reqQueryProtocolbuf, 0, request_size);
            }

            try {
                byte[] resbuf =
                    QueryDeviceOutput.newBuilder()
                    .setDeviceName(android.os.Build.MANUFACTURER + "/" + android.os.Build.MODEL)
                    .setDeviceClass("Phone")
                    .setOsVersion("Android")
                    .setIsAcerDevice(true)
                    .setDeviceHasCamera(true)
                    .build().toByteArray();
                sockOutput.writeInt(resbuf.length);
                sockOutput.write(resbuf);
            } catch (Exception e) {
                Log.e(LOG_TAG, "DX_REQUEST_QUERY_DEVICE_VALUE:: QueryDeviceOutput failed: ", e);
                sockOutput.writeInt(0);  // something went wrong - no (0 bytes) response
            }
        }

        private void handleDxRemoteProtocol() throws Exception
        {
            int rc = 0;
            int dxremote_protocol_size = sockInput.readInt();
            String playlist_dirpath;
            String ccdiservice_dirpath;
            DxRemoteMessage.Builder res_builder = DxRemoteMessage.newBuilder();
            byte[] reqProtocolbuf = new byte[dxremote_protocol_size];
            sockInput.readFully(reqProtocolbuf, 0, dxremote_protocol_size);
            DxRemoteMessage req = DxRemoteMessage.parseFrom(reqProtocolbuf);
            res_builder.setCommand(req.getCommand());
            switch(req.getCommand().getNumber()) {
            case Command.LAUNCH_PROCESS_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::LAUNCH_PROCESS() Start");
                {
                    String filename = req.getArgument(0).getValue().toLowerCase();
                    Log.i(LOG_TAG, "filename: " + filename);
                    if(filename.equals("ccd.exe")){
                        CcdiClientRemoteBinder mClient = new CcdiClientRemoteBinder(StatusActivity.this);
                        Log.i(LOG_TAG, "Starting CCD");
                        mClient.startCcdiService();
                        Log.i(LOG_TAG, "done starting CCD");
                        mClient.unbindService();
                    }
                }
                res_builder.setVplReturnCode(0);    // VPL_OK
                break;

            case Command.KILL_PROCESS_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::KILL_PROCESS() Start");
                {
                    String filename = req.getArgument(0).getValue().toLowerCase();
                    Log.i(LOG_TAG, "filename: " + filename);
                    if(filename.equals("ccd.exe")){
                        CcdiClientRemoteBinder mClient = new CcdiClientRemoteBinder(StatusActivity.this);
                        Log.i(LOG_TAG, "Stopping CCD");
                        mClient.stopCcdiService();
                        Log.i(LOG_TAG, "done stopping CCD");
                        mClient.unbindService();
                    }
                }
                res_builder.setVplReturnCode(0);    // VPL_OK
                break;

            case Command.VPLFS_OPENDIR_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFS_OPENDIR() Start");
                String openPath = req.getDirFolder().getDirPath();
                File dirOpen = new File(openPath);
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFS_OPENDIR() openPath: " + openPath);

                if (!dirOpen.exists()) {
                    rc = -9024; // VPL_ERR_NOENT
                    res_builder.setVplReturnCode(rc);
                    break;
                }
                res_builder.setVplReturnCode(0);    // VPL_OK

                String[] children = dirOpen.list();
                Queue<String> qChildren = new LinkedList<String>();
                for (String childPath : children) {
                    qChildren.offer(childPath);
                }
                dirMap.put(openPath, qChildren);
                break;

            case Command.VPLFS_READDIR_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFS_READDIR() Start");
                String readPath = req.getDirFolder().getDirPath();
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFS_READDIR() readPath: " + readPath);
                if (!dirMap.containsKey(readPath)) {
                    rc = -9024; // VPL_ERR_NOENT
                    res_builder.setVplReturnCode(rc);
                    break;
                }
                if (dirMap.get(readPath).isEmpty()) {
                    rc = -9009; // VPL_ERR_MAX
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                res_builder.setVplReturnCode(0);    // VPL_OK

                String returnChildPath = dirMap.get(readPath).element();
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFS_READDIR() \""
                      + readPath + "/" + returnChildPath + "\"");
                dirMap.get(readPath).poll();
                File returnFileChild = new File(readPath + "/" + returnChildPath);
                DxRemoteMessage.DxRemote_VPLFS_dirent_t readFileArg =
                    DxRemoteMessage.DxRemote_VPLFS_dirent_t.newBuilder()
                    .setType(returnFileChild.isDirectory() ? DxRemoteMessage.DxRemote_VPLFS_file_type_t.DxRemote_VPLFS_TYPE_DIR : DxRemoteMessage.DxRemote_VPLFS_file_type_t.DxRemote_VPLFS_TYPE_FILE )
                    .setFilename(returnChildPath).build();
                res_builder.setFolderDirent(readFileArg);
                break;

            case Command.VPLFS_CLOSEDIR_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFS_CLOSEDIR() Start");
                String closePath = req.getDirFolder().getDirPath();
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFS_CLOSEDIR() closePath: " + closePath);
                if (!dirMap.containsKey(closePath)) {
                    rc = -9024; // VPL_ERR_NOENT
                    res_builder.setVplReturnCode(rc);
                }

                res_builder.setVplReturnCode(0);    // VPL_OK
                dirMap.remove(closePath);
                break;

            case Command.VPLFS_STAT_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFS_STAT() Start");
                if (req.getArgumentCount() < 1) {
                    rc = -9007; // VPL_ERR_INVALID
                    res_builder.setVplReturnCode(rc);
                    break;
                }
                String statPath = req.getArgument(0).getValue();
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFS_STAT() statPath: " + statPath);

                /*
                 * Special case:
                 *      The dx_remote_agent can not access playlist folder (under /data/data/com.igware.android_cc_sdk.example_service_only/... ),
                 *      therefor, we need to send Intent to CcdiService and let CcdiService handle.
                 */
                playlist_dirpath = getMediaPlaylistPath();
                if (playlist_dirpath != null && statPath.startsWith(playlist_dirpath)) {
                    // Send Intent to CcdiService and retrieve file state from CcdiService.
                    DxRemoteMessage.DxRemote_VPLFS_stat_t.Builder file_stat_builder =
                            DxRemoteMessage.DxRemote_VPLFS_stat_t.newBuilder();
                    rc = requestCcdiServiceStatFileAndWait(statPath, file_stat_builder);
                    res_builder.setFileStat(file_stat_builder.build());
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                File statFile = new File(statPath);
                if (!statFile.exists()) {
                    rc = -9024; // VPL_ERR_NOENT
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                DxRemoteMessage.DxRemote_VPLFS_stat_t stat =
                    DxRemoteMessage.DxRemote_VPLFS_stat_t.newBuilder()
                    .setSize(statFile.length())
                    .setAtime(0)
                    .setMtime(statFile.lastModified())
                    .setCtime(0)
                    .setType(statFile.isFile() ? DxRemote_VPLFS_file_type_t.DxRemote_VPLFS_TYPE_FILE : DxRemote_VPLFS_file_type_t.DxRemote_VPLFS_TYPE_DIR)
                    .setIsHidden(statFile.isHidden() ? 1 : 0)
                    .setIsSymLink((statFile.getCanonicalFile().equals(statFile.getAbsoluteFile())) ? 0 : 1)
                    .build();

                res_builder.setFileStat(stat);
                res_builder.setVplReturnCode(0);
                break;

            case Command.UTIL_RM_DASH_RF_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::UTIL_RM_DASH_RF() Start");
                if (req.getArgumentCount() < 1) {
                    rc = -9007; // VPL_ERR_INVALID
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                String rmDashRFPath = req.getArgument(0).getValue();
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::UTIL_RM_DASH_RF() rmDashRFPath: " + rmDashRFPath);

                /*
                 * Special case:
                 *      The dx_remote_agent can not access playlist folder (under /data/data/com.igware.android_cc_sdk.example_service_only/... ),
                 *      therefor, we need to send Intent to CcdiService and let CcdiService handle.
                 */
                playlist_dirpath = getMediaPlaylistPath();
                if (playlist_dirpath != null && rmDashRFPath.startsWith(playlist_dirpath)) {
                    // Send Intent to CcdiService and let CcdiService delete such folder.
                    rc = requestCcdiServiceDeleteDirAndWait(rmDashRFPath);
                    break;
                }

                File rmDir = new File(rmDashRFPath);
                if (!rmDir.exists()) {
                    rc = -9024; // VPL_ERR_NOENT
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                Queue<String> dirs = new LinkedList<String>();
                Stack<String> emptyDirs = new Stack<String>();

                if(rmDir.isFile()) {
                    // Delete the file.
                    rmDir.delete();
                }
                else {
                    // prime the deletion process
                    dirs.add(rmDashRFPath);
                }

                while (!dirs.isEmpty()) {
                    String currDir = dirs.element();
                    dirs.remove();
                    Log.i(LOG_TAG, "Clearing directory \"" + currDir + "\"");
                    File currFileDir = new File(currDir);
                    String[] subDirs = currFileDir.list();
                    int dirAdded = 0;
                    for (String element : subDirs) {
                        String strSubToDelete = currDir + "/" + element;
                        File tempFile = new File(strSubToDelete);
                        // add directories to the delete later stack
                        // delete files immediately
                        if(tempFile.isDirectory()) {
                            dirs.add(strSubToDelete);
                            Log.i(LOG_TAG, "Add dir \"" + strSubToDelete + "\" for delete.");
                        }
                        else {
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

                res_builder.setVplReturnCode(0);    // VPL_OK
                break;

            case Command.VPLDIR_CREATE_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLDIR_CREATE() Start");
                if (req.getArgumentCount() < 1) {
                    rc = -9007; // VPL_ERR_INVALID
                    res_builder.setVplReturnCode(rc);
                    break;
                }
                String createPath = req.getArgument(0).getValue();
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLDIR_CREATE() createPath: " + createPath);

                /*
                 * Special case:
                 *      The dx_remote_agent can not access playlist folder (under /data/data/com.igware.android_cc_sdk.example_service_only/... ),
                 *      therefor, we need to send Intent to CcdiService and let CcdiService handle.
                 */
                ccdiservice_dirpath = getCcdiServicePath();
                Log.i(LOG_TAG, "createPath: " + createPath);
                Log.i(LOG_TAG, "ccdiservice_dirpath: " + ccdiservice_dirpath);
                if (ccdiservice_dirpath != null && createPath.startsWith(ccdiservice_dirpath)) {
                    // Send Intent to CcdiService and let CcdiService create such folder.
                    rc = requestCcdiServiceCreateDirAndWait(createPath);
                    break;
                }

                File createDirFile = new File(createPath);
                if (!createDirFile.mkdir()) {
                    if (createDirFile.exists() && createDirFile.isDirectory()) {
                        rc = -9043; // VPL_ERR_EXIST
                    } else {
                        rc = -9099; // VPL_ERR_FAIL
                    }
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                res_builder.setVplReturnCode(0);    // VPL_OK
                break;

            case Command.VPLFILE_RENAME_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFILE_RENAME() Start");

                /*
                 * Special case:
                 *      The dx_remote_agent can not access playlist folder (under /data/data/com.igware.android_cc_sdk.example_service_only/... ),
                 *      therefor, we need to send Intent to CcdiService and let CcdiService handle.
                 */
                playlist_dirpath = getMediaPlaylistPath();
                if (playlist_dirpath != null && (req.getRenameSource().startsWith(playlist_dirpath) || req.getRenameDestination().startsWith(playlist_dirpath))) {
                    // Send Intent to CcdiService and let CcdiService to perform file rename.
                    rc = requestCcdiServiceRenameFileAndWait(req.getRenameSource(), req.getRenameDestination());
                    break;
                }

                File srcRenameFile = new File(req.getRenameSource());
                File dstRenameFile = new File(req.getRenameDestination());
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFILE_RENAME() srcRenameFile: " + req.getRenameSource()
                      + " dstRenameFile: " + req.getRenameDestination());

                if (!srcRenameFile.renameTo(dstRenameFile)) {
                    rc = -9099; // VPL_ERR_FAIL
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                res_builder.setVplReturnCode(0);    // VPL_OK
                break;

            case Command.COPYFILE_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::COPYFILE() Start");
                if (req.getArgumentCount() < 2) {
                    rc = -9007; // VPL_ERR_INVALID
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                /*
                 * Special case:
                 *      The dx_remote_agent can not access playlist folder (under /data/data/com.igware.android_cc_sdk.example_service_only/... ),
                 *      therefor, we need to send Intent to CcdiService and let CcdiService handle.
                 */
                playlist_dirpath = getMediaPlaylistPath();
                if (playlist_dirpath != null && (req.getArgument(0).getValue().startsWith(playlist_dirpath) || req.getArgument(1).getValue().startsWith(playlist_dirpath))) {
                    // Send Intent to CcdiService and let CcdiService to perform file copy.
                    rc = requestCcdiServiceCopyFileAndWait(req.getArgument(0).getValue(), req.getArgument(1).getValue());
                    break;
                }

                File srcCopyFile = new File(req.getArgument(0).getValue());
                File dstCopyFile = new File(req.getArgument(1).getValue());
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFILE_RENAME() srcCopyFile: " + req.getArgument(0).getValue()
                      + " dstCopyFile: " + req.getArgument(1).getValue());
                InputStream in = new FileInputStream(srcCopyFile);
                rc = -104;
                res_builder.setVplReturnCode(4);

                OutputStream out = new FileOutputStream(dstCopyFile);

                byte[] bufCopy = new byte[1024];

                int copyLen = 0;
                while ( (copyLen = in.read(bufCopy)) > 0) {
                    out.write(bufCopy, 0, copyLen);
                }

                out.close();
                in.close();
                res_builder.setVplReturnCode(0);    // VPL_OK
                break;

            case Command.VPLFILE_DELETE_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFILE_DELETE() Start");
                if (req.getArgumentCount() < 1) {
                    rc = -9007; // VPL_ERR_INVALID
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                /*
                 * Special case:
                 *      The dx_remote_agent can not access playlist folder (under /data/data/com.igware.android_cc_sdk.example_service_only/... ),
                 *      therefor, we need to send Intent to CcdiService and let CcdiService handle.
                 */
                playlist_dirpath = getMediaPlaylistPath();
                if (playlist_dirpath != null && req.getArgument(0).getValue().startsWith(playlist_dirpath)) {
                    // Send Intent to CcdiService and let CcdiService to perform file delete.
                    rc = requestCcdiServiceDeleteFileAndWait(req.getArgument(0).getValue());
                    break;
                }

                File deleteFile = new File(req.getArgument(0).getValue());
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPLFILE_DELETE() deleteFile: " + req.getArgument(0).getValue());
                if (!deleteFile.exists()) {
                    rc = -9024; // VOL_ERR_NOENT
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                if (!deleteFile.delete()) {
                    rc = -9099; // VPL_ERR_FAIL
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                res_builder.setVplReturnCode(0);    // VPL_OK
                break;

            case Command.GET_UPLOAD_PATH_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::GET_UPLOAD_PATH() Start");
                String uploadPath = Environment.getExternalStorageDirectory().getPath()
                    +"/AOP/AcerCloud/dxshell_pushfiles";
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::GET_UPLOAD_PATH() uploadPath: " + uploadPath);
                File fUploadPath = new File(uploadPath);
                fUploadPath.mkdirs();
                DxRemoteMessage.DxRemoteArgument uploadpathArg =
                    DxRemoteMessage.DxRemoteArgument.newBuilder()
                    .setName(ArgumentName.DXARGUMENTDIRNAME)
                    .setValue(uploadPath).build();
                res_builder.addArgument(uploadpathArg);
                res_builder.setVplReturnCode(0);    // VPL_OK
                break;

            case Command.GET_CCD_ROOT_PATH_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::GET_CCD_ROOT_PATH() Start");
                String ccdRootPath = Environment.getExternalStorageDirectory().getPath() +"/AOP/AcerCloud";
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::GET_CCD_ROOT_PATH() ccdRootPath: " + ccdRootPath);
                DxRemoteMessage.DxRemoteArgument ccdRootPathArg =
                    DxRemoteMessage.DxRemoteArgument.newBuilder()
                    .setName(ArgumentName.DXARGUMENTDIRNAME)
                    .setValue(ccdRootPath).build();
                res_builder.addArgument(ccdRootPathArg);
                res_builder.setVplReturnCode(0);    // VPL_OK
                break;

            case Command.SET_CLEARFI_MODE_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::SET_CLEARFI_MODE() Start");
                if (req.getArgumentCount() < 1) {
                    rc = -9007; // VPL_ERR_INVALID
                    res_builder.setVplReturnCode(rc);
                    break;
                }

                String modeName = "clearfiMode";
                String modeType = req.getArgument(0).getValue();
                String config;
                String editMode = modeName + "=" + modeType;
                String ccdConfPath = Environment.getExternalStorageDirectory().getPath()
                    +"/AOP/AcerCloud/conf/ccd.conf";
                try {
                    File oldConf = new File(ccdConfPath);
                    FileInputStream fis = new FileInputStream(oldConf);
                    BufferedInputStream bis = new BufferedInputStream(fis);
                    DataInputStream dis = new DataInputStream(bis);

                    StringBuilder sb = new StringBuilder(2048);
                    while (dis.available() > 0) {
                        sb.append((char)dis.readByte());
                    }


                    dis.close();
                    bis.close();
                    fis.close();

                    if (sb.indexOf(modeName) < 0) {
                        sb.append(editMode);
                        sb.append("\n");
                    }
                    else {
                        sb.replace(sb.indexOf(modeName), sb.indexOf("\n", sb.indexOf(modeName)), editMode);
                    }

                    config = sb.toString();

                    FileOutputStream fos = new FileOutputStream(ccdConfPath, false);
                    BufferedOutputStream bos = new BufferedOutputStream(fos);
                    DataOutputStream dos = new DataOutputStream(bos);

                    dos.write(config.getBytes());
                    dos.close();
                    bos.close();
                    fos.close();
                }
                catch (Exception e) {
                    rc = -9099;
                }
                break;

            default:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::SET_CLEARFI_MODE() VPL_ERR_INVALID");
                rc = -9007; // VPL_ERR_INVALID
                res_builder.setVplReturnCode(rc);
                break;
            }

            if (rc == 0) {
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPL_OK");
            }
            else {
                Log.w(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE::VPL_FAILED (" + rc + ")");
            }

            byte[] resProtocolbuf = res_builder.build().toByteArray();
            sockOutput.writeInt(resProtocolbuf.length);
            if (resProtocolbuf.length > 0) {
                sockOutput.write(resProtocolbuf);
            }
        }

        private void handleDxRemoteTransferFiles() throws Exception
        {
            int rcInFile = 0;
            int dxremote_transfile_size = sockInput.readInt();
            long totalSize;
            DxRemoteFileTransfer.Builder res_transfer_builder = DxRemoteFileTransfer.newBuilder();
            byte[] reqFilebuf = new byte[dxremote_transfile_size];
            File tmpfile = null;
            String playlist_dirpath;
            sockInput.readFully(reqFilebuf, 0, dxremote_transfile_size);
            DxRemoteFileTransfer reqTransFile = DxRemoteFileTransfer.parseFrom(reqFilebuf);
            res_transfer_builder.setType(reqTransFile.getType());
            res_transfer_builder.setVplReturnCode(0);

            switch(reqTransFile.getType().getNumber()) {
            case DxRemoteAgentFileTransfer_Type.DX_REMOTE_PUSH_FILE_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_TRANSFER_FILES_VALUE::DX_REMOTE_PUSH_FILE() Start");
                res_transfer_builder.setRawError(0);
                byte[] resTransFilePushBuf = res_transfer_builder.build().toByteArray();
                sockOutput.writeInt(resTransFilePushBuf.length);
                if (resTransFilePushBuf.length > 0) {
                    sockOutput.write(resTransFilePushBuf);
                }

                totalSize = reqTransFile.getFileSize();
                long recvSize = 0;
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_TRANSFER_FILES_VALUE::DX_REMOTE_PUSH_FILE() FiletotalSize: " + totalSize);

                playlist_dirpath = getMediaPlaylistPath();
                BufferedOutputStream bofFile;
                if (playlist_dirpath != null && reqTransFile.hasPathOnAgent() && reqTransFile.getPathOnAgent().startsWith(playlist_dirpath)) {
                    String tmpfile_prefix = String.format("%d", System.currentTimeMillis());
                    tmpfile = File.createTempFile(tmpfile_prefix, ".tmp", Environment.getExternalStorageDirectory());
                    bofFile = new BufferedOutputStream(new FileOutputStream(tmpfile));
                    Log.i(LOG_TAG, "Create tmp file: " + tmpfile.getAbsolutePath());
                    Log.i(LOG_TAG, "Target filepath: " + reqTransFile.getPathOnAgent());
                } else {
                    bofFile = new BufferedOutputStream(new FileOutputStream(reqTransFile.getPathOnAgent()));
                }
                
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_TRANSFER_FILES_VALUE::DX_REMOTE_PUSH_FILE() PushFilePath: " + reqTransFile.getPathOnAgent());

                while (recvSize < totalSize) {
                    long recvThisRound = DxRemoteAgentPacket.DX_REMOTE_FILE_TRANS_PKT_SIZE_VALUE;
                    if ( (totalSize - recvSize) < recvThisRound) {
                        recvThisRound = totalSize - recvSize;
                    }
                    byte[] reqPushTmp = new byte[DxRemoteAgentPacket.DX_REMOTE_FILE_TRANS_PKT_SIZE_VALUE];
                    rcInFile = sockInput.read(reqPushTmp, 0, (int)recvThisRound);
                    bofFile.write(reqPushTmp, 0, rcInFile);
                    
                    long writeThisRound = rcInFile;
                    recvSize += writeThisRound;
                }

                bofFile.close();                    

                if (tmpfile != null) {
                    int rv = requestCcdiServiceCopyFileAndWait(tmpfile.getAbsolutePath(), reqTransFile.getPathOnAgent());
                    tmpfile.delete();
                    if (rv < 0) {
                        Log.e(LOG_TAG, String.format("Failed to copy file. (source: %s, target: %s)", tmpfile.getAbsolutePath(), reqTransFile.getPathOnAgent()));
                    }
                }

                sockOutput.writeLong(recvSize);
                break;

            case DxRemoteAgentFileTransfer_Type.DX_REMOTE_GET_FILE_VALUE:
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_TRANSFER_FILES_VALUE::DX_REMOTE_GET_FILE() Start");
                res_transfer_builder.setRawError(0);
                File tmGetFile = null;
                playlist_dirpath = getMediaPlaylistPath();
                if (playlist_dirpath != null && reqTransFile.hasPathOnAgent() && reqTransFile.getPathOnAgent().startsWith(playlist_dirpath)) {
                    String tmp_filename = String.format("%s/%d.tmp", Environment.getExternalStorageDirectory().getAbsolutePath(), System.currentTimeMillis());
                    Log.i(LOG_TAG, "Target filepath: " + reqTransFile.getPathOnAgent());
                    Log.i(LOG_TAG, "Move to tmp file: " + tmp_filename);
                    int rv = requestCcdiServiceCopyFileAndWait(reqTransFile.getPathOnAgent(), tmp_filename);
                    if (rv < 0) {
                        Log.e(LOG_TAG, String.format("Failed to copy file. (source: %s, target: %s)", tmpfile.getAbsolutePath(), reqTransFile));
                        break;
                    }
                    tmGetFile = new File(tmp_filename);
                    tmGetFile.deleteOnExit();
                } else {
                    tmGetFile = new File(reqTransFile.getPathOnAgent());
                }
                Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_TRANSFER_FILES_VALUE::DX_REMOTE_GET_FILE() PullFilePath: " + reqTransFile.getPathOnAgent());

                totalSize = 0;
                long sentSize = 0;
                if (tmGetFile.exists()) {
                    totalSize = tmGetFile.length();
                    Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_TRANSFER_FILES_VALUE::DX_REMOTE_GET_FILE() FiletotalSize: " + totalSize);
                }
                res_transfer_builder.setFileSize(totalSize);
                byte[] resTransFileGetbuf = res_transfer_builder.build().toByteArray();
                sockOutput.writeInt(resTransFileGetbuf.length);
                if (resTransFileGetbuf.length > 0) {
                    sockOutput.write(resTransFileGetbuf);
                }

                BufferedInputStream bifFile = new BufferedInputStream(new FileInputStream(tmGetFile));

                while (sentSize < totalSize) {
                    long readThisRound = 0, remoteRecv = 0;
                    byte[] reqGetTmp = new byte[DxRemoteAgentPacket.DX_REMOTE_FILE_TRANS_PKT_SIZE_VALUE];
                    readThisRound = bifFile.read(reqGetTmp, 0, DxRemoteAgentPacket.DX_REMOTE_FILE_TRANS_PKT_SIZE_VALUE);
                    sockOutput.write(reqGetTmp, 0, (int)readThisRound);
                    sentSize += readThisRound;
                }
                long remoteRecv = sockInput.readLong();
                if(remoteRecv != sentSize){
                    Log.e(LOG_TAG, "DX_REQUEST_DXREMOTE_TRANSFER_FILES_VALUE::DX_REMOTE_GET_FILE() size mismatch, remoteRecv:"+remoteRecv+"sentSize:"+sentSize);
                }
                break;

            default:
                res_transfer_builder.setVplReturnCode(-1);
                res_transfer_builder.setRawError(0);
                byte[] resTransFileOtherbuf = res_transfer_builder.build().toByteArray();
                sockOutput.writeInt(resTransFileOtherbuf.length);
                if (resTransFileOtherbuf.length > 0) {
                    sockOutput.write(resTransFileOtherbuf);
                }
                break;
            }
        }

        public void run()
        {
            try {
                this.bufferedInputStream = new BufferedInputStream(connectedSocket.getInputStream());
                this.sockInput = new DataInputStream(bufferedInputStream);
                this.sockOutput = new DataOutputStream(connectedSocket.getOutputStream());

                while(!connectedSocket.isClosed()) {
                    int type = bufferedInputStream.read();
                    if(type == -1) {
                        // end of stream.
                        break;
                    }

                    Log.i(LOG_TAG, "Start request type: " + type);

                    synchronized (StatusActivity.this) {
                        mNumRequests++;
                    }

                    switch (type) {
                    case RequestType.DX_REQUEST_PROTORPC_VALUE: // protorpc
                        Log.i(LOG_TAG, "DX_REQUEST_PROTORPC_VALUE");
                        handleProtoRpc();
                        break;

                    case RequestType.DX_REQUEST_HTTP_GET_VALUE:
                        Log.i(LOG_TAG, "DX_REQUEST_GET_VALUE");
                        handleHttpGet();
                        break;

                    case RequestType.DX_REQUEST_QUERY_DEVICE_VALUE:
                        Log.i(LOG_TAG, "DX_REQUEST_QUERY_DEVICE_VALUE");
                        handleQueryDevice();
                        break;

                    case RequestType.DX_REQUEST_DXREMOTE_PROTOCOL_VALUE:
                        Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_PROTOCOL_VALUE");
                        handleDxRemoteProtocol();
                        break;

                    case RequestType.DX_REQUEST_DXREMOTE_TRANSFER_FILES_VALUE:
                        Log.i(LOG_TAG, "DX_REQUEST_DXREMOTE_TRANSFER_FILES_VALUE");
                        handleDxRemoteTransferFiles();
                        break;

                    default:
                        Log.w(LOG_TAG, "unknown request type: " + type);
                        sockOutput.writeInt(ErrorCode.DX_ERR_UNKNOWN_REQUEST_TYPE_VALUE);
                        break;
                    }

                    synchronized (StatusActivity.this) {
                        mNumResponses++;
                    }
                    updateUi();

                    Log.i(LOG_TAG, "Done request type: " + type);

                    // TEMP: Only process one transaction per connection.
                    break;
                }
            } catch (Exception e) {
                e.printStackTrace();
                Log.w(LOG_TAG, "unexpected exception: " + e);
            }

            try {
                connectedSocket.close();
            } catch (IOException e) {
                Log.w(LOG_TAG, "failed to close socket: " + e);
            }

            Log.i(LOG_TAG, "End handling for connection (#" + id + ")");
        }
    }

    private class SocketListener implements Runnable
    {
        private volatile ServerSocket dxAgentSocket = null;

        public void run()
        {
            try {
                dxAgentSocket = new ServerSocket();
                dxAgentSocket.setReuseAddress(true);
                dxAgentSocket.bind(new InetSocketAddress(24000));

                StringBuilder statusMsg = new StringBuilder("Listening at " + dxAgentSocket.getLocalSocketAddress() + "\n");
                Enumeration<NetworkInterface> nets = NetworkInterface.getNetworkInterfaces();
                for (NetworkInterface netint : Collections.list(nets)) {
                    getInterfaceInformation(netint, statusMsg);
                }
                setStatusMessage(statusMsg.toString());
                Log.i(LOG_TAG, "SocketListener listening.");

                while(!dxAgentSocket.isClosed()) {
                    try {
                        Socket connectedSocket = dxAgentSocket.accept();
                        mNumConnections++;
                        updateUi();
                        Log.i(LOG_TAG, "Connection (#" + mNumConnections + ") from " + connectedSocket.getRemoteSocketAddress());
                        new Thread(new ServiceThread(connectedSocket, mNumConnections)).start();
                    } catch (SocketException e) {
                        // Socket closed.
                        Log.i(LOG_TAG, "dxAgentSocket has been closed.");
                    }
                }
            } catch (Exception e) {
                setStatusMessage("fatal unexpected exception: " + e);
            } finally {
                Log.i(LOG_TAG, "SocketListener exiting.");
                dxAgentSocket = null;
            }
        }

        public void closeSocket()
        {
            try {
                Log.i(LOG_TAG, "Close dxAgentSocket.");
                dxAgentSocket.close();
            }
            catch (Exception e) {
                Log.i(LOG_TAG, "Exception closing listener socket: " + e);
            }
        }
    }

    private class Result {
        private int returnCode;
        private Intent intent;

        public int getReturnCode() {
            return returnCode;
        }

        public void setReturnCode(int returnCode) {
            this.returnCode = returnCode;
        }

        public Intent getIntent() {
            return intent;
        }

        public void setIntent(Intent intent) {
            this.intent = intent;
        }
    }
}
