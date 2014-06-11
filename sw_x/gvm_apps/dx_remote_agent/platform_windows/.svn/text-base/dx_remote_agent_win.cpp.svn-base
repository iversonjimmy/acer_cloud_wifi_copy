#include "dx_remote_agent.h"
#include "DxRemoteQueryDeviceAgent.hpp"
#include <Windows.h>
#include <tchar.h>

#define _WIN32_DCOM
#include <comdef.h>
#include <Wbemidl.h>

#include <string>
#include <algorithm>

# pragma comment(lib, "wbemuuid.lib")

namespace std
{
    #ifdef _UNICODE
        typedef wstring tstring;
    #else
        typedef string tstring;
    #endif
};

typedef enum _Brand
{
    Brand_Non_Acer = 0,
    Brand_Acer = 1,
    Brand_Gateway = 2,
    Brand_PackardBell = 3,
    Brand_eMachine = 4,
    Brand_Founder = 5
} Brand;

typedef enum _SystemArch
{
    SystemArchUnknown = 0,
    SystemArchX86 = 1,
    SystemArchX64 = 2,
    SystemArchIA64 = 3
} SystemArch;

const static std::tstring MetadataPath = TEXT("SOFTWARE\\OEM\\Metadata");
const static std::tstring tstrBrand = TEXT("Brand");
const static std::tstring tstrMetaSys = TEXT("Sys");
const static std::tstring brandAcer = TEXT("acer");
const static std::tstring brandGateway = TEXT("gateway");
const static std::tstring brandPackardBell = TEXT("packard");
const static std::tstring brandeMachine = TEXT("emachines");
const static std::tstring brandFounder = TEXT("founder");
const static std::tstring tstrWMINamespace = TEXT("ROOT\\CIMV2");
const static std::tstring tstrWMIQL = TEXT("WQL");
const static std::tstring tstrWMIQuery = TEXT("SELECT * FROM Win32_ComputerSystemProduct");
const static std::tstring tstrWMIQueryDevClass = TEXT("SELECT * FROM Win32_SystemEnclosure");
const static std::tstring tstrWMIQueryOS = TEXT("SELECT * FROM Win32_OperatingSystem");
const static std::tstring tstrWMIQueryPNP = TEXT("SELECT * FROM Win32_PnPEntity");
const static std::tstring tstrWMIVendor = TEXT("Vendor");
const static std::tstring tstrWMICaption = TEXT("Caption");
const static std::tstring tstrWMIChassisTypes = TEXT("ChassisTypes");
const static std::tstring tstrWMIService = TEXT("Service");
const static std::tstring tstrWMIusbvideo = TEXT("usbvideo");
const static std::string strUnknown("[Unknown]");

Brand GetBrand();
bool IsMetadataExist();
Brand BrandInMetadata();
Brand BrandInWMI();
std::string ChassisTypesToString(int chassisType);
std::string GetWindowsCaption();

std::string DxRemoteQueryDeviceAgent::GetDeviceClass()
{
    std::string strClass;
    IWbemLocator *pLoc = NULL;
    IWbemServices *pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
        return strUnknown;
    }
    else if (FAILED(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL))) {
        CoUninitialize();
        return strUnknown;
    }
    else if (FAILED(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc))) {
        CoUninitialize();
        return strUnknown;
    }
    else if (FAILED(pLoc->ConnectServer(_bstr_t(tstrWMINamespace.c_str()), NULL, NULL, 0, NULL, 0, 0, &pSvc))) {
        pLoc->Release();
        CoUninitialize();
        return strUnknown;
    }
    else if (FAILED(CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE))) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return strUnknown;
    }
    else if (FAILED(pSvc->ExecQuery(_bstr_t(tstrWMIQL.c_str()), _bstr_t(tstrWMIQueryDevClass.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator))) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return strUnknown;
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) {
            break;
        }

        VARIANT vtProp;

        hr = pclsObj->Get(tstrWMIChassisTypes.c_str(), 0, &vtProp, 0, 0);
        LONG lLower, lUpper;

        SAFEARRAY FAR *psa = vtProp.parray;

        SafeArrayGetLBound(psa, 1, &lLower);
        SafeArrayGetUBound(psa, 1, &lUpper);

        for (LONG lIdx = lLower; lIdx <= lUpper; ++lIdx)
        {
            INT chassisType = 0;
            SafeArrayGetElement(psa, &lIdx, &chassisType);
            strClass.append(ChassisTypesToString(chassisType));
            if (lIdx != lUpper)
                strClass.append(", ");
        }

        SafeArrayDestroyData(psa);
        VariantClear(&vtProp);
        pclsObj->Release();
        pclsObj = NULL;
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    if (pclsObj != NULL)
        pclsObj->Release();
    CoUninitialize();

    return strClass;
}

std::string DxRemoteQueryDeviceAgent::GetOSVersion()
{
    return GetWindowsCaption();
}

bool DxRemoteQueryDeviceAgent::GetIsAcer()
{
    return !(GetBrand() == Brand_Non_Acer);
}

bool DxRemoteQueryDeviceAgent::GetHasCamera()
{
    std::tstring tstrService;
    bool bHasCamera = false;
    IWbemLocator *pLoc = NULL;
    IWbemServices *pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
        return bHasCamera;
    }
    else if (FAILED(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL))) {
        CoUninitialize();
        return bHasCamera;
    }
    else if (FAILED(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc))) {
        CoUninitialize();
        return bHasCamera;
    }
    else if (FAILED(pLoc->ConnectServer(_bstr_t(tstrWMINamespace.c_str()), NULL, NULL, 0, NULL, 0, 0, &pSvc))) {
        pLoc->Release();
        CoUninitialize();
        return bHasCamera;
    }
    else if (FAILED(CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE))) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return bHasCamera;
    }
    else if (FAILED(pSvc->ExecQuery(_bstr_t(tstrWMIQL.c_str()), _bstr_t(tstrWMIQueryPNP.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator))) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return bHasCamera;
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) {
            break;
        }

        VARIANT vtProp;
        hr = pclsObj->Get(tstrWMIService.c_str(), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && V_VT(&vtProp) == VT_BSTR && vtProp.bstrVal != NULL) {
            tstrService = std::tstring(vtProp.bstrVal);
            transform(tstrService.begin(), tstrService.end(), tstrService.begin(), _totlower);
            if (tstrService.compare(tstrWMIusbvideo) == 0) {
                bHasCamera = true;
            }
            VariantClear(&vtProp);
        } else {
            // skip null string and continue next one
        }

        pclsObj->Release();
        pclsObj = NULL;
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    if (pclsObj != NULL)
        pclsObj->Release();
    CoUninitialize();

    return bHasCamera;
}

Brand GetBrand()
{
    if (IsMetadataExist()) {
        Brand brand = BrandInMetadata();
        if (brand == Brand_Non_Acer)
            return BrandInWMI();
        else
            return brand;
    }
    else {
        return BrandInWMI();
    }
}

bool IsMetadataExist()
{
    LONG lRes = ERROR_SUCCESS;
    HKEY hKeyRoot = NULL;
    lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, MetadataPath.c_str(), 0, KEY_QUERY_VALUE, &hKeyRoot);
    if (lRes != ERROR_SUCCESS || hKeyRoot == NULL) {
        return false;
    }
    else {
        if (hKeyRoot != NULL) {
            RegCloseKey(hKeyRoot);
            hKeyRoot = NULL;
        }
        return true;
    }
}

Brand BrandInMetadata()
{
    LONG lRes = ERROR_SUCCESS;
    HKEY hKeyRoot = NULL;
    DWORD dwType = REG_SZ;
    DWORD dwLen = MAX_PATH;
    TCHAR *szData = new TCHAR[MAX_PATH + 1];
    lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, MetadataPath.c_str(), 0, KEY_QUERY_VALUE, &hKeyRoot);
    if (lRes != ERROR_SUCCESS) {
        if (hKeyRoot != NULL) {
            RegCloseKey(hKeyRoot);
            hKeyRoot = NULL;
        }
        return Brand_Non_Acer;
    }

    lRes = RegQueryValueEx(hKeyRoot, tstrBrand.c_str(), NULL, &dwType, (LPBYTE)szData, &dwLen);
    RegCloseKey(hKeyRoot);

    if (lRes != ERROR_SUCCESS) {
        return Brand_Non_Acer;
    }

    std::tstring tstrBrandName(szData);
    if (szData != NULL) {
        delete []szData;
        szData = NULL;
    }

    transform(tstrBrandName.begin(), tstrBrandName.end(), tstrBrandName.begin(), _totlower);

    if (tstrBrandName.find(brandAcer) != std::tstring::npos)
        return Brand_Acer;
    else if (tstrBrandName.find(brandGateway) != std::tstring::npos)
        return Brand_Gateway;
    else if (tstrBrandName.find(brandPackardBell) != std::tstring::npos)
        return Brand_PackardBell;
    else if (tstrBrandName.find(brandeMachine) != std::tstring::npos)
        return Brand_eMachine;
    else if (tstrBrandName.find(brandFounder) != std::tstring::npos)
        return Brand_Founder;
    else
        return Brand_Non_Acer;
}

Brand BrandInWMI()
{
    std::tstring tstrBrand;
    IWbemLocator *pLoc = NULL;
    IWbemServices *pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
        return Brand_Non_Acer;
    }
    else if (FAILED(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL))) {
        CoUninitialize();
        return Brand_Non_Acer;
    }
    else if (FAILED(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc))) {
        CoUninitialize();
        return Brand_Non_Acer;
    }
    else if (FAILED(pLoc->ConnectServer(_bstr_t(tstrWMINamespace.c_str()), NULL, NULL, 0, NULL, 0, 0, &pSvc))) {
        pLoc->Release();
        CoUninitialize();
        return Brand_Non_Acer;
    }
    else if (FAILED(CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE))) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return Brand_Non_Acer;
    }
    else if (FAILED(pSvc->ExecQuery(_bstr_t(tstrWMIQL.c_str()), _bstr_t(tstrWMIQuery.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator))) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return Brand_Non_Acer;
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) {
            break;
        }

        VARIANT vtProp;
        hr = pclsObj->Get(tstrWMIVendor.c_str(), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && V_VT(&vtProp) == VT_BSTR && vtProp.bstrVal != NULL) {
            tstrBrand = std::tstring(vtProp.bstrVal);
            VariantClear(&vtProp);
        } else {
            // skip null string and continue next one
        }
        pclsObj->Release();
        pclsObj = NULL;
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    if (pclsObj != NULL)
        pclsObj->Release();
    CoUninitialize();

    transform(tstrBrand.begin(), tstrBrand.end(), tstrBrand.begin(), _totlower);
    if (tstrBrand.find(brandAcer) != std::tstring::npos)
        return Brand_Acer;
    else if (tstrBrand.find(brandGateway) != std::tstring::npos)
        return Brand_Gateway;
    else if (tstrBrand.find(brandPackardBell) == 0)
        return Brand_PackardBell;
    else if (tstrBrand.find(brandeMachine) != std::tstring::npos)
        return Brand_eMachine;
    else if (tstrBrand.find(brandFounder) != std::tstring::npos)
        return Brand_Founder;
    else
        return Brand_Non_Acer;
}

std::string ChassisTypesToString(int chassisType)
{
    std::string strClass;
    switch (chassisType)
    {
    case 1:
        strClass = std::string("Other");
        break;
    case 2:
        strClass = std::string("Unknown");
        break;
    case 3:
        strClass = std::string("Desktop");
        break;
    case 4:
        strClass = std::string("Low Profile Desktop");
        break;
    case 5:
        strClass = std::string("Pizza Box");
        break;
    case 6:
        strClass = std::string("Mini Tower");
        break;
    case 7:
        strClass = std::string("Tower");
        break;
    case 8:
        strClass = std::string("Portable");
        break;
    case 9:
        strClass = std::string("Laptop");
        break;
    case 10:
        strClass = std::string("Notebook");
        break;
    case 11:
        strClass = std::string("Hand Held");
        break;
    case 12:
        strClass = std::string("Docking Station");
        break;
    case 13:
        strClass = std::string("All in One");
        break;
    case 14:
        strClass = std::string("Sub Notebook");
        break;
    case 15:
        strClass = std::string("Space-Saving");
        break;
    case 16:
        strClass = std::string("Lunch Box");
        break;
    case 17:
        strClass = std::string("Main System Chassis");
        break;
    case 18:
        strClass = std::string("Expansion Chassis");
        break;
    case 19:
        strClass = std::string("SubChassis");
        break;
    case 20:
        strClass = std::string("Bus Expansion Chassis");
        break;
    case 21:
        strClass = std::string("Peripheral Chassis");
        break;
    case 22:
        strClass = std::string("Storage Chassis");
        break;
    case 23:
        strClass = std::string("Rack Mount Chassis");
        break;
    case 24:
        strClass = std::string("Sealed-Case PC");
        break;
    default:
        strClass = std::string("<Unknown>");
        break;
    }

    return strClass;
}

std::string GetWindowsCaption()
{
    std::tstring tstrCaption;
    std::string strCaption;
    IWbemLocator *pLoc = NULL;
    IWbemServices *pSvc = NULL;
    char *utf8Caption = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED))) {
        return strUnknown;
    }
    else if (FAILED(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL))) {
        CoUninitialize();
        return strUnknown;
    }
    else if (FAILED(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc))) {
        CoUninitialize();
        return strUnknown;
    }
    else if (FAILED(pLoc->ConnectServer(_bstr_t(tstrWMINamespace.c_str()), NULL, NULL, 0, NULL, 0, 0, &pSvc))) {
        pLoc->Release();
        CoUninitialize();
        return strUnknown;
    }
    else if (FAILED(CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE))) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return strUnknown;
    }
    else if (FAILED(pSvc->ExecQuery(_bstr_t(tstrWMIQL.c_str()), _bstr_t(tstrWMIQueryOS.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator))) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return strUnknown;
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) {
            break;
        }

        VARIANT vtProp;
        hr = pclsObj->Get(tstrWMICaption.c_str(), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && V_VT(&vtProp) == VT_BSTR && vtProp.bstrVal != NULL) {
            tstrCaption = std::tstring(vtProp.bstrVal);
            VariantClear(&vtProp);
        } else {
            // skip null string and continue next one
        }
        pclsObj->Release();
        pclsObj = NULL;
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    if (pclsObj != NULL)
        pclsObj->Release();
    CoUninitialize();

    _VPL__wstring_to_utf8_alloc(tstrCaption.c_str(), &utf8Caption);
    strCaption.assign(utf8Caption);

    if (utf8Caption != NULL) {
        free(utf8Caption);
        utf8Caption = NULL;
    }

    return strCaption;
}
