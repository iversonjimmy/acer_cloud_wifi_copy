// ACloudSyncing.cpp : Implementation of CACloudSyncing
// To determine if the item is being synced (uploading or downloading).

#include "stdafx.h"
#include "ACloudSyncing.h"

// CACloudSyncing
#include <algorithm>
#include <cctype> 
#include <string>
#include <time.h>

// IShellIconOverlayIdentifier::GetOverlayInfo
// returns The Overlay Icon Location to the system
STDMETHODIMP CACloudSyncing::GetOverlayInfo(
             LPWSTR pwszIconFile,
             int cchMax,
             int* pIndex,
             DWORD* pdwFlags)
{
	TCHAR logPath[MAX_PATH];
	char logPathChar[MAX_PATH];
	int result = ShlExtUtility::GetLogDirectory(logPath);

	if (result == 0) {
		result = ShlExtUtility::ConvertFromTString(logPath, logPathChar, MAX_PATH);
		if (result == 0) {
			LOGInit("pc_shellext", logPathChar);
			int loglevel =ShlExtUtility::SetLogLevel();
			LOG_INFO("logDirectoryString: %s, loglevel: %d", logPathChar, loglevel);
		}
	}
	
	lastRecordedTime = NULL;

	// Get our module's full path
	GetModuleFileNameW(_AtlBaseModule.GetModuleInstance(), pwszIconFile, cchMax);

	// Use first icon in the resource
	*pIndex = 2; 
	*pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;

	return S_OK;
}

// IShellIconOverlayIdentifier::GetPriority
// returns the priority of this overlay 0 being the highest. 
STDMETHODIMP CACloudSyncing::GetPriority(int* pPriority)
{
	// we want highest priority 
	*pPriority = 0;
	return S_OK;
}

// IShellIconOverlayIdentifier::IsMemberOf
// Returns whether the object should have this overlay or not 
// This shell extension is used to set "SYNCING"
STDMETHODIMP CACloudSyncing::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	LOG_DEBUG("CACloudSyncing::IsMemberOf");
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
			LOG_INFO("File is syncroot folder: %s", filename.c_str());

			// first check to see if it is online
			if (!ShlExtUtility::isOnline()) {
				LOG_DEBUG("Offline");
				return S_FALSE;
			}

			// online, now check syncing status
			if (getSyncStateOut.feature_sync_state_summary_size() <= 0) {
				LOG_ERROR("No file to query for feature sync state: %d", rv);
				return S_FALSE;
			}

			int status = getSyncStateOut.feature_sync_state_summary(0).status();
			if (status == ccd::CCD_FEATURE_STATE_SYNCING) {
				LOG_DEBUG("Folder is syncing: %s", filename.c_str());
				ShlExtUtility::sendShellChangeNotify(pwszPath);
				return S_OK;
			}
			return S_FALSE;
		}

		ccd::SyncStateType_t syncState = getSyncStateOut.sync_states_for_paths(0).state();

		if (syncState == ccd::SYNC_STATE_NOT_IN_SYNC_FOLDER) {
			LOG_DEBUG("File is not in sync folder: %s", filename.c_str());
			return S_FALSE;
		}

		if (syncState == ccd::SYNC_STATE_FILTERED) {
			LOG_DEBUG("File is being filtered: %s", filename.c_str());
			return S_FALSE;
		}

		if (syncState == ccd::SYNC_STATE_NEED_TO_DOWNLOAD ||
			syncState == ccd::SYNC_STATE_NEED_TO_UPLOAD ||
			syncState == ccd::SYNC_STATE_NEED_TO_UPLOAD_AND_DOWNLOAD ||
			syncState == ccd::SYNC_STATE_UNKNOWN) {

			LOG_INFO("File is syncing: %s", filename.c_str());
			ShlExtUtility::sendShellChangeNotify(pwszPath);
			return S_OK;
		} else {
			LOG_DEBUG("File is synced: %s", filename.c_str());
		}
	}

	return S_FALSE;
}