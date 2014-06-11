//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//
#include "vcs_common_priv.hpp"

#include "vcs_common.hpp"
#include "gvm_misc_utils.h"
#include "vplu_format.h"
#include "scopeguard.hpp"

#include <string>
#include <sstream>

#include "log.h"

static bool isDigit(char letter)
{
    if(letter>='0' && letter <= '9') {
        return true;
    }
    return false;
}

// Returns true if an error is parsed out.
bool isVcsErrorTemplate(const std::string& body,
                        int& error_out)
{
    // http://wiki.ctbg.acer.com/wiki/index.php/RESTful_API_External_Error_Codes
    static const std::string VCS_ERR_TEMPLATE("ERRCODE(####)");
    error_out = 0;
    std::string errorNumStr;

    if(body.size() < VCS_ERR_TEMPLATE.size()) {
        return false;
    }

    for(u32 charIndex=0; charIndex < VCS_ERR_TEMPLATE.size(); ++charIndex)
    {
        if(VCS_ERR_TEMPLATE[charIndex]=='#') {
            if(!isDigit(body[charIndex])) {
                return false;
            }else{
                errorNumStr.append(1, body[charIndex]);
            }
        }else{
            if(VCS_ERR_TEMPLATE[charIndex] != body[charIndex]) {
                return false;
            }
        }
    }

    error_out = atoi(errorNumStr.c_str());

    // Map VCS Error to Client error, which is -3XXXX
    error_out = -30000 - error_out;
    return true;
}


std::string escapeJsonString(const std::string& input)
{
    std::ostringstream ss;
    for (std::string::const_iterator iter = input.begin();
         iter != input.end(); ++iter)
    {
        switch (*iter) {
            case '\\': ss << "\\\\"; break;
            case '"': ss << "\\\""; break;
            case '/': ss << "\\/"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << *iter; break;
        }
    }
    return ss.str();
}


int addSessionHeadersHelper(const VcsSession& vcsSession,
                            VPLHttp2& httpHandle,
                            u32 vcsApiVersion)
{   // See http://wiki.ctbg.acer.com/wiki/index.php/VCS_Design#Authentication
    int rc;
    std::stringstream ss;

    ss.str("");

    if (vcsApiVersion > 1) { // No need to specify ApiVersion for version 1
        ss << vcsApiVersion;
        rc = httpHandle.AddRequestHeader("X-ac-version", ss.str());
        if(rc != 0) {
            LOG_ERROR("AddRequestHeader:%d, %s:%d",
                      rc, "X-ac-version", vcsApiVersion);
            return rc;
        }
    }

    ss.str("");
    ss << vcsSession.userId;
    rc = httpHandle.AddRequestHeader("X-ac-userId", ss.str());
    if(rc != 0) {
        LOG_ERROR("AddRequestHeader:%d, %s:"FMTu64,
                  rc, "X-ac-userId", vcsSession.userId);
        return rc;
    }

    if (vcsSession.deviceId != 0) {
        ss.str("");
        ss << (s64)vcsSession.deviceId;
        rc = httpHandle.AddRequestHeader("X-ac-deviceId", ss.str());
        if(rc != 0) {
            LOG_ERROR("AddRequestHeader:%d, %s:"FMTu64,
                      rc, "X-ac-deviceId", vcsSession.deviceId);
            return rc;
        }
    }

    ss.str("");
    ss << (s64)vcsSession.sessionHandle;
    rc = httpHandle.AddRequestHeader("X-ac-sessionHandle", ss.str());
    if(rc != 0) {
        LOG_ERROR("AddRequestHeader:%d, %s:"FMTu64,
                  rc, "X-ac-sessionHandle", vcsSession.sessionHandle);
        return rc;
    }

    std::string serviceTicketEncodedBase64;
    {  // Encode the session into base64
        char* ticket64;
        rc = Util_EncodeBase64(vcsSession.sessionServiceTicket.data(),
                               vcsSession.sessionServiceTicket.size(),
                               &ticket64, NULL, VPL_FALSE, VPL_FALSE);
        if (rc < 0) {
            LOG_ERROR("error %d when generating service ticket", rc);
            return rc;
        }
        ON_BLOCK_EXIT(free, ticket64);
        serviceTicketEncodedBase64.assign(ticket64);
    }

    rc = httpHandle.AddRequestHeader("X-ac-serviceTicket",
                                     serviceTicketEncodedBase64);
    if(rc != 0) {
        LOG_ERROR("AddRequestHeader:%d, %s",
                  rc, "X-ac-serviceTicket");
        return rc;
    }

    rc = httpHandle.AddRequestHeader("Content-Type", "application/json");
    if(rc != 0) {
        LOG_ERROR("AddRequestHeader:%d, Content-Type",rc);
        return rc;
    }
    return rc;
}

static int parseVcsAccessInfoHeaderObject(cJSON2* cjsonHeaderObject,
                                          std::vector<VcsHeaderPair>& headers_out)
{
    int rv = 0;
    headers_out.clear();

    if(cjsonHeaderObject == NULL) {
        LOG_ERROR("No response");
        return -1;
    }

    if(cjsonHeaderObject->type == cJSON2_Object) {
        cJSON2* currObject = cjsonHeaderObject->child;
        if(currObject == NULL) {
            // Syncbox GET accessinfo for PUT does not have headers. Treat it as no error.
            // TODO: instead of logging anything here, it would be better to log the headers,
            //   but only if an error occurs when making the HTTP request.
            LOG_INFO("No headers");
        } else {
            if(currObject->prev != NULL) {
                LOG_ERROR("Unexpected prev value. Continuing.");
            }
        }

        while(currObject != NULL) {
            //LOG_ALWAYS("currObject "FMT_cJSON2_OBJ, VAL_cJSON2_OBJ(currObject));
            if(currObject->type == cJSON2_String) {
                VcsHeaderPair header;
                header.headerName = currObject->string;
                header.headerValue = currObject->valuestring;
                headers_out.push_back(header);
            }else{
                LOG_WARN("Unrecognized object element:"FMT_cJSON2_OBJ,
                         VAL_cJSON2_OBJ(currObject));
            }

            currObject = currObject->next;
        }
    }else{
        LOG_ERROR("Response returned was not a cjson object:%d",
                  cjsonHeaderObject->type);
        rv = -3;
    }
    return rv;
}

int parseVcsAccessInfoResponse(cJSON2* cjsonAccessInfo,
                               VcsAccessInfo& accessInfo_out)
{
    int rv = 0;
    int rc;

    accessInfo_out.clear();

    if(cjsonAccessInfo == NULL) {
        LOG_ERROR("No response");
        return -1;
    }
    if(cjsonAccessInfo->prev != NULL) {
        LOG_ERROR("Unexpected prev value. Continuing.");
    }
    if(cjsonAccessInfo->next != NULL) {
        LOG_ERROR("Unexpected next value. Continuing.");
    }

    if(cjsonAccessInfo->type == cJSON2_Object) {
        cJSON2* currObject = cjsonAccessInfo->child;
        if(currObject == NULL) {
            LOG_ERROR("Object returned has no children.");
            return -2;
        }
        if(currObject->prev != NULL) {
            LOG_ERROR("Unexpected prev value. Continuing.");
        }

        while(currObject != NULL) {
            //LOG_ALWAYS("currObject "FMT_cJSON2_OBJ, VAL_cJSON2_OBJ(currObject));
            if(currObject->type == cJSON2_Object) {
                if(strcmp("header", currObject->string)==0) {
                    rc = parseVcsAccessInfoHeaderObject(currObject,
                                                        accessInfo_out.header);
                    if(rc != 0) {
                        LOG_ERROR("parseVcsAccessInfoHeaderObject:%d", rc);
                        rv = rc;
                    }
                }
            }else if(currObject->type == cJSON2_String) {
                if(strcmp("accessUrl", currObject->string)==0) {
                    accessInfo_out.accessUrl = currObject->valuestring;
                }else if(strcmp("locationName", currObject->string)==0) {
                    accessInfo_out.locationName = currObject->valuestring;
                }else{
                    // do nothing, other fields should be ignored.
                }
            }else{
                LOG_WARN("Unrecognized object element:"FMT_cJSON2_OBJ,
                         VAL_cJSON2_OBJ(currObject));
            }

            currObject = currObject->next;
        }

    }else{
        LOG_ERROR("Response returned was not a cjson object:%d",
                  cjsonAccessInfo->type);
        rv = -3;
    }

    return rv;
}

