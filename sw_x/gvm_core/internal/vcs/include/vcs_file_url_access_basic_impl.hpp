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

#ifndef __VCS_FILE_URL_ACCESS_BASIC_IMPL_HPP_03_12_2014__
#define __VCS_FILE_URL_ACCESS_BASIC_IMPL_HPP_03_12_2014__

#include "vcs_file_url_access.hpp"

typedef VCSFileUrlOperation* (*CreateVCSFileUrlOperation)(const VcsAccessInfo& access_info,
                                                          bool verbose_log,
                                                          int &err_code__out);

/// Default implementation of VCSFileUrlAccess.
class VCSFileUrlAccessBasicImpl : public VCSFileUrlAccess
{
public:
    VCSFileUrlAccessBasicImpl(CreateVCSFileUrlOperation create_http_operation,
                              CreateVCSFileUrlOperation create_acer_ts_operation);

    ~VCSFileUrlAccessBasicImpl();

    VCSFileUrlOperation* createOperation(const VcsAccessInfo& access_info,
                                         bool verbose_log,
                                         int& err_code__out);

    void destroyOperation(VCSFileUrlOperation* vcs_file_url_operation);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(VCSFileUrlAccessBasicImpl);

    CreateVCSFileUrlOperation createHttpOperation;
    CreateVCSFileUrlOperation createAcerTsOperation;
};

#endif // __VCS_FILE_URL_ACCESS_HPP_03_12_2014__
