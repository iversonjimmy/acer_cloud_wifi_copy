// ACloudToBeSynced.cpp : Implementation of CACloudToBeSynced
// To determine if the syncbox root folder is out of sync or
// either the client device or archive storage device is offline

#include "stdafx.h"
#include "ACloudToBeSynced.h"


// CACloudToBeSynced

#include <string>
#include <time.h>

// IShellIconOverlayIdentifier::GetOverlayInfo
// returns The Overlay Icon Location to the system
STDMETHODIMP CACloudToBeSynced::GetOverlayInfo(
             LPWSTR pwszIconFile,
             int cchMax,
             int* pIndex,
             DWORD* pdwFlags)
{
	TCHAR logPath[MAX_PATH];
	char logPathChar[MAX_PATH];
	int result = ShlExtUtility::GetLogDirectory(logPath);
	
	if (result == 0 ) {
		result = ShlExtUtility::ConvertFromTString(logPath, logPathChar, MAX_PATH);
		if (result == 0) {
			LOGInit("pc_shellext", logPathChar);
			int loglevel =ShlExtUtility::SetLogLevel();

			LOG_INFO("logDirectoryString: %s, loglevel: %d", logPathChar, loglevel);
		}
	}

	//SET initial value
	lastRecordedTime = NULL;

	// Get our module's full path
	GetModuleFileNameW(_AtlBaseModule.GetModuleInstance(), pwszIconFile, cchMax);
	// Use third icon in the resource
	*pIndex=1; 
	*pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
	return S_OK;
}

// IShellIconOverlayIdentifier::GetPriority
// returns the priority of this overlay 0 being the highest. 
// This shell extension is used to set "TO_BE_SYNCED"
STDMETHODIMP CACloudToBeSynced::GetPriority(int* pPriority)
{
	// we want highest priority 
	*pPriority=0;
	return S_OK;
}
// IShellIconOverlayIdentifier::IsMemberOf
// Returns whether the object should have this overlay or not 
// This shell extension is used to set "TO_BE_SYNCED"
STDMETHODIMP CACloudToBeSynced::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	LOG_DEBUG("CACloudToBeSynced::IsMemberOf");
	ccd::GetSyncStateInput getSyncStateIn;
	ccd::GetSyncStateOutput getSyncStateOut;
	int rv = 0;

	char filenameChar[MAX_PATH];
	rv = ShlExtUtility::ConvertFromTString(pwszPath, filenameChar, MAX_PATH);
	if (rv == -1 || &filenameChar == NULL){
		LOG_ERROR("Fail to perform ConvertFromTString");
		return S_FALSE;
	}
	std::string filename(filenameChar);
	
	getSyncStateIn.add_get_sync_states_for_paths(filename);
	getSyncStateIn.add_get_sync_states_for_features(ccd::SYNC_FEATURE_SYNCBOX);

	rv = CCDIGetSyncState(getSyncStateIn, getSyncStateOut);

	if (rv != 0) {
		LOG_ERROR("CCDIGetSyncState for syncbox fail rv %d", rv);
		return S_FALSE;
	} else {
		if (getSyncStateOut.sync_states_for_paths_size() <= 0) {
			LOG_ERROR("No file to query for sync state: %d", rv);
			return S_FALSE;
		}

		if (getSyncStateOut.sync_states_for_paths(0).is_sync_folder_root()) {
			LOG_DEBUG("File is syncroot folder: %s", filename.c_str());

			// first check to see if it is online
			if (!ShlExtUtility::isOnline()) {
				LOG_DEBUG("Offline");
				ShlExtUtility::sendShellChangeNotify(pwszPath);
				return S_OK;
			}

			// online, now check out-of-sync status
			if (getSyncStateOut.feature_sync_state_summary_size() <= 0) {
				LOG_ERROR("No file to query for feature sync state: %d", rv);
				ShlExtUtility::sendShellChangeNotify(pwszPath);
				return S_OK;
			}

			if (getSyncStateOut.feature_sync_state_summary(0).status() == ccd::CCD_FEATURE_STATE_OUT_OF_SYNC) {
				LOG_DEBUG("Out-of-sync");
				ShlExtUtility::sendShellChangeNotify(pwszPath);
				return S_OK;
			}
		}
	}

	return S_FALSE;
}
