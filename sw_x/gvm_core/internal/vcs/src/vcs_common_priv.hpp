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

#ifndef __VCS_COMMON_PRIV_HPP_4_28_2014__
#define __VCS_COMMON_PRIV_HPP_4_28_2014__

#include "vcs_common.hpp"
#include "vcs_defs.hpp"
#include "vplu_types.h"
#include "vplex_http2.hpp"
#include "cJSON2.h"
#include <string>

// Internal helper functions/macros that are shared between multiple vcs_util
// functions.

#define FMT_cJSON2_OBJ "cjson Name:%s, Type:%d"
#define VAL_cJSON2_OBJ(s)  (s)->string, (s)->type

std::string escapeJsonString(const std::string& input);

int addSessionHeadersHelper(const VcsSession& vcsSession,
                            VPLHttp2& httpHandle,
                            u32 vcsApiVersion);

bool isVcsErrorTemplate(const std::string& body,
                        int& error_out);

int parseVcsAccessInfoResponse(cJSON2* cjsonAccessInfo,
                               VcsAccessInfo& accessInfo_out);

#endif // include guard
