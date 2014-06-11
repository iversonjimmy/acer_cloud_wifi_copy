//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.acer.ccd.serviceclient;

import com.acer.ccd.R;
import com.acer.ccd.util.CcdSdkDefines.FileExplorer;
import com.acer.ccd.util.FileInfo;
import com.acer.ccd.util.Utility;
import com.acer.ccd.util.igware.Constants;
import com.acer.ccd.util.igware.Dataset;
import com.acer.ccd.util.igware.DownloadProgress;
import com.acer.ccd.util.igware.Filter;
import com.acer.ccd.util.igware.Subfolder;
import com.acer.ccd.util.igware.Subfolder.Type;
import com.acer.ccd.util.igware.SyncListItem;
import com.acer.ccd.util.igware.SyncListItem.State;

import org.json.JSONObject;

import android.content.Context;
import android.util.Log;

import igware.gvm.pb.CcdTypes.OwnedTitleMenuData;
import igware.gvm.pb.CcdiRpc.AddDatasetInput;
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
import igware.vplex.pb.VsDirectoryServiceTypes.DatasetDetail;
import igware.vplex.pb.VsDirectoryServiceTypes.DatasetType;
import igware.vplex.pb.VsDirectoryServiceTypes.Subscription;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Locale;


public class CcdiClient {

    public static final int ERROR_CODE_RPC_LAYER = -100;

    private static final String LOG_TAG = "CcdiClient";

    public static final String CONTENT_TYPE_CAMERA = "CAMERA";

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
                return response.getPlayers().getPlayers(0).getUserId();
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

    public boolean subscribeDataset(Dataset folder) {
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
            return result >= 0;
        } catch (AppLayerException e) {
            return false;
        } catch (ProtoRpcException e) {
            Log.e(LOG_TAG, "subscribe", e);
            return false;
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
                    if (!subscribeDataset(ds)) {
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
}
