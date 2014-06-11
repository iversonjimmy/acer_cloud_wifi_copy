#ifndef __JSONHELPER_HPP__
#define __JSONHELPER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>

#include <cJSON2.h>

#include <string>

int JSON_getInt64(cJSON2* node, const char* attributeName, u64& value);
int JSON_getJSONObject(cJSON2* node, const char* attributeName, cJSON2** value);
int JSON_getString(cJSON2* node, const char* attributeName, std::string& value);

#endif // incl guard
