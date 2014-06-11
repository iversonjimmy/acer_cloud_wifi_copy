#include "CommonTool.h"
#include <shobjidl.h>	// Define IShellLibrary and other helper functions
#include <shlobj.h>
#include <knownfolders.h>
#include <propkey.h>
#pragma once


class CWinLib
{
public:
	CWinLib(void);
	~CWinLib(void);


TContentMap* m_litem;
void GetAllList(TContentMap* inlitem);

private	:
	BOOL GetWinLibPathByType(TContentMap* inlitem, MEDIA_TYPE inMediatype);
	BOOL GetFolderPath(IShellLibrary *pslLibrary, IShellItem *psiFolder, TCHAR szPath[]);
	BOOL GetFolderDisplay(IShellItem *psiFolder, TCHAR szDisplay[]);
	BOOL ScanPath(/*MediaType mediatype, */TCHAR szPath[]);
	void GetItemInfo(CString szPath);
};

