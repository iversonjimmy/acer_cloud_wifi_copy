#ifndef __COMMON_UTILS_HPP__
#define __COMMON_UTILS_HPP__

#include <vplu_types.h>

#include <string>
#include <map>
#include <log.h>
#include <stdlib.h>

#define OS_WINDOWS_RT "WindowsRT"
#define OS_WINDOWS    "Windows"
#define OS_ANDROID    "Android"
#define OS_iOS        "iOS"
#define OS_LINUX      "Linux"
#define OS_ORBE       "Orbe"

extern std::string dxtool_root;
extern LOGLevel g_defaultLogLevel;

/// Set the log level to a specific setting.
/// If using this to override the default level temporarily, you should call
/// resetDebugLevel() when you are done.
int setDebugLevel(int level);

/// Reset the log level to the default setting requested by the user.
void resetDebugLevel();

int waitCcd();

typedef int (*subcmd_fn)(int argc, const char *argv[]);

int dispatch(const std::map<std::string, subcmd_fn>& cmdMap, int argc, const char* argv[]);

int dispatch2ndLevel(const std::map<std::string, subcmd_fn>& cmdMap, int argc, const char* argv[]);

void printHelpFromCmdMap(const std::map<std::string, subcmd_fn>& cmdMap);

bool checkHelp(int argc, const char* argv[]);

/// Improved version of checkHelp.
/// Search for this in proc_cmd.cpp for example usage.
/// @param[out] rv_out Will be set to -1 if the request is definitely invalid, 0 otherwise.
/// @returns true if the caller needs to display help (either because the args explicitly
///      asked for help or because there are the wrong number of arguments).
bool checkHelpAndExactArgc(int argc, const char* argv[], int expected_argc, int& rv_out);
bool checkHelpAndMinArgc(int argc, const char* argv[], int expected_argc, int& rv_out);
bool isHeadOfLine(const std::string &lines, size_t pos);
bool isFollowedByEqual(const std::string &lines, size_t pos);
size_t findLineEnd(const std::string &lines, size_t pos);
void updateConfig(std::string &config, const std::string &key, const std::string &value);
int file_compare(const char* src, const char* dst);
int file_copy(const char* src, const char* dst);
bool isWindows(const std::string &os);
u8 toHex(const u8 &x);
char toLower(char in);
bool isPhotoFile(const std::string& dirent);
bool isMusicFile(const std::string& dirent);
int getUserIdAndClearFiDatasetId(u64& userId, u64& datasetId);
const std::string get_extension(const std::string &path);
void rmTrailingSlashes(const std::string& path_in, std::string& path_out);
void trimSlashes(const std::string& path_in, std::string& path_out);
void splitAbsPath(const std::string& absPath, std::string& path_out, std::string& name_out);

/// Deprecated.  Use Util_rmRecursive in gvm_rm_utils.hpp
/// Removes all elements of a given path.  Fairly dangerous operation, symbolic
/// links are not followed so long as VPL_Stat of a symbolic link NEVER returns
/// VPLFS_TYPE_DIR.
int Util_rm_dash_rf(const std::string& path);

void countPhotoNum(const std::string& cameraRollPath, u32& numPhotos);
void countMusicNum(const std::string& albumPath, u32& numMusic);

/// Perform a system call, logging the command and result.
/// This will make evident any hang that occurs during a system call,
/// failed system calls (that might cause test failure), and incorrect commands.
/// Does not apply for WinRT.
#ifndef VPL_PLAT_IS_WINRT
inline int doSystemCall(const char* cmd)
{
    int rv;
    LOG_ALWAYS("SYSTEM:\"%s\" START", cmd);
    rv = system(cmd);
    LOG_ALWAYS("SYSTEM:\"%s\" END rv=%d", cmd, rv);
    return rv;
}
#endif

#endif // __COMMON_UTILS_HPP__
