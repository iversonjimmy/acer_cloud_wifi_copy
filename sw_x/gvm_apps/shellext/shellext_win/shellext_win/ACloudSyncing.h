// ACloudSyncing.h : Declaration of the CACloudSyncing

#pragma once
#include "resource.h"       // main symbols

#include "shellext_win_i.h"
#include <shlobj.h>
#include <comdef.h>
#include "ShlExtUtility.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

#define OTHERS 0
#define SYNCED 1
#define SYNCING 2
#define TO_BE_SYNCED 3
#define COMPUTING 4


// CACloudSyncing

class ATL_NO_VTABLE CACloudSyncing :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CACloudSyncing, &CLSID_ACloudSyncing>,
	public IShellIconOverlayIdentifier
{
public:
	CACloudSyncing()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ACLOUDSYNCING)

DECLARE_NOT_AGGREGATABLE(CACloudSyncing)

BEGIN_COM_MAP(CACloudSyncing)
	COM_INTERFACE_ENTRY(IShellIconOverlayIdentifier)  
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
	std::string m_myCldRootPathStr;
	//time stamp for calling getSyncFolder api
	time_t lastRecordedTime;

	//List of sync folders names
	std::vector<std::string> syncFolderNameList;

	//List of sync folders have a corresponding list of device root path. 
	//This device root path list is used to check if current file path is under sync folder. 
	std::vector<std::string> deviceRootPathList;

public:
	// IShellIconOverlayIdentifier Methods
	STDMETHOD(GetOverlayInfo)(LPWSTR pwszIconFile,int cchMax,int *pIndex,DWORD* pdwFlags);
	STDMETHOD(GetPriority)(int* pPriority);
	STDMETHOD(IsMemberOf)(LPCWSTR pwszPath,DWORD dwAttrib);


};

OBJECT_ENTRY_AUTO(__uuidof(ACloudSyncing), CACloudSyncing)
