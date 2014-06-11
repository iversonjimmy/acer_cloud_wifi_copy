// ContextMenuShlExt.cpp : Implementation of CContextMenuShlExt

#include "stdafx.h"
#include "ContextMenuShlExt.h"


// CContextMenuShlExt

HRESULT CContextMenuShlExt::Initialize(LPCITEMIDLIST pidlFolder,
  LPDATAOBJECT pDataObj,
  HKEY hProgID )
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

	LOG_DEBUG("Initialize....");
	FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT,
                  -1, TYMED_HGLOBAL };
	STGMEDIUM stg = { TYMED_HGLOBAL };
	HDROP     hDrop;
 
	// Look for CF_HDROP data in the data object. If there is no such data, return an error back to Explorer.
	if ( FAILED( pDataObj->GetData ( &fmt, &stg ) ))
		return E_INVALIDARG;
 
	// Get a pointer to the actual data.
	hDrop = (HDROP) GlobalLock ( stg.hGlobal );
 
	// Make sure it worked.
	if ( NULL == hDrop )
		return E_INVALIDARG;

	UINT uNumFiles = DragQueryFile ( hDrop, 0xFFFFFFFF, NULL, 0 );
	HRESULT hr = S_OK;
  
	if ( 0 == uNumFiles )
    {
		GlobalUnlock ( stg.hGlobal );
		ReleaseStgMedium ( &stg );
		return E_INVALIDARG;
    }
 
	// Get the name of the first file and store it in our member variable m_szFile.
	if ( 0 == DragQueryFile ( hDrop, 0, m_srcFilePath, MAX_PATH ) ) {
		GlobalUnlock ( stg.hGlobal );
		ReleaseStgMedium ( &stg );
		return E_INVALIDARG;
	}
		
	GlobalUnlock ( stg.hGlobal );
	ReleaseStgMedium ( &stg );

	//convert file name to utf-8
	char m_srcFilePathChar[MAX_PATH];
	result = ShlExtUtility::ConvertFromTString(m_srcFilePath, m_srcFilePathChar, MAX_PATH);
	if (result==-1 || &m_srcFilePathChar == NULL){
		return E_FAIL;
	}
	std::string m_srcFilePathStr(m_srcFilePathChar);

	//Decide if item is sync folder or not
	result = ShlExtUtility::SyncFolderCheck(m_srcFilePathStr, m_isSyncFolder, m_inSyncFolder);
	if (result == -1) {
		return E_FAIL ;
	}
	LOG_DEBUG("IsSyncFolder, isSyncFolder: %d", m_isSyncFolder);

	return hr;
}

HRESULT CContextMenuShlExt::QueryContextMenu (
	HMENU hmenu, UINT uMenuIndex, UINT uidFirstCmd,
	UINT uidLastCmd, UINT uFlags )
{
/*
	// If the flags include CMF_DEFAULTONLY then we shouldn't do anything.
	if ( uFlags & CMF_DEFAULTONLY )
		return MAKE_HRESULT ( SEVERITY_SUCCESS, FACILITY_NULL, 0 );

	// First, create and populate a submenu.

    HMENU hSubmenu = CreatePopupMenu();
    UINT uID = uidFirstCmd;
	//if under "My Cloud" directory
	if (m_inSyncFolder) {
		InsertMenu ( hSubmenu, 0, MF_BYPOSITION, uID++, _T("View on web...") );
		InsertMenu ( hSubmenu, 1, MF_BYPOSITION, uID++, _T("Show path...") );
	}else {

		HMENU hSubmenuForCopy = CreatePopupMenu();
		HMENU hSubmenuForMove = CreatePopupMenu();
		
		int result = ShlExtUtility::ListSyncFolderName(syncFldNmList, deviceRootPathList);
		if (result == -1){
			LOG_ERROR("Can't get ListSyncFolderName");
			return E_FAIL;
		}
		
		size_t size = syncFldNmList.size();
		LOG_DEBUG("Get %d Sync Folders", size);
		for (int i=0; (size_t)i<size; i++){
			TCHAR syncFldName[MAX_PATH];
			result = ShlExtUtility::ConvertToTString(syncFldNmList.at(i).c_str(), syncFldName); 
			if (result == -1) {
				LOG_ERROR("ConvertToTString FAILS in QueryContextMenu");
				return E_FAIL;
			}
			InsertMenu ( hSubmenuForCopy, i, MF_BYPOSITION, uID++, syncFldName );
			InsertMenu ( hSubmenuForMove, i, MF_BYPOSITION, uID++, syncFldName );
		}

		//insert above submen into "Copy to My Cloud" submenu
		MENUITEMINFO miiForCopy = { sizeof(MENUITEMINFO) };

		miiForCopy.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
		miiForCopy.wID = uID++;
		miiForCopy.hSubMenu = hSubmenuForCopy;
		miiForCopy.dwTypeData = _T("Copy to My Cloud");

		InsertMenuItem ( hSubmenu, 0, TRUE, &miiForCopy );

		//insert above submen into "Move to My Cloud" submenu
		MENUITEMINFO miiForMove = { sizeof(MENUITEMINFO) };

		miiForMove.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
		miiForMove.wID = uID++;
		miiForMove.hSubMenu = hSubmenuForMove;
		miiForMove.dwTypeData = _T("Move to My Cloud");

		InsertMenuItem ( hSubmenu, 1, TRUE, &miiForMove );
	}

    // Insert the submenu into the ctx menu provided by Explorer.

    MENUITEMINFO mii = { sizeof(MENUITEMINFO) };

    mii.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
    mii.wID = uID++;
    mii.hSubMenu = hSubmenu;
    mii.dwTypeData = _T("My Cloud");

    InsertMenuItem ( hmenu, uMenuIndex, TRUE, &mii );

	return MAKE_HRESULT ( SEVERITY_SUCCESS, FACILITY_NULL,  uID - uidFirstCmd );
*/
	return MAKE_HRESULT ( SEVERITY_SUCCESS, FACILITY_NULL,  uidFirstCmd );
}

HRESULT CContextMenuShlExt::GetCommandString (
  UINT_PTR idCmd, UINT uFlags, UINT* pwReserved,
  LPSTR pszName, UINT cchMax )
{
	USES_CONVERSION;
 
	// Check idCmd, it must be 0 since we have only one menu item.
	if ( 0 != idCmd )
		return E_INVALIDARG;
 
	// If Explorer is asking for a help string, copy our string into the supplied buffer.
	if ( uFlags & GCS_HELPTEXT )
    {
		LPCTSTR szText = _T("Personal Cloud");
 
		if ( uFlags & GCS_UNICODE )
		{
			// We need to cast pszName to a Unicode string, and then use the Unicode string copy API.
			lstrcpynW ( (LPWSTR) pszName, T2CW(szText), cchMax );
		}
		else
		{
			// Use the ANSI string copy API to return the help string.
			lstrcpynA ( pszName, T2CA(szText), cchMax );
		}
 
		return S_OK;
    }
 
	return E_INVALIDARG;
}

HRESULT CContextMenuShlExt::InvokeCommand (
  LPCMINVOKECOMMANDINFO pCmdInfo )
{
	int result;
	// If lpVerb really points to a string, ignore this function call and bail out.
	if ( 0 != HIWORD( pCmdInfo->lpVerb ) )
		return E_INVALIDARG;
 
	// Get the command index - the only valid one is 0.
	if (!m_inSyncFolder) {
		int m_index = LOWORD(pCmdInfo->lpVerb);
		
		if (m_index %2 == 0) {//copy	

			int index =m_index/2;
			LOG_DEBUG("Copy Destination: %s", deviceRootPathList.at(index).c_str());
			
			//convert deviceRootPath to utf-16
			TCHAR m_copyDestPath[MAX_PATH];
			result = ShlExtUtility::ConvertToTString(deviceRootPathList.at(index).c_str(), m_copyDestPath); 
			if (result == -1) {
				LOG_ERROR("ConvertToTString FAILS in InvokeCommand for copy");
				return E_FAIL;;
			}

			//Purpose of following lines are for double null termiator which is required by SHFileOperation
			wcscat_s(m_srcFilePath, _T("|"));
			while (TCHAR* ptr = _tcsrchr(m_srcFilePath, _T('|'))) {
				*ptr = _T('\0'); 
			}
			wcscat_s(m_copyDestPath, _T("|"));
			while (TCHAR* ptr = _tcsrchr(m_copyDestPath, _T('|'))) {
				*ptr = _T('\0'); 
			}
			ShlExtUtility::CopyItem(m_srcFilePath, m_copyDestPath);

			return S_OK;
		
		}else {//move

			int index =m_index/2;
			LOG_DEBUG("Move Destination: %s", deviceRootPathList.at(index).c_str());
			
			//convert deviceRootPath to utf-16
			TCHAR m_moveDestPath[MAX_PATH];
			result = ShlExtUtility::ConvertToTString(deviceRootPathList.at(index).c_str(), m_moveDestPath); 
			if (result == -1) {
				LOG_ERROR("ConvertToTString FAILS in InvokeCommand for move");
				return E_FAIL;
			}

			//Purpose of following lines are for double null termiator which is required by SHFileOperation
			wcscat_s(m_srcFilePath, _T("|"));
			while (TCHAR* ptr = _tcsrchr(m_srcFilePath, _T('|'))) {
				*ptr = _T('\0'); 
			}
			wcscat_s(m_moveDestPath, _T("|"));
			while (TCHAR* ptr = _tcsrchr(m_moveDestPath, _T('|'))) {
				*ptr = _T('\0'); 
			}
			ShlExtUtility::MoveItem(m_srcFilePath, m_moveDestPath);
			
			return S_OK;
		}
	}else {//under sync folder
		switch ( LOWORD( pCmdInfo->lpVerb ) )
		{
			case 0:
			{
			
				result = ShlExtUtility::GetURLPrefix(opsURL);
				if (result == -1) {
					return E_FAIL;
				}
				LOG_INFO("Ops URL Prefix: %s", opsURL.c_str());

				//Get the my cloud root path
				result = ShlExtUtility::GetMyCloudRootPath(m_myCldPathStr);
				if (result == -1) {
					return E_FAIL;
				}
				LOG_DEBUG("GetMyCloudRootPath, myCldPath: %s", m_myCldPathStr.c_str());

				//get Dataset name
				std::string dirStr;
				result = ShlExtUtility::GetDirectoryStr(m_srcFilePath, m_myCldPathStr, dirStr);
				LOG_DEBUG("Directory String: %s", dirStr.c_str());
				opsURL.append("/ops/mycloud#");
				opsURL.append(dirStr);
				LOG_DEBUG("URL: %s", opsURL.c_str());
				
				TCHAR url[MAX_PATH];
				result = ShlExtUtility::ConvertToTString(opsURL.c_str(), url); 
				if (result == -1) {
					LOG_ERROR("ConvertToTString FAILS in InvokeCommand for opsURL");
					return E_FAIL;
				}
				ShellExecute(NULL, _T("open"), url, NULL, NULL, SW_SHOWNORMAL);
				return S_OK;
			}
			case 1:
			{
				TCHAR szMsg[MAX_PATH + 32];
				wsprintf ( szMsg, _T("The selected file was:\n\n%s"), m_srcFilePath );
				MessageBox ( pCmdInfo->hwnd, szMsg, _T("test"), MB_ICONINFORMATION );
	 
				return S_OK;
			}
			break;
	 
			default:
			return E_INVALIDARG;
			break;
		}

	}

}