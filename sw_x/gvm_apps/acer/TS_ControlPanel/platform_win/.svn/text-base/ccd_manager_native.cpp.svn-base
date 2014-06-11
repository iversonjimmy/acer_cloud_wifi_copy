//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "stdafx.h"
#include "ccd_manager_native.h"

#pragma unmanaged

#include <algorithm>
#include <fstream>
#include <map>
#include <ccdi.hpp>
#include <windows.h>

using namespace vplex::vsDirectory;

CcdManagerNative::CcdManagerNative()
{
    mUserId = 0;
}

CcdManagerNative::~CcdManagerNative()
{
}

int CcdManagerNative::AddSubscription(int dataSetIndex)
{
    if (mDataSets[dataSetIndex].type == CR_DOWN) {
        return this->AddSubscription(dataSetIndex, ccd::SUBSCRIPTION_TYPE_CONSUMER);
    } else if (mDataSets[dataSetIndex].type == CLEAR_FI) {
        int rv = this->AddSubscription(dataSetIndex, ccd::SUBSCRIPTION_TYPE_CLEARFI_CLIENT);
        if (rv != 0) {
            return rv;
        }
        return this->AddSubscription(dataSetIndex, ccd::SUBSCRIPTION_TYPE_CLEARFI_SERVER);
    }

    return this->AddSubscription(dataSetIndex, ccd::SUBSCRIPTION_TYPE_NORMAL);
}

int CcdManagerNative::AddSubscription(int dataSetIndex, ccd::SyncSubscriptionType_t type)
{
    if (dataSetIndex < 0 || dataSetIndex > this->NumDataSets()) {
        return 0;
    }

    ccd::AddSyncSubscriptionInput request;

    request.set_user_id(mUserId);
    request.set_dataset_id(mDataSets[dataSetIndex].id);
    request.set_subscription_type(type);

    int rv = CCDIAddSyncSubscription(request);
    if (rv == 0 || rv == -32228) {
        // -32228 means subscription has already been added
        // that error can be ignored
        rv = 0;
        mDataSets[dataSetIndex].isSubscribed = true;
    }

    return rv;
}

int CcdManagerNative::DeleteSubscription(int dataSetIndex)
{
    if (dataSetIndex < 0 || dataSetIndex > this->NumDataSets()) {
        return 0;
    }

    ccd::DeleteSyncSubscriptionsInput request;

    request.set_user_id(mUserId);
    request.add_dataset_ids(mDataSets[dataSetIndex].id);

    int rv = CCDIDeleteSyncSubscriptions(request);
    if (rv == 0) {
        mDataSets[dataSetIndex].isSubscribed = false;
    }

    return rv;
}

std::wstring CcdManagerNative::GetDataSetCurrentDirName(int index)
{
    if (index < 0 || index > this->NumDataSets()) {
        return std::wstring(L"");
    }

    return mDataSets[index].currentDirName;
}

std::wstring CcdManagerNative::GetDataSetCurrentDirEntryName(int dataSetIndex, int entryIndex)
{
    if (dataSetIndex < 0 || dataSetIndex > this->NumDataSets()) {
        return std::wstring(L"");
    }

    if (entryIndex < 0 || entryIndex > this->NumDataSetCurrentDirEntries(dataSetIndex)) {
        return std::wstring(L"");
    }

    return mDataSets[dataSetIndex].currentDirEntries[entryIndex].name;
}

std::wstring CcdManagerNative::GetDataSetName(int index)
{
    if (index < 0 || index > this->NumDataSets()) {
        return std::wstring(L"");
    }

    return mDataSets[index].name;
}

int CcdManagerNative::GetDataSetType(int index)
{
    if (index < 0 || index > this->NumDataSets()) {
        return 0;
    }

    return mDataSets[index].type;
}

std::wstring CcdManagerNative::GetDeviceName()
{
    ccd::GetSyncStateInput request;
    ccd::GetSyncStateOutput response;

    request.set_user_id(mUserId);
    request.set_get_device_name(true);

    CCDIGetSyncState(request, response);

    return this->ConvertToUtf16(response.my_device_name());
}

std::wstring CcdManagerNative::GetFilter(int dataSetIndex)
{
    if (dataSetIndex < 0 || dataSetIndex > this->NumDataSets()) {
        return std::wstring(L"");
    }

    return mDataSets[dataSetIndex].filter;
}

std::wstring CcdManagerNative::GetLocation()
{
    ccd::GetSyncStateInput request;
    ccd::GetSyncStateOutput response;

    request.set_user_id(mUserId);
    request.set_get_my_cloud_root(true);

    CCDIGetSyncState(request, response);

    return this->ConvertToUtf16(response.my_cloud_root());
}

std::wstring CcdManagerNative::GetUrl()
{
    ccd::GetInfraHttpInfoInput request;
    ccd::GetInfraHttpInfoOutput response;

    request.set_service(ccd::INFRA_HTTP_SERVICE_OPS);
    request.set_secure(true);

    CCDIGetInfraHttpInfo(request, response);

    return this->ConvertToUtf16(response.url_prefix());
}

bool CcdManagerNative::IsDataSetCurrentDirEntryDir(int dataSetIndex, int entryIndex)
{
    if (dataSetIndex < 0 || dataSetIndex > this->NumDataSets()) {
        return false;
    }

    if (entryIndex < 0 || entryIndex > this->NumDataSetCurrentDirEntries(dataSetIndex)) {
        return false;
    }

    return mDataSets[dataSetIndex].currentDirEntries[entryIndex].isDir;
}

bool CcdManagerNative::IsDeviceLinked()
{
    ccd::GetSyncStateInput request;
    ccd::GetSyncStateOutput response;

    request.set_user_id(mUserId);

    CCDIGetSyncState(request, response);

    return response.is_device_linked();
}

bool CcdManagerNative::IsLoggedIn()
{
    return mUserId != 0;
}

bool CcdManagerNative::IsSubscribed(int dataSetIndex)
{
    if (dataSetIndex < 0 || dataSetIndex > this->NumDataSets()) {
        return false;
    }

    return mDataSets[dataSetIndex].isSubscribed;
}

bool CcdManagerNative::IsSync()
{
    ccd::GetSyncStateInput request;
    ccd::GetSyncStateOutput response;

    request.set_user_id(mUserId);

    CCDIGetSyncState(request, response);

    return response.is_sync_agent_enabled();
}

int CcdManagerNative::Link(const wchar_t* deviceName)
{
    ccd::LinkDeviceInput request;
    std::string name = this->ConvertToUtf8(std::wstring(deviceName));

    request.set_user_id(mUserId);
    request.set_device_name(name);
	//Add by Vincent, PC version always installs on Acer Machine by spec., 20111006
	request.set_is_acer_device(true);

    return CCDILinkDevice(request);
}

int CcdManagerNative::Login(const wchar_t* user)
{
    return this->Login(user, NULL);
}

int CcdManagerNative::Login(const wchar_t* user, const wchar_t* pass)
{
    ccd::LoginInput request;
    ccd::LoginOutput response;
    std::string tuser = this->ConvertToUtf8(std::wstring(user));
    
    request.set_user_name(tuser);
    if (pass != NULL) {
        std::string tpass = this->ConvertToUtf8(std::wstring(pass));
        request.set_password(tpass);
    }
    request.set_save_login_token(true);
    
    int rv = CCDILogin(request, response);
    if (rv == 0) {
        mUserId = response.user_id();
    } else {
        mUserId = 0;
    }

    return rv;
}

int CcdManagerNative::Logout()
{
    ccd::LogoutInput request;

    request.set_local_user_id(mUserId);

    int rv = CCDILogout(request);
    // Treat as success; see Bug 10191.
    if (rv == CCD_ERROR_NOT_SIGNED_IN) {
        rv = 0;
    }
    if (rv == 0) {
        mUserId = 0;
    }

    return rv;
}

int CcdManagerNative::NumDataSetCurrentDirEntries(int dataSetIndex)
{
    if (dataSetIndex < 0 || dataSetIndex > this->NumDataSets()) {
        return 0;
    }

    return (int)mDataSets[dataSetIndex].currentDirEntries.size();
}

int CcdManagerNative::NumDataSets()
{
    return (int)mDataSets.size();
}

int CcdManagerNative::NumFilesDownload()
{
    // TODO: fix me
#if 0
    ccd::GetSyncStateInput request;
    ccd::GetSyncStateOutput response;

    request.set_user_id(mUserId);
    request.set_get_sync_state_summary(true);

    CCDIGetSyncState(request, response);

    ccd::SyncStateSummary summary = response.sync_state_summary();
    return summary.num_downloading() + summary.num_to_download();
#else
    return 0;
#endif
}

int CcdManagerNative::NumFilesUpload()
{
    // TODO: fix me
#if 0
    ccd::GetSyncStateInput request;
    ccd::GetSyncStateOutput response;

    request.set_user_id(mUserId);
    request.set_get_sync_state_summary(true);

    CCDIGetSyncState(request, response);

    ccd::SyncStateSummary summary = response.sync_state_summary();
    return summary.num_uploading() + summary.num_to_upload();
#else
    return 0;
#endif
}

int CcdManagerNative::RegisterStorageNode() 
{
	ccd::RegisterStorageNodeInput request;	
	request.set_user_id(mUserId);

	return CCDIRegisterStorageNode(request);
}
									 
int CcdManagerNative::UnregisterStorageNode() 
{
	ccd::UnregisterStorageNodeInput request;
	request.set_user_id(mUserId);

	return CCDIUnregisterStorageNode(request);
}

int CcdManagerNative::Register(const wchar_t* user, const wchar_t* pass, const wchar_t* email)
{
    char buffer[32] = {0};
    std::string postData;
    ccd::InfraHttpRequestInput request;
    ccd::InfraHttpRequestOutput response;
    std::string tuser = this->ConvertToUtf8(std::wstring(user));
    std::string tpass = this->ConvertToUtf8(std::wstring(pass));
    std::string temail = this->ConvertToUtf8(std::wstring(email));

    // use URL encoding not JSON

    postData = "userName=";
    postData += tuser;
    postData += "&userEmail=";
    postData += temail;
    postData += "&userPwd=";
    postData += tpass;
    postData += "&reenterUserPwd=";
    postData += tpass;
	postData += "&regKey=";
	postData += "4444"; //hard code this for now

    request.set_service(ccd::INFRA_HTTP_SERVICE_OPS);
    request.set_secure(true);
    request.set_privileged_operation(false);
    request.set_method(ccd::INFRA_HTTP_METHOD_POST);
    request.set_url_suffix("json/register?struts.enableJSONValidation=true");
    request.set_post_data(postData);

    int rv = CCDIInfraHttpRequest(request, response);
    if (rv != 0) {
        return rv;
    }

    // bad fields submitted
    rv = -1;

    // find status code in response
    // bad format:  {...,"statusCode":###,...}
    // good format: {"statusCode":0}
    size_t start = response.http_response().find("statusCode");
    if (start != std::string::npos) {
        start = response.http_response().find(':', start) + 1;
        if (start != std::string::npos) {
            size_t end = response.http_response().find(',', start);
            if (end == std::string::npos) {
                end = response.http_response().find('}', start);
            }
            if (end != std::string::npos) {
                int index = 0;
                for (size_t i = start; i < end; i++) {
                    if (index < 31) {
                        buffer[index++] = (response.http_response())[i];
                    }
                }
                rv = -(atoi(buffer));
            }
        }
    }

    return rv;
}

int CcdManagerNative::SetDataSetCurrentDir(int index, const wchar_t* path)
{
    if (index < 0 || index > this->NumDataSets()) {
        return 0;
    }

    // do nothing for non-USER and non-MEDIA data sets
    if (mDataSets[index].type != USER && mDataSets[index].type != MEDIA) {
        mDataSets[index].currentDirName = std::wstring(path);
        mDataSets[index].currentDirEntries.clear();
        return 0;
    }

    ccd::GetDatasetDirectoryEntriesInput request;
    ccd::GetDatasetDirectoryEntriesOutput response;
    std::string tpath = this->ConvertToUtf8(std::wstring(path));

    request.set_user_id(mUserId);
    request.set_dataset_id(mDataSets[index].id);
    request.set_directory_name(tpath);

    int rv = CCDIGetDatasetDirectoryEntries(request, response);
    if (rv != 0) {
        return rv;
    }

    mDataSets[index].currentDirName = std::wstring(path);
    mDataSets[index].currentDirEntries.clear();

    std::map<std::wstring, DirEntry> directories;
    std::map<std::wstring, DirEntry> files;
    std::map<std::wstring, DirEntry>::iterator it;

    for (int i = 0; i < response.entries_size(); i++) {
        ccd::DatasetDirectoryEntry entry = response.entries(i);

        DirEntry dirEntry;
        dirEntry.isDir = entry.is_dir();
        dirEntry.name = this->ConvertToUtf16(entry.name());

        // sort alphabetically after tolower()

        std::wstring name = dirEntry.name;
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);

        if (dirEntry.isDir == true) {
            directories.insert(std::pair<std::wstring, DirEntry>(name, dirEntry));
        } else {
            files.insert(std::pair<std::wstring, DirEntry>(name, dirEntry));
        }
    }

    // sort by directories first then files

    for (it = directories.begin(); it != directories.end(); ++it) {
        mDataSets[index].currentDirEntries.push_back((*it).second);
    }
    for (it = files.begin(); it != files.end(); ++it) {
        mDataSets[index].currentDirEntries.push_back((*it).second);
    }

    return rv;
}

int CcdManagerNative::SetDeviceName(const wchar_t* name)
{
    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;
    std::string tname = this->ConvertToUtf8(std::wstring(name));

    request.set_user_id(mUserId);
    request.set_set_my_device_name(tname);

    int rv = CCDIUpdateSyncSettings(request, response);
    if (rv == 0) {
        rv = response.set_my_device_name_err();
    }

    return rv;
}

int CcdManagerNative::SetFilter(int dataSetIndex, const wchar_t* filter)
{
    if (dataSetIndex < 0 || dataSetIndex > this->NumDataSets()) {
        return 0;
    }

    ccd::UpdateSyncSubscriptionInput request;
    std::string tfilter = this->ConvertToUtf8(std::wstring(filter));

    request.set_user_id(mUserId);
    request.set_dataset_id(mDataSets[dataSetIndex].id);
    request.set_new_filter(tfilter);

    return CCDIUpdateSyncSubscription(request);
}

int CcdManagerNative::SetLocation(const wchar_t* path)
{
    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;
    std::string tpath = this->ConvertToUtf8(std::wstring(path));

    request.set_user_id(mUserId);
    request.set_set_my_cloud_root(tpath);

    int rv = CCDIUpdateSyncSettings(request, response);
    if (rv == 0) {
        rv = response.set_my_cloud_root_err();
    }

    return rv;
}

int CcdManagerNative::SetSync(bool enable)
{
    ccd::UpdateSyncSettingsInput request;
    ccd::UpdateSyncSettingsOutput response;

    request.set_user_id(mUserId);
    request.set_enable_sync_agent(enable);

    int rv = CCDIUpdateSyncSettings(request, response);
    if (rv == 0) {
        rv = response.enable_sync_agent_err();
    }

    return rv;
}

int CcdManagerNative::Unlink()
{
    ccd::UnlinkDeviceInput request;

    request.set_user_id(mUserId);

    return CCDIUnlinkDevice(request);
}


// Add by Vincent, 20111006, Unlink with device ID (Unlink remote devices)
int CcdManagerNative::Unlink(::google::protobuf::uint64 deviceID)
{
	ccd::UnlinkDeviceInput request;

	request.set_user_id(mUserId);
	request.set_device_id(deviceID);

	return CCDIUnlinkDevice(request);
}

int CcdManagerNative::UpdateDataSets()
{
    ccd::ListOwnedDatasetsInput drequest;
    ccd::ListOwnedDatasetsOutput dresponse;
    ccd::ListSyncSubscriptionsInput srequest;
    ccd::ListSyncSubscriptionsOutput sresponse;

    drequest.set_user_id(mUserId);
    srequest.set_user_id(mUserId);

    int rv = CCDIListOwnedDatasets(drequest, dresponse);
    if (rv != 0) {
        return rv;
    }

    rv = CCDIListSyncSubscriptions(srequest, sresponse);
    if (rv != 0) {
        return rv;
    }

    mDataSets.clear();

    std::map<std::wstring, DataSet> sets;
    std::map<std::wstring, DataSet>::iterator it;

    for (int i = 0; i < dresponse.dataset_details_size(); i++) {
        vplex::vsDirectory::DatasetDetail details = dresponse.dataset_details(i);

        DataSet set;
        set.id = details.datasetid();
        set.isSubscribed = false;
        set.name = this->ConvertToUtf16(details.datasetname());
        set.type = details.datasettype();
        set.filter = std::wstring(L"");
        set.currentDirName = std::wstring(L"/");

        // sort alphabetically after tolower()

        std::wstring name = set.name;
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        sets.insert(std::pair<std::wstring, DataSet>(name, set));
    }

    for (it = sets.begin(); it != sets.end(); ++it) {
        mDataSets.push_back((*it).second);
    }

    for (int i = 0; i < sresponse.subscriptions_size(); i++) {
        vplex::vsDirectory::Subscription details = sresponse.subscriptions(i);
        
        for (int j = 0; j < this->NumDataSets(); j++) {
            if (details.datasetid() == mDataSets[j].id) {
                mDataSets[j].isSubscribed = true;
                mDataSets[j].filter = this->ConvertToUtf16(details.filter());
                break;
            }
        }
    }

    return 0;
}

//Add by Vincent, 20111006, List linked devices
int CcdManagerNative::ListLinkedDevices()
{
	ccd::ListLinkedDevicesInput drequest;
	ccd::ListLinkedDevicesOutput dresponse;
	
	drequest.set_user_id(mUserId);
	
	int rv = CCDIListLinkedDevices(drequest, dresponse);
	if (rv != 0) {
		return rv;
	}

	mDeviceInfo.clear();

	std::map<std::wstring, CloudDeviceInfo> devices;
	std::map<std::wstring, CloudDeviceInfo>::iterator it;

	for (int i = 0; i < dresponse.linked_devices_size(); i++) {
		//vplex::vsDirectory::DatasetDetail details = dresponse.dataset_details(i);
		vplex::vsDirectory::DeviceInfo details = dresponse.linked_devices(i);
		
		CloudDeviceInfo device;
		device.deviceID = details.deviceid();
		device.devicename = this->ConvertToUtf16(details.devicename());
		device.deviceclass = this->ConvertToUtf16(details.deviceclass());
		device.isAcerMachine = details.isacer();
		device.isAndroidDevice = details.hascamera();

		/*DataSet set;
		set.id = details.datasetid();
		set.isSubscribed = false;
		set.name = this->ConvertToUtf16(details.datasetname());
		set.type = details.datasettype();
		set.filter = std::wstring(L"");
		set.currentDirName = std::wstring(L"/");*/

		// sort alphabetically after tolower()
		std::wstring name = device.devicename;
		std::transform(name.begin(), name.end(), name.begin(), ::towlower);
		devices.insert(std::pair<std::wstring, CloudDeviceInfo>(name, device));

		/*std::wstring name = set.name;
		std::transform(name.begin(), name.end(), name.begin(), ::towlower);
		sets.insert(std::pair<std::wstring, DataSet>(name, set));*/


	}

	/*for (it = sets.begin(); it != sets.end(); ++it) {
		mDataSets.push_back((*it).second);
	}*/

	for (it = devices.begin(); it != devices.end(); ++it) {
		mDeviceInfo.push_back((*it).second);
	}

	//for (int i = 0; i < sresponse.subscriptions_size(); i++) {
	//	vplex::vsDirectory::Subscription details = sresponse.subscriptions(i);

	//	for (int j = 0; j < this->NumDataSets(); j++) {
	//		if (details.datasetid() == mDataSets[j].id) {
	//			mDataSets[j].isSubscribed = true;
	//			mDataSets[j].filter = this->ConvertToUtf16(details.filter());
	//			break;
	//		}
	//	}
	//}

	return 0;
}

int CcdManagerNative::NumDevices()
{
	return (int)mDeviceInfo.size();
}

std::wstring CcdManagerNative::GetDeviceInfo_Name(int index)
{
	if (index < 0 || index > this->NumDevices()) {
		return std::wstring(L"");
	}

	return mDeviceInfo[index].devicename;
}

std::wstring CcdManagerNative::GetDeviceInfo_Class(int index)
{
	if (index < 0 || index > this->NumDevices()) {
		return std::wstring(L"");
	}

	return mDeviceInfo[index].deviceclass;
}

::google::protobuf::int64 CcdManagerNative::GetDeviceInfo_ID(int index)
{
	if (index < 0 || index > this->NumDevices()) {
		return 0;
	}

	return mDeviceInfo[index].deviceID;
}

bool CcdManagerNative::GetDeviceInfo_IsAcerMachine(int index)
{
	if (index < 0 || index > this->NumDevices()) {
		return 0;
	}

	return mDeviceInfo[index].isAcerMachine;
}

bool CcdManagerNative::GetDeviceInfo_IsAndroidDevice(int index)
{
	if (index < 0 || index > this->NumDevices()) {
		return 0;
	}

	return mDeviceInfo[index].isAndroidDevice;
}


std::string CcdManagerNative::ConvertToUtf8(const wchar_t* buffer, int length)
{
    int nChars = ::WideCharToMultiByte(CP_UTF8, 0, buffer, length, NULL, 0, NULL, NULL);
    if (nChars == 0) {
        return "";
    }

    std::string newbuffer;
    newbuffer.resize(nChars);
    ::WideCharToMultiByte(
        CP_UTF8,
        0,
        buffer,
        length,
        const_cast<char*>(newbuffer.c_str()),
        nChars,
        NULL,
        NULL); 

    return newbuffer;
}

std::string CcdManagerNative::ConvertToUtf8(const std::wstring& input)
{
    return this->ConvertToUtf8(input.c_str(), (int)input.size());
}

std::wstring CcdManagerNative::ConvertToUtf16(const char* buffer, int length)
{
    int nChars = ::MultiByteToWideChar(CP_UTF8, 0, buffer, length, NULL, 0);
    if (nChars == 0) {
        return L"";
    }

    std::wstring newbuffer;
    newbuffer.resize(nChars);
    ::MultiByteToWideChar(
        CP_UTF8,
        0,
        buffer,
        length,
        const_cast<wchar_t*>(newbuffer.c_str()),
        nChars); 

    return newbuffer;
}

std::wstring CcdManagerNative::ConvertToUtf16(const std::string& input)
{
    return this->ConvertToUtf16(input.c_str(), (int)input.size());
}

#pragma managed
