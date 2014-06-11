#include "StdAfx.h"
#include "ShlExtUtility.h"
#include "Shlobj.h"
#include <algorithm>
#include <fstream>

ShlExtUtility::ShlExtUtility(void)
{
}

ShlExtUtility::~ShlExtUtility(void)
{
}

int ShlExtUtility::ConvertToTString(const char* astring, TCHAR* tstring) {
	int nwchars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, astring, -1, NULL, 0);
    if (nwchars == 0) {
        return -1;
    }


    nwchars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, astring, -1, tstring, nwchars);
    if (nwchars == 0) {
        return -1;
    }

	return 0;
}

int ShlExtUtility::ConvertFromTString(const TCHAR* tstring, char* astring, int abuflen) {
	int nbytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, tstring, -1, NULL, 0, NULL, NULL);
    if (nbytes == 0) {
        return -1;
    }
    if (nbytes > abuflen) {//??
        return -1;
    }

    nbytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, tstring, -1, astring, abuflen, NULL, NULL);
    if (nbytes == 0) {
        return -1;
    }
	return 0;

}

/*
* From app.config to get log level. 
* INFO=0; DEBUG=1; ERROR=2
*/
int ShlExtUtility::SetLogLevel()
{
	std::string filename = "app.config";
	std::ifstream logfile;
	int loglevel = 0;
	char letter;

	logfile.open(filename.c_str());
	if (!logfile) {
		LOGSetLevel(LOG_LEVEL_INFO, 1);//default one
		return 0;
	}

	logfile >> letter;
	while (letter != '=') {
		logfile >> letter;
	}
	logfile >> loglevel;
	
	//disable first
	LOG_DISABLE_LEVEL(LOG_LEVEL_INFO);
	LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

	if (loglevel == 0 ) {
		LOGSetLevel(LOG_LEVEL_INFO, 1);
	} else if (loglevel == 1){
		LOGSetLevel(LOG_LEVEL_INFO, 1);
		LOGSetLevel(LOG_LEVEL_DEBUG, 1);
	} else if (loglevel == 2) {
		LOGSetLevel(LOG_LEVEL_ERROR, 1);
	} else {
		LOGSetLevel(LOG_LEVEL_INFO, 1);
	}

	logfile.close();
	return loglevel;
}
 


int ShlExtUtility::GetLogDirectory(TCHAR* path){
	HRESULT result = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path);
	_tcscat_s(path, MAX_PATH, _T("\\iGware\\SyncAgent"));

	if (result == S_OK) 
		return 0;
	else 
		return -1;
}

/*
* Copy file/directory from source to destination
*/
void ShlExtUtility::CopyItem(LPCTSTR source, LPCTSTR destination){
	SHFILEOPSTRUCT file_op = { 0 };
    file_op.hwnd = NULL;
    file_op.wFunc = FO_COPY;
    file_op.pTo = destination;
    file_op.pFrom = source;
    //s.fFlags = FOF_SILENT;
    
	if(SHFileOperation( &file_op )!=0) {
        // error occured
        MessageBox(NULL, L"SHFileOperation failed", L"Error", MB_OK);
    }

}

/*
* Move file/directory from source to destination
*/
void ShlExtUtility::MoveItem(LPCTSTR source, LPCTSTR destination){
	SHFILEOPSTRUCT file_op = { 0 };
    file_op.hwnd = NULL;
    file_op.wFunc = FO_MOVE;
    file_op.pTo = destination;
    file_op.pFrom = source;
    //s.fFlags = FOF_SILENT;
    
	if(SHFileOperation( &file_op )!=0) {
        // error occured
        MessageBox(NULL, L"SHFileOperation failed", L"Error", MB_OK);
    }

}

/*
* Based on the path, decide the type. 1 means directory, 0 means file, -1 means others. 
*/
int ShlExtUtility::GetType(TCHAR* path ){
	int rc=-1;
	struct _stat64 filestat;
	rc = _tstat64(path, &filestat);

	if ((filestat.st_mode &  _S_IFDIR) ==  _S_IFDIR ) {
		rc=1;
	}
	else if((filestat.st_mode & _S_IFREG) == _S_IFREG ) {
		rc=0;
	}
	else {
		rc=-1;
	}
	return rc;
}

/*
* Retrieve the dataset name based on the passing path parameter. This is used for the Context Menu
* "View on Web". 
*/
int ShlExtUtility::GetDirectoryStr(TCHAR* path, std::string rootPathStr, std::string& dirStr) {
	char pathChar[MAX_PATH];
	int result = ShlExtUtility::ConvertFromTString(path, pathChar, MAX_PATH);
	if (result==-1 || &pathChar == NULL){
		return -1;
	}
	std::string pathStr(pathChar);

	size_t rootPathLength = rootPathStr.size();
	size_t pathLength = pathStr.size();
	std::replace(rootPathStr.begin(), rootPathStr.end(), '/', '\\');
	LOG_INFO("ROOT PATH %s", rootPathStr.c_str());
	size_t found = pathStr.find(rootPathStr);
	if ( found != std::string::npos) { //find
		std::string subStr = pathStr.substr(rootPathLength+1);
		//change \ to /
		std::replace(subStr.begin(), subStr.end(), '\\', '/');
		LOG_INFO("substring %s", subStr.c_str());
		//if it's file, the filename will be removed from the subStr
		int type = ShlExtUtility::GetType(path);
		if (type==0) {//file
			size_t index = subStr.find_last_of('/');
			if (index!=std::string::npos) {
				dirStr = subStr.substr(0, index);
			}else {
				dirStr = subStr.substr(0, 0);
			}
		}else if (type ==1) {//dir 
			dirStr = subStr;
		}else {
			LOG_ERROR("Passing path %s is file or directory", pathStr.c_str());
			return -1;
		}

	}else {
		LOG_ERROR("Passing path %s is not under root path %s", pathStr.c_str(), rootPathStr.c_str());
		return -1;
	}
	
	return 0;
}


/*
* 
* Retrieve the list of sync folder names for context submenu for regular folder/file. 
*/
int ShlExtUtility::ListSyncFolderName(std::vector<std::string>& syncFolderName, std::vector<std::string>& deviceRootPath) {
	ccd::ListSyncSubscriptionsInput  request;
	// Since we are only monitoring things, there is no need to check the infrastructure.
	// This will give us the exact list that SyncAgent knows about currently.
	request.set_only_use_cache(true);
	ccd::ListSyncSubscriptionsOutput  response;

	CCDIError error = CCDIListSyncSubscriptions(request, response);
	if (error!=CCD_OK){
		LOG_ERROR("CCDIListSyncSubscriptions call fail with error code %d", error);
		return -1;
	}
	
	const int size = response.subs_size();
	LOG_INFO("The size of ListSyncFolder is %d", size);
	syncFolderName.clear();
	deviceRootPath.clear();
	for (int i=0; i<size; i++) {
		syncFolderName.push_back(response.subs(i).dataset_details().datasetname());
		deviceRootPath.push_back(response.subs(i).absolute_device_root());
	}

	return 0;

}

/*
* Retrieve "My Cloud" root path and Make sure using "\", instead of "/"
*/
int ShlExtUtility::GetMyCloudRootPath(std::string& myCldPath) {
	/*
	ccd::GetSyncStateInput request;
	ccd::GetSyncStateOutput response;
	request.set_get_my_cloud_root(true);

	CCDIError error = CCDIGetSyncState(request, response);
	if (error!=CCD_OK){
		LOG_ERROR("CCDIGetSyncState call fail with error code %d", error);
		return -1;
	}
	
	myCldPath = response.my_cloud_root();
	std::replace(myCldPath.begin(), myCldPath.end(), '/', '\\');
	*/
	
	return 0;
}

/*
* Check if path is under My Cloud rootPath
*/
BOOL ShlExtUtility::IsUdrMyCloudRoot(TCHAR* path, TCHAR* rootPath) {
	size_t rootPathLength = _tcslen(rootPath);

	for (int i=0; (size_t)i<rootPathLength; i++){
		if (path[i]!=rootPath[i]) {
			LOG_INFO("Current folder is NOT under My Cloud folder.");
			return false;
		}
	}
	return true;
}

/*
* Check if path is (or under) SyncFolder
*/
int ShlExtUtility::SyncFolderCheck(std::string pathStr, BOOL& isSyncFolder, BOOL& inSyncFolder) {
	ccd::GetSyncStateInput request;
	ccd::GetSyncStateOutput response;
	request.add_get_sync_states_for_paths(pathStr);
	
	CCDIError error = CCDIGetSyncState(request, response);
	
	if (error!=CCD_OK){
		LOG_ERROR("CCDIGetSyncState for checking IsSyncFolder call fail with error code %d", error);
		return -1;
	}

	ccd::ObjectSyncState oss = response.sync_states_for_paths(0);
	isSyncFolder = oss.is_sync_folder_root();
	if (oss.state() == ccd::SYNC_STATE_NOT_IN_SYNC_FOLDER) {
		inSyncFolder = false;
	}else {
		inSyncFolder = true;
	}
	
	return 0;

}

void ShlExtUtility::SyncFolderCheck(std::vector<std::string> deviceRootPath, std::string filePath, BOOL& isSyncFolder, BOOL& inSyncFolder) {
	size_t size = deviceRootPath.size();
	LOG_DEBUG("Get %d Sync Folders device path", size);
	bool flag=false;
	for (int i=0; (size_t)i<size; i++){
		std::string currRootPath = deviceRootPath.at(i);

		//compare ignore cases
		std::transform(filePath.begin(), filePath.end(), filePath.begin(), tolower);
		std::transform(currRootPath.begin(), currRootPath.end(), currRootPath.begin(), tolower);
		
		size_t found = filePath.find(currRootPath);
		LOG_DEBUG("SyncFolderCheck: currFilePath %s compared with deviceRootPath %s", filePath.c_str(), currRootPath.c_str());
		if ( found != std::string::npos) { //find
			inSyncFolder = true;
			if (currRootPath.compare(filePath) == 0) {
				isSyncFolder = true;
			}else{
				isSyncFolder = false;
			}
			LOG_DEBUG("FIND, inSyncFolder is %d, isSyncFolder is %d", inSyncFolder, isSyncFolder);
			return;
		} else {
			continue;
		}	
	}
	inSyncFolder = false;
	isSyncFolder = false;
	LOG_DEBUG("NOT FIND, BOTH FALSE");
}


/*
* Retrieve the sync status of folders/files. 
*/
int ShlExtUtility::GetSyncStatus(std::string pathStr, int& status) {
	ccd::GetSyncStateInput request;	
	ccd::GetSyncStateOutput response;
	BOOL isSyncFolder;
	BOOL inSyncFolder;
	request.add_get_sync_states_for_paths(pathStr);

	CCDIError error = CCDIGetSyncState(request, response);
	if (error!=CCD_OK){
		LOG_ERROR("CCDIGetSyncState for getting sync state call fail with error code %d", error);
		return -1;
	}

	ccd::ObjectSyncState oss = response.sync_states_for_paths(0);
	isSyncFolder = oss.is_sync_folder_root();

	if (oss.state() == ccd::SYNC_STATE_NOT_IN_SYNC_FOLDER) {
		inSyncFolder = false;
		status = OTHERS;
	}else {
		inSyncFolder = true;
		//in the sync folder
		if (oss.state() == ccd::SYNC_STATE_UP_TO_DATE) {
			status = SYNCED;
		//}else if ((oss.state() == ccd::SYNC_STATE_DOWNLOADING) 
			//|| (oss.state() == ccd::SYNC_STATE_UPLOADING)){
			//status = SYNCING;
		}else if ((oss.state() == ccd::SYNC_STATE_NEED_TO_DOWNLOAD) 
			|| (oss.state() == ccd::SYNC_STATE_NEED_TO_UPLOAD)) {
			status = TO_BE_SYNCED;
		//}else if (oss.state() == ccd::SYNC_STATE_COMBINATION) {
			/*int syncing = oss.num_downloading()+oss.num_uploading();
			if (syncing > 0){
				status = SYNCING;
			}else {
				status = TO_BE_SYNCED;
			}*/
			//status = SYNCING;
		//} else if (oss.state() == ccd::SYNC_STATE_COMPUTING) {
            // TODO: Bug 703: Currently, all items within a Sync Folder will return COMPUTING.
			//status = COMPUTING;
		} else {
			status = OTHERS;
		}
	}

	return 0;

}

/*
* Retrieve the sync status of folders/files. 
*/
int ShlExtUtility::GetURLPrefix(std::string& url) {
	ccd::GetInfraHttpInfoInput request;
	ccd::GetInfraHttpInfoOutput response;
	request.set_service(ccd::INFRA_HTTP_SERVICE_OPS);
	request.set_secure(true);

	CCDIError error = CCDIGetInfraHttpInfo(request, response);
	if (error!=CCD_OK){
		LOG_ERROR("CCDIGetInfraHttpInfo for getting opsurl prefix call fail with error code %d", error);
		return -1;
	}

	url = response.url_prefix();
	LOG_INFO("Ops URL Prefix: %s", url.c_str());

	return 0;
}


int ShlExtUtility::getDeviceId(u64& deviceId) {
	int rv = 0;
	ccd::GetSystemStateInput getSystemStateIn;
	ccd::GetSystemStateOutput getSystemStateOut;
	
	getSystemStateIn.set_get_device_id(true);
	rv = CCDIGetSystemState(getSystemStateIn, getSystemStateOut);
	if (rv != 0) {
		LOG_ERROR("CCDIGetSystemState failed: %d", "CCDIGetSystemState", rv);
		goto exit;
	}
	deviceId = getSystemStateOut.device_id();

exit:
	return rv;
}

int ShlExtUtility::getUserId(u64& userId) {
	int rv = 0;
	ccd::GetSystemStateInput getSystemStateIn;
	ccd::GetSystemStateOutput getSystemStateOut;

	getSystemStateIn.set_get_players(true);
	rv = CCDIGetSystemState(getSystemStateIn, getSystemStateOut);
	if (rv != 0) {
	LOG_ERROR("%s failed: %d", "CCDIGetSystemState", rv);
	goto exit;
	}
	userId = getSystemStateOut.players().players(0).user_id();
	if (userId == 0) {
		LOG_ERROR("Not signed-in!");
		rv = -1;
		goto exit;
	}

exit:
	return rv;
}

int ShlExtUtility::getArchiveStorageDeviceId(u64 userId, u64& archiveStorageDeviceId) {
	int i;
	int rv = 0;
	s32 numDatasets = 0;	
	ccd::ListOwnedDatasetsInput listOwnedDatasetsIn;
	ccd::ListOwnedDatasetsOutput listOwnedDatasetsOut;

	listOwnedDatasetsIn.set_user_id(userId);
	listOwnedDatasetsIn.set_only_use_cache(false);
	rv = CCDIListOwnedDatasets(listOwnedDatasetsIn, listOwnedDatasetsOut);
	if (rv != CCD_OK) {
		LOG_ERROR("CCDIListOwnedDatasets fail: %d", rv);
		goto exit;
	} else {
	LOG_INFO("CCDIListOwnedDatasets OK");
	}

	numDatasets = listOwnedDatasetsOut.dataset_details_size();

	for (i = 0; i < numDatasets; i++) {
		if (listOwnedDatasetsOut.dataset_details(i).has_datasettype()) {
			vplex::vsDirectory::DatasetDetail datasetDetail = listOwnedDatasetsOut.dataset_details(i);
			if (datasetDetail.datasettype() == vplex::vsDirectory::SYNCBOX) {
				if (datasetDetail.archivestoragedeviceid_size() > 0) {
					archiveStorageDeviceId = datasetDetail.archivestoragedeviceid(0);
				}
			}
		}
	}

exit:
	return rv;
}

// TODO: don't call APIs if last call was within 1-2 mins
BOOL ShlExtUtility::isOnline() {
	int rv = 0;

	ccd::GetSystemStateInput getSystemStateIn;
	ccd::GetSystemStateOutput getSystemStateOut;

	getSystemStateIn.set_get_players(true);
	getSystemStateIn.set_get_device_id(true);
	rv = CCDIGetSystemState(getSystemStateIn, getSystemStateOut);
	if (rv != 0) {
		LOG_ERROR("%s failed: %d", "CCDIGetSystemState", rv);
		return false;
	}

	u64 userId = getSystemStateOut.players().players(0).user_id();
	if (userId == 0) {
		LOG_ERROR("Not signed-in!");
		return false;
	}

	u64 clientDeviceId = getSystemStateOut.device_id();

	u64 archiveStorageDeviceId = 0;
	rv = getArchiveStorageDeviceId(userId, archiveStorageDeviceId);
	if (rv != 0) {
		LOG_ERROR("CCDIListOwnedDatasets failed: %d", rv);
		return false;
	}

	ccd::ListLinkedDevicesInput listLinkedDevicesIn;
	ccd::ListLinkedDevicesOutput listLinkedDevicesOut;
	listLinkedDevicesIn.set_user_id(userId);
	listLinkedDevicesIn.set_only_use_cache(true);
	rv = CCDIListLinkedDevices(listLinkedDevicesIn, listLinkedDevicesOut);
	if (rv != 0) {
		LOG_ERROR("CCDIListLinkedDevices failed: %d", rv);
		return false;
	}

	int devicesSize = listLinkedDevicesOut.devices_size();
	if (devicesSize <= 0) {
		LOG_ERROR("No linked devices");
		return false;
	}

	int numDevicesFound = 0;
	for (int i = 0; i < devicesSize; i++) {
		const ccd::LinkedDeviceInfo& curr = listLinkedDevicesOut.devices(i);
		u64 deviceId = curr.device_id();
		if (deviceId == clientDeviceId || deviceId == archiveStorageDeviceId) {
			numDevicesFound++;
			int state = (int)curr.connection_status().state();
			if (state == ccd::DEVICE_CONNECTION_OFFLINE) {
				return false;
			}
		}
	}

	if (numDevicesFound < 2) {
		LOG_ERROR("Unable to find clientDeviceId or archiveStorageDeviceId from linked devices list");
		return false;
	}

	return true;
}

void ShlExtUtility::sendShellChangeNotify(LPCWSTR pwszPath) {
	DWORD time = 1000;
	Sleep(time);
	LOG_DEBUG("Send change notification because of current status is SYNCING");
	SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, pwszPath, NULL);
}