#ifndef DX_REMOTE_AGENT_UTIL_WINRT_H_
#define DX_REMOTE_AGENT_UTIL_WINRT_H_
#include <string>

int get_user_folderW(std::wstring &path);
int get_cc_folderW(std::wstring &path);
int get_cc_folder(std::string &path);
int create_picstream_path();

#endif