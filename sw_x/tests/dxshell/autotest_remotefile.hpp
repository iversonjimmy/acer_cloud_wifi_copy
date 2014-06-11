//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef AUTOTEST_REMOTEFILE_H_
#define AUTOTEST_REMOTEFILE_H_

int do_autotest_sdk_release_remotefile(int argc, const char* argv[]);
int do_autotest_sdk_release_remotefile_vcs(int argc, const char* argv[]);

#include "dx_common.h"

int autotest_sdk_release_remotefile_basic(bool is_media,
                                          bool is_support_media,
                                          bool full,
                                          u64 userId,
                                          u64 deviceId,
                                          const std::string &datasetId_str,
                                          const std::string &work_dir,
                                          const std::string &work_dir_client,
                                          const std::string &work_dir_local,
                                          const std::string &upload_dir,
                                          const std::vector< std::pair<std::string, VPLFS_file_size_t> > &temp_filelist,
                                          const std::string &cloudPCSeparator,
                                          const std::string &clientPCSeparator,
                                          const std::string &clientPCSeparator2);

int remotefile_feature_enable(int cloudpc,
                              int client,
                              u64 userId,
                              u64 deviceId,
                              bool enable_rf,
                              bool enable_media,
                              std::string alias_cloudpc,
                              std::string alias_client);

int remotefile_vcs_internet_clipboard_basic(u64 userId,
                                            const std::string &work_dir_md,
                                            const std::string &work_dir_local,
                                            const std::string &clientPCSeparator);

#endif // #ifndef AUTOTEST_REMOTEFILE_H_

