// ContextMenuShlExt.h : Declaration of the CContextMenuShlExt

#pragma once
#include "resource.h"       // main symbols

#include "shellext_win_i.h"
#include <shlobj.h>
#include <comdef.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <ccdi.hpp>
#include "log.h"
#include "ShlExtUtility.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif



// CContextMenuShlExt

class ATL_NO_VTABLE CContextMenuShlExt :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CContextMenuShlExt, &CLSID_ContextMenuShlExt>,
	public IShellExtInit,
	public IContextMenu
{
public:
	CContextMenuShlExt()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CONTEXTMENUSHLEXT)

DECLARE_NOT_AGGREGATABLE(CContextMenuShlExt)

BEGIN_COM_MAP(CContextMenuShlExt)
	COM_INTERFACE_ENTRY(IShellExtInit)
	COM_INTERFACE_ENTRY(IContextMenu)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

protected:
	TCHAR m_srcFilePath[MAX_PATH];
	std::string m_srcFilePathStr;

	BOOL m_isSyncFolder;
	BOOL m_inSyncFolder;
	std::string m_myCldPathStr;
	
	std::string opsURL;
	int m_type;

	std::vector<std::string> syncFldNmList;
	std::vector<std::string> deviceRootPathList;

	//int MAX_PATH_NONASCII = 3*MAX_PATH;

public:
	// IShellExtInit
	STDMETHODIMP Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY);
	
	// IContextMenu
	STDMETHODIMP GetCommandString(UINT_PTR, UINT, UINT*, LPSTR, UINT);
	STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO);
	STDMETHODIMP QueryContextMenu(HMENU, UINT, UINT, UINT, UINT);
	
};

OBJECT_ENTRY_AUTO(__uuidof(ContextMenuShlExt), CContextMenuShlExt)
