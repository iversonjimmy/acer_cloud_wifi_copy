#ifndef __EXECUTABLE_MANAGER_HPP__
#define __EXECUTABLE_MANAGER_HPP__

//============================================================================
/// @file
/// Manages registered remote executable.
/// See https://www.ctbg.acer.com/wiki/index.php/CCD_HTTP_Interface_for_Executing_Remote_Process
//============================================================================

#include <vplu_types.h>
#include <string>
#include <vector>

typedef struct RemoteExecutable {
    std::string app_key;
    std::string name;
    std::string absolute_path;
    u64 version_num;
} RemoteExecutable_t;

int RemoteExecutableManager_Init(const std::string& db_rootpath);

int RemoteExecutableManager_Destroy();

int RemoteExecutableManager_InsertOrUpdateExecutable(const std::string& app_key, const std::string& app_name, const std::string& absolute_path, u64 version_num);

int RemoteExecutableManager_RemoveExecutable(const std::string& app_key, const std::string& executable_name);

int RemoteExecutableManager_GetExecutable(const std::string& app_key, const std::string& executable_name, RemoteExecutable& output_remote_executable);

int RemoteExecutableManager_ListExecutables(const std::string& app_key, std::vector<RemoteExecutable>& output_remote_executables);

int RemoteExecutableManager_RemoveAllExecutables();

#endif // __EXECUTABLE_MANAGER_HPP__
