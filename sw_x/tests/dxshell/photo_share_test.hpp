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

#ifndef PHOTO_SHARE_TEST_HPP_4_16_2014_
#define PHOTO_SHARE_TEST_HPP_4_16_2014_

#include "vpl_plat.h"
#include "vplu_types.h"

#include "ccdi.hpp"

#include <string>
#include <vector>

extern const char* PHOTO_SHARE_STR;

int cmd_photo_share_enable(int argc, const char* argv[]);
int cmd_sharePhoto_store(int argc, const char* argv[]);
int cmd_sharePhoto_share(int argc, const char* argv[]);
int cmd_sharePhoto_unshare(int argc, const char* argv[]);
int cmd_sharePhoto_deleteSharedWithMe(int argc, const char* argv[]);
int cmd_sharePhoto_dumpSharedByMe(int argc, const char* argv[]);
int cmd_sharePhoto_dumpSharedWithMe(int argc, const char* argv[]);

int photo_share_enable(u64 userId);
int sharePhoto_store(u64 userId,
                     const std::string& absFilePath,
                     const std::string& opaqueMetadata,
                     const std::string& absPreviewPath,
                     u64& compId_out,
                     std::string& name_out);
int sharePhoto_share(u64 userId,
                     u64 compId,
                     const std::string& storedName,
                     const std::vector<std::string>& recipientEmails);
int sharePhoto_dumpFilesQuery(u64 userId,
                              ccd::SyncFeature_t syncFeature,
                              bool printResults,
                              ccd::SharedFilesQueryOutput& sfqRes_out);
int sharePhoto_unshare(u64 userId,
                       u64 compId,
                       const std::string& storedName,
                       const std::vector<std::string>& recipientEmails);
int sharePhoto_deleteSharedWithMe(u64 userId,
                                  u64 compId,
                                  const std::string& storedName);


#endif // PHOTO_SHARE_TEST_HPP_4_16_2014_
