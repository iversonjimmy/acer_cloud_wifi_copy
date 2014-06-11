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
#ifndef SHAREDFILES_HPP_4_14_2014
#define SHAREDFILES_HPP_4_14_2014

#include "vpl_plat.h"
#include "vplu_types.h"
#include <string>
#include <vector>

int SharedFiles_StoreFile(u32 activationId,
                         const std::string& file_absPath,
                         const std::string& opaque_metadata,
                         const std::string& preview_absPath,
                         u64& compId_out,
                         std::string& name_out);

int SharedFiles_ShareFile(u32 activationId,
                          const std::string& datasetRelPath,
                          u64 compId,
                          const std::vector<std::string>& recipientEmails);

int SharedFiles_UnshareFile(u32 activationId,
                            const std::string& datasetRelPath,
                            u64 compId,
                            const std::vector<std::string>& recipientEmails);

int SharedFiles_DeleteFile(u32 activationId,
                           const std::string& datasetRelPath,
                           u64 compId);

#endif // SHAREDFILES_HPP_4_14_2014
