#include "HttpSvc_Sn_Handler_rexe.hpp"

#include "vss_server.hpp"
#include "executable_manager.hpp"

#include <cJSON2.h>
#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <HttpStream.hpp>
#include <HttpStream_Helper.hpp>
#include <vplex_http_util.hpp>
#include <log.h>

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <TlHelp32.h>
#include <Winbase.h>
#include <Wtsapi32.h>
#include <Userenv.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "Wtsapi32.lib")
#elif defined(LINUX) || defined(CLOUDNODE)
#include <sys/types.h>
#include <sys/wait.h>
#endif

#define BUFSIZE 4096
static const size_t helperThread_StackSize = 128 * 1024;

HttpSvc::Sn::Handler_rexe::Handler_rexe(HttpStream *hs)
    : Handler(hs), app_key(""), executable_name(""), absolute_path(""), minimal_version_num(0), read_child_stdout_handle(0), helperThreadReturnValue(0)
{
}

HttpSvc::Sn::Handler_rexe::~Handler_rexe()
{
    if (read_child_stdout_handle) {
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        CloseHandle(read_child_stdout_handle);
#elif defined(LINUX) || defined(CLOUDNODE)
        close(read_child_stdout_handle);
#endif
    }
}

int HttpSvc::Sn::Handler_rexe::Run()
{
    int err = 0;
    err = parseRequest();
    if (err) {
        // parseRequest() logged msg and set HTTP response.
        return 0;  // reset error
    }

    err = checkExecutable();
    if (err) {
        // checkExecutable() logged msg and set HTTP response.
        return 0;  // reset error
    }

    err = runExecutable();
    if (err) {
        // runExecutable() logged msg and set HTTP response.
        return 0;  // reset error
    }

    return err;
}

int HttpSvc::Sn::Handler_rexe::parseRequest()
{
    int err = 0;

    // URI looks like /rexe/execute/<deviceid>/<executable name>/<app key>/<minimal version number>
    std::vector<std::string> uri_tokens;
    const std::string& uri = hs->GetUri();

    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() != 6) {
        LOG_ERROR("Handler_rexe[%p]: Unexpected number of segments; uri %s", this, uri.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected number of segments; uri " << uri << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return CCD_ERROR_PARAMETER;
    }

    err = VPLHttp_DecodeUri(uri_tokens[3], executable_name);
    if (err) {
        LOG_ERROR("Handler_rexe[%p]: Failed to decode; segment %s, err: %d", this, uri_tokens[3].c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to decode; segment " << uri_tokens[3] << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;  // reset error
    }

    err = VPLHttp_DecodeUri(uri_tokens[4], app_key);
    if (err) {
        LOG_ERROR("Handler_rexe[%p]: Failed to decode; segment %s, err: %d", this, uri_tokens[4].c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to decode; segment " << uri_tokens[4] << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;  // reset error
    }

    minimal_version_num = VPLConv_strToU64(uri_tokens[5].c_str(), NULL, 10);

    return err;
}

int HttpSvc::Sn::Handler_rexe::checkExecutable()
{
    int err = 0;
    RemoteExecutable output_remote_executable;
    err = RemoteExecutableManager_GetExecutable(app_key, executable_name, output_remote_executable);
    if (err) {
        LOG_ERROR("Handler_rexe[%p]: Can not find such executable. (app_key: %s, executable_name: %s)", this, app_key.c_str(), executable_name.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Can not find such executable.\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
        return CCD_ERROR_PARAMETER;
    }

    // Check the version number.
    if (output_remote_executable.version_num < minimal_version_num) {
        LOG_ERROR("Handler_rexe[%p]: Cannot satisfy minimum version requirement, current:"FMTu64" minimal:"FMTu64, this, output_remote_executable.version_num, minimal_version_num);
        HttpStream_Helper::SetCompleteResponse(hs, 412, "{\"errMsg\":\"Cannot satisfy minimum version requirement.\"}", "application/json");
        return CCD_ERROR_PARAMETER;
    }

    absolute_path = output_remote_executable.absolute_path;

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    err = _VPLFS_CheckTrustedExecutable(absolute_path.c_str());
    if (err == VPL_ERR_NOENT) {
        LOG_ERROR("Handler_rexe[%p]: Executable not found; path: %s", this, absolute_path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Executable not found\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return err;
    }
    if (err) {
        LOG_ERROR("Handler_rexe[%p]: Executable not trusted; path: %s, err: %d", this, absolute_path.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Executable not trusted\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return err;
    }
#endif // defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

    return err;
}

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

// Reference: http://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/31bfa13d-982b-4b1a-bff3-2761ade5214f/calling-createprocessasuser-from-service?forum=windowssecurity
int HttpSvc::Sn::Handler_rexe::runExecutable()
{
    int err = 0;
    HANDLE child_stdin_r  = NULL;
    HANDLE child_stdin_w  = NULL;
    HANDLE child_stdout_r = NULL;
    HANDLE child_stdout_w = NULL;
    wchar_t* cmdline = NULL;
    PROCESS_INFORMATION proc_info;
    STARTUPINFO startup_info;
    DWORD exit_code;
    BOOL is_success = FALSE;
    DWORD written_bytes_len;
    char buf[BUFSIZE];
    VPLDetachableThreadHandle_t helperThread;
    bool threadSpawned = false;

    err = _VPL__utf8_to_wstring(absolute_path.c_str(), &cmdline);
    if (err) {
        LOG_ERROR("Handler_rexe[%p]: Error in _VPL__utf8_to_wstring(), rv = %d", this, err);
        HttpStream_Helper::SetCompleteResponse(hs, 500);
        err = -1;
        goto end;
    }

    // Create pipes.
    {
        SECURITY_ATTRIBUTES security_attr;

        // Set the bInheritHandle flag so pipe handles are inherited.
        security_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
        security_attr.bInheritHandle = TRUE;
        security_attr.lpSecurityDescriptor = NULL;

        // Create a pipe for the child process's STDOUT.
        if ( !CreatePipe(&child_stdout_r, &child_stdout_w, &security_attr, 0) ) {
            LOG_ERROR("Handler_rexe[%p]: Failed to create pipe.", this);
            HttpStream_Helper::SetCompleteResponse(hs, 500);
            err = -1;
            goto end;
        }

        // Ensure the read handle to the pipe for STDOUT is not inherited.
        if ( !SetHandleInformation(child_stdout_r, HANDLE_FLAG_INHERIT, 0) ) {
            LOG_ERROR("Handler_rexe[%p]: Error in SetHandleInformation().", this);
            HttpStream_Helper::SetCompleteResponse(hs, 500);
            err = -1;
            goto end;
        }

        // Create a pipe for the child process's STDIN.
        if ( !CreatePipe(&child_stdin_r, &child_stdin_w, &security_attr, 0)) {
            LOG_ERROR("Handler_rexe[%p]: Failed to create pipe.", this);
            HttpStream_Helper::SetCompleteResponse(hs, 500);
            err = -1;
            goto end;
        }

        // Ensure the write handle to the pipe for STDIN is not inherited.
        if ( !SetHandleInformation(child_stdin_w, HANDLE_FLAG_INHERIT, 0) ) {
            LOG_ERROR("Handler_rexe[%p]: Error in SetHandleInformation().", this);
            HttpStream_Helper::SetCompleteResponse(hs, 500);
            err = -1;
            goto end;
        }
    }

    // Create the child process.
    {
        HANDLE current_user_access_token = NULL;
        HANDLE dup_current_user_access_token = NULL;
        DWORD dwSessionId = WTSGetActiveConsoleSessionId();
        LPVOID  p_env = NULL;
        DWORD creation_flag = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE;

        WTSQueryUserToken(dwSessionId, &current_user_access_token);
        DuplicateTokenEx(current_user_access_token, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &dup_current_user_access_token);

        // Set up members of the PROCESS_INFORMATION structure.
        ZeroMemory( &proc_info, sizeof(PROCESS_INFORMATION) );

        // Set up members of the STARTUPINFO structure.
        // This structure specifies the STDIN and STDOUT handles for redirection.
        ZeroMemory( &startup_info, sizeof(STARTUPINFO) );
        startup_info.cb         = sizeof(STARTUPINFO);
        startup_info.hStdError  = child_stdout_w;
        startup_info.hStdOutput = child_stdout_w;
        startup_info.hStdInput  = child_stdin_r;
        startup_info.dwFlags    |= STARTF_USESTDHANDLES;

        if (CreateEnvironmentBlock(&p_env, dup_current_user_access_token, FALSE)) {
            creation_flag |= CREATE_UNICODE_ENVIRONMENT;
        } else {
            LOG_WARN("Failed to create environment block.");
            p_env = NULL;
        }

        // Create the child process.
        is_success = CreateProcessAsUser(
            dup_current_user_access_token,
            NULL,
            cmdline,          // command line
            NULL,             // process security attributes
            NULL,             // primary thread security attributes
            TRUE,             // handles are inherited
            creation_flag,    // creation flags
            p_env,            // use parent's environment
            NULL,             // use parent's current directory
            &startup_info,    // STARTUPINFO pointer
            &proc_info);      // receives PROCESS_INFORMATION

        // If an error occurs, exit the application.
        if ( !is_success ) {
            LOG_ERROR("Handler_rexe[%p]: Failed to create process.", this);
            HttpStream_Helper::SetCompleteResponse(hs, 500, "{\"errMsg\":\"Failed to create process.\"}", "application/json");
            err = -1;
            goto end;
        }
    }

    {
        helperThreadReturnValue = 0;
        read_child_stdout_handle = child_stdout_r;
        err = Util_SpawnThread(helperThreadMain, this, helperThread_StackSize, /*isJoinable*/VPL_TRUE, &helperThread);
        if (err) {
            LOG_ERROR("Handler_rexe[%p]: Failed to spawn helper thread. (err = %d)", this, err);
            HttpStream_Helper::SetCompleteResponse(hs, 500);
            err = 0;  // reset error
            goto end;
        }
        threadSpawned = true;
    }

    // Read data from HttpStream and write to the standard input of the launched executable.
    {
        bool skippedHeader = false;
        char* data;
        u32 datasize = 0;
        while (true) {
            ssize_t bytes = hs->Read(buf, sizeof(buf) - 1);  // subtract 1 for EOL
            if (bytes < 0) {
                LOG_ERROR("Handler_rexe[%p]: Failed to read from HttpStream[%p]: err = "FMT_ssize_t, this, hs, bytes);
                HttpStream_Helper::SetCompleteResponse(hs, 500, "{\"errMsg\":\"Failed to read from HttpStream\"}", "application/json");
                err = -1;
                goto end;
            }

            if (bytes == 0) {
                break; // EOF
            }

            buf[bytes] = '\0';

            if (!skippedHeader) {
                char* boundary = strstr(buf, "\r\n\r\n");  // find header-body boundary
                if (!boundary) {
                    LOG_ERROR("Handler_rexe[%p]: Failed to find header-body boundary in request", this);
                    std::ostringstream oss;
                    oss << "{\"errMsg\":\"Failed to find header-body boundary in request\"}";
                    HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
                    err = 0;
                    goto end;
                }
                data = boundary + 4;
                datasize = bytes - (boundary + 4 - buf);
                skippedHeader = true;
            } else {
                data = buf;
                datasize = bytes;
            }

            LOG_INFO("Handler_rexe[%p]: Write "FMTu32" bytes into standard input of the process", this, datasize);

            is_success = WriteFile(child_stdin_w, data, (DWORD)datasize, &written_bytes_len, NULL);
            if ( !is_success ) {
                LOG_ERROR("Handler_rexe[%p]: Failed to write bytes to process.", this);
                break;
            }
        }

        // Close the pipe handle so the child process stops reading.
        if ( !CloseHandle(child_stdin_w) ) {
            LOG_ERROR("Handler_rexe[%p]: Failed to close handle.", this);
            HttpStream_Helper::SetCompleteResponse(hs, 500);
            err = 0;
            is_success = FALSE; // goto end;
        }
        child_stdin_w = NULL;
    }

    // Wait for the launched process to exit
    {
        if (is_success) {
            LOG_INFO("Handler_rexe[%p]: waiting for the process", this);

            // Wait for the process exits.
            WaitForSingleObject(proc_info.hProcess, INFINITE);

            // Get the exit code.
            GetExitCodeProcess(proc_info.hProcess, &exit_code);

            LOG_INFO("Handler_rexe[%p]: exit code: %d", this, exit_code);
        }

        CloseHandle(proc_info.hProcess);
        CloseHandle(proc_info.hThread);
    }

    // Close the pipe
    {
        if ( !CloseHandle(child_stdout_w) ) {
            LOG_ERROR("Handler_rexe[%p]: Failed to close handle.", this);
            HttpStream_Helper::SetCompleteResponse(hs, 500);
            err = 0;
            is_success = FALSE; // goto end;
        }
        child_stdout_w = NULL;
    }

    // Wait for the helper thread to exit
    if (threadSpawned) {
        int err2 = VPLDetachableThread_Join(&helperThread);
        if (err2) {
            LOG_ERROR("Handler_rexe[%p]: Failed to join helper thread. (err = %d)", this, err2);
            if (!err) {
                err = err2;
            }
        }
        threadSpawned = false;
    }

    if (helperThreadReturnValue) {
        // helper thread already logged msg and set HTTP response.
        is_success = false;
    }

    // Setup http header
    if (is_success) {
        HttpStream_Helper::AddStdRespHeaders(hs);
        hs->SetStatusCode(200);
        hs->SetRespHeader("Connection", "close");
        hs->SetRespHeader("Content-Type", "application/octet-stream");

        std::ostringstream oss;
        oss << stdout_buf_stream.str().size();
        hs->SetRespHeader("Content-Length", oss.str());
        hs->Flush();
    }

    // Write the STDOUT of launched process to TS
    if (is_success) {
        std::string dataStr = stdout_buf_stream.str();
        u32 total_size = dataStr.size();
        u32 written_bytes = 0;

        while (is_success && written_bytes < total_size) {
            const char* data = dataStr.c_str();
            ssize_t nbytes;
            nbytes = hs->Write(data + written_bytes, total_size - written_bytes);
            if (nbytes < 0) {
                LOG_ERROR("Handler_rexe[%p]: Error in HttpStream->Write(), rc = %d", this, nbytes);
                break;
            }
            written_bytes += nbytes;
        }
    }

end:
    if (child_stdin_r) {
        CloseHandle(child_stdin_r);
    }
    if (child_stdin_w) {
        CloseHandle(child_stdin_w);
    }
    if (child_stdout_r) {
        CloseHandle(child_stdout_r);
    }
    if (child_stdout_w) {
        CloseHandle(child_stdout_w);
    }

    if (cmdline) {
        free(cmdline);
    }

    return err;
}

#elif defined(LINUX) || defined(CLOUDNODE)

int HttpSvc::Sn::Handler_rexe::runExecutable()
{
    int err = 0;
    bool is_success = true;
    char buf[BUFSIZE];
    pid_t pid = (pid_t)NULL;
    int w_pipefd[2];
    int r_pipefd[2];
    int &child_stdin_r  = w_pipefd[0]; // child process << child process STDIN
    int &child_stdin_w  = w_pipefd[1]; // parent process >> child process STDIN
    int &child_stdout_r = r_pipefd[0]; // parent process << child process STDOUT
    int &child_stdout_w = r_pipefd[1]; // child process >> child process STDOUT
    VPLDetachableThreadHandle_t helperThread;
    bool threadSpawned = false;

    memset(buf, 0, sizeof(buf));

    err = pipe(w_pipefd);
    if (err) {
        LOG_ERROR("Handler_rexe[%p]: Failed to create pipe. (err = %d)", this, err);
        goto end;
    }

    err = pipe(r_pipefd);
    if (err) {
        LOG_ERROR("Handler_rexe[%p]: Failed to create pipe. (err = %d)", this, err);
        goto end;
    }

    read_child_stdout_handle = child_stdout_r;

    pid = fork();
    if (pid < 0) {
        LOG_ERROR("Handler_rexe[%p]: Failed to fork. (err = %d)", this, pid);
        goto end;
    }

    if (pid == 0) {
        int rc;
        std::string filename;

        close(child_stdin_w/*w_pipefd[1]*/);
        close(child_stdout_r/*r_pipefd[0]*/);

        dup2(child_stdin_r/*w_pipefd[0]*/, STDIN_FILENO);
        dup2(child_stdout_w/*r_pipefd[1]*/, STDOUT_FILENO);
        dup2(child_stdout_w/*r_pipefd[1]*/, STDERR_FILENO);

        rc = absolute_path.rfind("/");
        if (rc !=  string::npos && rc < absolute_path.length()) {
            filename = absolute_path.substr(rc + 1);
        }

        execl(absolute_path.c_str(), filename.c_str(), (char*) NULL);

        close(child_stdin_r/*w_pipefd[0]*/);
        close(child_stdout_w/*r_pipefd[1]*/);

        exit(0);
        return 0;
    }

    {
        int err2 = close(child_stdin_r/*w_pipefd[0]*/);
        if (err2) {
            LOG_ERROR("Handler_rexe[%p]: close(child_stdin_r) rc = %d", this, err2);
        }
        child_stdin_r = 0;
    }
    {
        int err2 = close(child_stdout_w/*r_pipefd[1]*/);
        if (err2) {
            LOG_ERROR("Handler_rexe[%p]: close(child_stdout_w) rc = %d", this, err2);
        }
        child_stdout_w = 0;
    }
    {
        helperThreadReturnValue = 0;
        err = Util_SpawnThread(helperThreadMain, this, helperThread_StackSize, /*isJoinable*/VPL_TRUE, &helperThread);
        if (err) {
            LOG_ERROR("Handler_rexe[%p]: Failed to spawn helper thread. (err = %d)", this, err);
            HttpStream_Helper::SetCompleteResponse(hs, 500);
            err = 0;  // reset error
            goto end;
        }
        threadSpawned = true;
    }

    // Read data from HttpStream and write to the standard input of the launched executable.
    {
        bool skippedHeader = false;
        char* data;
        u32 datasize = 0;
        while (true) {
            ssize_t bytes = hs->Read(buf, sizeof(buf) - 1);  // subtract 1 for EOL
            if (bytes < 0) {
                LOG_ERROR("Handler_rexe[%p]: Failed to read from HttpStream[%p]: err "FMT_ssize_t, this, hs, bytes);
                HttpStream_Helper::SetCompleteResponse(hs, 500, "{\"errMsg\":\"Failed to read from HttpStream\"}", "application/json");
                err = -1;
                goto end;
            }

            if (bytes == 0) {
                break; // EOF
            }

            buf[bytes] = '\0';

            if (!skippedHeader) {
                char* boundary = strstr(buf, "\r\n\r\n");  // find header-body boundary
                if (!boundary) {
                    LOG_ERROR("Handler_rexe[%p]: Failed to find header-body boundary in request", this);
                    std::ostringstream oss;
                    oss << "{\"errMsg\":\"Failed to find header-body boundary in request\"}";
                    HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
                    err = 0;
                    goto end;
                }
                data = boundary + 4;
                datasize = bytes - (boundary + 4 - buf);
                skippedHeader = true;
            } else {
                data = buf;
                datasize = bytes;
            }

            LOG_INFO("Handler_rexe[%p]: Write "FMTu32" bytes into standard input of the process", this, datasize);

            bytes = write(child_stdin_w, data, datasize);
            if (bytes != datasize) {
                LOG_ERROR("Handler_rexe[%p]: Failed to write data to STDIN of launched process: datasize: "FMTu32"  rc: "FMT_ssize_t, this, datasize, bytes);
                break;
            }
        }
    }
    {
        // Close the pipe handle so the child process stops reading.
        err = close(child_stdin_w);
        if (err) {
            LOG_ERROR("Handler_rexe[%p]: Failed to close handle. (err = %d)", this, err);
            HttpStream_Helper::SetCompleteResponse(hs, 500);
            err = 0;
            is_success = false; // goto end;
        }
        child_stdin_w = 0;
    }

    // Wait for the helper thread to exit
    if (threadSpawned) {
        LOG_INFO("Handler_rexe[%p]: wait for the helper thread to exit.", this);
        int err2 = VPLDetachableThread_Join(&helperThread);
        if (err2) {
            LOG_ERROR("Handler_rexe[%p]: Failed to join helper thread. (err = %d)", this, err2);
            if (!err) {
                err = err2;
            }
        }
        threadSpawned = false;
    }

    LOG_INFO("Handler_rexe[%p]: helper thread exit, rv = %d", this, helperThreadReturnValue);

    if (child_stdout_r) {
        int err2 = close(child_stdout_r);
        if (err2) {
            LOG_ERROR("Handler_rexe[%p]: Failed to close handle. (err = %d)", this, err2);
            HttpStream_Helper::SetCompleteResponse(hs, 500);
            err = 0;
            is_success = false; // goto end;
        }
        child_stdout_r = 0;
    }

    {
        int exit_status = 0;
        wait(&exit_status);
        LOG_INFO("Handler_rexe[%p]: Child process exit_status: %d", this, exit_status);
    }
    if (helperThreadReturnValue) {
        // helper thread already logged msg and set HTTP response.
        is_success = false;
    }

    // Setup http header
    if (is_success) {
        HttpStream_Helper::AddStdRespHeaders(hs);
        hs->SetStatusCode(200);
        hs->SetRespHeader("Connection", "close");
        hs->SetRespHeader("Content-Type", "application/octet-stream");

        std::ostringstream oss;
        oss << stdout_buf_stream.str().size();
        hs->SetRespHeader("Content-Length", oss.str());
        hs->Flush();
    }

    // Write the STDOUT of launched process to TS
    if (is_success) {
        std::string dataStr = stdout_buf_stream.str();
        u32 total_size = dataStr.size();
        u32 written_bytes = 0;

        while (written_bytes < total_size) {
            const char* data = dataStr.c_str();
            ssize_t nbytes;
            nbytes = hs->Write(data + written_bytes, total_size - written_bytes);
            if (nbytes < 0) {
                LOG_ERROR("Handler_rexe[%p]: Error in HttpStream->Write(), rc = %d", this, nbytes);
                break;
            }
            written_bytes += nbytes;
        }
    }

end:
    if (child_stdin_r) {
        int err2 = close(child_stdin_r);
        if (err2) {
            LOG_ERROR("Handler_rexe[%p]: Error in close(child_stdin_r), rc = %d", this, err2);
        }
    }
    if (child_stdin_w) {
        int err2 = close(child_stdin_w);
        if (err2) {
            LOG_ERROR("Handler_rexe[%p]: Error in close(child_stdin_w), rc = %d", this, err2);
        }
    }
    if (child_stdout_r) {
        int err2 = close(child_stdout_r);
        if (err2) {
            LOG_ERROR("Handler_rexe[%p]: Error in close(child_stdout_r), rc = %d", this, err2);
        }
    }
    if (child_stdout_w) {
        int err2 = close(child_stdout_w);
        if (err2) {
            LOG_ERROR("Handler_rexe[%p]: Error in close(child_stdout_w), rc = %d", this, err2);
        }
    }

    return err;
}

#endif

int HttpSvc::Sn::Handler_rexe::writeToBuffer(const char* buf, u32 size)
{
    stdout_buf_stream.write(buf, size);
    return size;
}

VPLTHREAD_FN_DECL HttpSvc::Sn::Handler_rexe::helperThreadMain(void *param)
{
    HttpSvc::Sn::Handler_rexe *handler = static_cast<HttpSvc::Sn::Handler_rexe*>(param);
    handler->helperThreadMain();
    return VPLTHREAD_RETURN_VALUE;
}
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
void HttpSvc::Sn::Handler_rexe::helperThreadMain()
{
    char buf[BUFSIZE];
    BOOL is_success = FALSE;
    DWORD read_bytes_len;

    // Read from pipe that is the standard output for child process.
    while (true) {
        u32 datasize;
        is_success = ReadFile( read_child_stdout_handle, buf, BUFSIZE, &read_bytes_len, NULL);

        if ( !is_success ) {
            DWORD error = GetLastError();
            if (error == ERROR_HANDLE_EOF || error == ERROR_BROKEN_PIPE) {
                break; // EOF
            }

            LOG_ERROR("Handler_rexe[%p]: Failed to read standard output from the process. ("FMTx32")", this, error);
            HttpStream_Helper::SetCompleteResponse(hs, 400, "{\"errMsg\":\"Failed to read standard output from the process.\"}", "application/json");
            helperThreadReturnValue = -1;
            break;
        }

        if( read_bytes_len == 0 ) {
            break; // EOF
        }

        datasize = read_bytes_len;

        LOG_INFO("Handler_rexe[%p]: Read "FMTu32" bytes from standard output of the process.", this, datasize);
        writeToBuffer(buf, datasize);
    }

    read_child_stdout_handle = 0;
    LOG_INFO("Handler_rexe[%p]: helperThreadMain() end.", this);
}

#elif defined(LINUX) || defined(CLOUDNODE)

void HttpSvc::Sn::Handler_rexe::helperThreadMain()
{
    char buf[BUFSIZE];
    while (true) {
        int rc;
        u32 datasize;

        rc = read(read_child_stdout_handle, buf, sizeof(buf));
        if (rc < 0 ) {
            LOG_ERROR("Handler_rexe[%p]: Failed to read standard output from the process. ("FMTx32")", this, rc);
            HttpStream_Helper::SetCompleteResponse(hs, 400, "{\"errMsg\":\"Failed to read standard output from the process.\"}", "application/json");
            helperThreadReturnValue = -1;
            break;
        }

        if (rc == 0) {
            break; // EOF
        }

        datasize = rc;

        LOG_INFO("Handler_rexe[%p]: Read "FMTu32" bytes from standard output of the process.", this, datasize);
        writeToBuffer(buf, datasize);
    }

    read_child_stdout_handle = 0;
    LOG_INFO("Handler_rexe[%p]: helperThreadMain() end.", this);
}

#endif
