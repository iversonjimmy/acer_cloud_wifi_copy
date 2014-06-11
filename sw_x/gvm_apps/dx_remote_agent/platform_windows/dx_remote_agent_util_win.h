#ifndef DX_REMOTE_AGENT_UTIL_WIN_H_
#define DX_REMOTE_AGENT_UTIL_WIN_H_
#include <string>
#include <netfw.h>

const std::string DX_REMOTE_AGENT_MUTEX_STRING = "dx_remote_agent_740CE0FF-6636-4B03-A5C8-CBFBADF4CE51";
const static int MAX_PATH_UNICODE = 32768;

int _getLocalAppDataWPathByKnownFolder(std::wstring &wpath);
int _getCcdAppDataWPathByKnownFolder(std::wstring &wpath);
int _startprocessW(const std::wstring& command);
int _dx_remote_startCCDW(const HANDLE hASM, const uint32_t szASM, const char* titleId);
int _shutdownprocessW(std::wstring procName);
int _shutdownprocess(std::string procName);
void registerToFirewall();
int getCurDir(std::string& dir);
int launch_dx_remote_android_agent();
int get_connected_android_device_ip(std::string &ipAddr);
int get_user_folderW(std::string &wpath);
int stop_dx_remote_android_agent();
int launch_dx_remote_android_cc_service();
int stop_dx_remote_android_cc_service();
int restart_dx_remote_android_agent();
int get_ccd_log_from_android();
int clean_ccd_log_on_android();
int check_android_net_status(const char *ipaddr);
void set_dx_ccd_name(std::string ccd_name);
std::string get_dx_ccd_name();
std::wstring get_dx_ccd_nameW();
bool is_sid(std::string sid);
/// Create @a shared memory mapping to carry parameter for CCDMonitorService to launch CCD.
/////
///// @param[in] max size of the memory mapping.
///// @param[out] mapping handle.
///// @retval NULL Failed to create @a shared memory mapping.
LPVOID CreateStartupParams(HANDLE *phMapping, size_t maxSize);

//For DxRemoteOSAgent::DxRemote_Set_Permission
int set_permission(const std::string path, const std::string mode);

#endif
