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
#include "vcs_file_url_access_basic_impl.hpp"

#include "vplex_assert.h"
#include <log.h>

VCSFileUrlAccessBasicImpl::VCSFileUrlAccessBasicImpl(
        CreateVCSFileUrlOperation create_http_operation,
        CreateVCSFileUrlOperation create_acer_ts_operation)
:   createHttpOperation(create_http_operation),
    createAcerTsOperation(create_acer_ts_operation)
{
    // Programmer error if no create operations are defined.
    ASSERT(create_http_operation != NULL || create_acer_ts_operation != NULL);
}

VCSFileUrlAccessBasicImpl::~VCSFileUrlAccessBasicImpl()
{}

VCSFileUrlOperation* VCSFileUrlAccessBasicImpl::createOperation(
        const VcsAccessInfo& accessInfo,
        bool verbose_log,
        int& err_code__out)
{
    err_code__out = 0;
    const std::string& url = accessInfo.accessUrl;
    // See wiki.ctbg.acer.com/wiki/index.php/Home_Storage_Virtual_Sync_Design#Design_Changes
    // Tunnel service URL format:
    //     acer-ts://<userId>/<deviceId>/<instanceId>/<urlForStorageNodeHandler>
    if (url.find("acer-ts://")==0)
    {
        if (createAcerTsOperation==NULL) {
            LOG_ERROR("Not initialized to createAcerTsOperation(%s).", url.c_str());
            err_code__out = VPL_ERR_NOMEM;
            return NULL;
        }
        return createAcerTsOperation(accessInfo, verbose_log, /*OUT*/ err_code__out);

    } else if (url.find("http://")==0 ||
               url.find("https://")==0)
    {
        if (createHttpOperation==NULL) {
            LOG_ERROR("Not initialized to createHttpOperation(%s).", url.c_str());
            err_code__out = -1;
            return NULL;
        }
        return createHttpOperation(accessInfo, verbose_log, /*OUT*/ err_code__out);

    } else {
        LOG_ERROR("Protocol in URL not recognized(%s)", url.c_str());
        err_code__out = -1;
        return NULL;
    }
}

void VCSFileUrlAccessBasicImpl::destroyOperation(VCSFileUrlOperation* vcsFileUrlOperation)
{
    delete vcsFileUrlOperation;
}
