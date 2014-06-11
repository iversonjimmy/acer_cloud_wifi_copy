#ifndef DX_REMOTE_AGENT_UTIL_H_
#define DX_REMOTE_AGENT_UTIL_H_
#include <string>

int startccd(const char* titleId);
int stopccd();
int startccd(int testInstanceNum, const char* titleId);
int stopccd(int testInstanceNum);
int startprocess(const std::string& command);
int get_user_folder(std::string &path);
int get_cc_folder(std::string &path);
unsigned short getFreePort();
int recordPortUsed(unsigned short port);
int clean_cc();

#endif
