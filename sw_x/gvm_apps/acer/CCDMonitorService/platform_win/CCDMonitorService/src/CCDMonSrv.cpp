//  Service.cpp : Defines the entry point for the console application.
//
#pragma once

#include "CCDMonSrv.h"
#include "log.h"
#include "ccdi.hpp"
#include "ccdi_client.hpp"
#include "CCDMonSrv_Defs.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "utils_ProtobufFileStream.h"
#include "utils_strings.h"

#include <Windows.h>
#include <process.h>
#include <sstream> 
#include <tchar.h>
#include <userenv.h>
#include <Tlhelp32.h>
#include <Sddl.h>
#include <ImageHlp.h>
#pragma comment(lib, "imageHlp.lib")
#include <Wtsapi32.h>
#include <Shlobj.h>
#include <WinSock.h>
#include <io.h>
#pragma comment(lib,"UserEnv.lib")
#pragma comment(lib,"Wtsapi32.lib")

#include <list>
#include <string>

#include "protobuf_file_reader.hpp"
#include "protobuf_file_writer.hpp"

#include "RegKey.h"

#define DEFAULT_PIPE_LEN 1024
#define DEFAULT_PIPE_TO_MS 100000
#define _CCDI_SUPPORT_CLOSE_GRACEFULLY

#include <Psapi.h>
#pragma comment(lib,"Psapi.lib")
bool isCCDExist();
#undef ADD_CCD_TO_WINDOWS_DEFENDER_EXCLUSION_LIST

using namespace std;

// MD_Tsai@acer.com.tw, 2012/07/05, Modify for Acer Cloud Node One Click setup, get one profile from specific network interface GUID
#include <wlanapi.h>

// Need to link with Wlanapi.lib and Ole32.lib
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
//

#define _trustees_enable

/// history of login user info
typedef struct USERINFO_STRUCT {
    TCHAR USERNAME[MAX_PATH];
    TCHAR ID[MAX_PATH];
    TCHAR LOCALPATH[MAX_PATH];
    PROCESS_INFORMATION PI;
    HANDLE STOP_WATCHDOG_EVENT;
    HANDLE hWatchdogThread;
    int    INST_NUM;
#ifdef _trustees_enable
    CCDMonSrv::REQINPUT req;
#endif
} USERINFO;
std::list<USERINFO> *Users;

typedef struct WATCHDOG_ARGS_STRUST {
    TCHAR* cmdline;
    TCHAR* appdatapath;
    TCHAR* workingdir;
    TCHAR sid[MAX_PATH];
    PROCESS_INFORMATION pi;
    HANDLE STOP_WATCHDOG_EVENT;
} WATCHDOG_ARGS;

#pragma region Function
/// Window Service
void ServiceMainProc();
void Install(TCHAR* pPath, TCHAR* pName);
void UnInstall(TCHAR* pName);
BOOL KillService(TCHAR* pName);
BOOL RunService(TCHAR* pName);
void ExecuteSubProcess();
unsigned __stdcall AppLayerThread(void *);
BOOL StartProcess();
BOOL StartCCDWithWatchdog(const LPTSTR lpCommandLine, const LPTSTR lpSid, HANDLE stopWdEvent, PROCESS_INFORMATION& pi, HANDLE& hWatchdogThread);
void stopCcdWatchDogThread(std::list<USERINFO>::iterator it);
BOOL LaunchProcess(const LPTSTR lpCmdline, const LPTSTR lpCWD, PROCESS_INFORMATION& pi, BOOL inheritable=FALSE);

void EndProcess();
void EndAllProcess();
static DWORD serviceStart();

void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
//void WINAPI ServiceHandler(DWORD fdwControl);
DWORD WINAPI ServiceHandler(DWORD fdwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
void CloseWindowManager();

/// operations of login user info
void users_Init();
void users_Load();
TCHAR* user_Load_UserName (TCHAR *appdatapath);
bool users_Add(TCHAR *sid, TCHAR *username, TCHAR *appdatapath, int instancenum);
bool users_UpdateProcess(TCHAR *sid, PROCESS_INFORMATION pi, int instancenum=0);
std::list<USERINFO>::iterator users_Remove(TCHAR *sid, int instancenum);
void users_SaveBack();

#ifdef _trustees_enable
static LPVOID CreateStartupParams(HANDLE *phMapping, size_t maxSize);
int users_loadTrustees(const char* sid, CCDMonSrv::REQINPUT& input);
void users_saveTrustees(CCDMonSrv::REQINPUT& input);
#endif

/// operation of handling named pipes
static DWORD handleRWNamedPipe();

/// routine of the thread for ccd watchdog
UINT __stdcall ccdWatchDogThread(void* arg);

std::string getCcdDataPath();

#pragma endregion

#pragma region Fields
/// varables for service
const int nBufferSize = MAX_PATH;
TCHAR pServiceName[nBufferSize+1];
TCHAR lpCmdLineData[nBufferSize+1];
BOOL ProcessStarted = TRUE;
HANDLE hStopCCDRelaunch = NULL;

SERVICE_STATUS_HANDLE hServiceStatusHandle; 
SERVICE_STATUS ServiceStatus; 
PROCESS_INFORMATION pProcInfo[MAX_NUM_OF_PROCESS];

SERVICE_TABLE_ENTRY lpServiceStartTable[] = 
{
    {pServiceName, ServiceMain},
    {NULL, NULL}
};

/// variables for threads
const char *pipeName= "\\\\.\\pipe\\com.acer.ccdm.sock.server";
static CRITICAL_SECTION g_CS;
static HANDLE hSrvPipe;
static bool bShutdown=false;

/// variables for launching ccd
static TCHAR ProcessDir[MAX_PATH+1]      = {0};
static TCHAR ProcessName[MAX_PATH+1]     = {0};
static TCHAR IOACUtilityPath[MAX_PATH+1] = {0};
static TCHAR iniPath[MAX_PATH+1]         = {0};
static TCHAR iniPath_tmp[MAX_PATH+1]     = {0};
char userTrusteesPath[MAX_PATH+1];

#pragma endregion

static FileStream *_fs;
DWORD createPipe();
DWORD writeMessage(CCDMonSrv::REQOUTPUT& input);
DWORD readMessage(CCDMonSrv::REQINPUT& output);
void releasePipe();

#include <Shellapi.h>

#ifdef ADD_CCD_TO_WINDOWS_DEFENDER_EXCLUSION_LIST
LONG addExcludeList2WindowsDefenderRegistry(TCHAR *ccdPath, TCHAR *logPath);
#endif

int getUserProfileImagePath(TCHAR* osUserId, TCHAR* profileImagePath);


static
int waitCcd()
{
    LOG_INFO("Waiting for CCD to be ready...\n");

    int total_to=10000, intv_to=100;
    int incr_to=0;

    ccd::GetSystemStateInput request;
    request.set_get_players(true);
    ccd::GetSystemStateOutput response;
    int rv;
    while(1) {
        rv = CCDIGetSystemState(request, response);
        if (rv == CCD_OK) {
            LOG_INFO("CCD is ready!\n");
            break;
        }

        incr_to += intv_to;
        if (incr_to > total_to) {
            LOG_INFO("Timed out after %d ms\n", total_to);
            break;
        }

        LOG_INFO("Still waiting for CCD to be ready...\n");
        Sleep(intv_to);
    }

    return (rv == 0) ? 0 : -1;
}

static
bool getProcessPath(TCHAR* processDir, TCHAR* processName)
{
    /// build absolute path of ccd
    bool bRt = false;
    HKEY hkResult;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE", 0, KEY_READ | KEY_WOW64_32KEY, &hkResult)) {
        DWORD pcbData = MAX_PATH;
        if (ERROR_SUCCESS == RegGetValue(hkResult, L"OEM\\AcerCloud", L"InstallPath", RRF_RT_REG_SZ, NULL, processDir, &pcbData)) {
            /// build absolute path for ccd.exe and EnableWakeUpOption utility
            if (processName != NULL) {
                _sntprintf(processName, (_tcslen(processDir)+_tcslen(_T("%sccd.exe")))*sizeof(TCHAR), _T("%sccd.exe"), processDir);
            }
            _sntprintf(IOACUtilityPath, sizeof(IOACUtilityPath), _T("%sEnableWakeUpOption.exe"), processDir);
            bRt = true;
            char* ptr = wchr2chr(processDir);
            LOG_INFO("InstallDir (Registry): %s",  ptr);
            free(ptr);
        }
    }
    else {
        LOG_ERROR("Get install path failed: %u", GetLastError());
    }
    if (!bRt) {
        if (!::SHGetSpecialFolderPath(NULL, processDir, CSIDL_PROGRAM_FILESX86, FALSE)) {
            LOG_ERROR("Failed to get CSIDL_PROGRAM_FILESX86 path");
            goto end;
        }
        /// build absolute path for ccd.exe and EnableWakeUpOption utility
        if (processName != NULL) {
            _sntprintf(processName, (_tcslen(processDir)+_tcslen(_T("%s\\Acer\\Acer Cloud\\ccd.exe")))*sizeof(TCHAR), _T("%s\\Acer\\Acer Cloud\\ccd.exe"), processDir);
        }
        _sntprintf(IOACUtilityPath, sizeof(IOACUtilityPath), _T("%s\\Acer\\Acer Cloud\\EnableWakeUpOption.exe"), processDir);
        _tcscat(processDir, _T("\\"));
        char* ptr = wchr2chr(processDir);
        LOG_INFO("InstallDir (Registry): %s",  ptr);
        free(ptr);
    }

end:
    return bRt;
}

int _tmain(int argc, _TCHAR* argv[])
{
    char logPath[MAX_PATH+1]={0};
    if (::SHGetSpecialFolderPathA(NULL, logPath, CSIDL_COMMON_APPDATA, FALSE)) {
    strcat_s(logPath, sizeof(logPath), "\\acer\\CCDMSrv");
        LOGInit("CCDMonitorService", logPath);
    }

    memset(userTrusteesPath, 0, MAX_PATH+1);
    _snprintf(userTrusteesPath, MAX_PATH, "%s\\Users", logPath);
    LOG_INFO("userTrusteesPath: %s", userTrusteesPath);
    if (!CreateDirectoryA(userTrusteesPath, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) {
            LOG_ERROR("create %s failed: %u", userTrusteesPath, err);
            goto end;
        }
    }

    LOGSetWriteToFile(VPL_TRUE);
    LOG_INFO("CCDMonitorService launch ...");

    InitializeCriticalSection(&g_CS);

    hStopCCDRelaunch = ::CreateEvent(NULL, TRUE, TRUE, L"CCDMonitorService_CCDRelaunchFlag");
    DWORD rv = GetLastError();
    if (hStopCCDRelaunch == NULL) {
        LOG_ERROR("Create stop ccd event failed, error code: %lu", rv);
    }
    else if (rv == ERROR_ALREADY_EXISTS) {
        LOG_WARN("Create stop ccd event, already exists!");
    }
    ::ResetEvent(hStopCCDRelaunch);
    
    /// build absolute path of ccd
    bool bRt = getProcessPath(ProcessDir, ProcessName);
    if (!bRt) goto end;

    if(argc >= 2) {
        _sntprintf(lpCmdLineData, sizeof(lpCmdLineData), argv[1]);
    }
    ServiceMainProc();

end:
    ::SetEvent(hStopCCDRelaunch);
    CloseHandle(hStopCCDRelaunch);
    return 0;
}

void ServiceMainProc()
{
    /// initialize variables for .exe and .log file names
    TCHAR pModuleFile[nBufferSize+1];
    TCHAR pExeFile[nBufferSize+1];
    DWORD dwSize = GetModuleFileName(NULL, pModuleFile, nBufferSize);

    pModuleFile[dwSize] = 0;
    if(dwSize>4 && pModuleFile[dwSize-4] == '.') {
        _sntprintf(pExeFile, sizeof(pExeFile), _T("%s"), pModuleFile);
    }
    _sntprintf(pServiceName, (size_t)sizeof(pServiceName), _T("CCDMonitorService"));

    if(_tcsicmp(_T("-i"),lpCmdLineData) == 0)
        Install(pExeFile, pServiceName);
    else if(_tcsicmp(_T("-k"),lpCmdLineData) == 0)
        KillService(pServiceName);
    else if(_tcsicmp(_T("-u"),lpCmdLineData) == 0)
        UnInstall(pServiceName);
    else if(_tcsicmp(_T("-s"),lpCmdLineData) == 0)
        RunService(pServiceName);
    else if(_tcsicmp(_T("-e"),lpCmdLineData) == 0) {
        HANDLE hThread;
        unsigned dwThreadID;
        hThread = (HANDLE)_beginthreadex(NULL,
                                         0,
                                         AppLayerThread,
                                         NULL,
                                         0,
                                         &dwThreadID);
        if(hThread == NULL) {
            DWORD nError = GetLastError();
            LOG_ERROR("StartService failed, error code = %lu", nError);
        }
        else {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
            hThread = NULL;
            google::protobuf::ShutdownProtobufLibrary();
            delete Users;
        }
    }
    else
        ExecuteSubProcess();

}

void Install(TCHAR* pPath, TCHAR* pName)
{  
    SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE); 
    if (schSCManager==0) 
    {
        DWORD nError = GetLastError();
        LOG_ERROR("OpenSCManager failed, error code = %lu", nError);
    }
    else
    {
        SC_HANDLE schService = CreateService
        (
            schSCManager,     /* SCManager database      */ 
            pName,            /* name of service         */ 
            pName,            /* service name to display */ 
            SERVICE_ALL_ACCESS,        /* desired access          */ 
            SERVICE_WIN32_OWN_PROCESS, /* |SERVICE_INTERACTIVE_PROCESS Bug: 12706 No interactive is allowed and Windows complains about this! */  /* service type            */ 
            SERVICE_AUTO_START,        /* start type              */ 
            SERVICE_ERROR_NORMAL,      /* error control type      */ 
            pPath,            /* service's binary        */ 
            NULL,             /* no load ordering group  */ 
            NULL,             /* no tag identifier       */ 
            NULL,             /* no dependencies         */ 
            NULL,             /* LocalSystem account     */ 
            NULL
        );                    /* no password             */ 
        if (schService==0) 
        {
            DWORD nError =  GetLastError();
            LOG_ERROR("Failed to create service, error code = %lu", nError);
        }
        else
        {
            LOG_INFO("Service installed");
            CloseServiceHandle(schService); 
        }
        CloseServiceHandle(schSCManager);

    }

    LOG_INFO("Service install complete");
}

void UnInstall(TCHAR* pName)
{
    SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
    if (schSCManager==0) 
    {
        DWORD nError = GetLastError();
        LOG_ERROR("OpenSCManager failed, error code = %lu", nError);
    }
    else
    {
        SC_HANDLE schService = OpenService( schSCManager, pName, SERVICE_ALL_ACCESS);
        if (schService==0) 
        {
            DWORD nError = GetLastError();
            LOG_ERROR("OpenService failed, error code = %lu", nError);
        }
        else {
            if(!DeleteService(schService)) {
                LOG_ERROR("Failed to delete service");
            }
            else {
                LOG_INFO("Service removed");
                users_Init();
                users_Load();
                EndProcess();
            }
            CloseServiceHandle(schService);
        }
        CloseServiceHandle(schSCManager);    
    }
}

BOOL KillService(TCHAR* pName) 
{
    /// kill service with given name
    SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
    if (schSCManager==0) 
    {
        DWORD nError = GetLastError();
        LOG_ERROR("OpenSCManager failed, error code = %lu", nError);
    }
    else
    {
        /// open the service
        SC_HANDLE schService = OpenService( schSCManager, pName, SERVICE_ALL_ACCESS);
        if (schService==0) 
        {
            DWORD nError = GetLastError();
            LOG_ERROR("OpenService failed, error code = %lu", nError);
        }
        else
        {
            /// call ControlService to kill the given service
            SERVICE_STATUS status;
            if(ControlService(schService,SERVICE_CONTROL_STOP,&status))
            {
                LOG_INFO("ControlService Stop");
                CloseServiceHandle(schService); 
                CloseServiceHandle(schSCManager);
                users_Init();
                users_Load();
                EndProcess();
                return TRUE;
            }
            else
            {
                DWORD nError = GetLastError();
                if (nError == ERROR_SERVICE_DOES_NOT_EXIST
                  || nError == ERROR_SERVICE_NOT_ACTIVE) {
                    LOG_WARN("ControlService failed, error code = %lu", nError);
                } else {
                    LOG_ERROR("ControlService failed, error code = %lu", nError);
                }
            }
            CloseServiceHandle(schService); 
        }
        CloseServiceHandle(schSCManager); 
    }
    return FALSE;
}

BOOL RunService(TCHAR* pName) 
{
    SERVICE_STATUS_PROCESS ssStatus; 
    DWORD dwOldCheckPoint; 
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;

    /// run service with given name
    SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
    if (schSCManager==0) 
    {
        DWORD nError = GetLastError();
        LOG_ERROR("OpenSCManager failed, error code = %lu", nError);
    }
    else
    {
        /// open the service
        SC_HANDLE schService = OpenService( schSCManager, pName, SERVICE_ALL_ACCESS);
        if (schService==0) 
        {
            DWORD nError = GetLastError();
            LOG_ERROR("OpenService failed, error code = %lu", nError);
        }
        else
        {
            // Check the status in case the service is not stopped.
            if (!QueryServiceStatusEx( 
                schService,                     // handle to service 
                SC_STATUS_PROCESS_INFO,         // information level
                (LPBYTE) &ssStatus,             // address of structure
                sizeof(SERVICE_STATUS_PROCESS), // size of structure
                &dwBytesNeeded ) )              // size needed if buffer is too small
            {
                LOG_ERROR("QueryServiceStatusEx failed (%lu)", GetLastError());
                CloseServiceHandle(schService); 
                CloseServiceHandle(schSCManager);
                return FALSE;
            }

            // Check if the service is already running.
            // Todo: stop service and start again
            if(ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
            {
                LOG_WARN("Cannot start the service because it is already running");
                CloseServiceHandle(schService); 
                CloseServiceHandle(schSCManager);
                return FALSE;
            }
            
            // Save the tick count and initial checkpoint.
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;

            // Wait for the service to stop before attempting to start it.
            while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
            {
                // Do not wait longer than the wait hint. A good interval is 
                // one-tenth of the wait hint but not less than 1 second  
                // and not more than 10 seconds. 
                dwWaitTime = ssStatus.dwWaitHint / 10;
                if( dwWaitTime < 1000 )
                    dwWaitTime = 1000;
                else if ( dwWaitTime > 10000 )
                    dwWaitTime = 10000;
                Sleep( dwWaitTime );

                // Check the status until the service is no longer stop pending. 
                if (!QueryServiceStatusEx( 
                    schService,                     // handle to service 
                    SC_STATUS_PROCESS_INFO,         // information level
                    (LPBYTE) &ssStatus,             // address of structure
                    sizeof(SERVICE_STATUS_PROCESS), // size of structure
                    &dwBytesNeeded ) )              // size needed if buffer is too small
                {
                    LOG_ERROR("QueryServiceStatusEx failed (%lu)", GetLastError());
                    CloseServiceHandle(schService); 
                    CloseServiceHandle(schSCManager);
                    return FALSE; 
                }

                if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
                {
                    // Continue to wait and check.
                    dwStartTickCount = GetTickCount();
                    dwOldCheckPoint = ssStatus.dwCheckPoint;
                }
                else
                {
                    if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
                    {
                        LOG_ERROR("Timeout waiting for service to stop");
                        CloseServiceHandle(schService); 
                        CloseServiceHandle(schSCManager);
                        return FALSE;
                    }
                }
            }

            /// call StartService to run the service
            if(StartService(schService, 0, (LPCTSTR *)NULL))
            {
                // Check the status until the service is no longer start pending. 
                if (!QueryServiceStatusEx( 
                    schService,                     // handle to service 
                    SC_STATUS_PROCESS_INFO,         // info level
                    (LPBYTE) &ssStatus,             // address of structure
                    sizeof(SERVICE_STATUS_PROCESS), // size of structure
                    &dwBytesNeeded ) )              // if buffer too small
                {
                    LOG_ERROR("QueryServiceStatusEx failed (%lu)", GetLastError());
                    CloseServiceHandle(schService); 
                    CloseServiceHandle(schSCManager);
                    return FALSE;
                }
 
                // Save the tick count and initial checkpoint.
                dwStartTickCount = GetTickCount();
                dwOldCheckPoint = ssStatus.dwCheckPoint;

                while (ssStatus.dwCurrentState == SERVICE_START_PENDING) 
                { 
                    // Do not wait longer than the wait hint. A good interval is 
                    // one-tenth the wait hint, but no less than 1 second and no 
                    // more than 10 seconds. 
                    dwWaitTime = ssStatus.dwWaitHint / 10;
                    if( dwWaitTime < 1000 )
                        dwWaitTime = 1000;
                    else if ( dwWaitTime > 10000 )
                        dwWaitTime = 10000;
                    Sleep( dwWaitTime );

                    // Check the status again. 
                    if (!QueryServiceStatusEx( 
                        schService,             // handle to service 
                        SC_STATUS_PROCESS_INFO, // info level
                        (LPBYTE) &ssStatus,             // address of structure
                        sizeof(SERVICE_STATUS_PROCESS), // size of structure
                        &dwBytesNeeded ) )              // if buffer too small
                    {
                        LOG_ERROR("QueryServiceStatusEx failed %lu", GetLastError());
                        break; 
                    }
 
                    if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
                    {
                        // Continue to wait and check.
                        dwStartTickCount = GetTickCount();
                        dwOldCheckPoint = ssStatus.dwCheckPoint;
                    }
                    else
                    {
                        if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
                        {
                            // No progress made within the wait hint.
                            break;
                        }
                    }
                } 

                // Determine whether the service is running.
                if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
                {
                    LOG_INFO("Service started successfully."); 
                }
                else 
                {
                    LOG_ERROR("Service not started, error code = %lu", GetLastError());
                    LOG_WARN("  Current State: %lu", ssStatus.dwCurrentState);
                    LOG_WARN("  Exit Code: %lu", ssStatus.dwWin32ExitCode);
                    LOG_WARN("  Check Point: %lu", ssStatus.dwCheckPoint);
                    LOG_WARN("  Wait Hint: %lu", ssStatus.dwWaitHint);
                    CloseServiceHandle(schService);
                    CloseServiceHandle(schSCManager);
                    return FALSE;
                }
            }
            else
            {
                LOG_ERROR("StartService failed, error code = %lu", GetLastError());
            }
            CloseServiceHandle(schService);
        }                         
        CloseServiceHandle(schSCManager);
    }
    return FALSE;
}

void ExecuteSubProcess()
{
    if(!StartServiceCtrlDispatcher(lpServiceStartTable))
    {
        DWORD nError = GetLastError();
        LOG_ERROR("StartServiceCtrlDispatcher failed, error code = %lu", nError);
    }
    
}

void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    LOG_INFO("CCDMonitorService start ...");

    ServiceStatus.dwServiceType        = SERVICE_WIN32; 
    ServiceStatus.dwCurrentState       = SERVICE_START_PENDING; 
    ServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_POWEREVENT; 
    ServiceStatus.dwWin32ExitCode      = 0; 
    ServiceStatus.dwServiceSpecificExitCode = 0; 
    ServiceStatus.dwCheckPoint         = 0; 
    ServiceStatus.dwWaitHint           = 0; 
 
    hServiceStatusHandle = RegisterServiceCtrlHandlerEx(pServiceName, ServiceHandler, NULL);
    if (hServiceStatusHandle==0) 
    {
        DWORD nError = GetLastError();
        LOG_ERROR("RegisterServiceCtrlHandlerEx failed, error code = %lu", nError);
        return; 
    } 
    LOG_INFO("RegisterServiceCtrlHandlerEx success");
 
    /// Initialization complete - report running status 
    ServiceStatus.dwCurrentState       = SERVICE_RUNNING; 
    ServiceStatus.dwCheckPoint         = 0; 
    ServiceStatus.dwWaitHint           = 0;  
    if(!SetServiceStatus(hServiceStatusHandle, &ServiceStatus)) 
    { 
        DWORD nError = GetLastError();
        LOG_ERROR("SetServiceStatus failed, error code = %lu", nError);
    } 

    users_Init();
    users_Load();
    EndAllProcess();
    StartProcess();
    serviceStart();
    
    /// release resources: protobuf and Users
    google::protobuf::ShutdownProtobufLibrary();
    delete Users;
}

DWORD WINAPI ServiceHandler(DWORD fdwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    switch(fdwControl) 
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            ProcessStarted = FALSE;
            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            ServiceStatus.dwCheckPoint    = 0; 
            ServiceStatus.dwWaitHint      = 0;
            // SERVICE_CONTROL_SHUTDOWN
            {
                LOG_INFO("SERVICE_CONTROL_SHUTDOWN");
            }
            /// terminate all processes started by this service before shutdown
            {
                bShutdown = true;
                EndProcess();
                if (!Users->empty())
                    Users->clear();
            }
            // SERVICE_CONTROL_SHUTDOWN

            break; 
        case SERVICE_CONTROL_POWEREVENT:
            if (dwEventType == PBT_APMSUSPEND) {
                // No longer to disable wifi & BT when suspended
                LOG_INFO("PBT_APMSUSPEND");
            }
            else if (dwEventType == PBT_APMRESUMEAUTOMATIC) {
            }
            break;
        case SERVICE_CONTROL_PAUSE:
            ServiceStatus.dwCurrentState = SERVICE_PAUSED; 
            break;
        case SERVICE_CONTROL_CONTINUE:
            ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
            break;
        case SERVICE_CONTROL_INTERROGATE:
            break;
        default:
            if(fdwControl>=128&&fdwControl<256) {
                int nIndex = fdwControl&0x7F;
                /// bounce a single process
                if(nIndex >= 0 && nIndex < MAX_NUM_OF_PROCESS) {
                    EndProcess();
                    StartProcess();
                }
                /// bounce all processes
                else if(nIndex==127) {
                    EndProcess();
                    StartProcess();
                }
            }
            else {
                LOG_ERROR("Unrecognized opcode %lu", fdwControl);
            }
    };
    if (!SetServiceStatus(hServiceStatusHandle,  &ServiceStatus)) { 
        DWORD nError = GetLastError();
        LOG_ERROR("SetServiceStatus failed, error code = %lu", nError);
    }

    return NO_ERROR;
}
BOOL GetProcessByPid(DWORD pid)
{
    HANDLE        hProcessSnap = NULL; 
    BOOL          bRet      = FALSE; 
    PROCESSENTRY32 pe32      = {0};

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); 
    if (hProcessSnap == INVALID_HANDLE_VALUE) 
        return FALSE; 

    pe32.dwSize = sizeof(PROCESSENTRY32); 

    if (Process32First(hProcessSnap, &pe32)) {  
        do {
            if (pid == pe32.th32ProcessID) {
                if (!_tcsnicmp(pe32.szExeFile, _T("ccd.exe"), sizeof(pe32.szExeFile))) {
                    bRet = TRUE;
                    break;
                }
            }
        } while (Process32Next(hProcessSnap, &pe32)); 
    }

    CloseHandle (hProcessSnap); 
    return (bRet);
}

void users_Init()
{
    ::EnterCriticalSection(&g_CS);
    char ccdmsrvPath[MAX_PATH+1] = {0};
    if (::SHGetSpecialFolderPathA(NULL, ccdmsrvPath, CSIDL_COMMON_APPDATA, FALSE)) {
        strcat_s(ccdmsrvPath, sizeof(ccdmsrvPath), "\\acer\\CCDMSrv\\");
        BOOL rt = ::MakeSureDirectoryPathExists(ccdmsrvPath);
        if (!rt)
            LOG_ERROR("Failed to create folder for users history, error = %lu", GetLastError());
    }
    Users = new std::list<USERINFO>();
    if (::SHGetSpecialFolderPath(NULL, iniPath, CSIDL_COMMON_APPDATA, FALSE)) {
        _tcscat_s(iniPath, sizeof(iniPath), _T("\\acer\\CCDMSrv\\users.ini"));
        {
            char *sPath = wchr2chr(iniPath);
            LOG_INFO("User file path: %s", sPath);
            free((LPVOID)sPath);
        }
    }
    if (::SHGetSpecialFolderPath(NULL, iniPath_tmp, CSIDL_COMMON_APPDATA, FALSE)) {
        _tcscat_s(iniPath_tmp, sizeof(iniPath_tmp), _T("\\acer\\CCDMSrv\\users.ini.tmp"));
        {
            char *sPath = wchr2chr(iniPath_tmp);
            LOG_INFO("Tmp file path: %s", sPath);
            free((LPVOID)sPath);
        }
    }
    ::LeaveCriticalSection(&g_CS);
}

void users_Load()
{
    ::EnterCriticalSection(&g_CS);

    int userIndex=0;
    while (true) {
        USERINFO user={0};

        TCHAR userSession[MAX_PATH] = {0};
        DWORD dwSize = MAX_PATH*sizeof(TCHAR);
        TCHAR userSettings[MAX_PATH];
        
        _sntprintf(userSession, sizeof(userSession), _T("User%d"), userIndex);
        ++userIndex;

        GetPrivateProfileString(userSession, _T("Id"), NULL, user.ID, dwSize, iniPath);
        int rv = GetLastError();
        if (rv != ERROR_SUCCESS) {
            LOG_WARN("Failed to load users history, error=%d", rv);
            break;
        }
        if (_tcscmp(user.ID, _T("")) == 0) {
            LOG_DEBUG("Load users finished.");
            break;
        }

        GetPrivateProfileString(userSession, _T("LocalPath"), NULL, user.LOCALPATH, dwSize, iniPath);
        rv = GetLastError();
        if (rv != ERROR_SUCCESS) {
            LOG_WARN("Failed to load users history, error=%d", rv);
            break;
        }
        if (_tcscmp(user.LOCALPATH, _T("")) == 0) {
            LOG_DEBUG("Load users finished.");
            break;
        }

        _stprintf(userSettings, _T("%s\\settings.txt"), user.LOCALPATH);
        if (_taccess(userSettings, 0) != -1)
        {
            /// load username from settings.ini
            char* settingsFile = wchr2chr(userSettings);
            FILE *fd = fopen(settingsFile, "r");
            free((LPVOID)settingsFile);
            char buf[1024]={0};
            if (fd != NULL) {
                while (fgets(buf, 1024, fd)!=NULL) { 
                    char *token0 = strtok(buf, "=\n");
                    if (token0 != NULL) {
                        if (stricmp("username", token0) == 0) {
                            char* UserName = strtok(NULL, "=\n");
                            if (UserName != NULL) {
                                wchar_t *username = chr2wchr(UserName);
                                _sntprintf(user.USERNAME, sizeof(user.USERNAME), username);
                                LOG_INFO("User%d: %s", userIndex-1, UserName);
                                free((LPVOID)username);
                            }
                            break;
                        }
                    }
                }

                fclose(fd);
            }
        }
        user.INST_NUM = 0;
        user.STOP_WATCHDOG_EVENT = NULL;
        Users->push_back(user);
    }

    ::LeaveCriticalSection(&g_CS);
}

TCHAR* user_Load_UserName (TCHAR *appdatapath)
{
    ::EnterCriticalSection(&g_CS);
    TCHAR *username = NULL;
    TCHAR userSettings[MAX_PATH] = {0};
    _stprintf(userSettings, _T("%s\\settings.txt"), appdatapath);
    if (_taccess(userSettings, 0) != -1)
    {
        /// load username from settings.ini
        char* settingsFile = wchr2chr(userSettings);
        FILE *fd = fopen(settingsFile, "r");
        free((LPVOID)settingsFile);
        char buf[1024]={0};
        if (fd != NULL) {
            while (fgets(buf, 1024, fd)!=NULL) { 
                char *token0 = strtok(buf, "=\n");
                if (token0 != NULL) {
                    if (stricmp("username", token0) == 0) {
                        char* UserName = strtok(NULL, "=\n");
                        if (UserName != NULL) {
                            username = chr2wchr(UserName);    
                            LOG_INFO("username: %s", username);
                            free((LPVOID)username);
                        }
                        break;
                    }
                }
            }

            fclose(fd);
        }
    }
    ::LeaveCriticalSection(&g_CS);
    
    return username;
}

bool users_Add(TCHAR *sid, TCHAR *username, TCHAR *appdatapath, int instancenum)
{
    bool bFnd=false;
    PROCESS_INFORMATION pi = {0};
    
    if (_tcslen(appdatapath) <= 0)
        return false;

    ::EnterCriticalSection(&g_CS);
    {
        /// update previous user
        std::list<USERINFO>::iterator it;
        for (it=Users->begin(); it!=Users->end(); ++it) {
            if (_tcsncmp(sid, it->ID, (size_t)_tcslen(sid)) == 0 && instancenum == it->INST_NUM) {
                memset(it->LOCALPATH, 0, sizeof(it->LOCALPATH));
                _sntprintf(it->LOCALPATH, sizeof(it->LOCALPATH), appdatapath);
                it->PI = pi;
                bFnd = true;
            }
        }
    }
    if (!bFnd) {
        // create new user
        USERINFO user={0};
        memset(user.ID, 0, sizeof(user.ID));
        memset(user.LOCALPATH, 0, sizeof(user.LOCALPATH));
        _sntprintf(user.ID, sizeof(user.ID), sid);
        _sntprintf(user.LOCALPATH, sizeof(user.LOCALPATH), appdatapath);
        memset(user.USERNAME, 0, sizeof(user.USERNAME));
        if (username != NULL) {
            _sntprintf(user.USERNAME, sizeof(user.USERNAME), username);
        }
        user.PI = pi;
        user.STOP_WATCHDOG_EVENT = NULL;
        user.INST_NUM = instancenum;
        // push back to Users list
        Users->push_back(user);
        bFnd = true;
    }

    ::LeaveCriticalSection(&g_CS);
    return bFnd;
}
bool users_UpdateProcess(TCHAR *sid, PROCESS_INFORMATION pi, int instancenum)
{
    bool rt=false;
    
    ::EnterCriticalSection(&g_CS);
    {
        /// update previous user
        std::list<USERINFO>::iterator it;
        for (it=Users->begin(); it!=Users->end(); ++it) {
            if (_tcsncmp(sid, it->ID, (size_t)_tcslen(sid)) == 0 && instancenum == it->INST_NUM) {
                //CloseHandle(it->PI.hProcess);
                it->PI = pi;
                rt = true;
            }
        }
    }
    ::LeaveCriticalSection(&g_CS);

    return rt;
}

std::list<USERINFO>::iterator users_Remove(TCHAR *sid, int instancenum)
{
    std::list<USERINFO>::iterator it;

    ::EnterCriticalSection(&g_CS);
    {
        for (it=Users->begin(); it!=Users->end(); ++it) {
            if (_tcsncmp(sid, it->ID, (size_t)_tcslen(sid)) == 0 && instancenum == it->INST_NUM) {
                //CloseHandle(it->PI.hProcess);
                it = Users->erase(it);
                break;
            }
        }
    }

    ::LeaveCriticalSection(&g_CS);
    return it;
}

void users_SaveBack()
{
    int userIndex=0;
    std::list<USERINFO>::iterator it;

    ::EnterCriticalSection(&g_CS);
    if (!DeleteFile(iniPath_tmp)) {
        DWORD rv = GetLastError();
        if (rv != ERROR_FILE_NOT_FOUND && rv != ERROR_PATH_NOT_FOUND) {
            char *inipath = wchr2chr(iniPath_tmp);
            LOG_ERROR("Delete %s failed, error = %lu", inipath, rv);
            free((LPVOID)inipath);
            goto out;
        }
    }
    if (Users->empty()) {
        DeleteFile(iniPath);
        goto out;
    }
    /// pre create ini with unicode encoding
    FILE *fp = _tfopen(iniPath_tmp, _T("w+b"));
    wchar_t m_strUnicode[1];
    m_strUnicode[0] = wchar_t(0XFEFF);
    fputwc(*m_strUnicode,fp);
    fclose(fp);

    for (it=Users->begin(); it != Users->end(); ++it) {
        TCHAR userSession[MAX_PATH] = {0};
        //DWORD dwSize = MAX_PATH*sizeof(TCHAR);
            
        _sntprintf(userSession, sizeof(userSession), _T("User%d"), userIndex);
        ++userIndex;

        BOOL rt = WritePrivateProfileString(userSession, _T("Id"), it->ID, iniPath_tmp);
        if (!rt) {
            DWORD rv = GetLastError();
            LOG_ERROR("Failed to save id to users history, error=%lu", rv);
            break;
        }
        rt = WritePrivateProfileString(userSession, _T("LocalPath"), it->LOCALPATH, iniPath_tmp);
        if (!rt) {
            DWORD rv = GetLastError();
            LOG_ERROR("Failed to save localpath to users history, error=%lu", rv);
            break;
        }
    }
    
    if (!MoveFileEx(iniPath_tmp, iniPath, MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        DWORD rv = GetLastError();
        LOG_ERROR("Rename %s to %s failed, error = %lu", iniPath_tmp, iniPath, rv);
        goto out;
    }

out:
    ::LeaveCriticalSection(&g_CS);
}

BOOL StartProcess()
{
    std::list<USERINFO>::iterator it;

    //::EnterCriticalSection(&g_CS);

    std::string   ccdDataPath = getCcdDataPath();
    if(0 >= ccdDataPath.length()) {
        goto out;
    }

    if (Users->empty())
        goto out;

    for (it=Users->begin(); it != Users->end(); ++it) {
        /// don't launch ccd if no username
        if (_tcslen(it->USERNAME) <= 0)
            continue;

        /// not yet launched
        it->STOP_WATCHDOG_EVENT = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        if (it->STOP_WATCHDOG_EVENT == NULL) {
            DWORD rv = GetLastError();
            LOG_ERROR("Create stop ccd watchdog event failed, error code: %lu", rv);
        }
        ::ResetEvent(it->STOP_WATCHDOG_EVENT);

#ifdef _trustees_enable
        /// load trustees from cache
        int nIndex = WideCharToMultiByte(CP_UTF8, 0, it->ID, -1, NULL, 0, NULL, NULL);
        char *sidStr = new char[nIndex + 1];
        WideCharToMultiByte(CP_UTF8, 0, it->ID, -1, sidStr, nIndex, NULL, NULL);
        int rc = users_loadTrustees(sidStr, it->req);
        free(sidStr);

        if (rc < 0) {
            LOG_WARN("Cannot load trustees and not to auto-launch ccd.exe, %d", rc);
            continue;
        }
        else if (it->req.trustees_size() == 0) {
            /// launch ccd & update process information
            // bug #10162 : if trustee is absent, set shared memory handle = 0 and size = 0
            std::string args = "\"" + it->req.localpath() + "\" " + it->req.sid() + " 0 0 \"" + ccdDataPath + "\"";
            TCHAR *cmdline = chr2wchr(args.c_str());

            /// launch ccd
            PROCESS_INFORMATION pi;
            StartCCDWithWatchdog(cmdline, it->ID, it->STOP_WATCHDOG_EVENT, pi, it->hWatchdogThread);
            //Move critical section here to prevent deadlock, bug 12112
            ::EnterCriticalSection(&g_CS);
            it->PI = pi;
            ::LeaveCriticalSection(&g_CS);
            free((PVOID)cmdline);
        }
        else {
            /// compose anonymous shared memory for ccd::CCDWin32StartParams
            ccd::CCDWin32StartParams param;
            for (int i=0; i < it->req.trustees_size(); i++) {
                ccd::TrusteeItem *item = param.add_trustees();
                item->set_sid(it->req.trustees(i).sid().c_str());
                item->set_attr(it->req.trustees(i).attr());
            }
            HANDLE hASM = NULL;
            uint32_t size = sizeof(uint8_t) * param.ByteSize();
            LPVOID data = CreateStartupParams(&hASM, size);
            if (data) {
                param.SerializeToArray(data, size);

                /// launch ccd & update process information
                std::ostringstream strHandle;
                strHandle << (INT64)(INT_PTR)hASM << " " << size;
                std::string args = "\"" + it->req.localpath() + "\" " + it->req.sid() + " " + strHandle.str() + " \"" + ccdDataPath + "\"";
                TCHAR *cmdline = chr2wchr(args.c_str());

                /// launch ccd
                PROCESS_INFORMATION pi;
                StartCCDWithWatchdog(cmdline, it->ID, it->STOP_WATCHDOG_EVENT, pi, it->hWatchdogThread);
                //Move critical section here to prevent deadlock, bug 12112
                ::EnterCriticalSection(&g_CS);
                it->PI = pi;
                ::LeaveCriticalSection(&g_CS);
                free((PVOID)cmdline);
            }
            else {
                // Unmap shared memory
                UnmapViewOfFile(data);
                CloseHandle(hASM);
            }
        }
#else
        /// launch ccd's
        size_t nCmdline = (_tcslen(it->LOCALPATH) + 3 + _tcslen(it->ID) + 1)*sizeof(TCHAR);
        TCHAR *cmdline=(TCHAR*)malloc(nCmdline);
        memset(cmdline, 0, nCmdline);
        _sntprintf(cmdline, nCmdline-sizeof(TCHAR), _T("\"%s\" %s"), it->LOCALPATH, it->ID);
        PROCESS_INFORMATION pi;
        StartCCDWithWatchdog(cmdline, it->ID, it->STOP_WATCHDOG_EVENT, pi, it->hWatchdogThread);
        it->PI = pi;
        free((PVOID)cmdline);
#endif
    }
    for (it=Users->begin(); it != Users->end(); ) {
        if (_tcslen(it->USERNAME) > 0) {
            /// set user sid to SDK
            char *userSid = wchr2chr(it->ID);
            ::CCDIClient_SetOsUserId(userSid);

            int rc = users_loadTrustees(userSid, it->req);
            if (rc < 0) {
                LOG_WARN("Cannot load trustees and not to auto-launch ccd.exe, %d", rc);
            }
            else {
                // wait until ccd is ready
                int rv = waitCcd();
                if (rv != 0) {
                    LOG_ERROR("Waiting for CCD to be ready timeout!!");
                }
                else {
                    // if not user logs in, shutdown ccd
                    ccd::GetSystemStateInput request;
                    request.set_get_players(true);
                    ccd::GetSystemStateOutput response;
                    rv = CCDIGetSystemState(request, response);
                    if (rv == CCD_OK) {
                        if (!response.has_players()
                         || response.players().players_size() == 0) {
                            // no players
                            rv = CCD_ERROR_NOT_SIGNED_IN;
                        }
                        else {
                            // has players, check if user_id is valid
                            for (int i=0; i < response.players().players_size(); i++) {
                                if (response.players().players(i).user_id() == 0) {
                                    rv = CCD_ERROR_NOT_SIGNED_IN;
                                    break;
                                }
                            }
                        }
                    }
                    else {
                        // failed to get players after ccd is ready
                        LOG_ERROR("CCDIGetSystemState failed: %d", rv);
                    }
                }
                if (rv != 0) {
                    // Terminate the CCD process if waitCcd() timeout or login failed
                    BOOL bTerminateCCD = FALSE;

                    /// stop ccd watchdog first
                    stopCcdWatchDogThread(it);
    #ifdef _CCDI_SUPPORT_CLOSE_GRACEFULLY
                    /// try to shutdown ccd gracefully
                    ccd::UpdateSystemStateInput input;
                    ccd::UpdateSystemStateOutput output;
                    input.set_do_shutdown(true);
                    rv = ccdi::client::CCDIUpdateSystemState(input, output);
                    if (rv == 0) {
                        ccd::UpdateSystemStateInput dummyReq;
                        ccd::UpdateSystemStateOutput dummyResp;
                        rv = ccdi::client::CCDIUpdateSystemState(dummyReq, dummyResp);
                        bTerminateCCD = TRUE;
                    }
                    else {
                        LOG_WARN("terminate ccd of %s by api failed, error = %d", userSid, rv);
                    }
    #else
                    // terminate the process by force
                    int retryCount = 5;
                    while (retryCount--) {
                        bTerminateCCD = TerminateProcess(it->PI.hProcess, 0);
                        if (bTerminateCCD)
                            break;
                        else {
                            LOG_ERROR("%d time to try to terminate ccd process, err=%lu", 5-retryCount, GetLastError());
                            Sleep(100);
                        }
                    }
    #endif
                    // cleanup process information
                    if (bTerminateCCD) {
                        PROCESS_INFORMATION empty = {0};
                        it->PI = empty;
                    }
                }
            }
            free((LPVOID)userSid);
        }
        ++it;
    }
out:
    //::LeaveCriticalSection(&g_CS);
    return true;
}

BOOL StartCCDWithWatchdog(LPTSTR lpCommandLine, LPTSTR lpSid, HANDLE stopWdEvent, PROCESS_INFORMATION& pi, HANDLE& hWatchdogThread)
{
    BOOL bResult = FALSE;
 
    /// build absolute path of ccd
    TCHAR processDir[MAX_PATH+1] = {0};
    TCHAR processName[MAX_PATH+1] = {0};
    bool bRt = getProcessPath(processDir, processName);
    if (!bRt) {
        goto Cleanup;
    }
    size_t nCmdline = (_tcslen(processName) + 3 + _tcslen(lpCommandLine) + 1)*sizeof(TCHAR);
    TCHAR *cmdline = (TCHAR*)malloc(nCmdline);
    memset((PVOID)cmdline, 0, nCmdline);
    _sntprintf(cmdline, nCmdline-sizeof(TCHAR), _T("\"%s\" %s"), processName, lpCommandLine);

    char log_str[MAX_PATH+1];
    wcstombs(log_str, cmdline,MAX_PATH+1);
    LOG_INFO("Launch CCD cmdline: %s ", log_str);

    bResult = ::LaunchProcess(cmdline, processDir, pi, TRUE);
    if (!bResult) {
        LOG_ERROR("Launch ccd process Error: %lu", GetLastError());
        goto Cleanup;
    }
    else {
        /// create thread for ccd state detection and ccd re-launch
        WATCHDOG_ARGS *wa;
        DWORD rv = 0;
        UINT ccdWatchDogThreadId;
        
        // compose WATCHDOG_ARGS:
        // store event to stop watch dog thread, and cmdline to re-launch ccd.exe
        wa= (WATCHDOG_ARGS *)malloc(sizeof(WATCHDOG_ARGS));
        memset(wa, 0, sizeof(WATCHDOG_ARGS));

        size_t ncmdline = (_tcslen(cmdline)+1)*sizeof(TCHAR);
        wa->cmdline = (LPTSTR)malloc(ncmdline);
        memset(wa->cmdline, 0, ncmdline);
        size_t nProcessDir = (_tcslen(processDir)+1)*sizeof(TCHAR);
        wa->workingdir = (LPTSTR)malloc(nProcessDir);
        memset(wa->workingdir, 0, nProcessDir);
        _sntprintf(wa->cmdline, ncmdline-sizeof(TCHAR), cmdline);    
        _sntprintf(wa->sid, MAX_PATH*sizeof(TCHAR), lpSid);
        _sntprintf(wa->workingdir, nProcessDir-sizeof(TCHAR), processDir);
        wa->pi = pi;
        wa->STOP_WATCHDOG_EVENT = stopWdEvent;

        hWatchdogThread = (HANDLE)_beginthreadex(NULL,
                                                 0,
                                                 ccdWatchDogThread,
                                                 (LPVOID)wa,
                                                 0,
                                                 &ccdWatchDogThreadId);
        if (hWatchdogThread == 0) {
            LOG_ERROR("ccd watchdog thread returned %lu", rv);
        }
    }

Cleanup:
    free((PVOID)cmdline);
    cmdline=NULL;

    return bResult;
}

BOOL LaunchProcess(const LPTSTR lpCommandLine, const LPTSTR lpCWD, PROCESS_INFORMATION& pi, BOOL inheritable)
{
    STARTUPINFO si;
    BOOL bResult = FALSE;
    DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS|CREATE_NO_WINDOW;
    HANDLE hPToken = NULL;
    HANDLE hProcess = NULL;
    //LPVOID pEnv =NULL;
    //TOKEN_MANDATORY_LABEL TIL = {0};
 
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb= sizeof(STARTUPINFO);
    //si.lpDesktop = "winsta0\\default";
    ZeroMemory(&pi, sizeof(pi));
    // Launch the process  in the client's logon session.
    bResult = CreateProcess(
        NULL,    // file to execute
        lpCommandLine,      // command line
        NULL,    // pointer to process SECURITY_ATTRIBUTES
        NULL,    // pointer to thread SECURITY_ATTRIBUTES
#ifdef _trustees_enable
        inheritable,        // handles inheritable or not
#else
        FALSE,              // handles are not inheritable
#endif
        dwCreationFlags,    // creation flags
        NULL, //pEnv,       // pointer to new environment block
        lpCWD,              // name of current directory
        &si,                // pointer to STARTUPINFO structure
        &pi                 // receives information about new process
    );
    if (!bResult) {
        // Convert lpCommandLine to char array
        int nIndex = WideCharToMultiByte(CP_UTF8, 0, lpCommandLine, -1, NULL, 0, NULL, NULL);
        char *cmdine = new char[nIndex + 1];
        WideCharToMultiByte(CP_UTF8, 0, lpCommandLine, -1, cmdine, nIndex, NULL, NULL);
        LOG_ERROR("Launch process %s error: %lu", cmdine, GetLastError());
        free(cmdine);
        goto Cleanup;
    }

Cleanup:
    //if(pEnv) 
    //    DestroyEnvironmentBlock( pEnv );
    if (hProcess != NULL) {
        CloseHandle(hProcess);
        hProcess = NULL;
    }
    if (hPToken != NULL) {
        CloseHandle(hPToken);
        hPToken = NULL;
    }

    return bResult;
}

void EndProcess()
{
    std::list<USERINFO>::iterator it;
    ::EnterCriticalSection(&g_CS);
    BOOL bTerminateCCD = FALSE;

    if (Users->empty())
        goto out;
    for (it=Users->begin(); it != Users->end(); ++it) {
        /// stop ccd watchdog first
        stopCcdWatchDogThread(it);
#ifdef _CCDI_SUPPORT_CLOSE_GRACEFULLY
        /// set user sid to SDK
        char *userSid = wchr2chr(it->ID);
        ::CCDIClient_SetOsUserId(userSid);
        /// try to shutdown ccd gracefully
        ccd::UpdateSystemStateInput input;
        ccd::UpdateSystemStateOutput output;
        input.set_do_shutdown(true);
        int rv = ccdi::client::CCDIUpdateSystemState(input, output);
        if (rv == 0) {
            ccd::UpdateSystemStateInput dummyReq;
            ccd::UpdateSystemStateOutput dummyResp;
            rv = ccdi::client::CCDIUpdateSystemState(dummyReq, dummyResp);
            bTerminateCCD = TRUE;
        }
        else {
            LOG_WARN("terminate ccd of %s by api failed, error = %d", userSid, rv);
        }
        free((LPVOID)userSid);
#else
        /// terminate the process by force
        int retryCount = 5;
        while (retryCount--) {
            bTerminateCCD = TerminateProcess(it->PI.hProcess, 0);
            if (bTerminateCCD)
                break;
            else {
                LOG_ERROR("%d time to try to terminate ccd process, err=%lu", 5-retryCount, GetLastError());
                Sleep(100);
            }
        }
#endif
        /// cleanup process information
        if (bTerminateCCD) {
            PROCESS_INFORMATION empty = {0};
            it->PI = empty;
        }
        
    }

out:
    ::LeaveCriticalSection(&g_CS);
}
void EndAllProcess()
{
    ::EnterCriticalSection(&g_CS);

    PROCESSENTRY32 ppe = {0};
    ppe.dwSize = sizeof (PROCESSENTRY32);

    HANDLE hSnapShot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    if (Process32First (hSnapShot, &ppe))
    {
        do
        {
            if (_tcsicmp (_T("ccd.exe"), ppe.szExeFile) == 0)
            {
                HANDLE hWinManagerProcess = OpenProcess (PROCESS_TERMINATE, FALSE, ppe.th32ProcessID);
                if (hWinManagerProcess)
                {
                    TerminateProcess (hWinManagerProcess, 0);
                    CloseHandle (hWinManagerProcess);
                }
            }
        }while (Process32Next (hSnapShot, &ppe));
    }
    CloseHandle (hSnapShot);

    ::LeaveCriticalSection(&g_CS);
}

unsigned __stdcall AppLayerThread(void *)
{
    //DWORD dwCode;
    std::list<USERINFO>::iterator it;

    users_Init();
    users_Load();
    EndAllProcess();
    StartProcess();
    serviceStart();

    return 0;
}

void CloseWindowManager()
{
    PROCESSENTRY32 ppe = {0};
    ppe.dwSize = sizeof (PROCESSENTRY32);

    HANDLE hSnapShot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    if (Process32First (hSnapShot, &ppe))
    {
        do
        {
            if (!_tcsicmp (_T("WindowManager.exe"), ppe.szExeFile) && (ppe.th32ProcessID != GetCurrentProcessId ()))
            {
                HANDLE hWinManagerProcess = OpenProcess (PROCESS_TERMINATE, FALSE, ppe.th32ProcessID);
                if (hWinManagerProcess)
                {
                    TerminateProcess (hWinManagerProcess, 0);
                    CloseHandle (hWinManagerProcess);
                }
            }
        }while (Process32Next (hSnapShot, &ppe));
    }
    CloseHandle (hSnapShot);
}

static DWORD
createPipe()
{
    LOG_INFO("Now using socket name \"%s\"", pipeName);
    DWORD rv = ERROR_SUCCESS;

    SECURITY_ATTRIBUTES sa;
    sa.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    InitializeSecurityDescriptor((PSECURITY_DESCRIPTOR)sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    /// ACL is set as NULL to allow all access to the pipe
    SetSecurityDescriptorDacl((PSECURITY_DESCRIPTOR)sa.lpSecurityDescriptor, TRUE, NULL, FALSE);
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    if (hSrvPipe != NULL) {
        CloseHandle(hSrvPipe);
        hSrvPipe = NULL;
    }

    hSrvPipe = ::CreateNamedPipeA (pipeName,
                                   PIPE_ACCESS_DUPLEX,
                                   PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                   PIPE_UNLIMITED_INSTANCES,
                                   DEFAULT_PIPE_LEN,
                                   DEFAULT_PIPE_LEN,
                                   DEFAULT_PIPE_TO_MS,
                                   &sa);

    if (hSrvPipe == INVALID_HANDLE_VALUE) {
        rv = GetLastError();
        LOG_ERROR("Create socket failed, error = %lu", rv);
        return rv;
    }

    /// init file stream
    int fd = _open_osfhandle((intptr_t)hSrvPipe, 0);
    if (fd == -1) {
        LOG_WARN("Receive: invalid handle!");
        return ERROR_INVALID_HANDLE;
    }
    _fs = new FileStream(fd);

    free((LPVOID)sa.lpSecurityDescriptor);
    return ERROR_SUCCESS;
}

UINT __stdcall
serviceServerThreadFn(void*)
{
    DWORD rv;
    if (FALSE == EmptyWorkingSet(GetCurrentProcess())) {
        LOG_ERROR("Reduce Working Set Failed! ErrCode = 0x%X", GetLastError());
    }

    while (true) {
        int srvFd = _open_osfhandle((intptr_t)hSrvPipe, 0);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        fd_set selectSet;

        FD_ZERO(&selectSet);
        FD_SET(srvFd, &selectSet);

        SOCKET connectedSocket_out=0;
        if (FD_ISSET(srvFd, &selectSet)) {
            BOOL success = ConnectNamedPipe(hSrvPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
            if (!success) {
                rv = GetLastError();
                LOG_ERROR("ConnectNamedPipe failed: %lu", rv);
                break;
            }
            
            /// Handle Read/Write named pipe
            handleRWNamedPipe();
            releasePipe();
            rv = createPipe();
            if (rv != ERROR_SUCCESS) {
                LOG_ERROR("createPipe failed: %lu", rv);
                break;
            }
        }

        if (bShutdown) {
            rv = ERROR_SUCCESS;
            LOG_INFO("manually shutdown");
            break;
        }
    }
    if (rv == ERROR_SUCCESS)
        releasePipe();
    return 0;
}

void stopCcdWatchDogThread(std::list<USERINFO>::iterator it)
{
    ::EnterCriticalSection(&g_CS);

    if (it->STOP_WATCHDOG_EVENT != NULL) {
        LOG_INFO("Trigger event[%p] to stop CCD with PID:%u", it->STOP_WATCHDOG_EVENT, it->PI.dwProcessId);
        ::SetEvent(it->STOP_WATCHDOG_EVENT);
        ::WaitForSingleObject(it->hWatchdogThread, INFINITE);
        ::CloseHandle(it->hWatchdogThread);
        it->hWatchdogThread = NULL;
        ::CloseHandle(it->STOP_WATCHDOG_EVENT);
        it->STOP_WATCHDOG_EVENT = NULL;
    }

    ::LeaveCriticalSection(&g_CS);
    return;
}

UINT __stdcall
ccdWatchDogThread(void* arg)
{
    WATCHDOG_ARGS *wa = (WATCHDOG_ARGS*)arg;
    HANDLE ghEvents[3];
    DWORD dwEvent = 0;
    DWORD exitCode = 0;
    BOOL bWatching = TRUE;

    ghEvents[0] = hStopCCDRelaunch;
    ghEvents[1] = wa->STOP_WATCHDOG_EVENT;
    ghEvents[2] = wa->pi.hProcess;

    while (bWatching) {
        dwEvent = ::WaitForMultipleObjects(3, ghEvents, FALSE, INFINITE);
        Sleep(100);  // avoid ccd service to make system busy
        switch(dwEvent) {
        case WAIT_OBJECT_0 + 0:
            LOG_INFO("event %p triggered, serice is stopped", ghEvents[0]);
            /// stop watchdog when CCD monitor service is stopped
            bWatching = FALSE;
            break;
        case WAIT_OBJECT_0 + 1:
            LOG_INFO("event[%p] triggered, CCD with PID:%u is stopped", ghEvents[1], wa->pi.dwProcessId);
            /// stop watchdog when CCD monitor service is requested to stop ccd
            bWatching = FALSE;
            ResetEvent(ghEvents[1]);
            break;
        case WAIT_OBJECT_0 + 2:
            LOG_INFO("event[%p] triggered, CCD needs to be restarted", ghEvents[2], wa->pi.dwProcessId);
            /// ccd terminates, relaunch it if the exit code is not 0
            {
                ::GetExitCodeProcess(wa->pi.hProcess, &exitCode);
                LOG_INFO("ccd exits with error exit code: %lu", exitCode);
                if (exitCode == 0) {
                    /// ccd exits not because of crash
                    bWatching = FALSE;
                }
                else {
                    /// launch the ccd process
                    CloseHandle(ghEvents[2]);
                    ghEvents[2] = NULL;
                
                    PROCESS_INFORMATION pi;
                    BOOL bResult = FALSE;
                    int retryCount = 3;
                    while (retryCount--) {
                        LOG_INFO("%d-time re-launch ccd...", 3-retryCount);
                        bResult = ::LaunchProcess(wa->cmdline, wa->workingdir, pi, TRUE);
                        if (!bResult) {
                            LOG_ERROR("Relaunch ccd process error: %lu", GetLastError());
                            CloseHandle(pi.hThread);
                            Sleep(500);
                        }
                        else {
                            LOG_INFO("Relaunch ccd success");
                            /// update information and history
                            ghEvents[2] = pi.hProcess;
                            ::users_UpdateProcess(wa->sid, pi);
                            /// try to login ccd
                            /// FIXME: appdatapath is not used anywhere, so user_Load_UserName will always return NULL
                            TCHAR* username = user_Load_UserName(wa->appdatapath);
                            if (username != NULL) {
                                // wait until ccd is ready
                                int rv = waitCcd();
                                if (rv != 0) {
                                    LOG_ERROR("Waiting for CCD to be ready timeout!!");
                                    bResult = FALSE;
                                }
                                if (!bResult) {
                                    /// then terminate the process by force
                                    int retryCount = 3;
                                    while (retryCount--) {
                                        if (TerminateProcess(pi.hProcess, 0)) {
                                            LOG_INFO("%d time to terminate ccd process success", 5-retryCount);
                                            bResult = FALSE;
                                            break;
                                        }
                                        else {
                                            LOG_ERROR("%d time to try to terminate ccd process, err=%lu", 5-retryCount, GetLastError());
                                            bResult = TRUE;
                                            Sleep(100);
                                        }
                                    }
                                }
                            }
                            break;
                        }
                    }
                    if (!bResult) {
                        LOG_ERROR("Launch ccd process failed after retry!");
                        bWatching = FALSE;
                        /// cleanup process information in users history
                        PROCESS_INFORMATION empty = {0};
                        ::users_UpdateProcess(wa->sid, empty);
                    }
                }
            }
            break;
        default:
            LOG_ERROR("Wait error: %lu", GetLastError());
            bWatching = FALSE;
            break;
        }
    }

    std::free((LPVOID)wa->cmdline);
    std::free((LPVOID)wa->workingdir);

    LOG_INFO("Watchdog thread for CCD with PID:%u exits!", wa->pi.dwProcessId);
    return 0;
}

static DWORD
handleRWNamedPipe ()
{
    CCDMonSrv::REQINPUT input;
    CCDMonSrv::REQOUTPUT output;

    DWORD rv = readMessage(input);    
    if (rv == 0) {

        std::string   ccdDataPath = getCcdDataPath();
        if(0 >= ccdDataPath.length()) {
            output.set_result(CCDMonSrv::ERROR_UNKNOWN);
            output.set_message("CCD Data Path was not found.");
            rv = writeMessage(output);
            goto out;
        }

        /// handle messages
        switch(input.type())
        {
        case CCDMonSrv::NEW_CCD:
            {
                bool bRt = false;
                std::list<USERINFO>::iterator it;
                int instancenum = input.has_instancenum() ? input.instancenum() : 0;
#ifdef _trustees_enable
                // update trustees
                users_saveTrustees(input);
#endif
                /// search user list
                if (!Users->empty()) {
                    for (it=Users->begin(); it!=Users->end(); ++it) {
                        wchar_t *inSid = chr2wchr(input.sid().c_str());
                        int len = input.sid().size();
                        if (_tcsncmp(inSid, it->ID, len) == 0 && instancenum == it->INST_NUM) {
                            /// found user in table
                            if (it->PI.dwProcessId != 0) {
                                DWORD exitCode=0;
                                ::GetExitCodeProcess(it->PI.hProcess, &exitCode);
                                if (exitCode == STILL_ACTIVE) {
                                    /// already launched, compose response
                                    output.set_result(CCDMonSrv::SUCCESS);
                                    output.set_message("ccd already launched");
                                    rv = writeMessage(output);
                                    bRt = true;
                                }
                            }
                            free ((LPVOID)inSid);
                            break;
                        }
                        else {
                            free ((LPVOID)inSid);
                        }
                    }
                }
                if (!bRt) {
                    /// not yet launch

#ifdef    ADD_CCD_TO_WINDOWS_DEFENDER_EXCLUSION_LIST
                    wchar_t* logPath = chr2wchr(input.localpath().c_str());
                    addExcludeList2WindowsDefenderRegistry(NULL, logPath);
                    free(logPath);
                    Sleep(300); //wait for Windows Defender to aware the change
#endif

                    wchar_t* wSid = chr2wchr(input.sid().c_str());
                    wchar_t* wPath = chr2wchr(input.localpath().c_str());
                    /// update local path
                    ::users_Add(wSid, NULL, wPath, instancenum);
                    ::users_SaveBack();
                    for (it=Users->begin(); it!=Users->end(); ++it) {
                        int len = input.sid().size();
                        if (_tcsncmp(wSid, it->ID, len) == 0 && instancenum == it->INST_NUM) {
                            /// found user in history table
                            break;
                        }
                    }
                    it->STOP_WATCHDOG_EVENT = ::CreateEvent(NULL, FALSE, FALSE, NULL);
                    if (it->STOP_WATCHDOG_EVENT == NULL) {
                        /// failed to create necessary event to stop watchdog thread, compose response
                        DWORD err = GetLastError();
                        char errMsg[MAX_PATH] = {0};
                        sprintf(errMsg, "failed to launch ccd, err = %lu", err);
                        LOG_ERROR("Create stop ccd watchdog event failed, error code: %lu", rv);
                        output.set_result(CCDMonSrv::ERROR_CREATE_PROCESS);
                        output.set_message(errMsg);
                        rv = writeMessage(output);
                    }
                    else {
                        ::ResetEvent(it->STOP_WATCHDOG_EVENT);
#ifdef _trustees_enable
                        /// compose anonymous shared memory for ccd::CCDWin32StartParams
                        ccd::CCDWin32StartParams param;
                        for (int i=0; i < input.trustees_size(); i++) {
                            ccd::TrusteeItem *item = param.add_trustees();
                            item->set_sid(input.trustees(i).sid().c_str());
                            item->set_attr(input.trustees(i).attr());
                        }
                        HANDLE hASM = NULL;
                        uint32_t size = sizeof(uint8_t) * param.ByteSize();
                        LPVOID data = CreateStartupParams(&hASM, size);
                        if (data) {
                            param.SerializeToArray(data, size);

                            /// launch ccd & update process information
                            std::ostringstream strHandle;
                            strHandle << (INT64)(INT_PTR)hASM << " " << size;
                            std::string args = "\"" + input.localpath() + "\" " + input.sid() + " " + strHandle.str() + " \"" + ccdDataPath + "\"";
                            TCHAR *cmdline = chr2wchr(args.c_str());

                            PROCESS_INFORMATION pi;
                            if(StartCCDWithWatchdog(cmdline, wSid, it->STOP_WATCHDOG_EVENT, pi, it->hWatchdogThread)) {
                                TCHAR *sid = chr2wchr(input.sid().c_str());
                                users_UpdateProcess(sid, pi, instancenum);
                                free((LPVOID)sid);
                                /// set user sid to SDK
                                ::CCDIClient_SetOsUserId(input.sid().c_str());
                                /// set test instance number (for testing purpose)
                                if (input.has_instancenum()) {
                                    ::CCDIClient_SetTestInstanceNum(input.instancenum());
                                } else {
                                    LOG_WARN("No instance number input");
                                }
                                int rt = waitCcd();
                                if (rt != 0) {
                                    LOG_ERROR("Waiting for CCD to be ready timeout!!");
                                    output.set_message("failed to launch ccd, timeout");
                                    output.set_result(CCDMonSrv::ERROR_CREATE_PROCESS);
                                }
                                else {
                                    output.set_result(CCDMonSrv::SUCCESS);
                                    output.set_message("Create new ccd for you");
                                }
                                LOG_WARN("Output: %s", output.DebugString().c_str());
                                rv = writeMessage(output);
                                bRt = true;
                            }
                            else {
                                DWORD err = GetLastError();
                                char errMsg[MAX_PATH] = {0};
                                sprintf(errMsg, "failed to launch ccd, err = %lu", err);
                                output.set_result(CCDMonSrv::ERROR_CREATE_PROCESS);
                                output.set_message(errMsg);
                                rv = writeMessage(output);
                                bRt = true;
                            }
                            free((LPVOID)wSid);
                            free((LPVOID)wPath);
                            free((PVOID)cmdline);
                        }
                        else {
                            DWORD rt = GetLastError();
                            char errMsg[MAX_PATH] = {0};

                            // Unmap shared memory
                            UnmapViewOfFile(data);
                            CloseHandle(hASM);

                            // Write back response to tell error
                            sprintf(errMsg, "failed to create startup params, err = %lu", rt);
                            output.set_result(CCDMonSrv::ERROR_CREATE_PROCESS);
                            output.set_message(errMsg);
                            rv = writeMessage(output);
                        }
#else
                        /// launch ccd & update process information
                        std::string args = "\"" + input.localpath() + "\" " + input.sid();                
                        TCHAR *cmdline = chr2wchr(args.c_str());

                        PROCESS_INFORMATION pi;
                        if(StartCCDWithWatchdog(cmdline, wSid, it->STOP_WATCHDOG_EVENT, pi, it->hWatchdogThread)) {
                            TCHAR *sid = chr2wchr(input.sid().c_str());
                            users_UpdateProcess(sid, pi, instancenum);
                            free((LPVOID)sid);
                            output.set_result(CCDMonSrv::SUCCESS);
                            output.set_message("Create new ccd for you");
                            rv = writeMessage(output);
                            bRt = true;
                        }
                        else {
                            DWORD err = GetLastError();
                            char errMsg[MAX_PATH] = {0};
                            sprintf(errMsg, "failed to launch ccd, err = %lu", err);
                            output.set_result(CCDMonSrv::ERROR_CREATE_PROCESS);
                            output.set_message(errMsg);
                            rv = writeMessage(output);
                            bRt = true;
                        }
                        free((LPVOID)wSid);
                        free((LPVOID)wPath);
                        free((PVOID)cmdline);
#endif
                    }
                }
            }
            break;
        case CCDMonSrv::CLOSE_CCD:
        case CCDMonSrv::CLOSE_DISABLE_CCD:
            {
                bool bRt = false;
                bool bSuccess = false;
                std::list<USERINFO>::iterator it;
                int instancenum = input.has_instancenum() ? input.instancenum() : 0;
                if (Users->empty()) {
                    /// cannot find any users
                    output.set_result(CCDMonSrv::ERROR_EMPTY_USER);
                    output.set_message("ccd doesn't exist");
                    rv = writeMessage(output);
                    break;
                }
                /// search user list
                for (it=Users->begin(); it!=Users->end(); ++it) {
                    wchar_t *inSid = chr2wchr(input.sid().c_str());
                    int len = input.sid().size();
                    if (_tcsncmp(inSid, it->ID, len) == 0 && instancenum == it->INST_NUM) {
                        bRt = true;
                        /// user found
                        if (it->PI.dwProcessId != 0) {
                            BOOL bTerminateCCD = FALSE;
                            /// stop ccd watchdog first
                            stopCcdWatchDogThread(it);
#ifdef _CCDI_SUPPORT_CLOSE_GRACEFULLY
                            /// set user sid to SDK
                            ::CCDIClient_SetOsUserId(input.sid().c_str());
                            /// set test instance number (for testing purpose)
                            if (input.has_instancenum()) {
                                ::CCDIClient_SetTestInstanceNum(input.instancenum());
                            }
                            /// try to shutdown ccd gracefully
                            ccd::UpdateSystemStateInput request;
                            ccd::UpdateSystemStateOutput response;
                            request.set_do_shutdown(true);
                            int rv = ccdi::client::CCDIUpdateSystemState(request, response);
                            if (rv == 0) {
                                ccd::UpdateSystemStateInput dummyReq;
                                ccd::UpdateSystemStateOutput dummyResp;
                                rv = ccdi::client::CCDIUpdateSystemState(dummyReq, dummyResp);
                                bTerminateCCD = TRUE;
                            }
                            else {
                                LOG_WARN("terminate ccd of %s by api failed, error = %d", input.sid().c_str(), rv);
                            }
#else
                            /// then terminate the process by force
                            int retryCount = 5;
                            while (retryCount--) {
                                bTerminateCCD = TerminateProcess(it->PI.hProcess, 0);
                                if (bTerminateCCD) {
                                    bSuccess = true;
                                    break;
                                }
                                else {
                                    LOG_ERROR("%d time to try to terminate ccd process, err=%lu", 5-retryCount, GetLastError());
                                    Sleep(100);
                                }
                            }
#endif
                            /// cleanup process information
                            if (bTerminateCCD) {
                                PROCESS_INFORMATION empty = {0};
                                it->PI = empty;
                                bSuccess = true;
                            }
                            free((LPVOID)inSid);
                            break;                            
                        }
                        else {
                            /// not yet launch
                            bSuccess = true;
                            free((LPVOID)inSid);
                            break;
                        }
                    }
                    free ((LPVOID)inSid);
                }
                if (bRt) {
                    if (input.type() == CCDMonSrv::CLOSE_DISABLE_CCD) {
                        /// delete user in users.ini
                        wchar_t* sid = chr2wchr(input.sid().c_str());
                        ::users_Remove(sid, instancenum);
                        free((LPVOID)sid);
                        ::users_SaveBack();
                    }
                    if (bSuccess) {    
                        /// send response
                        output.set_result(CCDMonSrv::SUCCESS);
                        output.set_message("ccd has been terminated");
                        rv = writeMessage(output);
                    }
                    else {
                        /// failed to open the process to be terminated
                        DWORD err = GetLastError();
                        char errMsg[MAX_PATH] = {0};
                        sprintf(errMsg, "failed to terminate ccd, err = %lu", err);
                        output.set_result(CCDMonSrv::ERROR_TERMINATE_PROCESS);
                        output.set_message(errMsg);
                        rv = writeMessage(output);
                    }
                }
                else {
                    /// send response
                    output.set_result(CCDMonSrv::ERROR_INVALID_INPUT);
                    output.set_message("cannot find corresponding ccd");
                    rv = writeMessage(output);
                }

                Sleep(300);//wait for CCD termination.
                if(!isCCDExist()) {
                    LOG_INFO("No CCD exists!");
                    if (FALSE == EmptyWorkingSet(GetCurrentProcess())) {
                        LOG_ERROR("Reduce Working Set Failed! ErrCode = 0x%X", GetLastError());
                    } else LOG_INFO("Reduce Memory Done!");
                } else LOG_INFO("CCD exists!");
            }
            break;

        case CCDMonSrv::CLOSE_CCDMS:
            bShutdown = true;
            /// send response
            output.set_result(CCDMonSrv::SUCCESS);
            output.set_message("ccd monitor service has been terminated");
            rv = writeMessage(output);
            break;

        case CCDMonSrv::SET_IOAC:
            if (input.has_ioac_option()) {
                int option = 0;
                size_t lCmdline = sizeof(TCHAR)*(_tcslen(IOACUtilityPath)+3);
                TCHAR *cmdline = (TCHAR*)malloc(lCmdline);
                memset(cmdline, 0, lCmdline);

                if (input.ioac_option() == CCDMonSrv::ENABLE) {
                    option = 1;
                }
                else {
                    option = 0;
                }
                _sntprintf(cmdline, lCmdline, _T("%s %d"), IOACUtilityPath, option);

                PROCESS_INFORMATION pi;
                LaunchProcess(cmdline, ProcessDir, pi);
                output.set_result(CCDMonSrv::SUCCESS);
                if (option == 1)
                    output.set_message("IOAC enabled");
                else
                    output.set_message("IOAC disabled");
                rv = writeMessage(output);
                
                free((LPVOID)cmdline);
            }
            else {
                LOG_WARN("SET_IOAC request doesn't set option.");
                output.set_result(CCDMonSrv::ERROR_INVALID_INPUT);
                output.set_message("No option for EnableWakeUpOption.exe");
                rv = writeMessage(output);
            }
            break;
        case CCDMonSrv::GET_WLAN_PROFILE:
            {
                string sInterfaceGUID = input.sid();

                HANDLE hClient = NULL;
                DWORD dwMaxClient = 2;
                DWORD dwCurVersion = 0;
                DWORD dwResult = 0;
                int iRet = 0;

                unsigned int i;
                PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
                PWLAN_INTERFACE_INFO pIfInfo = NULL;

                dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
                if (dwResult != ERROR_SUCCESS) {
                    // Can't open wifi handle, return fail
                    output.set_result(CCDMonSrv::ERROR_UNKNOWN);
                    output.set_message("Can't open wifi handle");
                    rv = writeMessage(output);
                } else {
                    dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
                    if (dwResult != ERROR_SUCCESS) {
                        // Can't enumerate interface, return fail
                        output.set_result(CCDMonSrv::ERROR_UNKNOWN);
                        output.set_message("Can't enumerate interface");
                        rv = writeMessage(output);
                    } else {
                        bool hasMatchedInterface = false;
                        for (i = 0; i < (int) pIfList->dwNumberOfItems; i++) {
                            pIfInfo = (WLAN_INTERFACE_INFO *) &pIfList->InterfaceInfo[i];
                            WCHAR wGuidString[39] = {0};

                            if(pIfInfo->isState != wlan_interface_state_connected)
                            {
                                // Interface not connected, try next
                                continue;
                            }

                            // For each interface, check if matches sInterfaceGUID
                            iRet = StringFromGUID2(pIfInfo->InterfaceGuid, (LPOLESTR) &wGuidString, sizeof(wGuidString)/sizeof(*wGuidString));

                            if (iRet == 0)
                            {
                                // No valid wGuidString, skip this interface, continue
                                continue;
                            } else {
                                // Convert wGuidString to char array
                                int nIndex = WideCharToMultiByte(CP_ACP, 0, wGuidString, -1, NULL, 0, NULL, NULL);
                                char *GuidString = new char[nIndex + 1];
                                WideCharToMultiByte(CP_ACP, 0, wGuidString, -1, GuidString, nIndex, NULL, NULL);

                                // Compare GuidString with sInterfaceGUID
                                if (sInterfaceGUID.compare(GuidString) == 0)
                                {
                                    hasMatchedInterface = true;

                                    // Same as we wish
                                    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
                                    DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
                                    WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

                                    dwResult = WlanQueryInterface (hClient,
                                                &(pIfInfo->InterfaceGuid),
                                                wlan_intf_opcode_current_connection,
                                                NULL,
                                                &connectInfoSize,
                                                (PVOID *) &pConnectInfo, 
                                                &opCode);

                                    if (dwResult != ERROR_SUCCESS) {
                                        // Can't get current connection
                                        output.set_result(CCDMonSrv::ERROR_UNKNOWN);
                                        output.set_message("Can't get current connection");
                                        rv = writeMessage(output);
                                    } else {
                                        // If there is profile
                                        if (wcslen(pConnectInfo->strProfileName) != 0) {
                                            LPWSTR profileXML;
                                            DWORD dwFlags = WLAN_PROFILE_GET_PLAINTEXT_KEY, dwAccess = 0;

                                            dwResult = WlanGetProfile (hClient,
                                                        &(pIfInfo->InterfaceGuid), 
                                                        pConnectInfo->strProfileName,
                                                        NULL,
                                                        &profileXML, &dwFlags, &dwAccess);

                                            if (dwResult != ERROR_SUCCESS) {
                                                // Can't get profile
                                                output.set_result(CCDMonSrv::ERROR_UNKNOWN);
                                                output.set_message("Can't get profile");
                                                rv = writeMessage(output);
                                            } else {
                                                int nIndex2 = WideCharToMultiByte(CP_UTF8, 0, profileXML, wcslen(profileXML), NULL, 0, NULL, NULL);
                                                char *xml = new char[nIndex2 + 1];
                                                WideCharToMultiByte(CP_ACP, 0, profileXML, wcslen(profileXML), xml, nIndex2, NULL, NULL);
                                                xml[nIndex2] = '\0';

                                                output.set_result(CCDMonSrv::SUCCESS);
                                                output.set_message(xml);
                                                rv = writeMessage(output);

                                                delete xml;
                                            }
                                        } else {
                                            // Profile name length == 0
                                            output.set_result(CCDMonSrv::ERROR_UNKNOWN);
                                            output.set_message("Profile name length is 0");
                                            rv = writeMessage(output);
                                        }
                                    }

                                } else {
                                    // Not the same interface guid, list nothing, fallback to hasMatchedInterface == false block
                                }

                                delete GuidString;
                            }
                        }

                        if(hasMatchedInterface == false) {
                            output.set_result(CCDMonSrv::ERROR_UNKNOWN);
                            output.set_message("No matched interface");
                            rv = writeMessage(output);
                        }
                    }

                    if (pIfList != NULL) {
                        WlanFreeMemory(pIfList);
                        pIfList = NULL;
                    }

                    WlanCloseHandle(hClient,NULL);
                }
            }
            break;
        case CCDMonSrv::LAUNCH_VDRIVE_INSTALLER:
            {
                string sCmdline = input.sid();
                wstring cmdline;
                cmdline.assign(sCmdline.begin(), sCmdline.end());

                PROCESS_INFORMATION pi;
                STARTUPINFO si;
                BOOL bResult = FALSE;
                DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS|CREATE_NO_WINDOW;
                HANDLE hPToken = NULL;
                HANDLE hProcess = NULL;
                //LPVOID pEnv =NULL;
                //TOKEN_MANDATORY_LABEL TIL = {0};
 
                ZeroMemory(&si, sizeof(STARTUPINFO));
                si.cb= sizeof(STARTUPINFO);
                //si.lpDesktop = "winsta0\\default";
                ZeroMemory(&pi, sizeof(pi));
               //size_t nCmdline = (_tcslen(ProcessName) + 3 + _tcslen(lpCommandLine) + 1)*sizeof(TCHAR);
                //TCHAR *cmdline = (TCHAR*)malloc(nCmdline);
                //memset((PVOID)cmdline, 0, nCmdline);
                //_sntprintf(cmdline, nCmdline-sizeof(TCHAR), _T("\"%s\" %s"), ProcessName, lpCommandLine);
    
                // Launch the process  in the client's logon session.
                bResult = CreateProcess(
                    NULL,    // file to execute
                    (LPTSTR)(cmdline.c_str()),    // command line
                    NULL,    // pointer to process SECURITY_ATTRIBUTES
                    NULL,    // pointer to thread SECURITY_ATTRIBUTES
                    FALSE,    // handles are not inheritable
                    dwCreationFlags,    // creation flags
                    NULL, //pEnv,    // pointer to new environment block
                    ProcessDir,    // name of current directory
                    &si,    // pointer to STARTUPINFO structure
                    &pi    // receives information about new process
                );
                
                if (!bResult) {
                    LOG_ERROR("Launch VDrive Installer process Error: %lu", GetLastError());
                    output.set_result(CCDMonSrv::ERROR_CREATE_PROCESS);
                    rv = writeMessage(output);
                }
                else
                {
                    WaitForSingleObject(pi.hProcess, INFINITE);
                    output.set_result(CCDMonSrv::SUCCESS);
                    rv = writeMessage(output);
                }
            }
            break;
        case CCDMonSrv::LAUNCH_IOAC_NET_TOOL:
            {
                LOG_INFO("Launch IOAC Net Tool");
                size_t lCmdline = sizeof(TCHAR)*(_tcslen(ProcessDir) + _tcslen(_T("IOACTool\\IOACNetTool.exe")) + 10);
                TCHAR *cmdline = (TCHAR*)malloc(lCmdline);
                memset(cmdline, 0, lCmdline);
                _sntprintf(cmdline, lCmdline, _T("%sIOACTool\\IOACNetTool.exe"), ProcessDir);
                char *ptr = wchr2chr(cmdline);
                LOG_INFO("Launch Process: %s", ptr);
                free(ptr);
                PROCESS_INFORMATION pi;
                LaunchProcess(cmdline, ProcessDir, pi);
                free(cmdline);
                output.set_result(CCDMonSrv::SUCCESS);
                output.set_message("Launch IOAC Net Tool");
                rv = writeMessage(output);
            }
            break;
        case CCDMonSrv::LAUNCH_TOOL:
            {
                string sCmdline = input.sid();//store command in sid field.
                string argv = "";
                if (input.has_argv()) {
                    argv = input.argv();
                } else { 
                    argv = "";
                }

                size_t lCmdline = sizeof(TCHAR)*(_tcslen(ProcessDir) + sCmdline.length() + argv.length() + 10);
                TCHAR *cmdline = (TCHAR*)malloc(lCmdline);
                memset(cmdline, 0, lCmdline);

                wchar_t *wptr = chr2wchr(sCmdline.c_str());
                _sntprintf(cmdline, lCmdline, _T("%s%s"), ProcessDir, wptr);
                free(wptr);

                char* ptr = wchr2chr(cmdline);
                LOG_INFO("File: \"%s\"", ptr);
                free(ptr);

                if (-1 != _waccess(cmdline, 0)) { //file exist

                    LOG_INFO("argv: \"%s\"", argv.c_str());

                    if (argv.length() > 0) {
                        wchar_t *wargv = chr2wchr(argv.c_str());
                        wptr = chr2wchr(sCmdline.c_str());
                        _sntprintf(cmdline, lCmdline, _T("\"%s%s\" %s"), ProcessDir, wptr, wargv);
                        free(wptr);
                        free(wargv);
                    } else {
                        wptr = chr2wchr(sCmdline.c_str());
                        _sntprintf(cmdline, lCmdline, _T("\"%s%s\""), ProcessDir, wptr);
                        free(wptr);
                    }

                    ptr = wchr2chr(cmdline);
                    LOG_INFO("Launch Command: \"%s\"", ptr);
                    free(ptr);
                    
                    PROCESS_INFORMATION pi;
                    LaunchProcess(cmdline, ProcessDir, pi);

                    output.set_result(CCDMonSrv::SUCCESS);
                    output.set_message("Launch Tool!");
                } else {
                    LOG_ERROR("File Doesn't Exist!");
                    output.set_result(CCDMonSrv::ERROR_FILE_ABSENT);
                    output.set_message("File Doesn't Exist!");
                }
                free(cmdline);
                rv = writeMessage(output);
            }
            break;
        case CCDMonSrv::LAUNCH_INSTALLER:
            {
                TCHAR osUserId[1024];
                TCHAR userImageProfilePath[1024];
                wstring programPath(input.localpath().begin(), input.localpath().end());
                wstring argv(input.argv().begin(), input.argv().end());

                wstring sid(input.sid().begin(), input.sid().end());
                _snwprintf(osUserId, ARRAY_SIZE_IN_BYTES(osUserId),L"%s", sid.c_str());

                if (ERROR_SUCCESS != getUserProfileImagePath(osUserId, userImageProfilePath)){
                    output.set_result(CCDMonSrv::ERROR_PATH_ABSENT);
                    output.set_message("Cannot find User Data Path! OS User ID may be wrong.");
                    rv = writeMessage(output);
                    break;
                }

                size_t lCmdline = sizeof(TCHAR)*(_tcslen(userImageProfilePath) + programPath.length() + argv.length() + 10);
                TCHAR *cmdline = (TCHAR*)malloc(lCmdline);
                memset(cmdline, 0, lCmdline);
                _sntprintf(cmdline, lCmdline, _T("%s\\%s"), userImageProfilePath, programPath.c_str());

                char* ptr = wchr2chr(cmdline);
                LOG_INFO("File: \"%s\"", ptr);
                free(ptr);

                if (-1 != _waccess(cmdline, 0)) { //file exist

                    _sntprintf(cmdline, lCmdline, _T("%s\\%s %s"), userImageProfilePath, programPath.c_str(), argv.c_str());
                    
                    ptr = wchr2chr(cmdline);
                    LOG_INFO("Launch Installer: \"%s\"", ptr);
                    free(ptr);
                    
                    PROCESS_INFORMATION pi;

                    //switch execute directory to the folder where the program locates
                    TCHAR tmpPath[MAX_PATH+1];
                    _sntprintf(tmpPath, MAX_PATH, _T("%s"), ProcessDir);

                    wstring tmp;
                    if(_tcsrchr(cmdline, L'\\')) {
                        tmp.assign(cmdline, _tcsrchr(cmdline, L'\\'));
                    } else {
                        tmp.assign(cmdline, cmdline + _tcslen(cmdline));
                    }

                    _sntprintf(ProcessDir, MAX_PATH, _T("%s"), tmp.c_str());
                    
                    LaunchProcess(cmdline, ProcessDir, pi);
                    
                    _sntprintf(ProcessDir, MAX_PATH, _T("%s"), tmpPath);

                    output.set_result(CCDMonSrv::SUCCESS);
                    output.set_message("Launch Installer");
                } else {
                    LOG_ERROR("File Doesn't Exist!");
                    output.set_result(CCDMonSrv::ERROR_FILE_ABSENT);
                    output.set_message("File Doesn't Exist!");
                }
                free(cmdline);
                rv = writeMessage(output);
            }//Launch install
            break;
        case CCDMonSrv::CCDMONSRV_INTERNAL:
            {
                //rv = addExcludeList2WindowsDefenderRegistry(ProcessName, NULL);
                if (ERROR_SUCCESS == rv) {
                    output.set_result(CCDMonSrv::SUCCESS);
                    output.set_message("CCDMonSrv Internal Done!");
                } else {
                    output.set_result(CCDMonSrv::ERROR_UNKNOWN);
                    output.set_message("Unknown Error!");
                }

                rv = writeMessage(output);
            }
            break;
        default:
            break;
        }

    }

out:
    return 0;
}

static DWORD
serviceStart()
{
    DWORD rv = 0;
    UINT srvThreadId;

#ifdef    ADD_CCD_TO_WINDOWS_DEFENDER_EXCLUSION_LIST
    addExcludeList2WindowsDefenderRegistry(ProcessName, NULL);
#endif

    rv = createPipe();
    if (rv != ERROR_SUCCESS) {
        LOG_ERROR("createPipe failed: %lu", rv);
        goto fail_create_socket;
    }
    LOG_INFO("Activated \"%s\"", pipeName);

    HANDLE hSrvThread = (HANDLE)_beginthreadex(NULL,
                                               0,
                                               serviceServerThreadFn,
                                               NULL,
                                               0,
                                               &srvThreadId);
    if (hSrvThread == 0) {
        LOG_ERROR("Util_SpawnThread returned %lu", GetLastError());
        goto fail_spawn_srv_thread;
    }

    ::WaitForSingleObject(hSrvThread, INFINITE);

fail_create_socket:
fail_spawn_srv_thread:

    if (hSrvThread != NULL) {
        CloseHandle(hSrvThread);
        hSrvThread = NULL;
    }

    return rv;
}

DWORD
writeMessage (CCDMonSrv::REQOUTPUT& output)
{
    DWORD rv=0;
    bool success=true;
    google::protobuf::uint32 bytes=0;

    if (hSrvPipe == NULL) {
        rv = 3;
    }
    else {
        if (_fs->getInputSteamErr() || _fs->getOutputStreamErr()) {
            rv = 2;
            goto out;
        }

        /// stream for writing request
        google::protobuf::io::ZeroCopyOutputStream *ouStream = _fs->getOutputStream();
        {
            /// Send request
            google::protobuf::io::CodedOutputStream out(ouStream);
            bytes = (google::protobuf::uint32)output.ByteSize();
            out.WriteVarint32(bytes);
            success &= !out.HadError();
        }
        if (!success) {
            LOG_ERROR("Failed to write");
            rv = 1;
        }
        else {
            success &= output.SerializeToZeroCopyStream(ouStream);
            if (success) {
                LOG_INFO("Response: %s, %d", output.message().c_str(), output.result());
            }
            else {
                LOG_ERROR("Failed to write response %s", output.DebugString());
                rv = 4;
            }
        }
        _fs->flushOutputStream();
    }
out:
    return rv;
}

DWORD
readMessage (CCDMonSrv::REQINPUT& input)
{
    DWORD rv=0;
    bool success=true;
    google::protobuf::uint32 bytes=0;

    if (hSrvPipe == NULL) {
        rv = 3;
    }
    else {
        if (_fs->getInputSteamErr() || _fs->getOutputStreamErr()) {
            rv = 2;
            goto out;
        }

        /// stream for reading response
        google::protobuf::io::ZeroCopyInputStream *inStream = _fs->getInputSteam();
        {
            google::protobuf::io::CodedInputStream in(inStream);
            success &= in.ReadVarint32(&bytes);
        }
        if (!success) {
            LOG_ERROR("Failed to read size");
            rv = 1;
        }
        else {
            google::protobuf::int64 bytesReadPre = inStream->ByteCount();
            if (bytes > 0) {
                success &= input.ParseFromBoundedZeroCopyStream(inStream, (int)bytes);
                if (success) {
                    LOG_INFO("Receive: %s ", input.DebugString().c_str());
                    rv = 0;
                }
                else {
                    int rt = _fs->getInputSteamErr();
                    if (rt == 0) {
                        google::protobuf::int64 bytesReadPost = inStream->ByteCount();
                        google::protobuf::uint32 bytesRead = (google::protobuf::uint32)(bytesReadPost - bytesReadPre);
                        if (bytesRead < bytes) {
                            inStream->Skip((int)(bytes - bytesRead));
                        }
                        LOG_INFO("Receive: %s ", input.DebugString().c_str());
                        rv = 0;
                    }
                    else {
                        LOG_ERROR("Receive: error code = %d", rt);
                        rv = 4;
                    }
                }    
            }
        }
    }

out:
    return rv;
}

void releasePipe()
{
    try {
        delete _fs;
    }
    catch (std::bad_alloc& e) {
        LOG_ERROR("allocation failed: %s", e.what());
    }
    catch (std::exception& e) {
        LOG_ERROR("Caught unexpected exception: %s", e.what());
    }

    CloseHandle(hSrvPipe);
    hSrvPipe = NULL;
}


#ifdef _trustees_enable
int users_loadTrustees(const char* sid, CCDMonSrv::REQINPUT& input)
{
    // Load trustees from file system
    int rv = 0;
    ProtobufFileReader reader;
    char filename[MAX_PATH+1]={0};
    _snprintf(filename, MAX_PATH, "%s\\%s", userTrusteesPath, sid);
    rv = reader.open(filename);
    if (rv < 0) {
        LOG_ERROR("reader.open(%s) returned %d", filename, rv);
    }
    else {
        LOG_INFO("Parsing from %s", filename);
        google::protobuf::io::CodedInputStream tempStream(reader.getInputStream());
        input.ParseFromCodedStream(&tempStream);
    }

    return rv;
}
void users_saveTrustees(CCDMonSrv::REQINPUT& input)
{
    // Store trustees back to file system
    ProtobufFileWriter writer;
    char filename[MAX_PATH+1]={0};
    _snprintf(filename, MAX_PATH, "%s\\%s", userTrusteesPath, input.sid().c_str());
    LOG_INFO("file name to save trustees: %s", filename);
    int rv = writer.open(filename);
    if (rv < 0) {
        LOG_ERROR("writer.open(%s) returned %d", filename, rv);
    }
    else {
        LOG_INFO("Serializing to %s", filename);
        google::protobuf::io::CodedOutputStream tempStream(writer.getOutputStream());
        input.SerializeToCodedStream(&tempStream);
    }
}

LPVOID CreateStartupParams(HANDLE *phMapping, size_t maxSize)
{
    LPVOID psp = NULL;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    HANDLE hMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
                                        &sa, PAGE_READWRITE, 0,
                                        maxSize, NULL);
    if (hMapping) {
        psp = (LPVOID)MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, 0);
        if (!psp) {
            printf("MapViewOfFile failed, %u\n", GetLastError());
            CloseHandle(hMapping);
            hMapping = NULL;
        }
        else
            printf("MapViewOfFile succeeded\n");
    }
    else
        printf("CreateFileMapping failed, %u\n", GetLastError());

    *phMapping = hMapping;
    return psp;
}
#endif

std::string  getCcdDataPath()
{
    char ccdPath[MAX_PATH+1] = {0};
    if (::SHGetSpecialFolderPathA(NULL, ccdPath, CSIDL_COMMON_APPDATA, FALSE)) {
        strcat_s(ccdPath, sizeof(ccdPath), "\\acer\\CCD");
        BOOL rt = ::MakeSureDirectoryPathExists(ccdPath);
        if (!rt) {
            LOG_ERROR("Failed to find CCD data path, error = %lu", GetLastError());
            return std::string("");
        }
    }

    return std::string(ccdPath);
}

bool isCCDExist()
{
    
    PROCESSENTRY32 ppe = {0};
    ppe.dwSize = sizeof (PROCESSENTRY32);

    HANDLE hSnapShot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    if (Process32First (hSnapShot, &ppe))
    {
        do
        {
            if (_tcsicmp (_T("ccd.exe"), ppe.szExeFile) == 0)
            {
                return true;
            }
        }while (Process32Next (hSnapShot, &ppe));
    }
    CloseHandle (hSnapShot);
    return false;

}

int getUserProfileImagePath(TCHAR* osUserId, TCHAR* profileImagePath)
{
    TCHAR keyPath[256];
    HKEY key;
    LONG errCode = ERROR_SUCCESS;

    std::string userId(osUserId, osUserId + wcslen(osUserId));
    LOG_INFO("osUserId: %s", userId.c_str());

    _snwprintf(keyPath, ARRAY_SIZE_IN_BYTES(keyPath),
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s", osUserId);
    if ((errCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &key)) != ERROR_SUCCESS) {
        std::string log(keyPath, keyPath + wcslen(keyPath));
        LOG_ERROR("RegOpenKey %s error: %d", log.c_str(), errCode);
        return errCode;
    }

    DWORD length = 1024;
    errCode = RegGetValueW(key, NULL, L"ProfileImagePath", RRF_RT_REG_SZ, NULL, profileImagePath, &length);
    if (errCode != ERROR_SUCCESS) {
        std::string log(keyPath, keyPath + wcslen(keyPath));
        LOG_ERROR("Failed to get ProfileImagePath from \"%s\": %d", log.c_str(), errCode);
        return errCode;
    }

    std::string path(profileImagePath, profileImagePath + wcslen(profileImagePath));
    LOG_INFO("User Image Path: %s", path.c_str());

    return ERROR_SUCCESS;
}

#ifdef   ADD_CCD_TO_WINDOWS_DEFENDER_EXCLUSION_LIST
/*
Because msmpEng.exe consumes much CPU resouce if CCD.exe executes on acer Iconia W3, we add this to 
Windows defender exclusion list to ease the symptom. 
*/
LONG addExcludeList2WindowsDefenderRegistry(TCHAR *ccdPath, TCHAR *logPath)
{
    std::list<RegValue> RegValues;
    RegValue    regData;
    LONG    rv = ERROR_SUCCESS;
    list<RegValue>::iterator it;
    bool    itemExist = false;
    TCHAR*    xProcessesSubKey = _T("SOFTWARE\\MicroSoft\\Windows Defender\\Exclusions\\Processes");
    TCHAR*    xPathsSubKey = _T("SOFTWARE\\MicroSoft\\Windows Defender\\Exclusions\\Paths");

    if (ccdPath) {
        itemExist = false;
        RegValues.clear();
        rv = getRegistryValues(HKEY_LOCAL_MACHINE, xProcessesSubKey, RegValues);

        if (ERROR_SUCCESS == rv || ERROR_FILE_NOT_FOUND == rv) {

            for (it = RegValues.begin(); it != RegValues.end(); it++) {
                if (_tcsstr((*it).name, _T("ccd.exe"))) {
                    itemExist = true;
                    break;
                }
            }

            if (!itemExist) {
                memset(regData.data, 0, sizeof(regData.data));
                regData.type = REG_DWORD;
                regData.size = sizeof(DWORD);
                _stprintf((TCHAR*)regData.name, ccdPath);
                RegValues.push_back(regData);
                rv = writeRegistryValues(HKEY_LOCAL_MACHINE, xProcessesSubKey, RegValues);
                if (ERROR_SUCCESS != rv) {
                    LOG_ERROR("Add Registry Value to Windows Defender Exclusion Processes failed! rv=%d", rv);
                }
            }

        } else {
            LOG_ERROR("Open Registry Key to Windows Defender Exclusion Processes failed! rv=%d", rv);
        }
    }

    if (logPath) {
        itemExist = false;
        RegValues.clear();
        rv = getRegistryValues(HKEY_LOCAL_MACHINE, xPathsSubKey, RegValues);

        if (ERROR_SUCCESS == rv || ERROR_FILE_NOT_FOUND == rv) {

            for (it = RegValues.begin(); it != RegValues.end(); it++) {
                if (_tcsstr((*it).name, logPath)) {
                    itemExist = true;
                    break;
                }
            }

            if (!itemExist) {
                memset(regData.data, 0, sizeof(regData.data));
                regData.type = REG_DWORD;
                regData.size = sizeof(DWORD);
                _stprintf((TCHAR*)regData.name, logPath);
                RegValues.push_back(regData);
                rv = writeRegistryValues(HKEY_LOCAL_MACHINE, xPathsSubKey, RegValues);
                if (ERROR_SUCCESS != rv) {
                    LOG_ERROR("Add Registry Value to Windows Defender Exclusion Paths failed! rv=%d", rv);
                }
            }

        } else {
            LOG_ERROR("Open Registry Key to Windows Defender Exclusion Paths failed! rv=%d", rv);
        }
    }

    return rv;

}

#endif
