//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.acer.dx.serviceclient;

import com.acer.dx.R;
import com.acer.dx.util.CcdSdkDefines.FileExplorer;
import com.acer.dx.util.FileInfo;
import com.acer.dx.util.Utility;
import com.acer.dx.util.igware.Constants;
import com.acer.dx.util.igware.Dataset;
import com.acer.dx.util.igware.DownloadProgress;
import com.acer.dx.util.igware.Filter;
import com.acer.dx.util.igware.Subfolder;
import com.acer.dx.util.igware.Subfolder.Type;
import com.acer.dx.util.igware.SyncListItem;
import com.acer.dx.util.igware.SyncListItem.State;
import com.acer.dx.util.igware.*;

import org.json.*;

import android.content.Context;
import android.util.Log;

import igware.gvm.pb.CcdTypes.OwnedTitleMenuData;
import igware.gvm.pb.CcdiRpc.*;
import igware.gvm.pb.CcdiRpc.AddDatasetOutput;
import igware.gvm.pb.CcdiRpc.AddSyncSubscriptionInput;
import igware.gvm.pb.CcdiRpc.DatasetDirectoryEntry;
import igware.gvm.pb.CcdiRpc.DeleteSyncSubscriptionsInput;
import igware.gvm.pb.CcdiRpc.GetDatasetDirectoryEntriesInput;
import igware.gvm.pb.CcdiRpc.GetDatasetDirectoryEntriesOutput;
import igware.gvm.pb.CcdiRpc.GetOwnedTitleListDataInput;
import igware.gvm.pb.CcdiRpc.GetOwnedTitleListDataOutput;
import igware.gvm.pb.CcdiRpc.GetPersonalCloudStateInput;
import igware.gvm.pb.CcdiRpc.GetPersonalCloudStateOutput;
import igware.gvm.pb.CcdiRpc.GetSyncStateInput;
import igware.gvm.pb.CcdiRpc.GetSyncStateOutput;
import igware.gvm.pb.CcdiRpc.GetSystemStateInput;
import igware.gvm.pb.CcdiRpc.GetSystemStateOutput;
import igware.gvm.pb.CcdiRpc.InfraHttpRequestInput;
import igware.gvm.pb.CcdiRpc.InfraHttpRequestMethod_t;
import igware.gvm.pb.CcdiRpc.InfraHttpRequestOutput;
import igware.gvm.pb.CcdiRpc.InfraHttpService_t;
import igware.gvm.pb.CcdiRpc.LinkDeviceInput;
import igware.gvm.pb.CcdiRpc.ListOwnedDatasetsInput;
import igware.gvm.pb.CcdiRpc.ListOwnedDatasetsOutput;
import igware.gvm.pb.CcdiRpc.ListSyncSubscriptionsInput;
import igware.gvm.pb.CcdiRpc.ListSyncSubscriptionsOutput;
import igware.gvm.pb.CcdiRpc.LoginInput;
import igware.gvm.pb.CcdiRpc.LoginOutput;
import igware.gvm.pb.CcdiRpc.LogoutInput;
import igware.gvm.pb.CcdiRpc.NewDatasetType_t;
import igware.gvm.pb.CcdiRpc.NoParamRequest;
import igware.gvm.pb.CcdiRpc.NoParamResponse;
import igware.gvm.pb.CcdiRpc.SyncSubscriptionDetail;
import igware.gvm.pb.CcdiRpc.SyncSubscriptionType_t;
import igware.gvm.pb.CcdiRpc.UnlinkDeviceInput;
import igware.gvm.pb.CcdiRpc.UpdateSyncSettingsInput;
import igware.gvm.pb.CcdiRpc.UpdateSyncSettingsOutput;
import igware.gvm.pb.CcdiRpc.UpdateSyncSubscriptionInput;
import igware.gvm.pb.CcdiRpc.UpdateSystemStateInput;
import igware.gvm.pb.CcdiRpc.UpdateSystemStateOutput;
import igware.gvm.pb.CcdiRpcClient.CCDIServiceClient;
import igware.protobuf.AppLayerException;
import igware.protobuf.ProtoRpcException;
import igware.vplex.pb.VsDirectoryServiceTypes.*;
import igware.vplex.pb.VsDirectoryServiceTypes.DatasetDetail;
import igware.vplex.pb.VsDirectoryServiceTypes.DatasetType;
import igware.vplex.pb.VsDirectoryServiceTypes.Subscription;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.*;
import java.io.*;
import java.net.URLEncoder;


public class CcdiClient {

    public static final int ERROR_CODE_RPC_LAYER = -100;

    private static final String LOG_TAG = "CcdiClient";

    public static final String CONTENT_TYPE_CAMERA = "CAMERA";
    
    public static long LAST_LOGIN_USERID = 0;

    private final CcdiClientRemoteBinder mClient;
    private final Context mContext;

    public CcdiClient(Context a) {
    	mClient = new CcdiClientRemoteBinder(a);
        mContext = a;
    }

    private CCDIServiceClient getCcdiRpcClient() throws ProtoRpcException {
        return mClient.getCcdiRpcClient();
    }

    public void onCreate() {
        mClient.startCcdiService();
    }

    public void onStart() {
        // From Android SDK docs:
        // "If you only need to interact with the service while your activity is visible, you should
        //  bind during onStart() and unbind during onStop().
        //  If you want your activity to receive responses even while it is stopped in the background,
        //  then you can bind during onCreate() and unbind during onDestroy(). Beware that this implies
        //  that your activity needs to use the service the entire time it's running (even in the
        //  background), so if the service is in another process, then you increase the weight of 
        //  the process and it becomes more likely that the system will kill it.
        //  Note: You should usually not bind and unbind during your activity's onResume() and onPause()..."
        mClient.bindService();
    }

    public void onResume() {
    }

    public void onPause() {
    }

    public void onStop() {
        mClient.unbindService();
    }

    public void onDestroy() {
    }
    
    public int createAccount(String userName, String password,
            String confirmpassword, String email, String regKey) {

        try {
            /*
            * //Comment this out for now until posting works //Note: need to
            * post in param=value&param2=value2 format as opposed to JSON
            * format! StringBuilder jsonBuilder = new StringBuilder();
            * jsonBuilder.append('{'); jsonBuilder.append("\"userName\":");
            * jsonBuilder.append("\"" + userName + "\",");
            * jsonBuilder.append("\"userPwd\":"); jsonBuilder.append("\"" +
            * password + "\","); if (email != null && email.length() > 0) {
            * jsonBuilder.append("\"userEmail\":"); jsonBuilder.append("\"" +
            * email + "\""); } jsonBuilder.append('}');
            * 
            * Log.d(LOG_TAG, jsonBuilder.toString());
            */

            String postData = "userName=" + userName + "&userPwd=" + password
                    + "&reenterUserPwd=" + confirmpassword + "&userEmail="
                    + email;
            Log.i(LOG_TAG, "create account form data: " + postData);

            InfraHttpRequestInput request = InfraHttpRequestInput
                    .newBuilder()
                    .setSecure(true)
                    .setPrivilegedOperation(false)
                    // .setMethod(InfraHttpRequestMethod_t.INFRA_HTTP_METHOD_POST)
                    .setMethod(InfraHttpRequestMethod_t.INFRA_HTTP_METHOD_GET)
                    .setUrlSuffix(
                            "json/register?struts.enableJSONValidation=true"
                                    + "&" + postData)
                    .setService(InfraHttpService_t.INFRA_HTTP_SERVICE_OPS)
                    .build();
            // .setPostData(postData).build();
            // .setPostData(jsonBuilder.toString()).build();

            InfraHttpRequestOutput.Builder responseBuilder = InfraHttpRequestOutput
                    .newBuilder();

            int errCode = getCcdiRpcClient()
                    .InfraHttpRequest(request, responseBuilder);
            InfraHttpRequestOutput response = responseBuilder.build();

            // TODO: normalize this.
            // We could also pass error message along to front-end if we want
            // to display more than just an error code.
            if (response.getResponseCode() != 200) {
                Log.i(LOG_TAG,
                        "response code!=200 - " + response.getResponseCode());
                return response.getResponseCode();
            }
            // if request fails at struts2 validation, return code will still be
            // 200. In this case, we need to look for "fieldErrors"
            String rspString = response.getHttpResponse();
            if (rspString != null && rspString.trim().length() > 0) {
                try {
                    // json string needs to be enclosed in {}. We need some
                    // pre-processing here since structs sometimes addes more
                    // characters.
                    rspString = rspString.substring(rspString.indexOf("{"));
                    rspString = rspString.substring(0,
                            rspString.lastIndexOf("}") + 1);
                    // end pre-processing.
                    // TODO: need future processing of the actual error message
                    // to get rid of [] and quotes added by struts2.

                    JSONObject obj = new JSONObject(rspString);
                    if (obj != null && obj.has("fieldErrors")) {
                        JSONObject errors = obj.getJSONObject("fieldErrors");
                        if (errors != null) {
                            if (errors.has("userPwd")) {
                                return Constants.ERR_CODE_REGISTER_PASSWORD;
                            }
                            if (errors.has("reenterUserPwd")) {
                                return Constants.ERR_CODE_REGISTER_PASSWORD;
                            }
                            if (errors.has("userEmail")) {
                                return Constants.ERR_CODE_REGISTER_EMAIL;
                            }
                            if (errors.has("userName")) {
                                return Constants.ERR_CODE_REGISTER_USERNAME;
                            }
                        }
                    }
                } catch (Exception e) {
                    Log.e(LOG_TAG,
                            "Error parsing http response." + e.getMessage());
                    return -1;
                }
            }
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "createAccount", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public boolean isLoggedIn() {
        return (getUserId() != 0);
    }

    public long getUserId() {
        try {
            Log.i(LOG_TAG, "getUserId begin : time = " + System.currentTimeMillis());
            GetSystemStateInput request = GetSystemStateInput.newBuilder()
                    .setGetPlayers(true).build();
            GetSystemStateOutput.Builder responseBuilder = GetSystemStateOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().GetSystemState(request,
                    responseBuilder);
            if (errCode < 0) {
                Log.e(LOG_TAG, "getUserId");
                return 0;
            }
            GetSystemStateOutput response = responseBuilder.build();
            if (response.getPlayers().getPlayersCount() >= 1) {
                Log.i(LOG_TAG, "getUserId end : time = " + System.currentTimeMillis());
                LAST_LOGIN_USERID = response.getPlayers().getPlayers(0).getUserId();
                return LAST_LOGIN_USERID;
            }
            return 0;
        } catch (AppLayerException e) {
            return 0;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getUserId", e);
            return 0;
        }
    }

    public int doLogin(String userName, String password) {
        Log.i(LOG_TAG, "doLogin begin : time = " + System.currentTimeMillis());
        try {
            LoginInput.Builder requestBuilder = LoginInput.newBuilder().setUserName(userName).setAutoLogoutIndex(true);
            if (password != null) {
                requestBuilder.setPassword(password).setSaveLoginToken(true);
            }
            LoginOutput.Builder responseBuilder = LoginOutput.newBuilder();
            int errCode = getCcdiRpcClient().Login(requestBuilder.build(), responseBuilder);
            // LoginOutput response = responseBuilder.build();

            // if login is successful, set mycloud root and enable camera roll
            if (errCode >= 0) {
                UpdateSyncSettingsInput req = UpdateSyncSettingsInput
                        .newBuilder().setUserId(getUserId())
                        .setSetMyCloudRoot(Constants.DEFAULT_FOLDER_PATH)
                        .setEnableCameraRoll(true)
                        .build();
                UpdateSyncSettingsOutput.Builder resBuilder = UpdateSyncSettingsOutput
                        .newBuilder();
                int returnCode = getCcdiRpcClient().UpdateSyncSettings(req,
                        resBuilder);
                if (returnCode < 0) {
                    Log.e(LOG_TAG, "Error calling UpdateSyncSettings. "
                            + returnCode);
                    // Setting setEnableCameraRoll to true multiple times in a row will result in an error.
                    // Looks like the current code doesn't do anything with the error other than log...
                    // See https://bugs.routefree.com/show_bug.cgi?id=10816
                }
            }
            Log.i(LOG_TAG, "doLogin end : time = " + System.currentTimeMillis());
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "login", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int doLogout() {

        try {
            Log.i(LOG_TAG, "doLogout begin : time = " + System.currentTimeMillis());
            LogoutInput request = LogoutInput.newBuilder().setPlayerIndex(0)
                    .build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();
            int errCode = getCcdiRpcClient().Logout(request, responseBuilder);
            Log.i(LOG_TAG, "doLogout end : time = " + System.currentTimeMillis());
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "logout", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public String getUserName() {

        try {
            GetSystemStateInput request = GetSystemStateInput.newBuilder()
                    .setGetPlayers(true).build();
            GetSystemStateOutput.Builder responseBuilder = GetSystemStateOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().GetSystemState(request,
                    responseBuilder);
            if (errCode < 0) {
                Log.e(LOG_TAG, "getUserName");
                return "";
            }
            GetSystemStateOutput response = responseBuilder.build();
            if (response.getPlayers().getPlayersCount() >= 1) {
                return response.getPlayers().getPlayers(0).getUsername();
            }
            return "";
        } catch (AppLayerException e) {
            return "";
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getUserName", e);
            return "";
        }
    }

    public boolean isDeviceLinked() {

        try {
            GetSyncStateInput request = GetSyncStateInput.newBuilder().build();
            GetSyncStateOutput.Builder responseBuilder = GetSyncStateOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().GetSyncState(request,
                    responseBuilder);
            if (errCode < 0) {
                Log.e(LOG_TAG, "isDeviceLinked");
                return false;
            }
            GetSyncStateOutput response = responseBuilder.build();
            return response.getIsDeviceLinked();
        } catch (AppLayerException e) {
            return false;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "isDeviceLinked", e);
            return false;
        }
    }

    public int linkDevice(String deviceName) {

        try {
            Log.i(LOG_TAG, "linkDevice begin : time = " + System.currentTimeMillis());
            LinkDeviceInput request = LinkDeviceInput.newBuilder()
                    .setUserId(getUserId()).setDeviceName(deviceName).setIsAcerDevice(Utility.isAcerDevice()).build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();
            int errCode = getCcdiRpcClient().LinkDevice(request,
                    responseBuilder);
            
            Log.i(LOG_TAG, "linkDevice end : time = " + System.currentTimeMillis());
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "linkDevice", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int unlinkDevice() {

        try {
            Log.i(LOG_TAG, "unlinkDevice begin : time = " + System.currentTimeMillis());
            UnlinkDeviceInput request = UnlinkDeviceInput.newBuilder()
                    .setUserId(getUserId()).build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();
            int errCode = getCcdiRpcClient().UnlinkDevice(request,
                    responseBuilder);
            Log.i(LOG_TAG, "unlinkDevice end : time = " + System.currentTimeMillis());
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "unlinkDevice", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public String getDeviceName() {

        try {
            GetSyncStateInput request = GetSyncStateInput.newBuilder()
                    .setGetDeviceName(true).build();
            GetSyncStateOutput.Builder responseBuilder = GetSyncStateOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().GetSyncState(request,
                    responseBuilder);
            if (errCode < 0) {
                Log.e(LOG_TAG, "getDeviceName");
                return "";
            }
            GetSyncStateOutput response = responseBuilder.build();
            return response.getMyDeviceName();
        } catch (AppLayerException e) {
            return "";
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getDeviceName", e);
            return "";
        }
    }

    public int renameDevice(String deviceName) {

        try {
            UpdateSyncSettingsInput request = UpdateSyncSettingsInput
                    .newBuilder().setUserId(getUserId())
                    .setSetMyDeviceName(deviceName).build();
            UpdateSyncSettingsOutput.Builder responseBuilder = UpdateSyncSettingsOutput
                    .newBuilder();
            getCcdiRpcClient().UpdateSyncSettings(request, responseBuilder);
            UpdateSyncSettingsOutput response = responseBuilder.build();
            int errCode = response.getSetMyDeviceNameErr();
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "renameDevice", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    // TODO: Remove; this is not needed for Personal Cloud Sync.
    public DownloadProgress[] getProgress() {

        GetOwnedTitleListDataInput request = GetOwnedTitleListDataInput
                .newBuilder().setUserId(getUserId()).build();
        GetOwnedTitleListDataOutput.Builder responseBuilder = GetOwnedTitleListDataOutput
                .newBuilder();
        try {
            getCcdiRpcClient().GetOwnedTitleListData(request, responseBuilder);
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "GetOwnedTitleListData", e);
            return new DownloadProgress[0];
        }
        GetOwnedTitleListDataOutput response = responseBuilder.build();
        DownloadProgress[] result = new DownloadProgress[response
                .getTitlesCount()];
        for (int i = 0; i < result.length; i++) {
            OwnedTitleMenuData curr = response.getTitles(i);
            result[i] = new DownloadProgress(curr.getDisplayName(), curr
                    .getDownloadStats().getLocationOnDisk(), curr
                    .getDownloadStats().getBytesDownloaded(), curr
                    .getDownloadStats().getTotalDownloadSizeBytes());
        }
        return result;
    }

    public Dataset[] listOwnedDataSets() {
        Log.d(LOG_TAG, "Calling listOwnedDataSets");
        ListOwnedDatasetsInput request = ListOwnedDatasetsInput.newBuilder()
                .setUserId(getUserId()).build();
        ListOwnedDatasetsOutput.Builder responseBuilder = ListOwnedDatasetsOutput
                .newBuilder();
        try {
            getCcdiRpcClient().ListOwnedDatasets(request, responseBuilder);
        } catch (AppLayerException e) {
            return new Dataset[0];
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "ListOwnedDatasetsOutput", e);
            return new Dataset[0];
        }
        ListOwnedDatasetsOutput response = responseBuilder.build();

        ArrayList<Dataset> result = new ArrayList<Dataset>();
        for (int i = 0; i < response.getDatasetDetailsCount(); i++) {
            DatasetDetail curr = response.getDatasetDetails(i);
            // filter the results of ListOwnedDatasets,
            // if dataset_details.contentType == "CAMERA" and
            // created_by_this_device == true
            if (!response.getCreatedByThisDevice(i)
                    || !curr.getContentType().equals(CONTENT_TYPE_CAMERA)) {

                Dataset ds = new Dataset(curr.getDatasetId(),
                        curr.getDatasetName());
                result.add(ds);
            }
        }
        Dataset[] toReturn = new Dataset[result.size()];
        toReturn = result.toArray(toReturn);
        return toReturn;
    }

    public Dataset[] listSyncSubscriptions() {
        Log.d(LOG_TAG, "Calling listSyncSubscriptions");
        ListSyncSubscriptionsInput request = ListSyncSubscriptionsInput
                .newBuilder().setUserId(getUserId()).build();

        ListSyncSubscriptionsOutput.Builder responseBuilder = ListSyncSubscriptionsOutput
                .newBuilder();
        try {
            getCcdiRpcClient().ListSyncSubscriptions(request, responseBuilder);
        } catch (AppLayerException e) {
            return new Dataset[0];
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "listSyncSubscriptions", e);
            return new Dataset[0];
        }
        ListSyncSubscriptionsOutput response = responseBuilder.build();
        Dataset[] result = new Dataset[response.getSubscriptionsCount()];
        
        for (int i = 0; i < result.length; i++) {
            Subscription curr = response.getSubscriptions(i);
            SyncSubscriptionDetail sub = response.getSubs(i);
            String absDeviceRoot = sub.getAbsoluteDeviceRoot();
            String filterstring = curr.getFilter();
            Dataset folder = new Dataset(curr.getDatasetId(),
                    curr.getDatasetName());
            folder.setFilterString(filterstring);
            folder.setHasSubscription();
            folder.setAbsoluteDeviceRoot(absDeviceRoot);
            result[i] = folder;
        }

        return result;
    }

    public int subscribeDataset(Dataset folder) {
        Log.d(LOG_TAG, "Calling subscribeDataset");

        // debug code begins
        if (folder.getSubscriptionState() == State.SUBSCRIBED) {
            String filterString = Filter.ConstructFilterString(folder
                    .getFilters());
            Log.d(LOG_TAG, "**********filter string for subscribed dataset: "
                    + filterString);
        }
        // debug code ends

        AddSyncSubscriptionInput request = AddSyncSubscriptionInput
                .newBuilder()
                .setUserId(getUserId())
                .setDatasetId(folder.getDatasetId())
                .setSubscriptionType(
                        igware.gvm.pb.CcdiRpc.SyncSubscriptionType_t.SUBSCRIPTION_TYPE_NORMAL)
                .setFilter(Filter.ConstructFilterString(folder.getFilters()))
                .build();
        NoParamResponse.Builder responseBuilder = NoParamResponse.newBuilder();
        try {
            int result = getCcdiRpcClient().AddSyncSubscription(request,
                    responseBuilder);
            return result;
        } catch (AppLayerException e) {
            return -1;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "subscribe", e);
            return -1;
        }
    }

    private boolean updateFilter(Dataset dataset) {
        Log.d(LOG_TAG, "Calling updateFilter");
        UpdateSyncSubscriptionInput request = UpdateSyncSubscriptionInput
                .newBuilder()
                .setUserId(getUserId())
                .setDatasetId(dataset.getDatasetId())
                .setNewFilter(
                        Filter.ConstructFilterString(dataset.getFilters()))
                .build();
        NoParamResponse.Builder responseBuilder = NoParamResponse.newBuilder();

        try {
            int result = getCcdiRpcClient().UpdateSyncSubscription(request,
                    responseBuilder);
            dataset.AfterCommit(result >= 0);
            return result >= 0;

        } catch (AppLayerException e) {
            return false;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "updateFilter", e);
            return false;
        }
    }

    public boolean savechange() {
        for (Dataset ds : Dataset.GetAllDatasets()) {
            Log.d(LOG_TAG,
                    "Saving dataset:" + ds.getName() + "state:"
                            + ds.getSubscriptionState() + " filter:"
                            + Filter.ConstructFilterString(ds.getFilters()));
            switch (ds.getSubscriptionState()) {
            case SUBSCRIBED:
                if (ds.isOriginallySubscribed()) {
                    if (!updateFilter(ds)) {
                        return false;
                    }
                } else {
                    if (subscribeDataset(ds) != 0) {
                        return false;
                    }
                }
                break;
            case UNSUBSCIRBED:
                if (ds.isOriginallySubscribed() && !unsubscribeDataset(ds)) {
                    return false;
                }
                break;
            case PARTIALLY_SUBSCRIBED:
                if (!ds.isOriginallySubscribed()) {
                    subscribeDataset(ds);
                } else if (!updateFilter(ds)) {
                    return false;
                }
                break;
            default:
                Log.e(LOG_TAG,
                        "Cant not save change for dataset " + ds.getName());
                return false;
            }

            ds.AfterCommit(true);
        }
        return true;
    }

    /*
     * A helper method to add explicitly specified filter all the way down to
     * the folder's level, to get ready for filter removal.
     */
    public void specify(Dataset dataset, Subfolder folder) {
        Log.d(LOG_TAG,
                "Calling specify for " + folder.getFolderFullPath()
                        + " original filter:"
                        + Filter.ConstructFilterString(dataset.getFilters()));
        if (dataset.isExplicitlySpecifiedInFilter(folder)) {
            Log.d(LOG_TAG, "specify - NOOP");
            return;
        }

        // from the root, starting from the first null filter on the path,
        // we need to specify every level of folders downwards.
        String path = folder.getPath();
        int depth = Filter.getDepth(path);

        for (int i = 0; i <= depth; i++) {
            String thisPath = Filter.getFirstN(path, i);
            Filter filter = dataset.getFilterNode(thisPath);
            int thisDepth = Filter.getDepth(thisPath);

            if (thisDepth == 0 && dataset.getFilters() == null || thisDepth > 0
                    && (filter == null || filter.getFilters() == null)) {
                SyncListItem[] thisLevelFolders = listSubFolders(
                        dataset.getDatasetId(), thisPath);
                dataset.specify(thisLevelFolders);
            }
        }

        Log.d(LOG_TAG,
                "Specify updated filter:"
                        + Filter.ConstructFilterString(dataset.getFilters()));
    }

    public boolean allSubfoldersCompletelySubscribed(Dataset dataset,
            String path) {
        Log.d(LOG_TAG, "Calling allSubfoldersCompletelySubscribed with path "
                + path);
        SyncListItem[] subs = listSubFolders(dataset.getDatasetId(), path);
        Filter filter = dataset.getFilterNode(path);// parent of interested node
                                                    // and siblings

        if (Filter.getDepth(path) == 0) {
            if (subs.length != dataset.getFilters().length) {
                // Let's be a little lazy here.
                // cases like a mismatched folder is neglegible for this
                // release.
                return false;
            }

            for (Filter fi : dataset.getFilters()) {
                if (fi.getFilters() != null) {
                    return false;
                }
            }

        } else {
            if (subs.length != filter.getFilters().length) {
                // Let's be a little lazy here.
                // cases like a mismatched folder is neglegible for this
                // release.
                return false;
            }

            for (Filter fi : filter.getFilters()) {
                if (fi.getFilters() != null) {
                    return false;
                }
            }
        }
        return true;
    }

    private boolean unsubscribeDataset(Dataset folder) {
        Log.d(LOG_TAG, "Calling unsubscribeDataset");
        DeleteSyncSubscriptionsInput request = DeleteSyncSubscriptionsInput
                .newBuilder().setUserId(getUserId())
                .addDatasetIds(folder.getDatasetId()).build();

        NoParamResponse.Builder responseBuilder = NoParamResponse.newBuilder();
        try {
            int result = getCcdiRpcClient().DeleteSyncSubscriptions(request,
                    responseBuilder);
            return result >= 0;
        } catch (AppLayerException e) {
            return false;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "unsubscribe", e);
            return false;
        }
    }

    public SyncListItem[] listSyncItems(long datasetId, String path) {
        Log.d(LOG_TAG, "Calling listSubFolders for path " + path);
        if (path == null) {
            path = "/";
        }
        GetDatasetDirectoryEntriesInput request = GetDatasetDirectoryEntriesInput
                .newBuilder().setUserId(getUserId()).setDatasetId(datasetId)
                .setDirectoryName(path).build();
        GetDatasetDirectoryEntriesOutput.Builder responseBuilder = GetDatasetDirectoryEntriesOutput
                .newBuilder();
        try {
            getCcdiRpcClient().GetDatasetDirectoryEntries(request,
                    responseBuilder);
        } catch (AppLayerException e) {
            return new Subfolder[0];
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "listSubFolders", e);
            return new Subfolder[0];
        }

        GetDatasetDirectoryEntriesOutput response = responseBuilder.build();
        ArrayList<SyncListItem> result = new ArrayList<SyncListItem>();
        for (int i = 0; i < response.getEntriesCount(); i++) {
            DatasetDirectoryEntry curr = response.getEntries(i);

            Subfolder folder = new Subfolder(curr.getName(), path,
                    (curr.getIsDir() ? Type.Folder : Type.File), curr.getMtime(), curr.getSize(), curr.getUrl());

            Dataset temp = Dataset.GetDatasetById(datasetId);
            folder.setDataset(temp);
            result.add(folder);
        }

        SyncListItem[] toReturn = new SyncListItem[result.size()];
        toReturn = result.toArray(toReturn);
        return toReturn;
    }

    public Subfolder[] listSubFolders(long datasetId, String path) {
        Log.d(LOG_TAG, "Calling listSubFolders for path " + path);
        if (path == null) {
            path = "/";
        }
        GetDatasetDirectoryEntriesInput request = GetDatasetDirectoryEntriesInput
                .newBuilder().setUserId(getUserId()).setDatasetId(datasetId)
                .setDirectoryName(path).build();
        GetDatasetDirectoryEntriesOutput.Builder responseBuilder = GetDatasetDirectoryEntriesOutput
                .newBuilder();
        try {
            getCcdiRpcClient().GetDatasetDirectoryEntries(request,
                    responseBuilder);
        } catch (AppLayerException e) {
            return new Subfolder[0];
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "listSubFolders", e);
            return new Subfolder[0];
        }

        GetDatasetDirectoryEntriesOutput response = responseBuilder.build();
        ArrayList<Subfolder> result = new ArrayList<Subfolder>();
        for (int i = 0; i < response.getEntriesCount(); i++) {
            DatasetDirectoryEntry curr = response.getEntries(i);
            if (curr.getIsDir()) {
                Subfolder folder = new Subfolder(curr.getName(), path,
                        Type.Folder, curr.getMtime(), curr.getSize(), curr.getUrl());
                Dataset temp = Dataset.GetDatasetById(datasetId);
                folder.setDataset(temp);
                result.add(folder);
            }
        }

        Subfolder[] toReturn = new Subfolder[result.size()];
        toReturn = result.toArray(toReturn);
        return toReturn;
    }

    public long getCameraSyncUploadFolderId() {
        try {
            Log.i(LOG_TAG, "getCameraSyncUploadFolderId begin : time = " + System.currentTimeMillis());
            Log.i(LOG_TAG, "Trigger getCameraSyncUploadFolderId");
            ListOwnedDatasetsInput request = ListOwnedDatasetsInput
                    .newBuilder().setUserId(getUserId()).build();
            Log.i(LOG_TAG, "request got!");
            ListOwnedDatasetsOutput.Builder responseBuilder = ListOwnedDatasetsOutput
                    .newBuilder();
            Log.i(LOG_TAG, "responseBuilder got!");
            getCcdiRpcClient().ListOwnedDatasets(request, responseBuilder);
            ListOwnedDatasetsOutput response = responseBuilder.build();
            int numFolders = response.getDatasetDetailsCount();
            Log.i(LOG_TAG, "numFolders = " + numFolders);
            for (int i = 0; i < numFolders; i++) {
                DatasetDetail curr = response.getDatasetDetails(i);
                DatasetType type = curr.getDatasetType();
                boolean isUploadType = type.equals(DatasetType.CR_UP);
                Log.i(LOG_TAG, "Current dataset type = " + type + ", CR_UP = "
                        + DatasetType.CR_UP);
                if (isUploadType) {
                    Log.i(LOG_TAG, "getCameraSyncUploadFolderId end : time = " + System.currentTimeMillis());
                    return curr.getDatasetId();
                }
            }
            Log.i(LOG_TAG, "getCameraSyncUploadFolderId end : time = " + System.currentTimeMillis());
            // Camera sync upload folder not found.
            Log.e(LOG_TAG, "Cannot find camera sync upload folder.");
            return -1;
        } catch (AppLayerException e) {
            return -1;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getCameraSyncUploadFolderId", e);
            return -1;
        }
    }
    
    public long getCameraSyncDownloadFolderId() {
        try {
            Log.i(LOG_TAG, "getCameraSyncDownloadFolderId begin : time = " + System.currentTimeMillis());
            ListOwnedDatasetsInput request = ListOwnedDatasetsInput
                    .newBuilder().setUserId(getUserId()).build();
            ListOwnedDatasetsOutput.Builder responseBuilder = ListOwnedDatasetsOutput
                    .newBuilder();
            getCcdiRpcClient().ListOwnedDatasets(request, responseBuilder);
            ListOwnedDatasetsOutput response = responseBuilder.build();
            int numFolders = response.getDatasetDetailsCount();
            for (int i = 0; i < numFolders; i++) {
                DatasetDetail curr = response.getDatasetDetails(i);
                DatasetType type = curr.getDatasetType();
                boolean isDownloadType = type.equals(DatasetType.CR_DOWN);
                Log.i(LOG_TAG, "Current dataset type = " + type
                        + ", CR_DOWN = " + DatasetType.CR_DOWN);
                if (isDownloadType) {
                    Log.i(LOG_TAG, "getCameraSyncDownloadFolderId end : time = " + System.currentTimeMillis());
                    return curr.getDatasetId();
                }
            }
            Log.i(LOG_TAG, "getCameraSyncDownloadFolderId end : time = " + System.currentTimeMillis());
            // Camera sync download folder not found.
            Log.e(LOG_TAG, "Cannot find camera sync download folder.");
            return -1;
        } catch (AppLayerException e) {
            return -1;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getCameraSyncDownloadFolderId", e);
            return -1;
        }
    }

    // This function should no longer be called, because the cloud will create
    // the camera sync folders automatically.
    @Deprecated
    public int createCameraSyncFolder() {

        // Find current owned datasets.
        ListOwnedDatasetsInput request = ListOwnedDatasetsInput.newBuilder()
                .setUserId(getUserId()).build();
        ListOwnedDatasetsOutput.Builder responseBuilder = ListOwnedDatasetsOutput
                .newBuilder();
        try {
            getCcdiRpcClient().ListOwnedDatasets(request, responseBuilder);
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "createCameraSyncFolder", e);
            return ERROR_CODE_RPC_LAYER;
        }
        ListOwnedDatasetsOutput response = responseBuilder.build();
        int numFolders = response.getDatasetDetailsCount();

        // Find unique sync folder name.
        int suffix = 0;
        boolean foundUniqueName = false;
        String proposedName;
        do {
            // Try a new sync folder name.
            if (suffix == 0) {
                proposedName = mContext
                        .getString(R.string.Label_CameraSyncFolder);
            } else {
                proposedName = mContext
                        .getString(R.string.Label_CameraSyncFolder)
                        + " ("
                        + suffix + ")";
            }
            suffix++;
            // Look for this name in the currently owned sync folders.
            boolean foundMatch = false;
            for (int i = 0; i < numFolders; i++) {
                DatasetDetail curr = response.getDatasetDetails(i);
                if (curr.getDatasetName().equals(proposedName)) {
                    foundMatch = true;
                }
            }
            foundUniqueName = !foundMatch;
        } while (!foundUniqueName);

        // Create the actual sync folder.
        try {
            AddDatasetInput request2 = AddDatasetInput.newBuilder()
                    .setUserId(getUserId()).setDatasetName(proposedName)
                    .setDatasetType(NewDatasetType_t.NEW_DATASET_TYPE_CAMERA)
                    .build();
            AddDatasetOutput.Builder responseBuilder2 = AddDatasetOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().AddDataset(request2,
                    responseBuilder2);
            // AddDatasetOutput response = responseBuilder.build();
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "createCamearSyncFolder", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int addCameraSyncUploadSubscription() {
        Log.i(LOG_TAG, "addCameraSyncUploadSubscription begin : time = " + System.currentTimeMillis());
        long datasetId = getCameraSyncUploadFolderId();
        Log.i(LOG_TAG, "getCameraSyncUploadFolderId returned: " + datasetId);
        
        if (datasetId < 0) {
            return 0;
        }
        
        try {
            AddSyncSubscriptionInput request = AddSyncSubscriptionInput
                    .newBuilder()
                    .setUserId(getUserId())
                    .setDatasetId(datasetId)
                    .setSubscriptionType(
                            SyncSubscriptionType_t.SUBSCRIPTION_TYPE_PRODUCER)
                    .setDeviceRoot(Constants.CAMERA_FOLDER_PATH)
                    .build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();
            int errCode = getCcdiRpcClient().AddSyncSubscription(request,
                    responseBuilder);
            Log.i(LOG_TAG, "addCameraSyncUploadSubscription end : time = " + System.currentTimeMillis());
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "addCameraSyncUploadSubscription", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }
    
    public int addCameraSyncDownloadSubscription(int maxSize, int maxFiles) {
        Log.i(LOG_TAG, "addCameraSyncDownloadSubscription begin : time = " + System.currentTimeMillis());
        long datasetId = getCameraSyncDownloadFolderId();
        Log.i(LOG_TAG, "getCameraSyncDownloadFolderId returned: " + datasetId);
        
        if (datasetId < 0) {
            return 0;
        }

        try {
            AddSyncSubscriptionInput request = AddSyncSubscriptionInput
                    .newBuilder()
                    .setUserId(getUserId())
                    .setDatasetId(datasetId)
                    .setSubscriptionType(
                            SyncSubscriptionType_t.SUBSCRIPTION_TYPE_CONSUMER)
                    .setMaxSize(maxSize)
                    .setMaxFiles(maxFiles)
                    .build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();
            int errCode = getCcdiRpcClient().AddSyncSubscription(request,
                    responseBuilder);
            Log.i(LOG_TAG, "addCameraSyncDownloadSubscription end : time = " + System.currentTimeMillis());
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "addCameraSyncDownloadSubscription", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }
    
    public int getCameraSyncDownloadMaxSize() {

        long datasetId = getCameraSyncDownloadFolderId();
        Log.i(LOG_TAG, "getCameraSyncDownloadFolderId returned: " + datasetId);

        if (datasetId < 0) {
            return 0;
        }

        try {
            ListSyncSubscriptionsInput request = ListSyncSubscriptionsInput
                    .newBuilder()
                    .setUserId(getUserId())
                    .build();
            ListSyncSubscriptionsOutput.Builder responseBuilder = ListSyncSubscriptionsOutput
                    .newBuilder();
            getCcdiRpcClient().ListSyncSubscriptions(request, responseBuilder);
            ListSyncSubscriptionsOutput response = responseBuilder.build();
            int numSubs = response.getSubscriptionsCount();
            for (int i = 0; i < numSubs; i++) {
                SyncSubscriptionDetail sub = response.getSubs(i);
                DatasetDetail detail = sub.getDatasetDetails();
                if (detail.getDatasetId() == datasetId) {
                    return (int)sub.getMaxSize();
                }
            }
            // Camera sync download folder does not have subscription.
            return 0;
        } catch (AppLayerException e) {
            return 0;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getCameraSyncDownloadMaxSize", e);
            return 0;
        }
    }
    
    public int getCameraSyncDownloadMaxFiles() {

        long datasetId = getCameraSyncDownloadFolderId();
        Log.i(LOG_TAG, "getCameraSyncDownloadFolderId returned: " + datasetId);

        if (datasetId < 0) {
            return 0;
        }

        try {
            ListSyncSubscriptionsInput request = ListSyncSubscriptionsInput
                    .newBuilder()
                    .setUserId(getUserId())
                    .build();
            ListSyncSubscriptionsOutput.Builder responseBuilder = ListSyncSubscriptionsOutput
                    .newBuilder();
            getCcdiRpcClient().ListSyncSubscriptions(request, responseBuilder);
            ListSyncSubscriptionsOutput response = responseBuilder.build();
            int numSubs = response.getSubscriptionsCount();
            for (int i = 0; i < numSubs; i++) {
                SyncSubscriptionDetail sub = response.getSubs(i);
                DatasetDetail detail = sub.getDatasetDetails();
                if (detail.getDatasetId() == datasetId) {
                    return (int)sub.getMaxFiles();
                }
            }
            // Camera sync download folder does not have subscription.
            return 0;
        } catch (AppLayerException e) {
            return 0;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getCameraSyncDownloadMaxFiles", e);
            return 0;
        }
    }
    
    public int updateCameraSyncDownloadSubscription(int maxSize, int maxFiles) {

        long datasetId = getCameraSyncDownloadFolderId();
        Log.i(LOG_TAG, "getCameraSyncDownloadFolderId returned: " + datasetId);
        
        if (datasetId < 0) {
            return 0;
        }
        
        try {
            UpdateSyncSubscriptionInput request = UpdateSyncSubscriptionInput
                    .newBuilder()
                    .setUserId(getUserId())
                    .setDatasetId(datasetId)
                    .setMaxSize(maxSize)
                    .setMaxFiles(maxFiles)
                    .build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();
            int errCode = getCcdiRpcClient().UpdateSyncSubscription(request,
                    responseBuilder);
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "updateCameraSyncDownloadSubscription", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int removeCameraSyncSubscription(long datasetId) {
        Log.i(LOG_TAG, "removeCameraSyncSubscription begin : time = " + System.currentTimeMillis());
        try {
            DeleteSyncSubscriptionsInput request = DeleteSyncSubscriptionsInput
                    .newBuilder()
                    .setUserId(getUserId())
                    .addDatasetIds(datasetId)
                    .build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();
            int errCode = getCcdiRpcClient().DeleteSyncSubscriptions(request,
                    responseBuilder);
            // DeleteSyncSubscriptionsOutput response = responseBuilder.build();
            Log.i(LOG_TAG, "removeCameraSyncSubscription end : time = " + System.currentTimeMillis());
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "removeCameraSyncSubscription", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public boolean isCameraSyncUploadFolderSubscribed() {

        long datasetId = getCameraSyncUploadFolderId();
        Log.i(LOG_TAG, "getCameraSyncUploadFolderId returned: " + datasetId);
        
        if (datasetId < 0) {
            return false;
        } else {
            // Look for folder id in list of subscribed folders.
            Dataset[] subscribedFolders = listSyncSubscriptions();
            for (int i = 0; i < subscribedFolders.length; ++i) {
                Dataset folder = subscribedFolders[i];
                if (folder.getDatasetId() == datasetId) {
                    return true;
                }
            }
            return false;
        }
    }
    
    public boolean isCameraSyncDownloadFolderSubscribed() {

        long datasetId = getCameraSyncDownloadFolderId();
        Log.i(LOG_TAG, "getCameraSyncDownloadFolderId returned: " + datasetId);
        
        if (datasetId < 0) {
            return false;
        } else {
            // Look for folder id in list of subscribed folders.
            Dataset[] subscribedFolders = listSyncSubscriptions();
            for (int i = 0; i < subscribedFolders.length; ++i) {
                Dataset folder = subscribedFolders[i];
                if (folder.getDatasetId() == datasetId) {
                    return true;
                }
            }
            return false;
        }
    }

    public boolean isSyncEnabled() {

        try {
            GetSyncStateInput request = GetSyncStateInput.newBuilder().build();
            GetSyncStateOutput.Builder responseBuilder = GetSyncStateOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().GetSyncState(request,
                    responseBuilder);
            if (errCode < 0) {
                Log.e(LOG_TAG, "isSyncEnabled");
                return false;
            }
            GetSyncStateOutput response = responseBuilder.build();
            return response.getIsSyncAgentEnabled();
        } catch (AppLayerException e) {
            return false;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "isSyncEnabled", e);
            return false;
        }
    }

    public int pauseSync() {

        try {
            UpdateSyncSettingsInput request = UpdateSyncSettingsInput
                    .newBuilder().setUserId(getUserId())
                    .setEnableSyncAgent(false).build();
            UpdateSyncSettingsOutput.Builder responseBuilder = UpdateSyncSettingsOutput
                    .newBuilder();
            getCcdiRpcClient().UpdateSyncSettings(request, responseBuilder);
            UpdateSyncSettingsOutput response = responseBuilder.build();
            int errCode = response.getEnableSyncAgentErr();
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "pauseSync", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int resumeSync() {

        try {
            UpdateSyncSettingsInput request = UpdateSyncSettingsInput
                    .newBuilder().setUserId(getUserId())
                    .setEnableSyncAgent(true).build();
            UpdateSyncSettingsOutput.Builder responseBuilder = UpdateSyncSettingsOutput
                    .newBuilder();
            getCcdiRpcClient().UpdateSyncSettings(request, responseBuilder);
            UpdateSyncSettingsOutput response = responseBuilder.build();
            int errCode = response.getEnableSyncAgentErr();
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "pauseSync", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int[] getSyncStatus() {

        int[] zeroes = { 0, 0, 0, 0 };
        Log.e(LOG_TAG, "Status API changed!! Use GetSyncStateOutput.dataset_sync_state_summary instead.");
        return zeroes;

        //        try {
        //            GetSyncStateInput request = GetSyncStateInput.newBuilder()
        //                    .setGetSyncStateSummary(true)
        //                    .build();
        //            GetSyncStateOutput.Builder responseBuilder = GetSyncStateOutput
        //                    .newBuilder();
        //            int errCode = getCcdiRpcClient().GetSyncState(request,
        //                    responseBuilder);
        //            if (errCode < 0) {
        //                Log.e(LOG_TAG, "getSyncStatus");
        //                return zeroes;
        //            }
        //            GetSyncStateOutput response = responseBuilder.build();
        //            SyncStateSummary summary = response.getSyncStateSummary();
        //            int numDownloading = summary.getNumDownloading();
        //            int numToDownload = summary.getNumToDownload();
        //            int numTotalDownload = numDownloading + numToDownload;
        //            int numUploading = summary.getNumUploading();
        //            int numToUpload = summary.getNumToUpload();
        //            int numTotalUpload = numUploading + numToUpload;
        //            int[] result = { numDownloading, numTotalDownload, numUploading,
        //                    numTotalUpload };
        //            return result;
        //        } catch (AppLayerException e) {
        //            return zeroes;
        //        } catch (ProtoRpcException e) {
        //            Log.e(LOG_TAG, "getSyncStatus", e);
        //            return zeroes;
        //        }
    }

    public long[] getQuotaStatus() {

        long[] zeroes = { 0, 0, 0, 0 };
        try {
            GetPersonalCloudStateInput request = GetPersonalCloudStateInput
                    .newBuilder().setUserId(getUserId())
                    .setGetInfraStorageQuota(true).build();
            GetPersonalCloudStateOutput.Builder responseBuilder = GetPersonalCloudStateOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().GetPersonalCloudState(request,
                    responseBuilder);
            if (errCode < 0) {
                Log.e(LOG_TAG, "getQuotaStatus");
                return zeroes;
            }
            GetPersonalCloudStateOutput response = responseBuilder.build();
            long usedBytes = response.getInfraStorageUsedBytes();
            long totalBytes = response.getInfraStorageTotalBytes();
            long freeBytes = totalBytes - usedBytes;
            long[] result = { freeBytes, totalBytes };
            return result;
        } catch (AppLayerException e) {
            return zeroes;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getQuotaStatus", e);
            return zeroes;
        }
    }

    public int ownershipSync() {
        try {
            Log.i(LOG_TAG, "ownershipSync begin : time = " + System.currentTimeMillis());
            NoParamRequest request = NoParamRequest.getDefaultInstance();
            NoParamResponse.Builder responseBuilder = NoParamResponse.newBuilder();
            int errCode = getCcdiRpcClient().OwnershipSync(request, responseBuilder);
            Log.i(LOG_TAG, "ownershipSync end : time = " + System.currentTimeMillis());
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            return ERROR_CODE_RPC_LAYER;
        }
    }

    /**
     * search the files in File Explorer using key as the keyword
     *
     * @param key
     *          the keyword used to search file
     * @param files
     *          the search results
     * @return  0, if it is executed successfully. -1, otherwise
     */
    public int searchFile(String key, FileInfo[] files) {

        return 0;
    }

    /**
     * search the files in Document Save & Go using key as the keyword
     *
     * @param key
     *          the keyword used to search file
     * @param files
     *          the search results
     * @return  0, if it is executed successfully. -1, otherwise
     */
    public int searchDoc(String key, FileInfo[] files) {

        return 0;
    }

    /**
     * browse files in File Explorer
     *
     * @param datasetId
     *          the dataset ID
     * @param files
     *          the browse results
     * @return  0, if it is executed successfully. -1, otherwise
     */
    public int doFileBrowse(long datasetId, String parentId, ArrayList<HashMap<String, Object>> fileList) {

        Dataset[] items = listOwnedDataSets();         //< ----  get first layer folder
        Dataset[] subscribeItems = listSyncSubscriptions();   // <---  list synced folder

        processSyncFolders(items, subscribeItems);
        Log.i(LOG_TAG, "dataset size: " + items.length);
        SyncListItem[] list = null;
        long dID;
        for (Dataset data : items) {
            dID = data.getDatasetId();
            Log.i(LOG_TAG, "data id: " + dID + ", data name: " + data.getName() + ", input id: " + datasetId);
            if (dID == datasetId) {
                list = listSyncItems(dID, parentId);   // <---  get sub folder
                Arrays.sort(list);
                break;
            }
        }
        if (list != null) {
            for (SyncListItem item : list) {
                Subfolder folder = (Subfolder) item;
                HashMap<String, Object> hm = new HashMap<String, Object>();
                boolean isDir = (folder.getType() == Subfolder.Type.Folder);
                hm.put(FileExplorer.NAME, folder.getName());
                hm.put(FileExplorer.FULLPATH, folder.getFolderFullPath());
                hm.put(FileExplorer.ISDIR, isDir);
                Date d = new Date(folder.getModifyTime());
                DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd'T'hh:mm:ssZ", Locale.US);
                String mTime = dateFormat.format(d);
                hm.put(FileExplorer.MODIFY_TIME, mTime);
                hm.put(FileExplorer.SIZE, folder.getSize());
                hm.put(FileExplorer.URL, folder.getUrl());
                fileList.add(hm);
            }
        }
        return 0;
    }

    /**
     * browse files in Document Save & Go
     *
     * @param deviceId
     *          the cloud PC ID
     * @param files
     *          the browse results
     * @return  0, if it is executed successfully. -1, otherwise
     */
    public int doDocBrowse(String deviceId, FileInfo[] files) {

        return 0;
    }

    /**
     * subscribe update events in File Explorer
     * @return  0, if it is executed successfully. -1, otherwise
     */
    public int subscribeFileUpdate() {
        return 0;
    }

    /**
     * subscribe update events in Document Save & Go
     * @return  0, if it is executed successfully. -1, otherwise
     */
    public int subscribeDocUpdate() {
        return 0;
    }

    /**
     * determine if the cloud PC is online or not
     * @return 1, if the PC is online. 0, otherwise
     */
    public int isCloudPCOnline() {

        return 1;
    }

    public Dataset[] getDataSetList() {
        ArrayList<Dataset> list = new ArrayList<Dataset>();
        Dataset[] items = listOwnedDataSets();
        for (Dataset data : items) {
            Log.i(LOG_TAG, "dataset name: " + data.getName() + ", dataset id: " + data.getDatasetId());
            if (data.getName().startsWith(FileExplorer.LIBRARY)) {
                list.add(data);
            }
        }
        Dataset[] ret = list.toArray(new Dataset[list.size()]);
        return ret;
    }

    /*
     * Using id as the key, if item from a1 exists in a2, mark it as subscribed.
     */
    private void processSyncFolders(Dataset[] a1, Dataset[] a2) {
        Hashtable<Long, Dataset> table = new Hashtable<Long, Dataset>();
        for (int i = 0; i < a2.length; i++) {
            table.put(a2[i].getDatasetId(), a2[i]);
        }
        for (int i = 0; i < a1.length; i++) {
            Dataset.StoreDataset(a1[i]);
            if (table.containsKey(a1[i].getDatasetId())) {
                a1[i].setHasSubscription();
                a1[i].setFilterString(table.get(a1[i].getDatasetId())
                        .getFilterString());
                String absDeviceRoot = table.get(a1[i].getDatasetId())
                        .getAbsoluteDeviceRoot();
                a1[i].setAbsoluteDeviceRoot(absDeviceRoot);
            }
        }
        // now sort
        Arrays.sort(a1);
    }
    
    
    private static final String TEMP_SD_CARD_PATH = "/sdcard/acerDx/";
    public int doFileUpload(String fileNameUnderAssets, long datasetId, InputStream is) {
        
        if( (fileNameUnderAssets == null) || (fileNameUnderAssets.trim().length() == 0) )
            return -1;
        
        try {
            String path = TEMP_SD_CARD_PATH;
            
            File f = new File(path);
            if( ! f.exists() )
                f.mkdirs();
            
            //write out to sdcard
            path += fileNameUnderAssets;
            f = new File(path);
            if( ! f.exists() )
            {
                OutputStream out=new FileOutputStream(f);
                byte buf[]=new byte[1024];
                int len;
                while((len=is.read(buf))>0)
                    out.write(buf,0,len);
                out.close();
                is.close();
            }
            
            FileUploadInput request = FileUploadInput
                    .newBuilder()
                    .setUid(getUserId())
                    .setDid(datasetId)
                    .setName(fileNameUnderAssets)
                    .setPath(path)
                    .build();
            FileUploadOutput.Builder responseBuilder = FileUploadOutput
                    .newBuilder();
            int returnValue = getCcdiRpcClient().FileUpload(request, responseBuilder);
            
            if (returnValue != 0) {
                Log.e(LOG_TAG, "Failed to add request to upload file");
                return returnValue;
            }
            
            FileUploadOutput response = responseBuilder.build();
            
            Log.e(LOG_TAG, "FileUpload OK");
            long handle = response.getHandle();
            Log.e(LOG_TAG, "handle: " + handle);
            
            int result = -1;
            
            while(true)
            {
                Log.d(LOG_TAG, "******* checking file upload");

                FileCheckProgressInput checkRequest = FileCheckProgressInput.newBuilder().setHandle(handle).build();
                FileCheckProgressOutput.Builder checkOutputBuilder = FileCheckProgressOutput.newBuilder();
                
                returnValue = getCcdiRpcClient().FileCheckProgress(checkRequest, checkOutputBuilder);
                
                if (returnValue != 0) {
                    Log.e(LOG_TAG, "Failed to add request to upload file");
                    return returnValue;
                }
                
                FileCheckProgressOutput checkResponse = checkOutputBuilder.build();
                Log.d(LOG_TAG, "checkResponse.getCompleted: " + checkResponse.getCompleted());

                if (checkResponse.getCompleted()) {
                    result = checkResponse.getResult();
                    if (checkResponse.getResult() == 0) 
                        Log.d(LOG_TAG, "Uploaded SUCCEEDED");
                    else 
                        Log.e(LOG_TAG, "Upload FAILED: " + checkResponse.getResult());
                    
                    break; //out of while loop
                }
                
                Log.d(LOG_TAG, "File upload operation in progress, transferred: " 
                        + checkResponse.getTransferred() + " size: " +  checkResponse.getSize());
               
                Thread.sleep(50);//ms
            }
            
            return result;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "doFileUpload", e);
            return ERROR_CODE_RPC_LAYER;
        } catch (IOException ioe) {
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            ioe.printStackTrace(pw);
            Log.e(LOG_TAG, sw.toString());
        } catch (InterruptedException e) {
            Log.e(LOG_TAG, "login", e);
        }
        
        return -1;
    }
    
    private static final String TEMP_SD_CARD_DOWNLOAD_PATH = "/sdcard/acerDx/download/";
    public int doFileDownload(String fileName, long datasetId) {
        
        if( (fileName == null) || (fileName.trim().length() == 0) )
            return -1;
        
        try {
            File f = new File(TEMP_SD_CARD_DOWNLOAD_PATH);
            if( ! f.exists() )
                f.mkdirs();
            
            FileDownloadInput request = FileDownloadInput
                    .newBuilder()
                    .setUid(getUserId())
                    .setDid(datasetId)
                    .setName(fileName)
                    .setPath(TEMP_SD_CARD_DOWNLOAD_PATH + fileName)
                    .build();
            FileDownloadOutput.Builder responseBuilder = FileDownloadOutput
                    .newBuilder();
            int returnValue = getCcdiRpcClient().FileDownload(request, responseBuilder);
            
            if (returnValue != 0) {
                Log.e(LOG_TAG, "Failed to add request to Download file");
                return returnValue;
            }
            
            FileDownloadOutput response = responseBuilder.build();
            
            Log.e(LOG_TAG, "FileDownload OK");
            long handle = response.getHandle();
            Log.e(LOG_TAG, "handle: " + handle);
            
            while(true)
            {
                Log.d(LOG_TAG, "******* checking file Download");

                FileCheckProgressInput checkRequest = FileCheckProgressInput.newBuilder().setHandle(handle).build();
                FileCheckProgressOutput.Builder checkOutputBuilder = FileCheckProgressOutput.newBuilder();
                
                returnValue = getCcdiRpcClient().FileCheckProgress(checkRequest, checkOutputBuilder);
                
                if (returnValue != 0) {
                    Log.e(LOG_TAG, "Failed to add request to Download file");
                    return returnValue;
                }
                
                FileCheckProgressOutput checkResponse = checkOutputBuilder.build();
                
                if (checkResponse.getCompleted()) {
                    if (checkResponse.getResult() == 0) 
                        Log.d(LOG_TAG, "Download SUCCEEDED");
                    else 
                        Log.e(LOG_TAG, "Download FAILED: " + checkResponse.getResult());
                    
                    break; //out of while loop
                }
                
                Log.d(LOG_TAG, "File Download operation in progress, transferred: " 
                        + checkResponse.getTransferred() + " size: " +  checkResponse.getSize());
               
                Thread.sleep(50);//ms
            }
            
            return 0;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "doFileDownload", e);
            return ERROR_CODE_RPC_LAYER;
        } catch (InterruptedException e) {
            Log.e(LOG_TAG, "login", e);
            return -1;
        }
        
    }
    
    public int uploadLog(String description, String logString) {

        try {

            String postData = "description=" + URLEncoder.encode(description, "UTF-8") 
                    + "&logData=" + URLEncoder.encode(logString, "UTF-8")
                    + "&userId=" + LAST_LOGIN_USERID;
            Log.i(LOG_TAG, "***** upload log to ops: " + postData);

            InfraHttpRequestInput request = InfraHttpRequestInput
                    .newBuilder()
                    .setSecure(true)
                    .setPrivilegedOperation(false)
                     .setMethod(InfraHttpRequestMethod_t.INFRA_HTTP_METHOD_POST)
//                    .setMethod(InfraHttpRequestMethod_t.INFRA_HTTP_METHOD_GET)
                    .setUrlSuffix("json/logUploadAsString?struts.enableJSONValidation=true&")
                    .setService(InfraHttpService_t.INFRA_HTTP_SERVICE_OPS)
                    .setPostData(postData)
                    .build();
            // .setPostData(jsonBuilder.toString()).build();

            InfraHttpRequestOutput.Builder responseBuilder = InfraHttpRequestOutput
                    .newBuilder();

            int errCode = getCcdiRpcClient()
                    .InfraHttpRequest(request, responseBuilder);
            InfraHttpRequestOutput response = responseBuilder.build();

            // TODO: normalize this.
            // We could also pass error message along to front-end if we want
            // to display more than just an error code.
            if (response.getResponseCode() != 200) {
                Log.i(LOG_TAG,
                        "response code!=200 - " + response.getResponseCode());
                return response.getResponseCode();
            }
            
            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "uploadLog", e);
            return ERROR_CODE_RPC_LAYER;
        }catch (java.io.UnsupportedEncodingException e) {
            Log.e(LOG_TAG, "uploadLog", e);
            return -1;
        }
    }
    
    private String getDeviceNameById(long id) {

        try {
            ListLinkedDevicesInput request = ListLinkedDevicesInput
                    .newBuilder().setUserId(getUserId()).build();
            ListLinkedDevicesOutput.Builder responseBuilder = ListLinkedDevicesOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().ListLinkedDevices(request, responseBuilder);
            if (errCode < 0) {
                Log.e(LOG_TAG, "getDeviceNameById");
                return "";
            }
            ListLinkedDevicesOutput response = responseBuilder.build();
            List<DeviceInfo> nodes = response.getLinkedDevicesList();
            if (null == nodes) {
                return "";
            }
            for (DeviceInfo node : nodes) {
//                Log.i(LOG_TAG, "id: " + node.getDeviceId());
                if (node.getDeviceId() == id) {
                    return node.getDeviceName();
                }
            }
            return "";
        } catch (AppLayerException e) {
            return "";
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getDeviceNameById", e);
            return "";
        }
    }

    public long eventsCreateQueue() {
        try {
            EventsCreateQueueInput request = EventsCreateQueueInput
                    .newBuilder().build();
            EventsCreateQueueOutput.Builder responseBuilder = EventsCreateQueueOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().EventsCreateQueue(request, responseBuilder);
            if (0 > errCode) { return errCode; }

            return responseBuilder.getQueueHandle();
        }
        catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "eventsCreateQueue", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int dumpQueue(long queueHandle, /*int maxCount,*/ int timeout, List<CcdiEvent> eventQueue) {
        try {
            EventsDequeueInput request = EventsDequeueInput
                    .newBuilder()
                    .setQueueHandle(queueHandle)
//                    .setMaxCount(maxCount)
                    .setTimeout(timeout)
                    .build();
            EventsDequeueOutput.Builder responseBuilder = EventsDequeueOutput
                    .newBuilder();

            int errCode = getCcdiRpcClient().EventsDequeue(request, responseBuilder);
            if ( 0 > errCode ) { return errCode; }

            List<CcdiEvent> events = responseBuilder.getEventsList();
            if (eventQueue!=null) {
                for (CcdiEvent event : events) {
                    eventQueue.add(event);
                }
            }
            return errCode;
        }
        catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "dumpQueue", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int destroyQueue(long queueHandle) {
        try {
            EventsDestroyQueueInput request = EventsDestroyQueueInput
                    .newBuilder()
                    .setQueueHandle(queueHandle)
                    .build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();

            int errCode = getCcdiRpcClient().EventsDestroyQueue(request, responseBuilder);
            return errCode;
        }
        catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "destroyQueue", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int getLinkedDeviceState(long userId, long deviceId, ArrayList<HashMap<String, String>> storageNodeList) {

        try {
            ListLinkedDevicesInput request = ListLinkedDevicesInput
                    .newBuilder()
                    .setUserId(userId)
                    .setStorageNodesOnly(true)
                    .setOnlyUseCache(true)
                    .build();
            ListLinkedDevicesOutput.Builder responseBuilder = ListLinkedDevicesOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().ListLinkedDevices(request, responseBuilder);
            if (0 > errCode) {
                return errCode;
            }

            ListLinkedDevicesOutput response = responseBuilder.build();
            List<LinkedDeviceInfo> nodes = response.getDevicesList();

            if (null != nodes) {
                for (LinkedDeviceInfo node : nodes) {
                    if (node.getIsStorageNode() && deviceId == node.getDeviceId()) {
                        HashMap<String, String> storageNode = new HashMap<String, String>();
                        storageNode.put( "device_id", String.valueOf(node.getDeviceId()));
                        storageNode.put( "storage_name", node.getDeviceName());
                        storageNode.put( "device_connection_status", String.valueOf(node.getConnectionStatus().getState().getNumber()));
                        storageNode.put( "standby_since", String.valueOf(node.getConnectionStatus().getStandbySince()));

                        storageNodeList.add(storageNode);
                    }
                }
            }

            return errCode;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getDeviceNameById", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int getLinkedDeviceConnectionState(long userId, long deviceId) {

        try {
            ListLinkedDevicesInput request = ListLinkedDevicesInput
                    .newBuilder()
                    .setUserId(userId)
                    .setStorageNodesOnly(true)
                    .setOnlyUseCache(true)
                    .build();
            ListLinkedDevicesOutput.Builder responseBuilder = ListLinkedDevicesOutput
                    .newBuilder();
            int errCode = getCcdiRpcClient().ListLinkedDevices(request, responseBuilder);
            if (0 > errCode) { return errCode; }

            ListLinkedDevicesOutput response = responseBuilder.build();
            List<LinkedDeviceInfo> nodes = response.getDevicesList();

            int state = -1;//com.acer.ccd.util.CcdSdkDefines.CCD_DEVICE_CONNECTION_OFFLINE;;
            if (null != nodes) {
                for (LinkedDeviceInfo node : nodes) {
                    if (node.getIsStorageNode() && deviceId == node.getDeviceId()) {
                        state = node.getConnectionStatus().getState().getNumber();
                        Log.e(LOG_TAG, "SCSCSC getLinkedDeviceConnectionState state  = " + node.getConnectionStatus().getState()
                                + " numbe = " + node.getConnectionStatus().getState().getNumber());
                    }
                }
            }

            return state;
        } catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "getLinkedDeviceConnectionState", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int remoteWakeup(long userId, long deviceId) {
        try {
            RemoteWakeupInput request = RemoteWakeupInput
                    .newBuilder()
                    .setUserId(userId)
                    .setDeviceToWake(deviceId)
                    .build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();

            int errCode = getCcdiRpcClient().RemoteWakeup(request, responseBuilder);
            return errCode;
        }
        catch (AppLayerException e) {
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "remoteWakeup", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }
    
    public long doPolledSwUpdateDownload( String guid, String version, JSONObject returnJson, long msToSleep, boolean doCancel)
    {
        Log.i( LOG_TAG, "swUpdateBeginDownload Begin!! guid = " + guid + ", version = " + version );
        long a = System.currentTimeMillis();
        long handle = swUpdateBeginDownload( guid , version );
        long b = System.currentTimeMillis();
        Log.i( LOG_TAG, "swUpdateBeginDownload time = " + ( b - a ) + " ms" );
        Log.i ( LOG_TAG, "SCSCSC swUpdateBeginDownload handle = " + handle );
        int rv;
        
        if(doCancel)
        {
            Log.i( LOG_TAG, "about to cancel download" );
            a = System.currentTimeMillis();
            rv = swDownloadCancel(handle);
            b = System.currentTimeMillis();
            Log.i( LOG_TAG, "swDownloadCancel time = " + ( b - a ) + " ms" );
            Log.i ( LOG_TAG, "SCSCSC swDownloadCancel result " + rv );
        }
        
        if ( handle > 0 ) {
            long totalSize = 0;
            long transferredSize = -1;
            SWUpdateDownloadState_t downloadState = SWUpdateDownloadState_t.SWU_DLSTATE_IN_PROGRESS;
            JSONObject output = new JSONObject();
            
            try
            {
                returnJson.put( "result", 0);

                while ( (SWUpdateDownloadState_t.SWU_DLSTATE_IN_PROGRESS == downloadState) && (transferredSize < totalSize)  ) 
                {
                    a = System.currentTimeMillis();
                    rv =  swUpdateGetDownloadProgress( handle, output );
                    b = System.currentTimeMillis();
                    Log.i( LOG_TAG, "swUpdateGetDownloadProgress time = " + ( b - a ) + " ms" + " result: " + rv);
                    
                    downloadState = SWUpdateDownloadState_t.valueOf(output.getInt( "DownloadState" ));
                    transferredSize = output.getLong( "BytesTransferredCnt" );
                    totalSize = output.getLong( "TotalTransferSize" );
                    Log.i( LOG_TAG, "SCSCSC swUpdateGetDownloadProgress downloadState = " + downloadState );
                    Log.i( LOG_TAG, "SCSCSC swUpdateGetDownloadProgress transferredSize = " + transferredSize );
                    Log.i( LOG_TAG, "SCSCSC swUpdateGetDownloadProgress totalSize = " + totalSize );

                    Thread.sleep(msToSleep);//ms
                }
            } catch ( JSONException e ) 
            {
                Utils.logException(e);
                try { returnJson.put( "result", -1);}
                catch ( JSONException ex ) {Utils.logException(ex);}
            }
            catch (InterruptedException e) {
                Utils.logException(e);
                try { returnJson.put( "result", -1);}
                catch ( JSONException ex ) {Utils.logException(ex);}
            }
        }

        return handle;
    }
    
    private static final int SWU_DLSTATE_IN_PROGRESS_VALUE = 1;
    private static final int SWU_DLSTATE_FAILED_VALUE = 2;
    private static final int SWU_DLSTATE_STOPPED_VALUE = 3;
    private static final int SWU_DLSTATE_DONE_VALUE = 4;
    private static final int SWU_DLSTATE_CANCELED_VALUE = 5;
    public long doEventDrivenSwUpdateDownload(long queueHandle, String guid, String version, boolean doCancel)
    {
        Log.i( LOG_TAG, "doEventDrivenSwUpdateDownload Begin!! guid = " + guid + ", version = " + version );
        long a = System.currentTimeMillis();
        long handle = swUpdateBeginDownload( guid , version );
        long b = System.currentTimeMillis();
        Log.i( LOG_TAG, "swUpdateBeginDownload time = " + ( b - a ) + " ms" );
        Log.i ( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload handle = " + handle );
        
        if(doCancel)
        {
            Log.i( LOG_TAG, "about to cancel download" );
            a = System.currentTimeMillis();
            int rv = swDownloadCancel(handle);
            b = System.currentTimeMillis();
            Log.i( LOG_TAG, "swDownloadCancel time = " + ( b - a ) + " ms" );
            Log.i ( LOG_TAG, "SCSCSC swDownloadCancel result " + rv );
        }
        
        boolean done = false;
        
        if ( handle > 0 ) {
        while ( ! done)
        {
            long totalSize = 0;
            long transferredSize = -1;
            SWUpdateDownloadState_t downloadState = SWUpdateDownloadState_t.SWU_DLSTATE_IN_PROGRESS;
            JSONObject output = new JSONObject();
            
            List<CcdiEvent> eventQueue = new ArrayList<CcdiEvent>();
            
            int result = dumpQueue(queueHandle, /*int maxCount,*/ 10000, eventQueue);
            Log.i ( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload dumpQueue result = " + result );
            Log.i ( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload dumpQueue eventQueue size = " + eventQueue.size() );
            
            CcdiEvent event;
            EventSWUpdateProgress progress;
            for(int i = 0; i < eventQueue.size(); i++)
            {
                event = eventQueue.get(i);
                if( ! event.hasSwUpdateProgress())
                {
                    Log.i ( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload event does not have progress " + event + " index: " + i );
                    continue;
                }
                
                progress = event.getSwUpdateProgress();
                switch(progress.getState().getNumber())
                {
                case SWU_DLSTATE_DONE_VALUE:
                    transferredSize = progress.getBytesTransferredCnt();
                    totalSize = progress.getTotalTransferSize();
                    Log.i( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload downloadState = " + SWUpdateDownloadState_t.SWU_DLSTATE_DONE );
                    Log.i( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload transferredSize = " + transferredSize );
                    Log.i( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload totalSize = " + totalSize );
                    done = true;
                    break;
                    
                case SWU_DLSTATE_IN_PROGRESS_VALUE: {
                    transferredSize = progress.getBytesTransferredCnt();
                    totalSize = progress.getTotalTransferSize();
                    Log.i( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload downloadState = " + SWUpdateDownloadState_t.SWU_DLSTATE_IN_PROGRESS );
                    Log.i( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload transferredSize = " + transferredSize );
                    Log.i( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload totalSize = " + totalSize );
                    break;
                    }
                case SWU_DLSTATE_FAILED_VALUE:
                    Log.i( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload Download failed");
                    done = true;
                    break;
                case SWU_DLSTATE_STOPPED_VALUE:
                    Log.i( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload Download stopped");
                    done = true;
                    break;
                case SWU_DLSTATE_CANCELED_VALUE:
                    Log.i( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload Download cancelled");
                    done = true;
                    break;
                default:
                    Log.i( LOG_TAG, "SCSCSC doEventDrivenSwUpdateDownload Download unknown");
                    done = true;
                    break;
                    
                }
            }
        }
        }
        
        return handle;
    }
    
    public long swUpdateBeginDownload( String guid, String version ) {
        try {
            SWUpdateBeginDownloadInput request = SWUpdateBeginDownloadInput
                    .newBuilder()
                    .setAppGuid( guid )
                    .setAppVersion( version )
                    .build();
            SWUpdateBeginDownloadOutput.Builder responseBuilder = SWUpdateBeginDownloadOutput
                    .newBuilder();

            int errCode = getCcdiRpcClient().SWUpdateBeginDownload( request, responseBuilder );
            if ( 0 > errCode ) { return errCode; }

            return responseBuilder.getHandle();
        }
        catch (AppLayerException e) {
            Utils.logException(e);
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "swUpdateBeginDownload", e);
            Utils.logException(e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int swUpdateCheck( String guid, String version, JSONObject output ) {
        try {
            SWUpdateCheckInput request = SWUpdateCheckInput
                    .newBuilder()
                    .setAppGuid( guid )
                    .setAppVersion( version )
                    .build();
            SWUpdateCheckOutput.Builder responseBuilder = SWUpdateCheckOutput
                    .newBuilder();

            int errCode = getCcdiRpcClient().SWUpdateCheck( request, responseBuilder );
            if ( 0 <= errCode ) {
                if ( null == output) {
                    output = new JSONObject();
                }

                output.put( "UpdateMask", responseBuilder.getUpdateMask() );
                output.put( "LatestAppVersion", responseBuilder.getLatestAppVersion() );
                output.put( "LatestCcdVersion", responseBuilder.getLatestCcdVersion() );
                output.put( "ChangeLog", responseBuilder.getChangeLog() );
            }

            return errCode;
        }
        catch (AppLayerException e) {
            Utils.logException(e);
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Utils.logException(e);
            Log.e(LOG_TAG, "swUpdateSetCcdVersion", e);
            return ERROR_CODE_RPC_LAYER;
        }
        catch ( JSONException e ) {
            Utils.logException(e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public static final String TEMP_SD_CARD_SW_PATH = "/sdcard/acerDx/swUpdate/";
    public static final String TEMP_SD_CARD_SW_FILE = "/sdcard/acerDx/swUpdate/appFile";
    
    public int swUpdateEndDownload( long handle, String filePath){
        try {
            File f = new File(TEMP_SD_CARD_SW_PATH);
            if( ! f.exists() )
                f.mkdirs();
            
            f = new File(filePath);
            if( f.exists() )
                f.delete();
            
            Log.i(LOG_TAG, "******** swUpdateEndDownload created file directories");

            SWUpdateEndDownloadInput request = SWUpdateEndDownloadInput
                    .newBuilder()
                    .setHandle( handle )
                    .setFileLocation( filePath )
                    .build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();

            int errCode = getCcdiRpcClient().SWUpdateEndDownload( request, responseBuilder );
            return errCode;
        }
        catch (AppLayerException e) {
            Utils.logException(e);
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Utils.logException(e);
            Log.e(LOG_TAG, "swUpdateSetCcdVersion", e);
            return ERROR_CODE_RPC_LAYER;
        }catch (SecurityException e) {
            Utils.logException(e);
            return -1;
        }
    }

    public int swUpdateGetDownloadProgress( long handle, JSONObject output ) {
        try {
            SWUpdateGetDownloadProgressInput request = SWUpdateGetDownloadProgressInput
                    .newBuilder()
                    .setHandle( handle )
                    .build();
            SWUpdateGetDownloadProgressOutput.Builder responseBuilder = SWUpdateGetDownloadProgressOutput
                .newBuilder();

            int errCode = getCcdiRpcClient().SWUpdateGetDownloadProgress( request, responseBuilder );
            if ( 0 <= errCode ) {
                if ( null == output) {
                    output = new JSONObject();
                }

                output.put( "BytesTransferredCnt", responseBuilder.getBytesTransferredCnt() );
                output.put( "TotalTransferSize", responseBuilder.getTotalTransferSize() );
                output.put( "DownloadState", responseBuilder.getState().getNumber() );
            }

            return errCode;
        }
        catch (AppLayerException e) {
            Utils.logException(e);
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Utils.logException(e);
            Log.e(LOG_TAG, "swUpdateSetCcdVersion", e);
            return ERROR_CODE_RPC_LAYER;
        }
        catch ( JSONException e ) {
            Utils.logException(e);
            return ERROR_CODE_RPC_LAYER;
        }
    }

    public int swUpdateSetCcdVersion( String guid, String version ) {
        try {
            SWUpdateSetCcdVersionInput request = SWUpdateSetCcdVersionInput
                    .newBuilder()
                    .setCcdGuid( guid )
                    .setCcdVersion( version )
                    .build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();

            int errCode = getCcdiRpcClient().SWUpdateSetCcdVersion( request, responseBuilder );
            return errCode;
        }
        catch (AppLayerException e) {
            Utils.logException(e);
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Utils.logException(e);
            Log.e(LOG_TAG, "swUpdateSetCcdVersion", e);
            return ERROR_CODE_RPC_LAYER;
        }
    }
    
    public int swDownloadCancel(long handle)
    {
        int rv = -1;
        try
        {
            SWUpdateCancelDownloadInput request =  SWUpdateCancelDownloadInput
                    .newBuilder()
                    .setHandle(handle)
                    .build();
            NoParamResponse.Builder responseBuilder = NoParamResponse
                    .newBuilder();
            
            rv = getCcdiRpcClient().SWUpdateCancelDownload( request, responseBuilder );
            
            if ( rv != 0 ) 
                Log.i(LOG_TAG, "******** swDownloadCancel Failed SWUpdateCancelDownload");
            else
                Log.i(LOG_TAG, "******** swDownloadCancel Download canceled!");
        }
        catch (AppLayerException e) {
            Utils.logException(e);
            return e.getAppStatus();
        } catch (ProtoRpcException e) {
            Utils.logException(e);
            Log.e(LOG_TAG, "swUpdateSetCcdVersion", e);
            return ERROR_CODE_RPC_LAYER;
        }

        return rv;
    }
    
}
