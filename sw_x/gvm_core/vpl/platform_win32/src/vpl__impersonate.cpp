// defines and includes for generic function names to map to unicode versions
// http://msdn.microsoft.com/en-us/library/dd374107%28v=VS.85%29.aspx
// http://msdn.microsoft.com/en-us/library/dd374061%28v=VS.85%29.aspx
#ifndef UNICODE
# define UNICODE
#endif
#ifndef _UNICODE
# define _UNICODE
#endif

#include "vplu_mutex_autolock.hpp"
#include "vpl_lazy_init.h"
#include "vpl__impersonate.hpp"
#include "vplex_private.h"
#include "vplu_sstr.hpp"
#include <map>
#include <windows.h>
#include <TlHelp32.h>
#pragma comment(lib, "advapi32.lib")

static std::map<DWORD, bool> threadIds; //record if the thread is already impersonated.
static VPLLazyInitMutex_t impersonateMutex = VPLLAZYINITMUTEX_INIT;

#define DBG_OUTPUT(msg, ...) { \
    WCHAR msgbuf[1024]; \
    wsprintf(msgbuf, msg,   ##__VA_ARGS__); \
    OutputDebugString(msgbuf); \
}

//http://msdn.microsoft.com/en-us/library/windows/desktop/aa446670(v=vs.85).aspx
int getLogonSID (HANDLE hToken, PSID *ppsid) 
{
    int rv = VPL_OK;
    DWORD dwLength = 0;
    PTOKEN_GROUPS pTokenGrp = NULL;
    DWORD dwErr;

    if (NULL == ppsid)
        goto end;

    if (0 == GetTokenInformation(hToken, TokenGroups, (LPVOID) pTokenGrp, 0, &dwLength)) {
        //failed!
        dwErr  = GetLastError();
        if (dwErr != ERROR_INSUFFICIENT_BUFFER) {
            rv = VPLError_XlatWinErrno(dwErr);
            DBG_OUTPUT(L"GetTokenInformation Failed err=%u threadId=%X\n", dwErr, GetCurrentThread());
            //VPL_LIB_LOG_ERR(VPL_SG_FS, "GetTokenInformation Failed! err="FMTu32" rv=%d", dwErr, rv);
            goto end;
        }

        pTokenGrp = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);

        if (pTokenGrp == NULL) {
            //VPL_LIB_LOG_INFO(VPL_SG_FS, "HeapAlloc Failed!");
            DBG_OUTPUT(L"HeapAlloc Failed Failed err=%u threadId=%X\n", GetLastError(), GetCurrentThread());
            rv = VPL_ERR_NOMEM;
            goto end;
        }
    }

    if (0 == GetTokenInformation(hToken, TokenGroups, (LPVOID) pTokenGrp, dwLength, &dwLength)) {
        //failed!
        dwErr  = GetLastError();
        rv = VPLError_XlatWinErrno(dwErr);
        //VPL_LIB_LOG_ERR(VPL_SG_FS, "GetTokenInformation Failed! err="FMTu32" rv=%d", dwErr, rv);
        DBG_OUTPUT(L"GetTokenInformation Failed Failed err=%u threadId=%X\n", dwErr, GetCurrentThread());
        goto end;
    }

    //VPL_LIB_LOG_INFO(VPL_SG_FS, "got token group!");

    for (DWORD i = 0; i < pTokenGrp->GroupCount; i++) {

        if ((pTokenGrp->Groups[i].Attributes & SE_GROUP_LOGON_ID) ==  SE_GROUP_LOGON_ID) {
            // Found the logon SID; make a copy of it.
            dwLength = GetLengthSid(pTokenGrp->Groups[i].Sid);
            *ppsid = (PSID) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
            if (*ppsid == NULL) {
                //VPL_LIB_LOG_ERR(VPL_SG_FS, "HeapAlloc Failed! ");
                DBG_OUTPUT(L"HeapAlloc Failed Failed err=%u threadId=%X\n", GetLastError(), GetCurrentThread());
                rv = VPL_ERR_NOMEM;
                goto end;
            }
            if (0 == CopySid(dwLength, *ppsid, pTokenGrp->Groups[i].Sid)) {
                dwErr  = GetLastError();
                HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
                rv = VPLError_XlatWinErrno(dwErr);
                //VPL_LIB_LOG_ERR(VPL_SG_FS, "CopySid Failed! err="FMTu32" rv=%d", dwErr, rv);
                //OutputDebugString(L"CopySid Failed ");
                DBG_OUTPUT(L"CopySid Failed Failed err=%u threadId=%X\n", dwErr, GetCurrentThread());

                goto end;
            }
            break;
        }
   }
   rv = VPL_OK;

end: 
   if (pTokenGrp != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)pTokenGrp);

   return rv;
}

void freeLogonSID (PSID *ppsid)
{
    HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
}

HANDLE getProcessByName(PCWSTR name)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hProcess = NULL;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (wcsicmp(entry.szExeFile,name) == 0)
            {  
                hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
                break;
            }
        }
    }

    CloseHandle(snapshot);

    return hProcess;
}

int vpl_impersonatedLoggedOnUser()
{
    int rv = VPL_OK;
    int rc = 0;

    DWORD curThreadId = GetCurrentThreadId();
    static std::map<DWORD, bool>::iterator itr;

    MutexAutoLock autolock(::VPLLazyInitMutex_GetMutex(&impersonateMutex));
    itr = threadIds.find(curThreadId);
    if (itr != threadIds.end()) {
        return rv;
    }

    HANDLE hTarget = NULL;
    HANDLE hLogonUserToken;
    PSID psid;
    hTarget = getProcessByName(L"explorer.exe");

    if (hTarget == NULL) {
        //VPL_LIB_LOG_ERR(VPL_SG_FS, "Target process is absent!");
        DBG_OUTPUT(L"Target Failed Failed  threadId=%X\n",  GetCurrentThread());
        rv = VPL_ERR_NOENT;
        goto end;
    }

    if (0 == OpenProcessToken( hTarget, TOKEN_ALL_ACCESS, &hLogonUserToken )){
        DWORD dwErr = GetLastError();
        rv = VPLError_XlatWinErrno(dwErr);
        //VPL_LIB_LOG_ERR(VPL_SG_FS, "OpenProcessToken Failed! err="FMTu32" rv=%d", dwErr, rv);
        DBG_OUTPUT(L"OpenProcessToken Failed err=%u threadId=%X\n", dwErr, GetCurrentThread());
        goto open_process_failed;
    }

    rc = getLogonSID(hLogonUserToken, &psid);
    if (rc != VPL_OK) {
        //VPL_LIB_LOG_ERR(VPL_SG_FS, "getLogonSID Failed! rc=%d",rc);
        DBG_OUTPUT(L"getLogonSID Failed threadId=%X\n",  GetCurrentThread());
        rv = rc;
    } else {
        if(ImpersonateLoggedOnUser(hLogonUserToken)){
            rv = VPL_OK;
            //VPL_LIB_LOG_INFO(VPL_SG_FS, "ThreadId: "FMTu32, curThreadId);
            DBG_OUTPUT(L"ImpersonateLoggedOnUser Done! threadId=%X\n",  curThreadId);
            threadIds[curThreadId] = true;
        }else{
            DWORD dwErr = GetLastError();
            rv = VPLError_XlatWinErrno(dwErr);
            //VPL_LIB_LOG_ERR(VPL_SG_FS, "ImpersonateLoggedOnUser Failed! err="FMTu32" rv=%d", dwErr, rv);
            DBG_OUTPUT(L"ImpersonateLoggedOnUser failed! err=%u threadId=%X\n",  dwErr, curThreadId);
        }
    }

    freeLogonSID(&psid);
open_process_failed:
    CloseHandle(hTarget);
end:
    return rv;
}

int vpl_revertToSelf()
{

    DWORD curThreadId = GetCurrentThreadId();
    static std::map<DWORD, bool>::iterator itr;
    MutexAutoLock autolock(::VPLLazyInitMutex_GetMutex(&impersonateMutex));

    itr = threadIds.find(curThreadId);
    if (itr == threadIds.end()) //Not in the impersonated therad list.
        return VPL_OK;

    if (RevertToSelf()) {
        threadIds.erase(itr);
        //VPL_LIB_LOG_INFO(VPL_SG_FS, "ThreadId: "FMTu32, curThreadId);
        DBG_OUTPUT(L"Revert Done! threadId=%X\n",  curThreadId);
        return VPL_OK;
    } else {
        ;//VPL_LIB_LOG_ERR(VPL_SG_FS, "revertToSelf Failed! threadId: "FMTu32" err:"FMTu32, curThreadId, GetLastError());
        DBG_OUTPUT(L"Revert failed! err=%u threadId=%X\n",  GetLastError(), curThreadId);
    }
    return VPL_ERR_FAIL;
}

