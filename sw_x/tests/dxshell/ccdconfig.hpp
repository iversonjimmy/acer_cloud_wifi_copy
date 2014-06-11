#ifndef __CCDCONFIG_HPP__
#define __CCDCONFIG_HPP__

#include <vplu_types.h>

int dispatch_ccdconfig_cmd(int argc, const char* argv[]);

int ccdconfig_set(const std::string &key, const std::string &value);
int ccdconfig_get(const std::string &key, std::string &value);
int ccdconfig_pull(const std::string &filepath);
int ccdconfig_push(const std::string &filepath);
int ccdconfig_reset();

#endif // __CCDCONFIG_HPP__
