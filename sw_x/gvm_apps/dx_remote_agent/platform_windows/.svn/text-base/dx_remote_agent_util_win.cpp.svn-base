#include <vplex_file.h>
#include <vpl_fs.h>
#include <dx_remote_agent_util.h>
#include "dx_remote_agent_util_win.h"
#include <string>
#include <Shlobj.h>
#include <log.h>
#include <sstream>
#include <TlHelp32.h>
#include <set>
#include <iphlpapi.h>
#include <Shellapi.h>
#include <fstream>
#include <iostream>
#include "../common_utils.hpp"
#include <gvm_file_utils.h>
#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <sddl.h> // For SID conversion
#include <scopeguard.hpp>

#ifdef VPL_PLAT_IS_WIN_DESKTOP_MODE
#include <Sddl.h>
#include "CCDMonSrvClient.h"
#endif // VPL_PLAT_IS_WIN_DESKTOP_MODE

static std::string dx_ccd_name = "";

int setRegistry()
{
    //Set HKLM\SOFTWARE\OEM\AcerCloud\InstallPath to current dir
    const char *HKEY_PREFIX = "SOFTWARE\\OEM\\AcerCloud\\";
    HKEY hKey = NULL;
    
    DWORD dwType = REG_SZ;
    DWORD dwDataLen = MAX_PATH;
    char  szDataBuf[MAX_PATH+1] = {0};

    wchar_t curPath[MAX_PATH_UNICODE+2] = {0};
    DWORD lenDataBuf = 0;
    lenDataBuf = GetCurrentDirectory(MAX_PATH_UNICODE, curPath);
    if(lenDataBuf == 0 || lenDataBuf > MAX_PATH_UNICODE){
        LOG_ERROR("Fail to get current dir, %d", lenDataBuf);
        return -1;
    }else{
        curPath[lenDataBuf] = L'\\';
    }

    if(ERROR_SUCCESS != RegCreateKeyA(HKEY_LOCAL_MACHINE, HKEY_PREFIX, &hKey)){
        LOG_ERROR("Fail to Create/Open registry %s", HKEY_PREFIX);
        return -1;
    }
    ON_BLOCK_EXIT(RegCloseKey, hKey);

    if(ERROR_SUCCESS != RegSetKeyValue(hKey, NULL, L"InstallPath", REG_SZ, curPath, wcslen(curPath)*sizeof(WCHAR))){
        LOG_ERROR("Fail to set registry %sInstallPath", HKEY_PREFIX);
        return -1;
    }

    if(ERROR_SUCCESS != RegQueryValueExA(hKey, "InstallPath", 0, &dwType, (BYTE *)szDataBuf, &dwDataLen)){
        LOG_ERROR("Fail to query registry %sInstallPath", HKEY_PREFIX);
        return -1;
    }

    LOG_INFO("Registry %sInstallPath: %s", HKEY_PREFIX, szDataBuf);

    return 0;
}

int startccd(const char* titleId)
{
    int rv = 0;
    #ifdef WIN32
    //setup CCDWin32StartParams for ccd
    ccd::CCDWin32StartParams param;

    HANDLE hASM = NULL;
    LPVOID data = NULL;

    do{
        HANDLE hToken = NULL;
        BOOL bResult = TRUE;

        bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
        if (!bResult) {
            LOG_ERROR("OpenProcessToken failed");
            break;
        }

        DWORD dwSize = 0;
        // Get token of the user
        bResult = GetTokenInformation(hToken, TokenUser, NULL, dwSize, &dwSize);
        if (!bResult) {
            DWORD rt = GetLastError();
            if (rt != ERROR_INSUFFICIENT_BUFFER) {
                LOG_ERROR("GetTokenInformation user get size, error: %u\n", rt);
                break;
            }
        }

        PTOKEN_USER pTokenUserInfo = (PTOKEN_USER) GlobalAlloc(GPTR, dwSize);
        bResult = GetTokenInformation(hToken, TokenUser, pTokenUserInfo, dwSize, &dwSize);
        if (!bResult) {
            LOG_ERROR("GetTokenInformation user, error: %d\n", GetLastError());
            break;
        }else{
            //handle user
            //CCDMonSrv::TRUSTEE* trustee = input.add_trustees();
            LPSTR userSid=NULL;
            if(!ConvertSidToStringSidA(pTokenUserInfo->User.Sid, &userSid)){
                LOG_ERROR("Failed to convert SID to string: %lu", GetLastError());
                break;
            }
            ON_BLOCK_EXIT(LocalFree, userSid);

            ccd::TrusteeItem* trustee = param.add_trustees();
            trustee->set_sid(userSid);
            trustee->set_attr(pTokenUserInfo->User.Attributes);
            LOG_INFO("userSid: %s", userSid);
        }

        // Get token of the groups
        dwSize = 0;
        bResult = GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize);
        if (!bResult) {
            DWORD rt = GetLastError();
            if (rt != ERROR_INSUFFICIENT_BUFFER) {
                LOG_ERROR("GetTokenInformation groups get size, error: %u\n", rt);
                break;
            }
        }
        PTOKEN_GROUPS pTokenGroupInfo = (PTOKEN_GROUPS) GlobalAlloc(GPTR, dwSize);
        bResult = GetTokenInformation(hToken, TokenGroups, pTokenGroupInfo, dwSize, &dwSize);
        if (!bResult) {
            LOG_ERROR("GetTokenInformation groups, error: %d\n", GetLastError());
            break;
        }
        else {
            //handle group

            PSID pSIDadmin = NULL;
            PSID pSIDauth  = NULL;
            PSID pSIDserv  = NULL;  //service
            SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
            
            //Get authenticated user group sid
            if(! AllocateAndInitializeSid( &SIDAuth, 2,
                        SECURITY_BUILTIN_DOMAIN_RID,
                        DOMAIN_ALIAS_RID_ADMINS,
                        0, 0, 0, 0, 0, 0,
                        &pSIDadmin) ) 
            {
                LOG_ERROR("AllocateAndInitializeSid for admin Error %u\n", GetLastError() );
                break;
            }
            ON_BLOCK_EXIT(FreeSid, pSIDadmin);

            //Get admin group sid
            if(! AllocateAndInitializeSid( &SIDAuth, 1,
                        SECURITY_AUTHENTICATED_USER_RID,
                        0,
                        0, 0, 0, 0, 0, 0,
                        &pSIDauth) ) 
            {
                LOG_ERROR("AllocateAndInitializeSid for auth Error %u\n", GetLastError() );
                break;
            }
            ON_BLOCK_EXIT(FreeSid, pSIDauth);

            //Get service group sid
            if(! AllocateAndInitializeSid( &SIDAuth, 1,
                        SECURITY_SERVICE_RID,
                        0,
                        0, 0, 0, 0, 0, 0,
                        &pSIDserv) ) 
            {
                LOG_ERROR("AllocateAndInitializeSid for service Error %u\n", GetLastError() );
                break;
            }
            ON_BLOCK_EXIT(FreeSid, pSIDserv);

            //search for authenticated user and admin group, skip them, so ccd won't get/have those privilege
            for (int i=0; i<(int)pTokenGroupInfo->GroupCount; i++) {
                if(EqualSid(pSIDadmin, pTokenGroupInfo->Groups[i].Sid)){
                    LOG_INFO("Admin Group found!");
                    continue;
                }
                if(EqualSid(pSIDauth, pTokenGroupInfo->Groups[i].Sid)){
                    LOG_INFO("Auth Group found!");
                    continue;
                }
                if(EqualSid(pSIDserv, pTokenGroupInfo->Groups[i].Sid)){
                    LOG_INFO("Service Group found!");
                    continue;
                }

                LPSTR strSid = NULL;
                if(!ConvertSidToStringSidA(pTokenGroupInfo->Groups[i].Sid, &strSid)){
                    LOG_ERROR("Failed to convert SID to string: %lu", GetLastError());
                    continue;
                }
                ON_BLOCK_EXIT(LocalFree, strSid);
            
                ccd::TrusteeItem* trustee = param.add_trustees();
                trustee->set_sid(strSid);
                trustee->set_attr(pTokenGroupInfo->Groups[i].Attributes);
                LOG_INFO("Sid: %s", strSid);
                LOG_INFO("Group: %016llX", pTokenGroupInfo->Groups[i].Attributes);
            }
        }

    }while(0);

    uint32_t size = 0;
    //write the CCDWin32StartParams to shared memory(file mapping) for ccd
    if(param.trustees_size()){
        size = sizeof(uint8_t) * param.ByteSize();
        data = CreateStartupParams(&hASM, size);
        if(data){
            param.SerializeToArray(data, size);
            LOG_INFO("hASM:"FMT0xPTR" size: %d", hASM, size);
        }else{
            // data and hASM are NULL when failed
            // no need to do cleanup
            LOG_ERROR("CreateStartupParams failed");
        }
    }

    rv = _dx_remote_startCCDW(hASM, size, titleId);
    rv = waitCcd();

    // Unmap shared memory
    if(data){
        UnmapViewOfFile(data);
        data = NULL;
    }
    if(hASM){
        CloseHandle(hASM);
        hASM = NULL;
    }
    #endif
    return rv;
}

//Copy from start_ccd_by_srv
int startccd_by_srv(int testInstanceNum, const char* titleId)
{
    int rv = -1;
    HANDLE hToken = NULL;
    BOOL bResult = TRUE;
    DWORD dwSize = 0;
    LPSTR userSid=NULL;

    std::string ccdName = get_dx_ccd_name();


    CCDMonSrv_initClient();

    bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
    if (!bResult) {
        goto out;
    }
    {
        DWORD rt;
        CCDMonSrv::REQINPUT input;
        CCDMonSrv::REQOUTPUT output;

        // Get token of the user
        bResult = GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize);
        PTOKEN_USER pTokenUserInfo = (PTOKEN_USER) GlobalAlloc(GPTR, dwSize);
        bResult = GetTokenInformation(hToken, TokenUser, pTokenUserInfo, dwSize, &dwSize);
        if (!bResult)
            goto out;
        if (!ConvertSidToStringSidA(pTokenUserInfo->User.Sid, &userSid))
            goto out;

        {
            input.set_type(CCDMonSrv::NEW_CCD);
            input.set_sid(userSid);
            std::string path;
            //rv = getCcdAppDataPath(path);
            rv = get_cc_folder(path);
            if (rv != 0)
                goto out;

            input.set_localpath(path.c_str());
            input.set_instancenum(testInstanceNum);
            LOG_INFO("%s", input.DebugString().c_str());
            rt = ::CCDMonSrv_sendRequest(input, output);
            if (rt == 0) {
                switch(output.result()) {
                    case CCDMonSrv::SUCCESS:
                        LOG_INFO("success: %s", output.DebugString().c_str());
                        rv = 0;
                        break;
                    case CCDMonSrv::ERROR_CREATE_PROCESS:
                    case CCDMonSrv::ERROR_UNKNOWN:
                        LOG_ERROR("failed: %s", output.DebugString().c_str());
                        break;
                }
            }
            if (userSid != NULL)
                LocalFree(userSid);
        }
    }

out:
    return rv;
}

int startccd(int testInstanceNum, const char* titleId)
{
    int rv = 0;
    #ifdef WIN32
    std::wstring wCcdConfPath;
    char *ccdConfigPath = NULL;
    char *fileContentBuf = NULL;
    int fileSize = -1;
    std::string config;

    // 1. get ccd config path
    rv = _getCcdAppDataWPathByKnownFolder(wCcdConfPath);
    if (rv != 0) {
        LOG_ERROR("Get local appdata wpath failed (%d).", rv);
        goto err;
    }
    wCcdConfPath.append(L"\\conf\\ccd.conf");
    rv = _VPL__wstring_to_utf8_alloc(wCcdConfPath.c_str(), &ccdConfigPath);
    if (rv != 0) {
        LOG_ERROR("Fail to convert local appdata wpath to utf8 (%d).", rv);
        goto err;
    }
    // XXX convert \\ to / ?

    // 2. open the ccd config file
    rv = VPLFile_CheckAccess(ccdConfigPath, VPLFILE_CHECKACCESS_EXISTS);
    if (rv != VPL_OK) {
        LOG_ERROR("Config file %s doesn't exist! rv = %d", ccdConfigPath, rv);
        goto err;
    }

    //read conf
    fileSize = Util_ReadFile(ccdConfigPath, (void**)&fileContentBuf, 0);
    if (fileSize < 0) {
        LOG_ERROR("Fail to read %s, rv = %d", ccdConfigPath, fileSize);
        rv = fileSize;
        goto err;
    }
    config.assign(fileContentBuf, fileSize);

    // 3. update testInstanceNum to the ccd config
    {
        std::stringstream ss;
        ss << testInstanceNum;
        LOG_INFO("Updating testInstanceNum to %d", testInstanceNum);
        updateConfig(config, "testInstanceNum", ss.str());
    }

    // 4. write back to the config file
    rv = Util_WriteFile(ccdConfigPath, config.data(), config.size());
    if (rv != VPL_OK) {
        LOG_ERROR("Fail to writeback config to %s, rv = %d", ccdConfigPath, rv);
        goto err;
    }

    setRegistry();

    CCDIClient_SetTestInstanceNum(testInstanceNum);
    // call startccd() at the end
    startccd_by_srv(testInstanceNum, titleId);

 err:
    if (ccdConfigPath != NULL) {
        free(ccdConfigPath);
        ccdConfigPath = NULL;
    }
    if (fileContentBuf != NULL) {
        free(fileContentBuf);
        fileContentBuf = NULL;
    }
    #endif
    return rv;
}

//refer stop_ccd_by_srv()
int stopccd_by_srv(int testInstanceNum)
{
    int rv = -1;
    HANDLE hToken = NULL;
    BOOL bResult = TRUE;
    DWORD dwSize = 0;
    LPSTR userSid=NULL;

    CCDMonSrv_initClient();

    bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);
    if (!bResult) {
        goto out;
    }
    {
        DWORD rt;
        CCDMonSrv::REQINPUT input;
        CCDMonSrv::REQOUTPUT output;

        // Get token of the user
        bResult = GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize);
        PTOKEN_USER pTokenUserInfo = (PTOKEN_USER) GlobalAlloc(GPTR, dwSize);
        bResult = GetTokenInformation(hToken, TokenUser, pTokenUserInfo, dwSize, &dwSize);
        if (!bResult)
            goto out;
        if (!ConvertSidToStringSidA(pTokenUserInfo->User.Sid, &userSid))
            goto out;

        input.set_type(CCDMonSrv::CLOSE_CCD);
        input.set_sid(userSid);
        input.set_instancenum(testInstanceNum);
        LOG_INFO("%s", input.DebugString().c_str());
        rt= ::CCDMonSrv_sendRequest(input, output);
        if (rt == 0) {
            switch(output.result()) {
                case CCDMonSrv::SUCCESS:
                    LOG_INFO("%s", output.DebugString().c_str());
                    rv = 0;
                    break;
                case CCDMonSrv::ERROR_EMPTY_USER:
                case CCDMonSrv::ERROR_TERMINATE_PROCESS:
                case CCDMonSrv::ERROR_UNKNOWN:
                    LOG_ERROR("%s", output.DebugString().c_str());
                    break;
            }
        }
        if (userSid != NULL)
            LocalFree(userSid);
    }

out:
    return rv;

}

int stopccd(int testInstanceNum)
{
    return stopccd_by_srv(testInstanceNum);
}

int stopccd()
{
    int rv = 0;
    //rv = _shutdownprocess("CCD.exe");
    //_shutdownprocess("ccd.exe");
    //return rv;
    std::string ccdName = get_dx_ccd_name();
    if (!ccdName.empty() && is_sid(ccdName)) {
        ccdi::client::CCDIClient_SetOsUserId(ccdName.c_str());
    }
    ccd::UpdateSystemStateInput request;
    request.set_do_shutdown(true);
    ccd::UpdateSystemStateOutput response;
    rv = CCDIUpdateSystemState(request, response);

    // Need to send a second request to deal with limitation in our Windows impl.
    VPLThread_Sleep(VPLTime_FromMillisec(500));
    (IGNORE_RESULT)CCDIUpdateSystemState(request, response);

    return rv;
}

int _getLocalAppDataWPathByKnownFolder(std::wstring &wpath)
{
    int rc = 0;
    wchar_t *appDataWPath = NULL;
    do
    {
        if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &appDataWPath) != S_OK)
        {
            LOG_ERROR("Fail to determine LocalAppData path");
            rc = -1;
            break;
        }

        wpath.assign(appDataWPath);
    } while (false);

    if (appDataWPath != NULL) {
        CoTaskMemFree(appDataWPath);
        appDataWPath = NULL;
    }

    return rc;
}

int _getCcdAppDataWPathByKnownFolder(std::wstring &wpath)
{
    int rc = 0;
    std::wstringstream wss;
    std::wstring wpathbuf;
    do
    {
        rc = _getLocalAppDataWPathByKnownFolder(wpathbuf);
        if (rc != 0)
            break;
        wss << wpathbuf << L"\\iGware";
        CreateDirectory(wss.str().c_str(), NULL);

        std::wstring wstrCcdName = get_dx_ccd_nameW();
        wss << L"\\SyncAgent";
        if (!wstrCcdName.empty()) {
            wss << L"_" << get_dx_ccd_nameW();
        }
        CreateDirectory(wss.str().c_str(), NULL);

        wpath.assign(wss.str());
        wss.str(L"");
        wss.clear();
    } while (false);

    return rc;
}

int _startprocessW(const std::wstring& command)
{
    STARTUPINFO si;
    SECURITY_ATTRIBUTES saAttr;
    int rv = 0;
    PROCESS_INFORMATION pi;
    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
    {
        LOG_ERROR("Fail StdoutRd CreatePipe");
    }

    if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    {
        LOG_ERROR("Fail Stdout SetHandleInformation");
    }

     ZeroMemory(&si, sizeof(si));
     ZeroMemory(&pi, sizeof(pi));

     DWORD processCreationFlags = 0;
     processCreationFlags = CREATE_NEW_CONSOLE;
     // set bInheritHandles  to true bcz we are using shared memory (file mapping)
     if (!CreateProcess(NULL, (LPTSTR)command.c_str(), NULL, NULL, TRUE /*bInheritHandles*/, processCreationFlags, NULL, NULL, &si, &pi))
     {
         LOG_ERROR("CreateProcess failed (%d).", GetLastError());
         rv = -1;
     }

     return rv;
}

int _dx_remote_startCCDW(const HANDLE hASM, const uint32_t szASM, const char* titleId)
{
    int rc = 0;
    std::wstring wAppPath;
    std::wstringstream wssCmd;
    wchar_t *ccd_dir = NULL;
    size_t requiredSize = 0;
    wchar_t currentFilePath[32768];
    GetModuleFileName(NULL, currentFilePath, 32767);
    std::wstring wstrCurrentFilePath(currentFilePath);
    wstrCurrentFilePath = wstrCurrentFilePath.substr(0, wstrCurrentFilePath.find_last_of(L'\\'));

    do
    {
        rc = _getCcdAppDataWPathByKnownFolder(wAppPath);
        if (rc != 0)
        {
            break;
        }


        _wgetenv_s(&requiredSize, NULL, 0, L"DX_CCD_EXE_PATH");
        if (requiredSize != 0)
        {
            ccd_dir = new wchar_t[requiredSize];
            _wgetenv_s(&requiredSize, ccd_dir, requiredSize, L"DX_CCD_EXE_PATH");
        }

        if (ccd_dir != NULL)
        {
            wssCmd << ccd_dir;
            delete[] ccd_dir;
            ccd_dir = NULL;
        }
        else
        {
            wssCmd << wstrCurrentFilePath;
        }

        wssCmd << L"\\ccd.exe";

        if (!wAppPath.empty()) {
            wssCmd << L" \"" << wAppPath << L"\"";
        } else {
            wssCmd << L" \"\"";
        }
        std::string str_tmp_ccd_name = get_dx_ccd_name();
        if (!str_tmp_ccd_name.empty() && is_sid(str_tmp_ccd_name)) {
            std::wstring tmp_ccd_name = get_dx_ccd_nameW();
            if (!tmp_ccd_name.empty()) {
                wssCmd << L" \"" << tmp_ccd_name << L"\"";
            } else {
                wssCmd << L" \"\"";
            }
        } else {
            wssCmd << L" \"\"";
        }

        //setup the arg3/arg4 with shared mem handle and size
        std::ostringstream strHandle;
        if(hASM && szASM){
            LOG_INFO("hASM:"FMT0xPTR, hASM);
            strHandle << (INT64)(INT_PTR)hASM << " " << szASM;
            LOG_INFO("strHandle: %s", strHandle.str().c_str());
            //argv[3], shared mem handle
            //argv[4], shared mem size
            std::string args = " " + strHandle.str();
            std::wstring wargs(args.begin(), args.end());
            wssCmd << wargs;
            //argv[5], netman, which we don't care
            wssCmd << L" \"\"";
        }else{
            wssCmd << L"  \"\"  \"\"  \"\""; //argv[3], argv[4], argv[5] 
        }

        if (titleId != NULL){
            std::string titleIdString(titleId);
            std::wstring w_titleId(titleIdString.begin(), titleIdString.end());
            wssCmd << L" \"" << w_titleId << L"\"";
        } else {
            wssCmd << L" \"\"";
        }

        //print out cmd, just for debugging 
        char *cmd;
        int rv = _VPL__wstring_to_utf8_alloc(wssCmd.str().c_str(), &cmd);
        if (rv != 0) {
            LOG_ERROR("Fail to convert local appdata wpath to utf8 (%d).", rv);
        }else{
            LOG_INFO("Cmd: %s", cmd);
            free(cmd);
        }

        rc = _startprocessW(wssCmd.str());
        wssCmd.str(L"");
        wssCmd.clear();
    } while (false);

    return rc;
}

int _shutdownprocessW(std::wstring procName)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    DWORD reValue = 0;
    DWORD errorCodes = 0;
    int retryCount = 0;
    int ret = 0;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE) {
        while (Process32Next(snapshot, &entry) == TRUE) {
            if (_wcsicmp(entry.szExeFile, procName.c_str()) == 0) {
                HANDLE hHandle;
                DWORD dwExitCode = 0;
                hHandle = ::OpenProcess(PROCESS_ALL_ACCESS, 0, entry.th32ProcessID);

                ::GetExitCodeProcess(hHandle, &dwExitCode);
                ::TerminateProcess(hHandle, dwExitCode);

                do {
                    reValue = ::WaitForSingleObject(hHandle, 5000);
                    if(reValue == WAIT_OBJECT_0) {
                        break;
                    } else if (reValue == WAIT_TIMEOUT) {
                        retryCount++;
                        if(retryCount == 5) {
                            ret = -1;
                        }
                    } else if (reValue == WAIT_FAILED) {
                        errorCodes = GetLastError();
                        LOG_ERROR("errorCodes = "FMT0x32" ",errorCodes);
                        ret = -1;
                        break;
                    } else if (reValue == WAIT_ABANDONED) {
                        LOG_ERROR("Object not release the thread!!!");
                        ret = -1;
                        break;
                    }
                } while (retryCount < 5); // Reach 25s timeout or 5 times WAIT_TIMEOUT, so give up
                CloseHandle(hHandle);
            }
        }
    }
    CloseHandle(snapshot);
    return ret;
}

unsigned short getFreePort()
{
    // Just use port 0 for autoselect.
    return 0;
}

int recordPortUsed(unsigned short port) 
{
    int rv = 0;
    char szModule[MAX_PATH];

    GetModuleFileNameA(NULL, szModule, MAX_PATH);
    std::string strModule(szModule);
    std::string portFile = strModule.substr(0, strModule.find_last_of('\\') + 1);
    portFile.append("port");
    std::string ccdName = get_dx_ccd_name();
    if (ccdName.length() > 0) {
        portFile.append("_");
        portFile.append(ccdName);
    }

    portFile.append(".txt");
    std::string portToBind;
    std::stringstream ss;
    ss << "port=";
    ss << port;
    portToBind = ss.str();
    ss.str(""); // clear buffer
    ss.clear(); // clear flag

    if (VPLFile_CheckAccess(portFile.c_str(), VPLFILE_CHECKACCESS_EXISTS) == VPL_OK) {
        // File found. Delete it.
        rv = VPLFile_Delete(portFile.c_str());
        if(rv != VPL_OK) {
            LOG_ERROR("Failed to delete %s (%d).", portFile.c_str(), rv);
            goto exit;
        }
    }

    VPLFile_handle_t fd;

    fd = VPLFile_Open(portFile.c_str(),
    VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY, 0777);
    if (! VPLFile_IsValidHandle(fd)) {
        rv = -1;
        LOG_ERROR("Failed to delete %s.", portFile.c_str());
        goto exit;
    }
    else {
        rv = VPLFile_Write(fd, portToBind.c_str(), portToBind.size());
        if(rv != portToBind.size()) {
            LOG_ERROR("Failed to write %s (%d).", portFile.c_str(), rv);
            goto exit;
        }
        else {
            rv = VPLFile_Close(fd);
            if (rv != VPL_OK) {
                LOG_ERROR("Failed to close %s (%d).", portFile.c_str(), rv);
                goto exit;
            }
        }
    }

 exit:
    return rv;
}

int startprocess(const std::string& command)
{
    int rc = 0;
    wchar_t *wCmd = NULL;
    std::wstring wstrCmd;
    do
    {
        rc = _VPL__utf8_to_wstring(command.c_str(), &wCmd);
        if (rc != 0)
            break;

        wstrCmd.assign(wCmd);
        rc = _startprocessW(wstrCmd);
    } while (false);

    if (wCmd != NULL)
    {
        free(wCmd);
        wCmd = NULL;
    }

    return rc;
}

int _shutdownprocess(std::string procName)
{
    int rc = 0;
    wchar_t *wProc = NULL;
    std::wstring wstrProc;
    do
    {
        rc = _VPL__utf8_to_wstring(procName.c_str(), &wProc);
        if (rc != 0)
            break;

        wstrProc.assign(wProc);
        rc = _shutdownprocessW(wstrProc);
    } while (false);

    if (wProc != NULL)
    {
        free(wProc);
        wProc = NULL;
    }

    return rc;
}

void registerToFirewall()
{
    HRESULT hr;
    BSTR bstrRuleName = SysAllocString(L"dx_remote_agent");
    BSTR bstrRuleDescription = SysAllocString(L"Dxshell Remote Agent For Auto Testing");
    BSTR bstrRuleInterfaceType = SysAllocString(L"All");
    BSTR bstrRuleAddress = SysAllocString(L"*");
    BSTR bstrProgramName = NULL;
    BSTR bstrRuleGroup = SysAllocString(L"dx_remote_agent");
    INetFwPolicy2 *pNetFwPolicy2 = NULL;
    INetFwRules *pFwRules = NULL;
    INetFwRule *pFwRule = NULL;
    INetFwRule2 *pFwRule2 = NULL;
    wchar_t curExecPath[MAX_PATH_UNICODE];

    GetModuleFileName(NULL, curExecPath, MAX_PATH_UNICODE);

    do
    {
        if (GetModuleFileName(NULL, curExecPath, MAX_PATH_UNICODE) != S_OK)
            break;

        bstrProgramName = SysAllocString(curExecPath);

        hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
        if (hr != RPC_E_CHANGED_MODE && FAILED(hr))
            break;

        hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
        if (FAILED(hr))
            break;

        hr = pNetFwPolicy2->get_Rules(&pFwRules);
        if (FAILED(hr))
            break;

        hr = CoCreateInstance(__uuidof(NetFwRule), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pFwRule);
        if (FAILED(hr))
            break;

        hr = pFwRule->put_Name(bstrRuleName);
        hr = pFwRule->put_Grouping(bstrRuleGroup);
        hr = pFwRule->put_Description(bstrRuleDescription);
        hr = pFwRule->put_Direction(NET_FW_RULE_DIR_IN);
        hr = pFwRule->put_Action(NET_FW_ACTION_ALLOW);
        hr = pFwRule->put_ApplicationName(bstrProgramName);
        hr = pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
        hr = pFwRule->put_Profiles(NET_FW_PROFILE2_ALL);
        hr = pFwRule->put_Enabled(VARIANT_TRUE);
        hr = pFwRule->put_InterfaceTypes(bstrRuleInterfaceType);
        hr = pFwRule->put_LocalAddresses(bstrRuleAddress);
        hr = pFwRule->put_RemoteAddresses(bstrRuleAddress);

        hr = pFwRules->QueryInterface(__uuidof(INetFwRule2), (void**)&pFwRule2);
        if (SUCCEEDED(hr))
        {
            pFwRule2->put_EdgeTraversalOptions(NET_FW_EDGE_TRAVERSAL_TYPE_ALLOW);
            if (FAILED(hr)) {
                break;
            }
        }
        else
        {
            hr = pFwRule->put_EdgeTraversal(VARIANT_TRUE);
            if (FAILED(hr)) {
                break;
            }
        }

        pFwRules->Remove(bstrRuleName);

        hr = pFwRules->Add(pFwRule);
        if (FAILED(hr))
            break;

    } while (false);

    if (pFwRule != NULL) {
        pFwRule->Release();
        pFwRule = NULL;
    }

    if (pFwRule2 != NULL) {
        pFwRule2->Release();
        pFwRule2 = NULL;
    }

    if (pNetFwPolicy2 != NULL) {
        pNetFwPolicy2->Release();
        pNetFwPolicy2 = NULL;
    }

    if (bstrProgramName != NULL) {
        SysFreeString(bstrProgramName);
        bstrProgramName = NULL;
    }
    SysFreeString(bstrRuleGroup);
    SysFreeString(bstrRuleName);
    SysFreeString(bstrRuleDescription);
    SysFreeString(bstrRuleInterfaceType);
    SysFreeString(bstrRuleAddress);

    CoUninitialize();
}

int get_connected_android_device_ip(std::string &ipAddr)
{
    int rc = 0;

    doSystemCall(".\\AdbTool\\adb.exe shell ifconfig wlan0 > ipAndroid.txt");

    std::string line;
    std::string ipToken("ip ");
    std::string maskToken(" mask");
    std::string wlanToken("wlan0");
    std::ifstream myfile("./ipAndroid.txt");
    int wlan0Idx = 0;
    rc = 0;

    do
    {
        if (!myfile.is_open()) {
            rc = -1;
            break;
        }

        while (myfile.good())
        {
            std::getline(myfile,line);
            wlan0Idx = line.find(wlanToken);
            if (wlan0Idx != std::string::npos)
                break;
        }

        myfile.close();

        if (wlan0Idx == std::string::npos) {
            rc = -1;
            break;
        }

        int ipIdx = line.find(ipToken);
        if (ipIdx == std::string::npos) {
            rc = -1;
            break;
        }
        int maskIdx = line.find(maskToken);
        if (maskIdx == std::string::npos) {
            rc = -1;
            break;
        }

        std::string strIpAddr("");
        strIpAddr = line.substr(ipIdx + ipToken.size(), maskIdx - ipIdx - ipToken.size());
        std::string strPing = ("ping ") + strIpAddr + (" > pingResult.txt");
        doSystemCall(strPing.c_str());

        int iRequestTimedOut = 0;
        int iTimeOutCount = 0;
        int iLostAll = 0;
        int iNoHost = 0;
        int iBadHost = 0;
        std::ifstream mypingfile ("./pingResult.txt");
        if (!mypingfile.is_open()) {
            rc = -1;
            break;
        }

        std::string TimeOutToken("Request timed out.");
        std::string LostAllToken("Packets: Sent = 4, Received = 0, Lost = 4");
        std::string NoHostToken("Ping request could not find host");
        std::string BadHostToken("Destination host unreachable");

        while (mypingfile.good())
        {
            std::getline(mypingfile,line);

            iRequestTimedOut = line.find(BadHostToken);
            if (iRequestTimedOut != std::string::npos) {
                iBadHost++;
                BadHostToken.append("\n");
                rc = -1;
                break;
            }

            iRequestTimedOut = line.find(NoHostToken);
            if (iRequestTimedOut != std::string::npos) {
                iNoHost++;
                NoHostToken.append("\n");
                rc = -1;
                break;
            }

            iRequestTimedOut = line.find(LostAllToken);
            if (iRequestTimedOut != std::string::npos) {
                iLostAll++;
                LostAllToken.append("\n");
                rc = -1;
                break;
            }
        }

    } while (false);

    return rc;
}

int clean_cc()
{
    int rc = 0;
    std::wstring ccPath;
    _getCcdAppDataWPathByKnownFolder(ccPath);
    ccPath.append(L"\\cc");

    wchar_t *dir = new wchar_t[ccPath.size() + 2];
    memset(dir, 0, (ccPath.length() + 2) * sizeof(wchar_t));
#ifdef WIN32
    memcpy_s(dir, (ccPath.length() + 2) * sizeof(wchar_t), ccPath.c_str(), ccPath.length() * sizeof(wchar_t));
#else
    std::copy(ccPath.begin(), ccPath.end(), dir);
#endif
    SHFILEOPSTRUCT file_op;
    file_op.hwnd = NULL;
    file_op.wFunc = FO_DELETE;
    file_op.pFrom = dir;
    file_op.pTo = NULL;
    file_op.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
    file_op.fAnyOperationsAborted = FALSE;
    file_op.lpszProgressTitle = NULL;
    file_op.hNameMappings = NULL;
    rc = SHFileOperation(&file_op);
    if (dir != NULL) {
        delete[] dir;
        dir = NULL;
    }

    if (rc == ERROR_FILE_NOT_FOUND || rc == ERROR_PATH_NOT_FOUND)
        rc = ERROR_SUCCESS;

    return rc;
}

int get_user_folderW(std::wstring &wpath)
{
    int rc = 0;
    std::wstring wstrPath;
    do
    {
        if ( (rc = _getLocalAppDataWPathByKnownFolder(wstrPath)) != VPL_OK) {
            break;
        }

        wstrPath.append(L"\\igware\\SyncAgent");
        std::wstring wstrCcdName = get_dx_ccd_nameW();
        if (wstrCcdName.size() > 0) {
            wstrPath.append(L"_");
            wstrPath.append(wstrCcdName);
        }
        wstrPath.append(L"\\dxshell_pushfiles");

        wpath = wstrPath;
    } while (false);

    return rc;
}

int get_user_folder(std::string &path)
{
    int rc = 0;
    std::wstring wstrPath;
    char *szPath = NULL;
    do
    {
        if ( (rc = get_user_folderW(wstrPath)) != 0) {
            break;
        }

        if ( (rc = _VPL__wstring_to_utf8_alloc(wstrPath.c_str(), &szPath)) != VPL_OK) {
            break;
        }

        path = std::string(szPath);

        //rc = VPLDir_Create(szPath, 0755);
        rc = Util_CreatePath(szPath, TRUE);
        if (rc == VPL_ERR_EXIST) {
            rc = VPL_OK;
        }
    } while (false);

    if (szPath) {
        free(szPath);
        szPath = NULL;
    }
    return rc;
}

int get_cc_folder(std::string &path)
{
    int rc = 0;
    char *szPath = NULL;
    std::wstring wstrPath;
    do
    {
        if ( (rc = _getLocalAppDataWPathByKnownFolder(wstrPath)) != VPL_OK) {
            break;
        }

        wstrPath.append(L"\\igware\\SyncAgent");
        std::wstring wstrCcdName = get_dx_ccd_nameW();
        if (wstrCcdName.size() > 0) {
            wstrPath.append(L"_");
            wstrPath.append(wstrCcdName);
        }

        if ( (rc = _VPL__wstring_to_utf8_alloc(wstrPath.c_str(), &szPath)) != VPL_OK) {
            break;
        }

        path = std::string(szPath);

    } while (false);

    return rc;
}

int launch_dx_remote_android_agent()
{
    //install apk
    doSystemCall(".\\AdbTool\\adb.exe install ./AdbTool/dx_remote_agent-release.apk");

    //launch apk
    doSystemCall(".\\AdbTool\\adb.exe shell am start -a android.intent.action.MAIN -n com.igware.dx_remote_agent/.StatusActivity");

    return 0;
}

int stop_dx_remote_android_agent()
{
    //uninstall apk
    doSystemCall(".\\AdbTool\\adb.exe uninstall com.igware.dx_remote_agent");

    return 0;
}

int launch_dx_remote_android_cc_service()
{
    //uninstall apk
    doSystemCall(".\\AdbTool\\adb.exe uninstall com.igware.android_cc_sdk.example_service_only");

    //install apk
    doSystemCall(".\\AdbTool\\adb.exe install ./AdbTool/cc_service_for_dx-release.apk");

    return 0;
}

int stop_dx_remote_android_cc_service()
{
    //force-stop apk, only works for android 3.0 or above
    doSystemCall(".\\AdbTool\\adb.exe am force-stop com.igware.android_cc_sdk.example_service_only");

    return 0;
}

int restart_dx_remote_android_agent()
{
    // force stop dx_remote_android_agent and android_cc_service
    doSystemCall(".\\AdbTool\\adb.exe shell pm clear com.igware.dx_remote_agent");
    doSystemCall(".\\AdbTool\\adb.exe shell pm clear com.igware.android_cc_sdk.example_service_only");
    // start dx_remote_android_agent_app
    doSystemCall(".\\AdbTool\\adb.exe shell am start -a android.intent.action.MAIN -n com.igware.dx_remote_agent/.StatusActivity");

    return 0;
}

int get_ccd_log_from_android()
{
    //pull ccd log from android device
    doSystemCall(".\\AdbTool\\adb.exe pull /sdcard/AOP/AcerCloud/logs .");

    return 0;
}

int clean_ccd_log_on_android()
{
    //delete ccd log on android device
    doSystemCall(".\\AdbTool\\adb shell rm /sdcard/AOP/AcerCloud/logs/cc/*.log");

    return 0;
}

int check_android_net_status(const char *ipaddr)
{
    std::string cmd;
    std::string ip(ipaddr);
    std::stringstream ping_cmd;

    ping_cmd << ".\\AdbTool\\adb shell ping -c 5 " << ip;
    cmd = ping_cmd.str();
    LOG_ALWAYS("cmd is %s", cmd.c_str());

    //check andoird outgoing network status
    doSystemCall(cmd.c_str());
    doSystemCall(".\\AdbTool\\adb shell netcfg");

    return 0;
}

void set_dx_ccd_name(std::string ccd_name)
{
    dx_ccd_name = ccd_name;
}

std::string get_dx_ccd_name()
{
    return dx_ccd_name;
}

std::wstring get_dx_ccd_nameW()
{
    wchar_t *wsz_ccd_name = NULL;
    _VPL__utf8_to_wstring(dx_ccd_name.c_str(), &wsz_ccd_name);
    std::wstring wstr_ccd_name(wsz_ccd_name);
    if (wsz_ccd_name != NULL) {
        free(wsz_ccd_name);
        wsz_ccd_name = NULL;
    }

    return wstr_ccd_name;
}

bool is_sid(std::string sid)
{
    PSID lsid;
    BOOL isOk = ConvertStringSidToSidA((char*)sid.c_str(), &lsid);

    return isOk != FALSE ? true : false;
}

//copy from CCDMonSrv.cpp
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
            LOG_ERROR("MapViewOfFile failed, %u\n", GetLastError());
            CloseHandle(hMapping);
            hMapping = NULL;
        }
        else
            LOG_INFO("MapViewOfFile succeeded\n");
    }
    else
        LOG_ERROR("CreateFileMapping failed, %u\n", GetLastError());

    *phMapping = hMapping;
    return psp;
}

int set_permission(const std::string path, const std::string mode)
{
    int rv = 0;
    std::string cmd;

    rv = VPLFile_CheckAccess(path.c_str(), VPLFILE_CHECKACCESS_EXISTS);
    if (rv != VPL_OK) {
        LOG_ERROR("path %s doesn't exist! rv = %d", path.c_str(), rv);
        return rv;
    }

    char *userid = NULL;
    rv = VPL_GetOSUserId(&userid);
    if (rv != VPL_OK) {
        LOG_ERROR("get userid failed! rv = %d", rv);
        return rv;
    }
    ON_BLOCK_EXIT(VPL_ReleaseOSUserId, userid);


    cmd = "icacls " + path + " /inheritance:d";
    doSystemCall(cmd.c_str());
    cmd = "icacls " + path + " /remove:g Users";
    doSystemCall(cmd.c_str());
    cmd = "icacls " + path + " /remove:g INTERACTIVE";
    doSystemCall(cmd.c_str());
    cmd = "icacls " + path + " /remove:g BATCH";
    doSystemCall(cmd.c_str());
    cmd = "icacls " + path + " /remove:g \"CREATOR OWNER\"";
    doSystemCall(cmd.c_str());
    cmd = "icacls " + path + " /remove:g *" + userid;
    doSystemCall(cmd.c_str());
    cmd = "icacls " + path + " /remove:g \"Authenticated Users\"";
    doSystemCall(cmd.c_str());

    if(mode == "rx"){
        cmd = "icacls " + path + " /deny Users:m";
        cmd += " & ";
        cmd += "cacls " + path + " /E /G Users:R";

    }else if(mode == "rwx"){
        cmd = "icacls " + path + " /grant:r Users:m";
    }else if(mode == "n"){
        cmd = "icacls " + path + " /deny Users:f";
    }else{
        rv = -1;
        cmd = "";
        LOG_ERROR("mode not support: %s\n", mode.c_str());
        goto out;
    }

    if(!cmd.empty())
        doSystemCall(cmd.c_str());

    //show final acl for debugging
    cmd = "icacls " + path; 
    doSystemCall(cmd.c_str());

out:
    return rv;
}

int getCurDir(std::string& dir)
{
    int rv = 0;
#define CURDIR_MAX_LENGTH       1024
    wchar_t curDir[CURDIR_MAX_LENGTH];
    GetCurrentDirectory(CURDIR_MAX_LENGTH, curDir);
    char *utf8 = NULL;
    rv = _VPL__wstring_to_utf8_alloc(curDir, &utf8);
    if (rv != VPL_OK) {
        LOG_ERROR("Fail to generate current directory");
        goto exit;
    }
    dir.assign(utf8);
    if (utf8 != NULL) {
        free(utf8);
    }

exit:
    return rv;
}
