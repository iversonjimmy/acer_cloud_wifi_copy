#include "JsonHelper.hpp"

#include <log.h>

int JSON_getInt64(cJSON2* node, const char* attributeName, u64& value)
{
    if (node==NULL) {
        LOG_ERROR("Node is null");
        return -1;
    }

    cJSON2* json_value;
    if (JSON_getJSONObject(node, attributeName, &json_value)) {
        LOG_INFO("Can't find \"%s\"", attributeName);
        return -1;
    }

    value = json_value->valueint;
    return 0;
}

int JSON_getJSONObject(cJSON2* node, const char* attributeName, cJSON2** value)
{
    if (node==NULL) {
        LOG_ERROR("Node is null");
        return -1;
    }

    cJSON2* json_value;
    json_value = cJSON2_GetObjectItem(node, attributeName);
    if (json_value==NULL) {
        LOG_INFO("Can't find \"%s\"", attributeName);
        return -1;
    }
    *value = json_value;
    return 0;
}

int JSON_getString(cJSON2* node, const char* attributeName, std::string& value)
{
    if (node==NULL) {
        LOG_ERROR("Node is null");
        return -1;
    }

    cJSON2* json_value;
    if (JSON_getJSONObject(node, attributeName, &json_value)) {
        LOG_INFO("Can't find \"%s\"", attributeName);
        return -1;
    }

    value = json_value->valuestring;
    return 0;
}
