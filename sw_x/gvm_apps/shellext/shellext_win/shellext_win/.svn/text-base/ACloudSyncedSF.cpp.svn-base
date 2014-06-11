// ACloudSyncedSF.cpp : Implementation of CACloudSyncedSF
// To determine if the item is the root of a sync folder.

#include "stdafx.h"
#include "ACloudSyncedSF.h"
#include "dllmain.h"

// CACloudSyncedSF

#include <string>
#include <time.h>


// IShellIconOverlayIdentifier::GetOverlayInfo
// returns The Overlay Icon Location to the system
STDMETHODIMP CACloudSyncedSF::GetOverlayInfo(
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
			LOG_INFO(">>>>> CACloudSyncedSF::GetOverlayInfo");
			LOG_INFO("logDirectoryString: %s, loglevel: %d", logPathChar, loglevel);
		}
	}

	//SET initial value
	lastRecordedTime = NULL;

	// Get our module's full path
	GetModuleFileNameW(_AtlBaseModule.GetModuleInstance(), pwszIconFile, cchMax);
	// Use first icon in the resource
	*pIndex=3; 
	*pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
	return S_OK;
}

// IShellIconOverlayIdentifier::GetPriority
// returns the priority of this overlay 0 being the highest. 
STDMETHODIMP CACloudSyncedSF::GetPriority(int* pPriority)
{
	// we want highest priority 
	*pPriority=0;
	return S_OK;
}

// IShellIconOverlayIdentifier::IsMemberOf
// Returns whether the object should have this overlay or not 
// This shell extension is used to set "SyncFolder" + "SYNCED"
STDMETHODIMP CACloudSyncedSF::IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib)
{
	// This overlay shell extension is NOT being used at this moment.
	return S_FALSE;
}