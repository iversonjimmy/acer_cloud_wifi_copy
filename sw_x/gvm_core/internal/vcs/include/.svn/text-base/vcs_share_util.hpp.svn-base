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
#ifndef VCS_SHARE_UTIL_HPP_4_15_2014_
#define VCS_SHARE_UTIL_HPP_4_15_2014_

#include "vcs_common.hpp"
#include "vcs_defs.hpp"
#include "vplex_http2.hpp"

// Share API is its own catagory separate from MediaMetadata, and will maintain
// its own API version (according to Vincent).
// http://wiki.ctbg.acer.com/wiki/index.php/Photo_Sharing_Design#APIs.2FInterface.2C_CCD_-_Infra

int VcsShare_share(const VcsSession& vcsSession,
                   VPLHttp2& httpHandle,
                   const VcsDataset& dataset,
                   const std::string& componentName,
                   u64 compId,
                   const std::vector<std::string>& recipientEmail,
                   bool printLog);

int VcsShare_unshare(const VcsSession& vcsSession,
                     VPLHttp2& httpHandle,
                     const VcsDataset& dataset,
                     const std::string& componentName,
                     u64 compId,
                     const std::vector<std::string>& recipientEmail,
                     bool printLog);

int VcsShare_deleteFileSharedWithMe(const VcsSession& vcsSession,
                                    VPLHttp2& httpHandle,
                                    const VcsDataset& dataset,
                                    const std::string& path,
                                    u64 compId,
                                    bool printLog);

#endif // VCS_SHARE_UTIL_HPP_4_15_2014
