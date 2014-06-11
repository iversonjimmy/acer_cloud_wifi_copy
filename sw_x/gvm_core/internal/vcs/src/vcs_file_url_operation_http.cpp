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
#include "vcs_file_url_operation_http.hpp"
#include "vplex_assert.h"
#include "vpl_th.h"
#include <log.h>

class VCSFileUrlOperation_HttpImpl : public VCSFileUrlOperation
{
public:
    VCSFileUrlOperation_HttpImpl(
            const VcsAccessInfo& accessInfo,
            bool verbose_log)
    :   accessInfo(accessInfo),
        verbose_log(verbose_log)
    {
    }

    virtual ~VCSFileUrlOperation_HttpImpl() {}

    virtual int GetFile(const std::string& dest_file_abs_path)
    {
        // Skipping the use-only-once check here, since the underlying httpHandle will enforce it.
        int rc;
        rc = vcs_s3_getFileHelper(accessInfo,
                                  httpHandle,
                                  NULL, NULL,
                                  dest_file_abs_path,
                                  verbose_log);
        if (rc != 0) {
            LOG_ERROR("vcs_s3_getFileHelper(%s):%d", dest_file_abs_path.c_str(), rc);
        }
        return rc;
    }

    virtual int PutFile(const std::string& src_file_abs_path)
    {
        // Skipping the use-only-once check here, since the underlying httpHandle will enforce it.
        int rc = vcs_s3_putFileHelper(accessInfo,
                                      httpHandle,
                                      NULL, NULL,
                                      src_file_abs_path,
                                      verbose_log);
        if (rc != 0) {
            LOG_ERROR("vcs_s3_putFileHelper(%s):%d", src_file_abs_path.c_str(), rc);
        }
        return rc;
    }

    virtual int PutFile(const std::string& src_file_abs_path, std::string& hash__out)
    {
        // Skipping the use-only-once check here, since the underlying httpHandle will enforce it.
        int rc = vcs_s3_putFileHelper(accessInfo,
                                      httpHandle,
                                      NULL, NULL,
                                      src_file_abs_path,
                                      verbose_log,
                                      /*out*/ hash__out);
        if (rc != 0) {
            LOG_ERROR("vcs_s3_putFileHelper(%s):%d", src_file_abs_path.c_str(), rc);
        }
        return rc;
    }

    virtual int DeleteFile()
    {
        FAILED_ASSERT("Not currently supported");
        return -1;
    }

    virtual int Cancel()
    {
        int rc = httpHandle.Cancel();
        if (rc != 0) {
            LOG_ERROR("httpHandle.Cancel():%d", rc);
        }
        return rc;
    }

private:
    VPL_DISABLE_COPY_AND_ASSIGN(VCSFileUrlOperation_HttpImpl);

    VcsAccessInfo accessInfo;
    VPLHttp2 httpHandle;
    bool verbose_log;
};

VCSFileUrlOperation* CreateVCSFileUrlOperation_HttpImpl(
        const VcsAccessInfo& accessInfo,
        bool verbose_log,
        int& errCode_out)
{
    VCSFileUrlOperation_HttpImpl* newOperation =
            new (std::nothrow) VCSFileUrlOperation_HttpImpl(accessInfo, verbose_log);
    if (newOperation == NULL) {
        LOG_ERROR("Out of mem");
        errCode_out = VPL_ERR_NOMEM;
        return NULL;
    }
    return newOperation;
}
