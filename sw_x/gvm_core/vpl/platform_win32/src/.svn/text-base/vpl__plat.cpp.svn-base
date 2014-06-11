#include "vpl_plat.h"
#include "vplu.h"
#include "scopeguard.hpp"

#include <string>

#include <winsock2.h>

#include <sddl.h> // For SID conversion
#include <iphlpapi.h> // Hack, for fetching MAC address as hardware uuid

#include <Objbase.h>

#ifdef _MSC_VER

// WMI
#include <comutil.h>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "wbemuuid.lib")

static BOOL GetBIOSSerialNumber(char** serialNumber);

#endif

static int gInitialized = 0;

static BOOL GetCurrentUserSID(PSID *ppsid);
static void FreeLogonSID(PSID ppsid);
static BOOL CheckLocalAdapter(char* pAdapterName);

ComInitGuard::~ComInitGuard() 
{
    if (needToUninit) {
        CoUninitialize();
    }
}

HRESULT ComInitGuard::init(DWORD dwCoInit) 
{
    HRESULT hr = CoInitializeEx(NULL, dwCoInit);
    needToUninit = SUCCEEDED(hr);
    return hr;
}

int VPL_Init(void)
{
    WSADATA wsaData;
    int err;

    if ( gInitialized != 0 ) {
        return VPL_ERR_IS_INIT;
    }

    // Some VPLNet functions use Winsock calls that
    // must come after Winsock has been initialized.
    // 2.2 is the latest version.
    err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        VPL_REPORT_FATAL("WSAStartup returned %d", err);
        return VPL_ERR_FAIL;
    }

    if (LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2) {
        VPL_REPORT_FATAL("Wrong version of winsock: expected 2.2, got "FMTu8"."FMTu8,
                HIBYTE(wsaData.wVersion), LOBYTE(wsaData.wVersion));
        WSACleanup();
        return VPL_ERR_FAIL;
    }

    gInitialized = 1;
    return VPL_OK;
}

VPL_BOOL VPL_IsInit()
{
    if (gInitialized) {
        return VPL_TRUE;
    } else {
        return VPL_FALSE;
    }
}

int VPL_Quit(void)
{
    if ( gInitialized == 0 ) {
        return VPL_ERR_NOT_INIT;
    }
    WSACleanup();
    gInitialized = 0;
    return VPL_OK;
}

int VPL_GetOSUserName(char** osUserName_out)
{
    int iRet = VPL_ERR_FAIL;
    PSID ppsid = NULL;

    SID_NAME_USE snuSIDNameUse;
    DWORD        dwUserNameLength=MAX_PATH;
    char         szDomain[MAX_PATH];
    DWORD        dwDomainNameLength = ARRAY_ELEMENT_COUNT(szDomain);

    if (osUserName_out == NULL) {
        return VPL_ERR_INVALID;
    }
    *osUserName_out = NULL;
    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }
 
    // Call the GetCurrentUserSID()
    if(!GetCurrentUserSID(&ppsid)) {
        VPL_REPORT_WARN("Failed to get current user's SID: %lu", GetLastError());
        goto end;
    }
    
    // Retrieve user name and domain name based on user's SID.
    *osUserName_out = (char*)malloc(dwUserNameLength);
    if (*osUserName_out == NULL) {
        iRet = VPL_ERR_NOMEM;
        goto end;
    }
    if (LookupAccountSidA(NULL,
        ppsid,
        *osUserName_out,
        &dwUserNameLength,
        szDomain,
        &dwDomainNameLength,
        &snuSIDNameUse)) {
        iRet = VPL_OK;
    }
    else {
        VPL_REPORT_WARN("Failed to look up account name by current user's SID: %lu", GetLastError());
        free(*osUserName_out);
        *osUserName_out = NULL;
    }

end:
    // Release the allocation for ppsid
    FreeLogonSID(ppsid);
    return iRet;
}

void VPL_ReleaseOSUserName(char* osUserName)
{
    if (osUserName != NULL) {
        free(osUserName);
    }
}

int VPL_GetOSUserId(char** osUserId_out)
{
    int iRet=VPL_ERR_FAIL;
    PSID ppsid = NULL;

    if (osUserId_out == NULL) {
        return VPL_ERR_INVALID;
    }
    *osUserId_out = NULL;
    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }
 
    // Call the GetCurrentUserSID()
    if(!GetCurrentUserSID(&ppsid)) {
        VPL_REPORT_WARN("Failed to get current user's SID: %lu",  GetLastError());
        goto end;
    }
    // Convert the logon sid to SID string format
    // Must call LocalFree on the buffer finished with it.
    if(ConvertSidToStringSidA(ppsid, osUserId_out)) {
        iRet = VPL_OK;
    } else {
        VPL_REPORT_WARN("Failed to convert current user's SID to string: %lu", GetLastError());
    }

end:
    // Release the allocation for ppsid
    FreeLogonSID(ppsid);
    return iRet;
}

void VPL_ReleaseOSUserId(char* osUserId)
{
    if (osUserId != NULL) {
        LocalFree(osUserId);
    }
}

int VPL_GetHwUuid(char** hwUuid_out)
{
    int iRet=VPL_ERR_FAIL;
    // Hack!!
    // Fetch the first MAC address of the first Ethernet network card
    // as hardware uuid
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    DWORD dwRetVal;
    UINT i;
    // Initial guess; this will be increased if there are multiple adapters.
    ULONG ulOutBufLen=sizeof(IP_ADAPTER_INFO);

    if (hwUuid_out == NULL) {
        return VPL_ERR_INVALID;
    }
    *hwUuid_out = NULL;
    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }
#ifdef _MSC_VER
    if (GetBIOSSerialNumber(hwUuid_out)) {
        iRet = VPL_OK;
        goto end;
    }
#endif

    pAdapterInfo = (IP_ADAPTER_INFO*)HeapAlloc(GetProcessHeap(), 0, ulOutBufLen);
    if (pAdapterInfo == NULL) {
        iRet = VPL_ERR_NOMEM;
        goto end;
    }
    // Make an initial call to GetAdaptersInfo to get
    // the necessary size into the ulOutBufLen variable
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        HeapFree(GetProcessHeap(), 0, pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)HeapAlloc(GetProcessHeap(), 0, ulOutBufLen);
        if (pAdapterInfo == NULL) {
            iRet = VPL_ERR_NOMEM;
            goto end;
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        // Search for the first Ethernet adapter.
        while (pAdapter != NULL) {
            if (pAdapter->Type == MIB_IF_TYPE_ETHERNET) {
                /// check if it is physical NIC
                if (CheckLocalAdapter(pAdapter->AdapterName))
                    break;
            }
            pAdapter = pAdapter->Next; 
        }
        if (pAdapter == NULL) {
            // No Ethernet adapter found; use the first PCI interface NIC in the list.
            pAdapter = pAdapterInfo;
            while (pAdapter != NULL) {
                /// check if it is physical NIC
                if (CheckLocalAdapter(pAdapter->AdapterName))
                    break;
                pAdapter = pAdapter->Next; 
            }
        }
        if (pAdapter == NULL) {
            // Cannot find any PCI interface NIC, use the first NIC.
            // Todo: query other type of unique hardware id
            pAdapter = pAdapterInfo;
        }
        if (pAdapter == NULL) {
            // There weren't any items in the list!
            *hwUuid_out = new char[1]; // Use new char[] to be consistent with _com_util::ConvertBSTRToString.
            *hwUuid_out[0] = '\0';
        } else {
            *hwUuid_out = new char[MAX_PATH]; // Use new char[] to be consistent with _com_util::ConvertBSTRToString.
            for (i = 0; i < pAdapter->AddressLength; i++) {
                if (i==0)
                    snprintf(*hwUuid_out, MAX_PATH, "%.2X", (int) pAdapter->Address[i]);
                else
                    snprintf(*hwUuid_out, MAX_PATH, "%s-%.2X", *hwUuid_out, (int) pAdapter->Address[i]);
            }
        }
        iRet = VPL_OK;
    }

end:
    if (pAdapterInfo != NULL) {
        HeapFree(GetProcessHeap(), 0, pAdapterInfo);
    }
    return iRet;
}

void VPL_ReleaseHwUuid(char* hwUuid)
{
    if (hwUuid != NULL) {
        // hwUuid should have been allocated via "new char[]" or _com_util::ConvertBSTRToString().
        delete[] hwUuid;
    }
}

/// Get the logon SID and convert it to SID string.
/// This allocates a buffer for \a ppsid; you must call #FreeLogonSID() on the result when
/// finished with it. 
static BOOL GetCurrentUserSID(PSID *ppsid)
{
    BOOL bSuccess = FALSE;
    DWORD dwLength = 0;
    HANDLE hToken = NULL;
    PTOKEN_USER ptUser = NULL;
     
    // Verify the parameter passed in is not NULL.
    // Although we just provide an empty buffer...
    if(ppsid == NULL) {
        goto end;
    }
    
    // Open a handle to the access token for the calling process.
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        goto end;
    }
    
    // Get the required buffer size and allocate the TOKEN_GROUPS buffer.
    if(!GetTokenInformation(hToken,         // handle to the access token
                            TokenUser,      // get info about the user
                            NULL,           // pointer to TOKEN_GROUPS buffer
                            0,              // size of buffer
                            &dwLength       // receives required buffer size
        )) {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto end;
        }
        ptUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
        if(ptUser == NULL) {
            goto end;
        }
    }
     
    // Get the token group information from the access token.
    if(!GetTokenInformation(hToken,
                            TokenUser,
                            (LPVOID) ptUser,
                            dwLength,
                            &dwLength)) {
        goto end;
    }
     
    // If the SID is found then make a copy of it.
    dwLength = GetLengthSid(ptUser->User.Sid);
    // Allocate a storage
    *ppsid = (PSID) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
    if(*ppsid == NULL) {
        goto end;
    }
    // If Copying the SID fails...
    if(!CopySid(dwLength, *ppsid, ptUser->User.Sid)) {
        HeapFree(GetProcessHeap(), 0, *ppsid);
        *ppsid = NULL;
        goto end;
    }

    // If everything OK, returns a clean slate...
    bSuccess = TRUE;
    
end:
    if (ptUser != NULL) {
        HeapFree(GetProcessHeap(), 0, (LPVOID)ptUser);
    }
    if (hToken != NULL) {
        CloseHandle(hToken);
    }
    return bSuccess;
}

// The following function release the buffer allocated by the GetCurrentUserSID() function.
static void FreeLogonSID(PSID ppsid)
{
    if (ppsid != NULL) {
        HeapFree(GetProcessHeap(), 0, ppsid);
    }
}

// Check if the network adapter is the physical one or not
BOOL CheckLocalAdapter(char* pAdapterName)
{
    BOOL rv=FALSE;
    char szDataBuf[MAX_PATH+1]={0};
    DWORD dwDataLen = MAX_PATH;
    DWORD dwType = REG_SZ;
    HKEY hKey = NULL;
    HKEY hConnKey = NULL;

    static char NIC_KEY[]="System\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
 
    if(ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE, NIC_KEY, 0, KEY_READ, &hKey))
        return FALSE;

    _snprintf(szDataBuf, sizeof(szDataBuf), "%s\\Connection", pAdapterName);
    if(ERROR_SUCCESS != RegOpenKeyExA(hKey ,szDataBuf ,0 ,KEY_READ, &hConnKey)) {
        RegCloseKey(hKey);
        return FALSE;
    }

    dwDataLen = MAX_PATH;
    if (ERROR_SUCCESS != RegQueryValueExA(hConnKey, "PnpInstanceID", 0, &dwType, (BYTE *)szDataBuf, &dwDataLen)) {
        goto out;
    }
    if (strncmp(szDataBuf, "PCI", strlen("PCI")) != 0)
        goto out;
    rv = TRUE;

 out:
    RegCloseKey(hConnKey);
    RegCloseKey(hKey);
    return rv;
}

#ifdef _MSC_VER
BOOL GetBIOSSerialNumber(char** serialNumber)
{
    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
    if (FAILED(hres)) {
        return FALSE;                  // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------
    // Note: If you are using Windows 2000, you need to specify -
    // the default authentication credentials for a user by using
    // a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
    // parameter of CoInitializeSecurity ------------------------

    hres = CoInitializeSecurity(
        NULL, 
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
        );

                      
    if (FAILED(hres)) {
        CoUninitialize();
        return FALSE;                    // Program has failed.
    }
    
    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator, (LPVOID *) &pLoc);
 
    if (FAILED(hres)) {
        CoUninitialize();
        return FALSE;                 // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices *pSvc = NULL;
	
    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (e.g. Kerberos)
         0,                       // Context object 
         &pSvc                    // pointer to IWbemServices proxy
         );
    
    if (FAILED(hres)) {
        pLoc->Release();     
        CoUninitialize();
        return FALSE;                // Program has failed.
    }


    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
       pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name 
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return FALSE;               // Program has failed.
    }

    // Step 6: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    // For example, get the name of the operating system
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"), 
        bstr_t("SELECT * FROM Win32_BIOS"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator);
    
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;               // Program has failed.
    }

    // Step 7: -------------------------------------------------
    // Get the data from the query in step 6 -------------------
 
    IWbemClassObject *pclsObj;
    ULONG uReturn = 0;
   
    while (pEnumerator) {
        VARIANT vtProp;
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
            &pclsObj, &uReturn);

        if (0 == uReturn) {
            break;
        }

        // Get the value of the Name property
        hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        *serialNumber = _com_util::ConvertBSTRToString(vtProp.bstrVal);
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    // Cleanup
    // ========
    
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

    return TRUE;   // Program successfully completed.
}

#endif


class UserSid {
public:
    UserSid() : sid(NULL) {}
    ~UserSid() { freeSidIfNecessary(); }
    int set(const PSID _sid) {
        freeSidIfNecessary();
        sid = _sid;
        return VPL_OK;
    }
    int set(const char *sidStr) {
        PSID lsid;
        BOOL isOk = ConvertStringSidToSidA((char*)sidStr, &lsid);
        return isOk ? set(lsid) : VPLError_XlatWinErrno(GetLastError());
    }
    PSID get() { return sid; }
private:
    PSID sid;
    void freeSidIfNecessary() {
        if (sid != NULL) {
            LocalFree(sid);
            sid = NULL;
        }
    }
};

static UserSid userSid;

int _VPL__SetUserSid(const char *sidStr)
{
    return userSid.set(sidStr);
}

PSID _VPL__GetUserSid()
{
    return userSid.get();
}

int _VPL__GetUserSidStr(char** sidStr)
{
    int rv = VPL_ERR_FAIL;

    if (sidStr == NULL) {
        return VPL_ERR_INVALID;
    }
    *sidStr = NULL;
    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }

    if (userSid.get() == NULL)
        return ::VPL_ERR_NOENT;

    if (::ConvertSidToStringSidA(userSid.get(), sidStr))
        rv = VPL_OK;

    return rv;
}

int VPL_GetDeviceInfo(char** manufacturer, char **model)
{
    //int rv = VPL_OK;
    if (manufacturer == NULL) {
        return VPL_ERR_INVALID;
    }
    if (model == NULL) {
        return VPL_ERR_INVALID;
    }

    *manufacturer = NULL;
    *model = NULL;

    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }


    HRESULT hr;
    ComInitGuard comInit;
    hr = comInit.init(COINIT_MULTITHREADED);

    if (FAILED(hr)) {
        VPL_REPORT_FATAL("ComInit failed: "FMT_HRESULT, hr);
        return VPL_ERR_FAIL;                  // Program has failed.
    }


    IWbemLocator* pWbemLocator = NULL;
    IWbemServices* pCIMV2Service = NULL;

    hr = CoInitializeSecurity(NULL,
            -1,
            NULL,
            NULL,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE,
            NULL);

    if (FAILED(hr) && !(hr == RPC_E_TOO_LATE)) {
        VPL_REPORT_FATAL("CoInitializeSecurity failed, %d", hr);
        return VPL_ERR_FAIL;                  // Program has failed.
    }


    hr = CoCreateInstance(CLSID_WbemLocator,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IWbemLocator,
            (void**)&pWbemLocator);

    if (FAILED(hr)) {
        VPL_REPORT_FATAL("CoCreateInstance failed, %d", hr);
        return VPL_ERR_FAIL;                  // Program has failed.
    }
    ON_BLOCK_EXIT_OBJ(*pWbemLocator, &IWbemLocator::Release);


    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject*     pClassObject = NULL;
    unsigned long         uEnumCount = 0;
    VARIANT vtRead;

    //logTime(L"Is Acer Platform - start",true);

    hr = pWbemLocator->ConnectServer(L"root\\CIMV2",
            NULL,
            NULL,
            0,
            NULL,
            0,
            0,
            &pCIMV2Service);

    if(FAILED(hr)){
        VPL_REPORT_FATAL("WMI constructor, ConnectServer() for CIMV2 Service failed, %d", hr);
        return VPL_ERR_FAIL;                  // Program has failed.
    }
    ON_BLOCK_EXIT_OBJ(*pCIMV2Service, &IWbemServices::Release);
    //logTime(L"Connect Server",true);

    hr = CoSetProxyBlanket(pCIMV2Service,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE);


    if(FAILED(hr)){
        VPL_REPORT_FATAL("WMI constructor, CoSetProxyBlanket() failed for CIMV2, %d", hr);
        return VPL_ERR_FAIL;                  // Program has failed.
    }

    hr = pCIMV2Service->ExecQuery(L"WQL",
            L"SELECT * FROM Win32_ComputerSystem",
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);

    if(FAILED(hr)){
        VPL_REPORT_FATAL("WQL failed ,%d", hr);
        return VPL_ERR_FAIL;                  // Program has failed.
    }
    ON_BLOCK_EXIT_OBJ(*pEnumerator, &IEnumWbemClassObject::Release);


    hr = (pEnumerator->Next(WBEM_INFINITE,
                1,
                &pClassObject,
                &uEnumCount));
    if(FAILED(hr)){
        VPL_REPORT_FATAL("Enumerator failed ,%d", hr);
        return VPL_ERR_FAIL;                  // Program has failed.
    }
    ON_BLOCK_EXIT_OBJ(*pClassObject, &IWbemClassObject::Release);

    if(uEnumCount!=0)
    {
        int rv = VPL_OK;
    
        hr = (pClassObject->Get(L"Model",
                    0,
                    &vtRead,
                    0,
                    0));

        //logTime(L"object Get",true);

        if(FAILED(hr)){
            VPL_REPORT_FATAL("Get Model failed ,%d", hr);
            goto cleanup;
        }

        if(vtRead.bstrVal == NULL){
            VPL_REPORT_FATAL("Model string is empty");
            goto cleanup;
        }
        rv = _VPL__wstring_to_utf8_alloc(vtRead.bstrVal, model);
        VariantClear(&vtRead);
        if(rv != VPL_OK){
            VPL_REPORT_FATAL("Convert to utf8 failed,%d", rv);
            goto cleanup;
        }

        //Get Manufacturer
        hr = (pClassObject->Get(L"Manufacturer",
                    0,
                    &vtRead,
                    0,
                    0));

        if(FAILED(hr)){
            VPL_REPORT_FATAL("Get Manufacturer failed ,%d", hr);
            goto cleanup;
        }

        if(vtRead.bstrVal == NULL){
            VPL_REPORT_FATAL("Manufacturer string is empty");
            goto cleanup;
        }
        rv = _VPL__wstring_to_utf8_alloc(vtRead.bstrVal, manufacturer);
        VariantClear(&vtRead);
        if(rv != VPL_OK){
            VPL_REPORT_FATAL("Convert to utf8 failed,%d", rv);
            goto cleanup;
        }

    }else{
        VPL_REPORT_FATAL("Enumerator return 0 object");
        return VPL_ERR_FAIL;                  // Program has failed.
    }

    return VPL_OK;

    //cleanup
cleanup:

    VPL_ReleaseDeviceInfo(*manufacturer, *model);
    return VPL_ERR_FAIL;
    
}

void VPL_ReleaseDeviceInfo(char* manufacturer, char* model)
{
    if (manufacturer != NULL) {
        // should have been allocated via _VPL__wstring_to_utf8_alloc (malloc) function
        free(manufacturer);
        manufacturer = NULL;
    }

    if (model != NULL) {
        // should have been allocated via _VPL__wstring_to_utf8_alloc (malloc) function
        free(model);
        model = NULL;
    }
}

int VPL_GetOSVersion(char** osVersion_out)
{
    int rv = VPL_OK;

    if (osVersion_out == NULL) {
        return VPL_ERR_INVALID;
    }

    *osVersion_out = NULL;

    if (!gInitialized) {
        return VPL_ERR_NOT_INIT;
    }

    OSVERSIONINFO OsVersionInfo;
    ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    std::string osVersion = "Windows (desktop mode)"; //default set to Windows (desktop mode)

    if (GetVersionEx(&OsVersionInfo)) 
    {

        if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            if (OsVersionInfo.dwMajorVersion == 4) {
                osVersion = "Windows NT";
            } else if (OsVersionInfo.dwMajorVersion == 5 && OsVersionInfo.dwMinorVersion == 0) {
                osVersion = "Windows 2000";
            } else if (OsVersionInfo.dwMajorVersion == 5 && OsVersionInfo.dwMinorVersion == 1) {
                osVersion = "Windows XP";
            } else if (OsVersionInfo.dwMajorVersion == 5 && OsVersionInfo.dwMinorVersion == 2) {
                osVersion = "Windows 2003";
            } else if (OsVersionInfo.dwMajorVersion == 6 && OsVersionInfo.dwMinorVersion == 0) {
                osVersion = "Windows VISTA";
            } else if (OsVersionInfo.dwMajorVersion == 6 && OsVersionInfo.dwMinorVersion == 1) {
                osVersion = "Windows 7";
            } else if (OsVersionInfo.dwMajorVersion == 6 && OsVersionInfo.dwMinorVersion == 2) {
                osVersion = "Windows 8";
            } else if (OsVersionInfo.dwMajorVersion == 6 && OsVersionInfo.dwMinorVersion == 3) {
                osVersion = "Windows 8.1";
            }
        }

    }

    char *buf = NULL;
    
    buf = strdup(osVersion.c_str());
    if(buf == NULL){
        rv = VPL_ERR_NOMEM;
        goto cleanup;
    }
    *osVersion_out = buf;

cleanup:
    
    if(buf != NULL && rv != VPL_OK){
        free(buf);
        buf = NULL;
        *osVersion_out = NULL;
    }


    return rv;
}

void VPL_ReleaseOSVersion(char* osVersion)
{
    if (osVersion != NULL) {
        // should have been allocated via strdup(malloc)
        free(osVersion);
        osVersion = NULL;
    }

}

