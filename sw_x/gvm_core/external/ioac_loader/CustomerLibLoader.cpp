#include "PreDefineData.h"
#include <Iptypes.h>
#include <wlanapi.h>

#include <cfgmgr32.h>
#include <SetupAPI.h>
#include <vector>


CUSTOMER_RETURN_CODE dummyBeginProgram(void) { return CUSTOMER_CONTROL_OK; }
CUSTOMER_RETURN_CODE dummyBeginSession(void) { return CUSTOMER_CONTROL_OK; }
CUSTOMER_RETURN_CODE dummyCustomerControl(  DWORD dwIoControlCode,
                                            LPVOID lpInBuffer, DWORD nInBufferSize,
                                            LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                            LPDWORD lpBytesReturned) { return CUSTOMER_CONTROL_OK; }
CUSTOMER_RETURN_CODE dummyEndSession(void) { return CUSTOMER_CONTROL_OK; }
CUSTOMER_RETURN_CODE dummyEndProgram(void) { return CUSTOMER_CONTROL_OK; }

/******************************************************************************
 * Global vers define;
 *****************************************************************************/
BOOL GetAllExistNetworkDriverKeyNum();
DWORD   dwDebugMode     = 0;
DWORD   dwNetAdapterNum = 0;
tNetAdpaterInfo G[10]   = {0};
std::vector <unsigned int> vDriverKey;

//#define SHOW_GET_EXIST_NET_DRIVERS_PROCESS

/*
 * Vendor ID , Device ID , Sub Vendor ID, SubSystem ID
 */
tOpt DeviceOptList[] =
{
    // the first is test only
    //{L"14E4",L"16B0",L"1025",L"0579"},
    {L"8086",L"1503",L"1025",L"069A"},
    {L"8086",L"1503",L"1025",L"063D"},
    {L"8086",L"1503",L"1025",L"0660"},
    {L"8086",L"1503",L"1025",L"066C"},
    {L"8086",L"153A",L"1025",L"0752"},
    {L"8086",L"153A",L"1025",L"0793"},
    {L"8086",L"153B",L"1025",L"078A"},
    {L"8086",L"153B",L"1025",L"078C"}
};


// 20121016 added by Walker
#define RTL8111FA_SSID_NUMBER 17
tOpt RTL8111FA_DeviceOpt[] = {{L"10EC",L"8168",L"1025",L""}};
WCHAR *RTL8111FA_SSID[] = { L"0592", L"0618", L"061C", L"063E", L"063F",
                            L"0640", L"0641", L"0661", L"0664", L"070A",
                            L"070B", L"070C", L"070E", L"073F", L"0744"
                            L"074A", L"074E", L"8000"};

tOpt QCAWB222_DeviceOpt[] = {{L"168C",L"0034",L"11AD",L"6621"}, // LiteOn
                             {L"168C",L"0034",L"105B",L"E052"}  // Foxconn
                            };

tOpt QCAMD222_DeviceOpt[] = {{L"168C",L"0034",L"11AD",L"6651"}, // LiteOn
                             {L"168C",L"0034",L"105B",L"E058"}  // Foxconn
                            };

tOpt BCM43228_DeviceOpt[] = {{L"14E4",L"4359",L"11AD",L"6603"}, // LiteOn
                             {L"14E4",L"4359",L"105B",L"E04B"}  // Foxconn
                            };

tOpt BCM4313iPA_DeviceOpt[] = {{L"14E4",L"4359",L"105B",L"E04C"}};  // Foxconn
//tOpt BCM4313iPA_DeviceOpt = {L"0489",L"4359",L"11AD",L"6603"};


/*
 * Ndis Driver list
 * We'll ignore the driver checking on this list
 */
WCHAR * NdisDriverList[] =
{
    {L"LanmanServer"},
    {L"NdisCap"},
    {L"LanmanWorkstation"},
    NULL
};

char * WlanSecurityType[] =
{
    NULL,
    {"open"},
    {"sharedkey"},
    {"wpa"},
    {"wpa-psk"},
    {"wpa-none"},
    {"wpa2"},
    {"wpa2-psk"},
    NULL
};

__inline int GuidToString(GUID *guid, WCHAR *p)
{
    return  swprintf( p, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                      guid->Data1, guid->Data2, guid->Data3,
                      guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                      guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
}

//#define SHOW_GET_EXIST_NET_DRIVERS_PROCESS
//#define OUTPUT_DEBUG_LOG_FILE
#ifndef OUTPUT_DEBUG_LOG_FILE
static void KdPrint (const char * format,...)
{
    if (dwDebugMode)
    {
        char buf[MAX_LEN];
        va_list ap;
        va_start(ap, format);
        _vsnprintf(buf, sizeof(buf), format, ap);
        printf(buf);
        va_end(ap);
    }
    return;
}
#else
FILE *gfp;
void KdPrint(char *fmt, ...){
    int n = 0;
    char systime[20];
    char msg[200];
    va_list ap;
    SYSTEMTIME st;

    if(gfp==NULL){
        printf("[LOG_INFO_FILE]Invalid FILE pointer!!\n");
        return;
    }

    GetSystemTime(&st);
    va_start(ap, fmt);
    _snprintf(systime, sizeof(systime), "[%02d:%02d:%02d.%03d]", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    n = _snprintf(msg, sizeof(msg), "%s ", systime);

    n += vsnprintf(msg + n, sizeof(msg) - n, fmt, ap);
    msg[n] = 0;

    if(gfp){
        fwrite(msg, 1, n, gfp);
        fflush(gfp);
    }

    return;
}

// Open Log File ---------------------
BOOLEAN CreateLogFile(void){
    SYSTEMTIME stLocal;
    //char file[200];       // file name;
    char file[] = "C:\\Users\\Public\\IOAC_CUSTOMER_LOADER_LOG_FILE.txt";
    char time_msg[200];

    ::GetLocalTime(&stLocal);

    //_snprintf(file, sizeof(file) - 1, "DETECTION_IOAC_LOG-[%04d%02d%02d-%02d%02d%02d].txt",
    //      stLocal.wYear,stLocal.wMonth,stLocal.wDay,stLocal.wHour,stLocal.wMinute, stLocal.wSecond);
    _snprintf(time_msg, sizeof(time_msg) - 1, "START_LOADER_LOG-[%04d/%02d/%02d-%02d:%02d%:%02d] --------------------------------",
            stLocal.wYear,stLocal.wMonth,stLocal.wDay,stLocal.wHour,stLocal.wMinute, stLocal.wSecond);

    //printf("file name = %s\n", file);
    gfp = fopen(file, "a+");
///gfp = fopen("IOAC_DIAGNOSIS_TOOL_LOG_FILE.txt", "a+");

    if(gfp == NULL){
        printf("[CreateLogFile] Open Log File fail!!\n");
        printf("error code = 0x%x\n", GetLastError());
        return FALSE;
    }

    //KdPrint("%s\n", time_msg);

    return TRUE;
}
#endif


int
IsBcmJupiter(WCHAR *name)
{
    int len;
    WCHAR **atn, *ws, *ptr;
    static WCHAR *BroadNames[] = {
        L"Broadcom 802.11n",
        NULL };

    for (int i  = 0 ; i < wcslen(name) - wcslen(L"Broadcom 802.11n"); i++)
    {
        ptr = name+i;
        for(atn = BroadNames; ws = *atn++;) {
            len = wcslen(ws);
            if( !wcsncmp(ptr, ws, len))         {
                //printf("find out broadn \n");
                return(1);
            }
        }
    }
    return (0);
}


int
IsAthJupiter(WCHAR *name)
{
    int len;
    WCHAR **atn, *ws, *ptr;

    static WCHAR *AthNames[] = {
        L"Atheros AR946x",
        L"Atheros AR5BWB222",
        L"Atheros AR5BMD222",
        //L"Broadcom 802.11n",  // for test only
        NULL };

    /*
     * searching AthNames under the name string.
     */
    for (int i  = 0 ; i < wcslen(name) - wcslen(L"Atheros AR5BMD222"); i++)
    {
        ptr = name+i;
        for(atn = AthNames; ws = *atn++;) {
            len = wcslen(ws);
            if( !wcsncmp(ptr, ws, len)){
                return(1);
            }
        }
    }

    return(0);
}

//int GetWlanInterfacesInfo(){
GUID* GetWlanInterfacesInfo()
{
    // Declare and initialize variables.
    HANDLE hClient = NULL;
    DWORD dwCurVersion = 0;
    DWORD dwResult = 0;
    //GUID *temp = NULL;
    // variables used for WlanEnumInterfaces
    //PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    WLAN_INTERFACE_INFO_LIST *pIfList;
    WLAN_INTERFACE_INFO *pIfInfo;

    int inum;
    int i;
    int nif;
    //int iret;
    //WCHAR guid[40];

    dwResult = WlanOpenHandle(WLAN_API_VERSION_2_0, NULL, &dwCurVersion, &hClient);
    if(dwResult != ERROR_SUCCESS){
        //KdPrint("<GetWlanInterfacesInfo> WlanOpenHandle function failed with error\n");
        return 0;
    }

    dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
    if(dwResult != ERROR_SUCCESS){
        //printf("WlanEnumInterfaces function failed with error\n");
        return 0;
    }

    inum = pIfList->dwNumberOfItems;
    nif = 0;
    for(i=0; i<inum; i++)
    {
        pIfInfo = pIfList->InterfaceInfo + i;

        wprintf(L"  %ws\n", pIfInfo->strInterfaceDescription);

        if(!IsAthJupiter(pIfInfo->strInterfaceDescription))
        {
            //Wifi_module_vendor = ATHEROS_WIFI_DLL;
            //printf("atheros card found\n");
            continue;
        }
        nif++;
    }
    //temp = &pIfInfo->InterfaceGuid;
    //iret = StringFromGUID2((const GUID &)pIfInfo->InterfaceGuid, (LPOLESTR)guid, 39);
    //wprintf(L"  %ws\n", guid);
    //printf("%ws\n",pIfInfo->InterfaceGuid);
    //printf("test\n");
    /*
    if (pIfList != NULL) {
        WlanFreeMemory(pIfList);
        pIfList = NULL;
    }
    */
    //return temp;
    if(nif == 0)
        return(NULL);
    else
        return &pIfInfo->InterfaceGuid;
}


HRESULT RegGetString(HKEY hKey, LPCTSTR szValueName, LPTSTR * lpszResult)
{
    // Given a HKEY and value name returns a string from the registry.
    // Upon successful return the string should be freed using free()
    // eg. RegGetString(hKey, TEXT("my value"), &szString);

    DWORD dwType=0, dwDataSize=0, dwBufSize=0;
    LONG lResult;

    // Incase we fail set the return string to null...
    if (lpszResult != NULL) *lpszResult = NULL;

    // Check input parameters...
    if (hKey == NULL || lpszResult == NULL) return E_INVALIDARG;

    // Get the length of the string in bytes (placed in dwDataSize)...
    lResult = RegQueryValueEx(hKey, szValueName, 0, &dwType, NULL, &dwDataSize );

    // Check result and make sure the registry value is a string(REG_SZ)...
    if (lResult != ERROR_SUCCESS) return HRESULT_FROM_WIN32(lResult);
    else if (dwType != REG_SZ)    return DISP_E_TYPEMISMATCH;

    // Allocate memory for string - We add space for a null terminating character...
    dwBufSize = dwDataSize + (1 * sizeof(TCHAR));
    *lpszResult = (LPTSTR)malloc(dwBufSize);

    if (*lpszResult == NULL) return E_OUTOFMEMORY;

    // Now get the actual string from the registry...
    lResult = RegQueryValueEx(hKey, szValueName, 0, &dwType, (LPBYTE) *lpszResult, &dwDataSize );

    // Check result and type again.
    // If we fail here we must free the memory we allocated...
    if (lResult != ERROR_SUCCESS)
    {
        if (lpszResult != NULL)
        {
            free(*lpszResult);
            lpszResult = NULL;
        }
        return HRESULT_FROM_WIN32(lResult);
    }
    else if (dwType != REG_SZ)
    {
        if (lpszResult != NULL)
        {
            free(*lpszResult);
            lpszResult = NULL;
        }
        return DISP_E_TYPEMISMATCH;
    }

    // We are not guaranteed a null terminated string from RegQueryValueEx.
    // Explicitly null terminate the returned string...
    (*lpszResult)[(dwBufSize / sizeof(TCHAR)) - 1] = TEXT('\0');

    return NOERROR;
}

HRESULT RegGetDWord(HKEY hKey, LPCTSTR szValueName, DWORD * lpdwResult)
{
    // Given a value name and an hKey returns a DWORD from the registry.
    // eg. RegGetDWord(hKey, TEXT("my dword"), &dwMyValue);

    LONG lResult;
    DWORD dwDataSize = sizeof(DWORD);
    DWORD dwType = 0;

    // Check input parameters...
    if (hKey == NULL || lpdwResult == NULL) return E_INVALIDARG;

    // Get dword value from the registry...
    lResult = RegQueryValueEx(hKey, szValueName, 0, &dwType, (LPBYTE) lpdwResult, &dwDataSize );

    // Check result and make sure the registry value is a DWORD(REG_DWORD)...
    if (lResult != ERROR_SUCCESS) return HRESULT_FROM_WIN32(lResult);
    else if (dwType != REG_DWORD) return DISP_E_TYPEMISMATCH;

    return NOERROR;
}

//int
//CheckDeviceVid(WCHAR * targetKey, DWORD target_nType)
//{
//  int dwIndex = -1;
//  ptPciID PciConfig;
//  tPciID  Pcic = {0};
//  //KdPrint("<CheckDeviceVid> targetkey = %ws\n", targetKey);
//
//  if ((!targetKey) || (memcmp(targetKey,L"PCI\\",8 ) != 0)){
//      //KdPrint("<CheckDeviceVid> - Unknown Driver Vendor\n");
//      return dwIndex;
//  }
//
//  PciConfig = (ptPciID)targetKey;
//  memcpy(Pcic.vid,targetKey + OFF_VID,4 * sizeof (WCHAR));
//
//  ////KdPrint(" -- %ws --\n",Pcic.vid);
//  //KdPrint("<CheckDeviceVid> - Driver Vendor ID = %ws\n",Pcic.vid);
//
//  if (!wcsncmp(Pcic.vid,L"8086",wcslen(L"8086"))) return  INTEL;
//  if (!wcsncmp(Pcic.vid,L"10EC",wcslen(L"10EC"))) return  REALTEK;
//
//  if (!wcsncmp(Pcic.vid,L"14E4",wcslen(L"14E4"))) {
//      if(target_nType == IF_TYPE_IEEE80211){
//          return BROADCOM_WIFI;
//      }else{
//          return BROADCOM_LAN;
//      }
//  }
//
//  if (!wcsncmp(Pcic.vid,L"168C",wcslen(L"168C"))) return  ATHEROS_WIFI;
//  if (!wcsncmp(Pcic.vid,L"1969",wcslen(L"1969"))) return  ATHEROS_LAN;
//
//  return (dwIndex);
//}



bool CheckIntelLAN(tPciID dPcic){
    int i = 0;
    int num = sizeof(DeviceOptList)/sizeof(tOpt);
    int idlen = wcslen(dPcic.ssid);

    //KdPrint("<CheckIntelLAN> dPciID : VID = %ws, DID = %ws, SVID = %ws, SSID = %ws\n", dPcic.vid, dPcic.did, dPcic.svid, dPcic.ssid);
    printf("<CheckIntelLAN> dPciID : VID = %ws, DID = %ws, SVID = %ws, SSID = %ws\n", dPcic.vid, dPcic.did, dPcic.svid, dPcic.ssid);
    for (i = 0 ; i < num ; i++){
        //KdPrint("<CheckIntelLAN> Opt 0%d : VID = %ws, DID = %ws, SVID = %ws, SSID = %ws\n", i, DeviceOptList[i].vid, DeviceOptList[i].did, DeviceOptList[i].svid, DeviceOptList[i].ssid);
        //printf("<CheckIntelLAN> Opt 0%d : VID = %ws, DID = %ws, SVID = %ws, SSID = %ws\n", i, DeviceOptList[i].vid, DeviceOptList[i].did, DeviceOptList[i].svid, DeviceOptList[i].ssid);
        //if( !wcsncmp(dPcic.ssid,DeviceOptList[i].ssid, idlen) &&
        //  !wcsncmp(dPcic.svid,DeviceOptList[i].svid, idlen) &&
        //    !wcsncmp(dPcic.did,DeviceOptList[i].did, idlen) &&
        //    !wcsncmp(dPcic.vid,DeviceOptList[i].vid, idlen))
        //  return true;
        if( !wcsncmp(dPcic.did,DeviceOptList[i].did, idlen) &&
            !wcsncmp(dPcic.vid,DeviceOptList[i].vid, idlen)){
            return true;
        }
    }
    return false;
}


int CheckAdatperModuleFun(tPciID dPcic, ExtendMagicPacketModule module){
    tOpt *module_pcid_ptr;
    WCHAR **RTL_ssid;
    int idlen = wcslen(dPcic.ssid);
    int num = 0;
    int i = 0;

    if(module == QCAWB222){
        module_pcid_ptr = QCAWB222_DeviceOpt;
        num = sizeof(QCAWB222_DeviceOpt)/sizeof(tOpt);
    }
    else if(module == QCAMD222){
        module_pcid_ptr = QCAMD222_DeviceOpt;
        num = sizeof(QCAMD222_DeviceOpt)/sizeof(tOpt);
    }
    else if(module == BCM43228){
        module_pcid_ptr = BCM43228_DeviceOpt;
        num = sizeof(BCM43228_DeviceOpt)/sizeof(tOpt);
    }
    else if(module == RTL8111FA){
        module_pcid_ptr = RTL8111FA_DeviceOpt;
        num = sizeof(RTL8111FA_DeviceOpt)/sizeof(tOpt);
        RTL_ssid = RTL8111FA_SSID;
    }else{
        return -1;
    }

    //KdPrint("<CheckAdatperModuleFun> Module Type = %d, Number = %d, ID Length = %d\n",module, num, idlen);
    //KdPrint("<CheckAdatperModuleFun> Get dPciID : VID = %ws, DID = %ws, SVID = %ws, SSID = %ws\n", dPcic.vid, dPcic.did, dPcic.svid, dPcic.ssid);
    for( i=0; i<num; i++){
        tOpt module_pcid = *(module_pcid_ptr + i);
        //KdPrint("<CheckAdatperModuleFun> Module [%d] : VID = %ws, DID = %ws, SVID = %ws, SSID = %ws\n", i, module_pcid.vid, module_pcid.did, module_pcid.svid, module_pcid.ssid);

        if( !wcsncmp(dPcic.svid, module_pcid.svid, idlen) && !wcsncmp(dPcic.did, module_pcid.did, idlen) &&
            !wcsncmp(dPcic.vid, module_pcid.vid, idlen))
        {
            if(module != RTL8111FA){
                if(!wcsncmp(dPcic.ssid, module_pcid.ssid , idlen))
                    return 1;
            }else{
                //KdPrint("<CheckAdatperModuleFun> RTL8111FA SSID Check :\n");
                for(int i=0; i < RTL8111FA_SSID_NUMBER; i++){
                    WCHAR *ssid = RTL_ssid[i];
                    //KdPrint("<CheckAdatperModuleFun>  - %d : %ws\n", i, ssid);
                    if(!wcsncmp(dPcic.ssid, ssid, idlen)){
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}


int
CheckDeviceVidDidSVidSSid(WCHAR * targetKey, BYTE *mpFmt)
{
    int i;
    tPciID  Pcic = {0};

    //KdPrint("<CheckDeviceVidDidSVidSSid> targetKey = %ws\n", targetKey);
    if (memcmp(targetKey,L"PCI\\",wcslen(L"PCI\\")))    return -1;

    memcpy(Pcic.vid,targetKey + OFF_VID,4 * sizeof (WCHAR));    // OFF_VID  = 8
    memcpy(Pcic.did,targetKey + OFF_DID,4 * sizeof (WCHAR));    // OFF_DID  = 17
    memcpy(Pcic.svid,targetKey+ OFF_SVID,4 * sizeof (WCHAR));   // OFF_SVID = 33
    memcpy(Pcic.ssid,targetKey+ OFF_SSID,4 * sizeof (WCHAR));   // OFF_SSID = 29

    //KdPrint("<CheckDeviceVidDidSVidSSid> %ws.%ws.%ws.%ws\n", Pcic.vid,Pcic.did, Pcic.svid, Pcic.ssid);

    //KdPrint("<CheckDeviceVidDidSVidSSid> Check QCA WB222 module ----------------\n");
    if(CheckAdatperModuleFun(Pcic, QCAWB222)){
        //KdPrint("<CheckDeviceVidDidSVidSSid> This is Atheros WiFi module (WB222)\n");
        *mpFmt = MAGIC_PACKET_TYPE_EXTENDED;
        return ATHEROS_WIFI;
    }

    //KdPrint("<CheckDeviceVidDidSVidSSid> Check QCA MD222 module ----------------\n");
    if(CheckAdatperModuleFun(Pcic, QCAMD222)){
        //KdPrint("<CheckDeviceVidDidSVidSSid> This is Atheros WiFi module (MD222)\n");
        *mpFmt = MAGIC_PACKET_TYPE_EXTENDED;
        return ATHEROS_WIFI;
    }

    //KdPrint("<CheckDeviceVidDidSVidSSid> Check BCM 43228 module ----------------\n");
    if(CheckAdatperModuleFun(Pcic, BCM43228)){
        //KdPrint("<CheckDeviceVidDidSVidSSid> This is Broadcom WiFi module (43228)\n");
        *mpFmt = MAGIC_PACKET_TYPE_EXTENDED;
        return BROADCOM_WIFI;
    }

    //KdPrint("<CheckDeviceVidDidSVidSSid> Check Intel LAN module ----------------\n");
    if(CheckIntelLAN(Pcic)){
        //KdPrint("<CheckDeviceVidDidSVidSSid> This is Intel LAN module\n");
        *mpFmt = MAGIC_PACKET_TYPE_ACER_SHORT;
        return INTEL;
    }

    //KdPrint("<CheckDeviceVidDidSVidSSid> Check RTL8111FA module ----------------\n");
    if(CheckAdatperModuleFun(Pcic, RTL8111FA)){
        //KdPrint("<CheckDeviceVidDidSVidSSid> This is Realtek LAN module (RTL8111FA)\n");
        *mpFmt = MAGIC_PACKET_TYPE_EXTENDED;
        return REALTEK;
    }

    //KdPrint("<CheckDeviceVidDidSVidSSid> This is other IOAC adapter\n");
    return OTHER_IOAC_ADAPTER;
}


int RedundancyCheck(WCHAR * sNetCfgInstanceId)
{
    int i;
    for ( i = 0 ; i < dwNetAdapterNum; i++ ){
        if (0 == wcsncmp(G[i].szNetCfgInstanceId, sNetCfgInstanceId, wcslen(G[i].szNetCfgInstanceId)))
            return 1;
    }
    return 0;
}

void GetUsersTargetGuid(WCHAR * szGuid, char * szTargetGuid)
{

    WideCharToMultiByte(    CP_ACP,
                            WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                            szGuid,
                            wcslen(szGuid),
                            (char *)szTargetGuid,
                            wcslen(szGuid),
                            0,
                            0);

}

int GetConnectionStatus(WCHAR * Id)
{
    HANDLE      hDevice;
    //DWORD     dwOpCode = 65812;
    DWORD       dwOpCode = 0;
    DWORD       dwRuten;
    ULONGLONG   ulOpData;
    WCHAR       wzDeviceId[MAX_LEN] = {0};
    BOOLEAN     bRet = FALSE;
    wsprintf(wzDeviceId,L"\\\\.\\%ws",Id);
    //KdPrint("<GetConnectionStatus> - Get Connection Status\n");
    hDevice = CreateFile( wzDeviceId, GENERIC_READ, 0, NULL,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    // let's assume that it's in connection condition.
    if (hDevice == INVALID_HANDLE_VALUE){
        //KdPrint("<GetConnectionStatus>   |- CreateFile() failure\n");
        //KdPrint("<GetConnectionStatus>   |- Error Code = 0x%x (Driver was disabled)\n", GetLastError());
        return (0);
    }

    dwOpCode = OID_GEN_MEDIA_CONNECT_STATUS;
    DeviceIoControl( hDevice, IOCTL_NDIS_QUERY_GLOBAL_STATS,
                     &dwOpCode, sizeof(dwOpCode),
                     (ULONG *)&ulOpData, sizeof(ULONGLONG),
                     &dwRuten, NULL);

    if((DWORD)ulOpData == 1){
        //KdPrint("<GetConnectionStatus>   |- Disconnection\n");
        bRet = FALSE;       // disconnect
    }
    else{
        //KdPrint("<GetConnectionStatus>   |- Connection\n",ulOpData);
        bRet = TRUE;        // connect
    }

    if(!CloseHandle(hDevice)){ // close failure
        //KdPrint("<GetConnectionStatus(>   |- Close hDevice failure\n");
    }

    return bRet;
}

int EnumerateSubKeys(HKEY RootKey, WCHAR * subKey, WCHAR * tarGuid, WLAN_INTERFACE_INFO *pIfInfo)
{

    HKEY  hKey;
    DWORD cSubKeys;         //Used to store the number of Subkeys
    DWORD maxSubkeyLen;     //Longest Subkey name length
    DWORD cValues;          //Used to store the number of Subkeys
    DWORD maxValueLen;      //Longest Subkey name length
    DWORD retCode;          //Return values of calls
    DWORD dwRet;
    DWORD dwTarget = -1;
    DWORD dwPnPCapabilities;
    WCHAR szDriverPath[MAX_LEN]         = {0};
    WCHAR szDeviceInstanceID[MAX_LEN]   = {0};
    WCHAR szNetCfgInstanceId[MAX_LEN]   = {0};
    WCHAR szDriverDesc[MAX_LEN]         = {0};
    WCHAR szComponentId[MAX_LEN]        = {0};
    DWORD dwIfType = 0;

    DWORD rcc = ERROR_SUCCESS;

    dwRet = RegOpenKeyEx(RootKey, subKey, 0, KEY_READ, &hKey);

    if (dwRet != ERROR_SUCCESS)
    {
        //KdPrint("<EnumerateSubKeys> RegOpenKeyEx fail\n");
        goto exit;
    }

    RegQueryInfoKey(hKey,            // key handle
                    NULL,            // buffer for class name
                    NULL,            // size of class string
                    NULL,            // reserved
                    &cSubKeys,        // number of subkeys
                    &maxSubkeyLen,    // longest subkey length
                    NULL,            // longest class string
                    &cValues,        // number of values for this key
                    &maxValueLen,    // longest value name
                    NULL,            // longest value data
                    NULL,            // security descriptor
                    NULL);            // last write time

    if(cSubKeys > 0)
    {
        WCHAR currentSubkey[MAX_PATH];

       //for(int i=0;i < cSubKeys;i++){
        for(int i = 0; i < vDriverKey.size(); i++){
            DWORD currentSubLen=MAX_PATH;

            HKEY  hKeyS1;
            DWORD dwLen = MAX_LEN;
            WCHAR * subKeyPath0 = new WCHAR[currentSubLen + wcslen(subKey)];
            DWORD dwLanType = CONNECTION_WIRED;     // Default is LAN;

            //KdPrint("<EnumerateSubKeys> Determine (%d) network driver key /%04d --------------------------\n", i+1,vDriverKey[i]);

                wsprintf(subKeyPath0, L"%ws\\%04d", subKey, vDriverKey[i]);
                ////KdPrint("subKeyPath0 = %ws \n", subKeyPath0);

                memset(szDeviceInstanceID,0,MAX_LEN* sizeof (WCHAR));
                memset(szDriverPath,      0,MAX_LEN* sizeof (WCHAR));
                memset(szNetCfgInstanceId,0,MAX_LEN* sizeof (WCHAR));
                memset(szComponentId,     0,MAX_LEN* sizeof (WCHAR));
                memset(szDriverDesc,      0,MAX_LEN* sizeof (WCHAR));

                dwIfType = 0;
                dwPnPCapabilities = 0;

                dwRet   = RegOpenKeyEx(RootKey, subKeyPath0, 0, KEY_READ, &hKeyS1);
                delete subKeyPath0;
                if(dwRet != ERROR_SUCCESS){
                    //KdPrint("<EnumerateSubKeys>  - Open driver key %04d fail\n", vDriverKey[i]);
                    continue;
                }else{
                    //KdPrint("<EnumerateSubKeys>  - Open driver key %04d success\n", vDriverKey[i]);
                }

                dwLen   = sizeof(DWORD);
                retCode = RegQueryValueEx(hKeyS1, L"*IfType", NULL, NULL, (LPBYTE)&dwIfType, &dwLen);
                //printf("IfType = %d ; 0x%x\n", dwIfType, retCode);

                dwLen   = MAX_LEN;
                retCode = RegQueryValueEx(hKeyS1, L"NetCfgInstanceId", NULL, NULL,
                                            (LPBYTE)&szNetCfgInstanceId, &dwLen);

                dwLen   = MAX_LEN;
                retCode = RegQueryValueEx(hKeyS1, L"DeviceInstanceID", NULL, NULL,
                                            (LPBYTE)&szDeviceInstanceID, &dwLen);

                dwLen   = MAX_LEN;
                retCode = RegQueryValueEx(hKeyS1, L"DriverDesc", NULL, NULL,
                                            (LPBYTE)&szDriverDesc, &dwLen);

                dwLen   = MAX_LEN;
                retCode = RegQueryValueEx(hKeyS1, L"IoacHwCapability", NULL, NULL,
                                            (LPBYTE)&szDriverPath, &dwLen);

                //DWORD dwOEMType = CheckDeviceVid(szDeviceInstanceID, dwIfType);
                //BYTE bMagicPacketFmt = MAGIC_PACKET_TYPE_ACER_SHORT;
                BYTE bMagicPacketFmt = 0;
                DWORD dwOEMType = CheckDeviceVidDidSVidSSid(szDeviceInstanceID, &bMagicPacketFmt);


                //if (!tarGuid) // Lan check
                if(dwIfType != IF_TYPE_IEEE80211)
                {
                    dwLanType = CONNECTION_WIRED;
                    // no IoacHwCapability Key
                    if (retCode != ERROR_SUCCESS ){
                        // not intel or SMBIOS not support always connect
                        //if ((CheckDeviceVid(szDeviceInstanceID)!=INTEL) || (G[0].dwGSmBios != 0x03))
                        if ((INTEL != dwOEMType) || (G[0].dwGSmBios != 0x3))
                        {
                            //KdPrint("<EnumerateSubKeys>  - LAN device(%04d) has no \"IoacHwCapability\" key\n\n", vDriverKey[i]);
                            rcc = RegCloseKey(hKeyS1);
                            continue;
                        }
                    }
                }
                else    //Wireless Lan Check
                {       //szNetCfgInstanceId might be 0
                    if (retCode != ERROR_SUCCESS){
                        //KdPrint("<EnumerateSubKeys>  - WLAN device(%04d) has no \"IoacHwCapability\" key\n\n", vDriverKey[i]);
                        rcc = RegCloseKey(hKeyS1);
                        continue;
                    }

                    dwLanType = CONNECTION_WIRELESS;
                }
                //Find a IOAC adapter
                dwLen   = MAX_LEN;
                retCode = RegQueryValueEx(hKeyS1, L"ComponentId", NULL, NULL, (LPBYTE)&szComponentId, &dwLen);

                /*
                 * in order to deal with 2 identical net adatper's issue.
                 * We need "RedundancyCheck".
                 */
                DWORD  dwRedRet = FALSE;
                dwRedRet = RedundancyCheck(szNetCfgInstanceId);

                if (retCode == ERROR_SUCCESS && dwRedRet == FALSE)
                {
                    dwLen   = sizeof(DWORD);
                    retCode = RegQueryValueEx(hKeyS1, L"PnPCapabilities", NULL, NULL,
                                                (LPBYTE)&dwPnPCapabilities, &dwLen);

                    if (retCode != ERROR_SUCCESS)   dwPnPCapabilities = 0xff;

                    //if (dwLanType == LAN)
                    G[dwNetAdapterNum].szCsu = GetConnectionStatus(szNetCfgInstanceId);

                    GetUsersTargetGuid(szNetCfgInstanceId,G[dwNetAdapterNum].szGuid);

                    //printf("G[dwNetAdapterNum].szGuid = %s \n",G[dwNetAdapterNum].szGuid);
                    //KdPrint("szDriverPath...... = %ws\n",szDriverPath);
                    //KdPrint("szNetCfgInstanceId = %ws\n",szNetCfgInstanceId);
                    //KdPrint("szGuid............ = %s \n",G[dwNetAdapterNum].szGuid);
                    //KdPrint("szDeviceInstanceID = %ws\n",szDeviceInstanceID);
                    //KdPrint("szComponentId..... = %w \n",szComponentId);
                    //KdPrint("dwPnPCapabilities. = %x \n",dwPnPCapabilities);
                    //KdPrint("dwLanType.........   = %x \n",dwLanType);
                    //KdPrint("dwOEMType.........   = %x \n",dwOEMType);
                    //KdPrint("bMagicPacketFmt... = 0x%x\n\n", bMagicPacketFmt);

                    G[dwNetAdapterNum].szOemType = dwOEMType;
                    G[dwNetAdapterNum].szNetType = dwLanType;
                    G[dwNetAdapterNum].dwPnpCap  = dwPnPCapabilities;
                    G[dwNetAdapterNum].scCcs.nPacketFmt = bMagicPacketFmt;
                    memcpy(G[dwNetAdapterNum].szDriverPath, szDriverPath, wcslen(szDriverPath) * sizeof (WCHAR));
                    memcpy(G[dwNetAdapterNum].szNetCfgInstanceId, szNetCfgInstanceId, wcslen(szNetCfgInstanceId) * sizeof (WCHAR));
                    memcpy(G[dwNetAdapterNum].szDeviceInstanceID, szDeviceInstanceID, wcslen(szDeviceInstanceID) * sizeof (WCHAR));
                    memcpy(G[dwNetAdapterNum].szComponentId,szComponentId, wcslen(L"PCI\\VEN_0000&DEV_0000") * sizeof (WCHAR));

                    dwNetAdapterNum++;

                     /* only for wifi adapter.
                    if (pIfInfo){
                        rcc = RegCloseKey(hKeyS1);
                        break;
                    }*/
                }
                //if(ERROR_SUCCESS != RegCloseKey(hKeyS1))
                rcc = RegCloseKey(hKeyS1);
        }
    }

    if(rcc != ERROR_SUCCESS){
        //KdPrint("<EnumerateSubKeys> # Close Key: hKeyS1 failure! Error Code = 0x%x (rc = 0x%x)\n",GetLastError(), rcc);
    }

exit:
    if(ERROR_SUCCESS != RegCloseKey(hKey)){
        //KdPrint("<EnumerateSubKeys> # Close Key: hKey failure! Error Code = 0x%x (rc = 0x%x)\n",GetLastError(), rcc);
    }

    return (dwTarget);
}

CUSTOMER_RETURN_CODE
dummyProgram(void)
{
#ifdef OUTPUT_DEBUG_LOG_FILE
    //KdPrint("<dummyProgram> Call DMProgram\n");
#endif
    return CUSTOMER_CONTROL_OK;
}

void OmeDllLoad(tNetAdpaterInfo * p)
{
    HINSTANCE hInstLibrary;

    //if ((p->szOemType == INTEL) || (p->szOemType == ATHEROS_LAN))
    //if (p->szOemType == INTEL)
    //  p->scCcs.nPacketFmt = MAGIC_PACKET_TYPE_ACER_SHORT;
    //else
    //  p->scCcs.nPacketFmt = MAGIC_PACKET_TYPE_EXTENDED;

    ////KdPrint("p->szNetType = %x \n",p->szNetType);
    switch(p->szOemType)
    {
    case REALTEK:
        //KdPrint("<OmeDllLoad> Load Realtek\n");
        break;
    case ATHEROS_WIFI:
        //KdPrint("<OmeDllLoad> Load Atheros WiFi\n");
        break;
    case INTEL:
        //KdPrint("<OmeDllLoad> Load Intel \n");
        break;
    case BROADCOM_WIFI:
        //KdPrint("<OmeDllLoad> Load Broadcom WiFi\n");
        break;
    case ATHEROS_LAN:
        //KdPrint("<OmeDllLoad> Load Atheros Lan\n");
        break;
    case BROADCOM_LAN:
        //KdPrint("<OmeDllLoad> Load Broadcom Lan\n");
        break;
    default:
        //KdPrint("<OmeDllLoad> Load other IOAC adapter's DLL\n");
        break;
    }

    //KdPrint("<OmeDllLoad> Magic Packet Format = 0x%x (Type = %d)\n", p->scCcs.nPacketFmt, p->szOemType);

    if(INTEL == p->szOemType){
        //p->scCcs.nPacketFmt = MAGIC_PACKET_TYPE_ACER_SHORT;
        hInstLibrary = LoadLibrary(TEXT("inteldll.dll")); // null
    }else{
        hInstLibrary = LoadLibrary(p->szDriverPath);
    }

    #ifndef OUTPUT_DEBUG_LOG_FILE
    if(hInstLibrary == NULL){
        printf("[Error] Load DLL file failure!\n");
        printf("[Error] Load DLL file failure!\n");
        printf("[Error] Load DLL file failure!\n\n");
        FreeLibrary(hInstLibrary);
        return;
    }else{
        printf("Load DLL file successfully!\n\n");
    }
    #else
    if(hInstLibrary == NULL){
        //KdPrint("<OmeDllLoad> Load DLL file failure!\n");
        //KdPrint("<OmeDllLoad> Load DLL file failure!\n");
        //KdPrint("<OmeDllLoad> Load DLL file failure!\n");
        FreeLibrary(hInstLibrary);
        return;
    }else{
        //KdPrint("<OmeDllLoad> Load DLL file successfully!\n\n");
    }

    #endif

    IIDFromString(p->szNetCfgInstanceId, &p->scCcs.nId);
    p->scCcs.beginProgram   = (pfbeginProgram)GetProcAddress(hInstLibrary, "beginProgram");
    p->scCcs.beginSession   = (pfbeginProgram)GetProcAddress(hInstLibrary, "beginSession");
    p->scCcs.closeProgram   = (pfbeginProgram)GetProcAddress(hInstLibrary, "closeProgram");
    p->scCcs.closeSession   = (pfbeginProgram)GetProcAddress(hInstLibrary, "closeSession");
    p->scCcs.CustomerControl= (pfCustomerControl)GetProcAddress(hInstLibrary, "CustomerControl");
    p->scCcs.nType          = (CONNECTION_TYPE)p->szNetType;

    if(!p->scCcs.beginProgram) p->scCcs.beginProgram = dummyProgram;
    if(!p->scCcs.beginSession) p->scCcs.beginSession = dummyProgram;
    if(!p->scCcs.closeProgram) p->scCcs.closeProgram = dummyProgram;
    if(!p->scCcs.closeSession) p->scCcs.closeSession = dummyProgram;
    if(!p->scCcs.CustomerControl) p->scCcs.CustomerControl = dummyCustomerControl;

    // Check which magic packe formate that this adapter supports
    printf("[Loader Debug] p->scCcs.nPacketFmt = 0x%x\n", p->scCcs.nPacketFmt);
    if(p->scCcs.nPacketFmt == 0){
        // New chip
        CUSTOMER_RETURN_CODE cRet;
        BYTE bOutBuffer = 0;
        DWORD br = 0;

        p->scCcs.beginProgram();
        p->scCcs.beginSession();

        printf("[Loader Debug] Call CustomerControl(IOCTL_CUSTOMER_WAKEPATTERNSUPPORT_QUERY, .....)\n");
        cRet = p->scCcs.CustomerControl(IOCTL_CUSTOMER_WAKEPATTERNSUPPORT_QUERY, NULL, 0, &bOutBuffer, sizeof(bOutBuffer), &br);
        if(CUSTOMER_CONTROL_OK == cRet){
            if(bOutBuffer != 0){
                p->scCcs.nPacketFmt = bOutBuffer;
                printf("IOCTL_CUSTOMER_WAKEPATTERNSUPPORT_QUERY API return value = 0x%x\n", bOutBuffer);
                printf(" - %s Acer extended magic packet\n", (bOutBuffer & MAGIC_PACKET_TYPE_EXTENDED)? "Support":"Not Support");
                printf(" - %s Acer short magic packet\n", (bOutBuffer & MAGIC_PACKET_TYPE_ACER_SHORT)? "Support":"Not Support");
            }else{
                printf("[Error] IOCTL_CUSTOMER_WAKEPATTERNSUPPORT_QUERY API return not support wake pattern!\n");
                printf("[Error] - bOutBuffer = %d\n", bOutBuffer);
            }
        }else{
            printf("[Error] Called IOCTL_CUSTOMER_WAKEPATTERNSUPPORT_QUERY API failed!\n");
        }

        if(p->scCcs.nPacketFmt == 0){
            // The default magic packet format is short magic packet
            p->scCcs.nPacketFmt = MAGIC_PACKET_TYPE_ACER_SHORT;
        }

        p->scCcs.closeSession();
        p->scCcs.closeProgram();
    }
}

CUSTOMER_RETURN_CODE
CallIoctl(tNetAdpaterInfo * p)
{

    char *msg;
    DWORD br, x;
    UINT32 mask, ret;
    int n, supported;
    CUSTOMER_RETURN_CODE rc;
    CUSTOMER_KEEPALIVE_SET ka;
    CUSTOMER_KEEPALIVE_QUERY kaq;
    CUSTOMER_WAKEUP_MATCH_SET match;
    NDIS630_PM_CAPABILITIES pm_caps;
    CUSTOMER_RETURN_CODE    crcRet = CUSTOMER_CONTROL_OK;
    supported = 1;

    // querying ndis capabilities;

    memset(&pm_caps, 0, sizeof(pm_caps));

    pm_caps.Header.Type = NDIS_HEADER_TYPE;
    pm_caps.Header.Revision = NDIS_PM_CAPABILITIES_REVISION;
    pm_caps.Header.Size = sizeof(pm_caps);


    br = 0;

    crcRet = p->scCcs.CustomerControl(IOCTL_CUSTOMER_NDIS_QUERY_PM_CAPABILITIES,
                                    &pm_caps, sizeof(pm_caps), &pm_caps, sizeof(pm_caps), &br);


//  if (crcRet == CUSTOMER_CONTROL_OK)  //KdPrint("<CallIoctl> CustomerControl call successfully \n");
    return crcRet;
}

int EnumIoacWlanAdapter()
{
    // Declare and initialize variables.
    HANDLE                      hClient         = NULL;
    DWORD                       dwCurVersion    = 0;
    DWORD                       dwResult        = 0;
    DWORD                       err             = 0;
    WLAN_INTERFACE_INFO_LIST    *pIfList;
    WLAN_INTERFACE_INFO         *pIfInfo;
    WLAN_INTERFACE_INFO         *ifinfo;
    WLAN_INTERFACE_INFO_LIST    *iflist;
    int inum;
    int i;
    int nif;
    WLAN_OPCODE_VALUE_TYPE      opcode;
    WLAN_CONNECTION_ATTRIBUTES  *cinfo;
    DWORD                       cinfosz;

    dwResult = WlanOpenHandle(WLAN_API_VERSION_2_0, NULL, &dwCurVersion, &hClient);
    if(dwResult != ERROR_SUCCESS){

        //KdPrint("<EnumIoacWlanAdapter> WlanOpenHandle function failed with error\n");

        return 0;
    }

    dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
    if(dwResult != ERROR_SUCCESS){

        //KdPrint("<EnumIoacWlanAdapter> WlanEnumInterfaces function failed with error\n");

        return 0;
    }

    inum = pIfList->dwNumberOfItems;
    nif  = 0;
    for(i = 0 ; i < inum ; i++)
    {

        pIfInfo = pIfList->InterfaceInfo + i;

        if(IsAthJupiter(pIfInfo->strInterfaceDescription) ||  IsBcmJupiter(pIfInfo->strInterfaceDescription))
        {
            //Wifi_module_vendor = ATHEROS_WIFI_DLL;
            WCHAR szGuid[MAX_LEN] = {0};

            // after EnumerateSubKeys() dwNetAdapterNum will plus one a
            ptNetAdapterInfo p    = (ptNetAdapterInfo)&G[dwNetAdapterNum];

            //printf("atheros(test case is broadcom) card found\n");
            GuidToString(&pIfInfo->InterfaceGuid, szGuid);
            EnumerateSubKeys(   HKEY_LOCAL_MACHINE,L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}", szGuid, pIfInfo);

            opcode  = wlan_opcode_value_type_invalid;

            err     = WlanQueryInterface(   hClient,
                                            &pIfInfo->InterfaceGuid,
                                            wlan_intf_opcode_current_connection,
                                            NULL,
                                            &cinfosz,
                                            (PVOID *)&cinfo,
                                            &opcode);

            if(err != ERROR_SUCCESS) {
                //printf("Wlan QueryInterface fail \n");
                p->szCsu = 0;
                break;
            }

            if (cinfo->isState == wlan_interface_state_connected)   p->szCsu = 1;

            //printf("sec -- > %s  \n",WlanSecurityType[cinfo->wlanSecurityAttributes.dot11AuthAlgorithm]);
            sprintf(p->szWse,"%s",WlanSecurityType[cinfo->wlanSecurityAttributes.dot11AuthAlgorithm]);
            continue;
        }
        nif++;
    }

    if (pIfList != NULL) {
        WlanFreeMemory(pIfList);
        pIfList = NULL;
    }

    return 0;
}

int
EnumAllIoacAdapter()
{
    int  ret;
    // for realtek/intel Lan adapter enum
    ret = EnumerateSubKeys( HKEY_LOCAL_MACHINE,L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}", 0, NULL);
    return ret;
}


void CreateNetAdapterInstance()
{
    CUSTOMER_RETURN_CODE    crRet = CUSTOMER_CONTROL_OK;
    tNetAdpaterInfo *       p;
    DWORD                   i = 0;

    for ( p = &G[0], i = 0 ; i < dwNetAdapterNum; p++, i++ )
    {
        OmeDllLoad(p);
        /*
        p->scCcs.beginProgram();
        p->scCcs.beginSession();
        crRet = CallIoctl(p);
        p->scCcs.closeSession();
        p->scCcs.closeProgram();

        if (crRet == CUSTOMER_CONTROL_OK)       p->szAsp = 1;
        else                                    p->szAsp = 0;
        */
        //setting power configureation
    }
}

int EnumIoacNetAdapter(char *  szGuidArray[] )
{
    int index, i;

    #ifdef OUTPUT_DEBUG_LOG_FILE
    if(!CreateLogFile()){
        return 0;
    }
    //KdPrint("<EnumIoacNetAdapter> Loader start: Call EnumIoacNetAdapter() --------------\n");
    #endif

    // Assume the platform in AC mode, and its BIOS support IOAC feature (Because to delete the checking by WMI)
    for(int index = 0; index < 10 ; ++index){
        G[index].szAc       = 1;    // 1 = The current power state is AC mode
        G[index].dwGSmBios  = 0x3;  // 0x3 = Always connect
    }
    /*{
        CWMI DetectSmBiosInfo;
        DetectSmBiosInfo.Diagnose();
    }*/

    if ( szGuidArray == NULL)
    {
        GetAllExistNetworkDriverKeyNum();
        //EnumIoacWlanAdapter();
        //EnumIoacLanAdapter();
        EnumAllIoacAdapter();
        CreateNetAdapterInstance();
    }
    else
    {
    for (i = 0 ; i < dwNetAdapterNum ; i++)
        szGuidArray[i]  = G[i].szGuid;
    }

    //KdPrint("<EnumIoacNetAdapter> EnumIoacNetAdapter() END (Findout %d IOAC adapter)\n\n\n", dwNetAdapterNum);

    return (dwNetAdapterNum);
}

GUID* EnumIOACNIC(int *nicCount)
{
    HKEY kKey = NULL, kKey2 = NULL;
    //char path[255];
    //wchar_t wpath[512];
    TCHAR tpath[512] = {};
    DWORD dwRet = 0;
    DWORD cbData = 0;
    DWORD idx = 0;
    LSTATUS st = 0;
    LPTSTR szIoacHwCapability = NULL;
    LPTSTR szNetGUID = NULL;
    HRESULT hr = ERROR_SUCCESS;
    GUID* IoacHwGUID = NULL;
    GUID* TempGUID = NULL;
    DWORD IoacHwIndex = 0;
    int iret;
    WCHAR guid[40];




    // Initialize nicCount
    *nicCount = 0;

    // Open the network adapter device registry
    //strcpy(path, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}");
    //mbstowcs(wpath, path, 255);
    StringCchCopy(tpath, _countof(tpath), _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"));

    // Can't open the network adapter device registry, return
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, tpath, 0, KEY_READ, &kKey) != ERROR_SUCCESS)
    {
        *nicCount = 0;
        return 0;
    }

    // Go through to see total count of IoacHwCapability NIC
    dwRet = -1;
    do {
        cbData = 255;
        dwRet = RegEnumKeyEx(kKey, idx++, tpath, &cbData, 0, 0, 0, 0);

        if (dwRet != ERROR_SUCCESS)
            break;

        if (RegOpenKeyEx(kKey, tpath, 0, KEY_READ, &kKey2) != ERROR_SUCCESS)
            continue;

        hr = RegGetString(kKey2, TEXT("IoacHwCapability"), &szIoacHwCapability);
        //hr = RegGetString(kKey2, TEXT("NetCfgInstanceId"), &szIoacHwCapability);

        // The NIC is IoacHwCapable
        if(!FAILED(hr))
        {
            (*nicCount)++;
        }

        // Free szIocaHwCapability
        if (szIoacHwCapability != NULL)
        {
            free(szIoacHwCapability);
            szIoacHwCapability = NULL;
        }
    } while(dwRet == ERROR_SUCCESS);

    //if(GetWlanInterfacesInfo())
        //printf("No atheros card found\n");
        //(*nicCount)++;
    TempGUID = GetWlanInterfacesInfo();
    //iret = StringFromGUID2((const GUID &)TempGUID, (LPOLESTR)guid, 39);
    if(TempGUID != NULL)
        (*nicCount)++;



    // Allocate the IoacHwGUID
    IoacHwGUID = (GUID*)malloc(sizeof(GUID) * (*nicCount));
    ZeroMemory(IoacHwGUID, sizeof(GUID) * (*nicCount));

    // Store IoacHwCapability NIC
    idx = 0;
    dwRet = -1;
    do {
        cbData = 255;

        // RegEnumKeyEx for all network adapeter device in subkey
        dwRet = RegEnumKeyEx(kKey, idx++, tpath, &cbData, 0, 0, 0, 0);

        if (dwRet != ERROR_SUCCESS)
            break;

        if (RegOpenKeyEx(kKey, tpath, 0, KEY_READ, &kKey2) != ERROR_SUCCESS)
            continue;

        hr = RegGetString(kKey2, TEXT("IoacHwCapability"), &szIoacHwCapability);
        if (FAILED(hr))
            continue;

        hr = RegGetString(kKey2, TEXT("NetCfgInstanceId"), &szNetGUID);
        if (FAILED(hr))
            continue;

        if (IIDFromString(szNetGUID, &IoacHwGUID[IoacHwIndex++]) == S_OK)
        {
            ; // Success
        }

        if (szNetGUID != NULL)
        {
            free(szNetGUID);
            szNetGUID = NULL;
        }

        if (szIoacHwCapability != NULL)
        {
            free(szIoacHwCapability);
            szIoacHwCapability = NULL;
        }

    } while(dwRet == ERROR_SUCCESS);

    if (kKey2 != NULL)
    {
        RegCloseKey(kKey2);
        kKey2 = NULL;
    }

    if (kKey != NULL)
    {
        RegCloseKey(kKey);
        kKey = NULL;
    }

    if(TempGUID != NULL)
        IoacHwGUID[IoacHwIndex++] = *TempGUID;
    //printf("test\n");


    return IoacHwGUID;
}


PCUSTOMER_CONTROL_SET LoadCustCtlSet(const char  * szTarGuid)
{
    int i;
    int r;

    for (i = 0 ; i < dwNetAdapterNum ; i++){
        r = strcmp(szTarGuid,G[i].szGuid);
        if (szTarGuid == G[i].szGuid || r == 0){
            return &G[i].scCcs;
        }
    }
    return NULL;
}


// Retrun driver key number ------------------------------------------------------------------------
unsigned int GetDriverKeyNum(TCHAR* strBuf, unsigned int strLeng){
    int iRet = 0;
    WCHAR wkey[4] = {0};

    wkey[3] = strBuf[strLeng - 1];
    wkey[2] = strBuf[strLeng - 2];
    wkey[1] = strBuf[strLeng - 3];
    wkey[0] = strBuf[strLeng - 4];

    iRet = _wtoi(wkey);
    if( iRet < 0 ) iRet = 0;

    return iRet;
}


// Check this Device node (DevNode) is a newtork adapter that supports IOAC ------------------------
BOOLEAN IsPossibleNetworkDevice(DEVNODE DevNode, HMACHINE hMachine, unsigned int &key)
{
    TCHAR   strType;
    //TCHAR *strValue;
    //LPTSTR    Buffer;
    TCHAR Buffer[MAX_DEVICE_ID_LEN] = {0};
    ULONG BufferLen = MAX_DEVICE_ID_LEN * sizeof(TCHAR);
    TCHAR keyBuf[MAX_PROFILE_LEN] = {0};
    ULONG keyBufLen = MAX_PROFILE_LEN * sizeof(TCHAR);
    TCHAR mfcBuf[MAX_PROFILE_LEN] = {0};
    ULONG mfcBufLen = MAX_PROFILE_LEN * sizeof(TCHAR);

    DWORD errCode = 0;

    //int  BufferSize = MAX_PATH + MAX_DEVICE_ID_LEN;
    //ULONG  BufferLen = BufferSize * sizeof(TCHAR);
    //strValue = (TCHAR*) malloc(BufferSize);
    //Buffer  = strValue;

    errCode = CM_Get_DevNode_Registry_Property_Ex(DevNode, CM_DRP_DEVICEDESC, NULL, Buffer, &BufferLen, 0, hMachine);
    if(CR_SUCCESS != errCode){
#ifdef SHOW_GET_EXIST_NET_DRIVERS_PROCESS
    //KdPrint("Get Device Desc failure (node = %d): 0x%x\n", DevNode, errCode);
#endif
        return FALSE;
    }

#ifdef SHOW_GET_EXIST_NET_DRIVERS_PROCESS
    //KdPrint("Device Desc = %ws\n", Buffer);
#endif
    // if Vendor = "Microsoft" or "Deterministic Networks"
    errCode = CM_Get_DevNode_Registry_Property_Ex(DevNode, CM_DRP_MFG , NULL, mfcBuf, &mfcBufLen, 0, hMachine);
    if (CR_SUCCESS == errCode ){
        TCHAR mfcMicrosoft[] = L"Microsoft";
        TCHAR mfcDeterministic[] = L"Deterministic Networks";

#ifdef SHOW_GET_EXIST_NET_DRIVERS_PROCESS
        //KdPrint(" - Vendor = %ws\n",  mfcBuf);
        //KdPrint("\n");
#endif

        if((0 == wcsncmp(mfcMicrosoft,mfcBuf, wcslen(mfcBuf))) || (0 == wcsncmp(mfcDeterministic, mfcBuf, wcslen(mfcBuf))))
            return TRUE;

    }else{
#ifdef SHOW_GET_EXIST_NET_DRIVERS_PROCESS
        //KdPrint(" - Get Vendor failure: 0x%x\n", errCode);
#endif
        return FALSE;
    }

    errCode = CM_Get_DevNode_Registry_Property_Ex(DevNode, CM_DRP_DRIVER, NULL, keyBuf, &keyBufLen, 0, hMachine);
    if(CR_SUCCESS == errCode){
        key = GetDriverKeyNum(keyBuf, wcslen(keyBuf));

#ifdef SHOW_GET_EXIST_NET_DRIVERS_PROCESS
        //KdPrint(" - Driver Path = %ws\n",  keyBuf);
        //KdPrint("   => Key Num = %d\n", key);
#endif

    }else{
#ifdef SHOW_GET_EXIST_NET_DRIVERS_PROCESS
        //KdPrint(" - Get Driver Path failure : 0x%x\n", errCode);
#endif
        return FALSE;
    }

    //free(strValue);
    return TRUE;
}
// -------------------------------------------------------------------------------------------------


// Use Recursion way to find all device note -----------------------------------------------------------
BOOL RetrieveSubNodes(DEVINST parent, DEVINST sibling, DEVNODE dn,
     PSP_CLASSIMAGELIST_DATA pImageListData,HMACHINE hMachine)
{
    BOOL bRet = FALSE;
    DEVNODE dnSibling, dnChild;

    do
    {
        //tmp_record_count++; /////////////////////////////////
        CONFIGRET CrRet = CM_Get_Sibling_Ex(&dnSibling, dn, 0, hMachine);

        if (CR_SUCCESS != CrRet)
            dnSibling = NULL;
        //printf(" dn = %d (dnSibling = %d)\n", dn, dnSibling);

        TCHAR GuidString[MAX_GUID_STRING_LEN] = {0};
        ULONG Size = sizeof(GuidString);

        CrRet = CM_Get_DevNode_Registry_Property_Ex(dn, CM_DRP_CLASSGUID, NULL,GuidString, &Size, 0, hMachine);

        if(CR_SUCCESS == CrRet){
            GUID Guid;
            int Index = 0;
            ::CLSIDFromString(GuidString, &Guid);

            if (SetupDiGetClassImageIndex(pImageListData, &Guid, &Index)){
                TCHAR NetworkDeviceClassGuid[] = L"{4d36e972-e325-11ce-bfc1-08002be10318}";

                if(0 == _tcsicmp(NetworkDeviceClassGuid,GuidString)){
                    //CheckIsNetworkDevice(dn,hMachine);
                    unsigned int dkey_num = 0;
                    if(IsPossibleNetworkDevice(dn,hMachine, dkey_num)){
                        vDriverKey.push_back(dkey_num);
                    }
                }
            }
        }

        CrRet  = CM_Get_Child_Ex(&dnChild, dn, 0, hMachine);
        if (CR_SUCCESS == CrRet){
            bRet = RetrieveSubNodes(dn, NULL, dnChild,pImageListData,hMachine);
            if(bRet) break;
        }

        dn = dnSibling;
    }while(NULL != dn);

    return bRet;
}
//------------------------------------------------------------------------------------


//  Get All Network Device Status ----------------------------------------------------
BOOL GetAllExistNetworkDriverKeyNum()
{
    BOOL bRet = FALSE;
    const int iShiftNum = 2;
    TCHAR strComputerName[MAX_PATH] = {0};
    DWORD dwSize = MAX_PATH - iShiftNum;
    GetComputerName(strComputerName+iShiftNum, &dwSize);
    strComputerName[0] = _T('\\');
    strComputerName[1] = _T('\\');

    CONFIGRET CrRet;
    HMACHINE hMachine;

    int ti = 0;

    CrRet = CM_Connect_Machine(strComputerName, &hMachine);

    if (CR_SUCCESS != CrRet){
        //KdPrint("<GetAllExistNetworkDriverKeyNum> Machine Connection failed, CrRet= < 0X%X > \n", CrRet);
        return bRet;
    }

    //Set Image List
    SP_CLASSIMAGELIST_DATA ImageListData;
    ImageListData.cbSize = sizeof(ImageListData);
    SetupDiGetClassImageList(&ImageListData);

    DEVNODE dnRoot;
    CM_Locate_DevNode_Ex(&dnRoot, NULL, 0, hMachine);

    DEVNODE dnFirst;
    CM_Get_Child_Ex(&dnFirst, dnRoot, 0, hMachine);

    bRet = RetrieveSubNodes(dnRoot, NULL, dnFirst,&ImageListData, hMachine);

    SetupDiDestroyClassImageList(&ImageListData);

    //KdPrint("<GetAllExistNetworkDriverKeyNum> Find %d possible Network driver in device manager!\n\n", vDriverKey.size());
    //for( ti = 0; ti < vDriverKey.size(); ti++)    printf(" index %d = %d\n", ti, vDriverKey[ti]);

    ////KdPrint("# Call RetrieveSubnodes count = %d\n",  tmp_record_count);/////////////
    CM_Disconnect_Machine(hMachine);
    return bRet;
}
// --------------------------------------------------------------------------------------
