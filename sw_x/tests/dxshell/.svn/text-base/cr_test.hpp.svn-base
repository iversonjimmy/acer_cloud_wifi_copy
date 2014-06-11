//
//  Copyright 2012 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef CR_TEST_HPP_02172012
#define CR_TEST_HPP_02172012

#include <vplu_types.h>
#include <string>
#include <vector>

extern const char* PICSTREAM_STR;
#define CR_TEST_UP_DSET_NAME     "CR Upload"
#define CR_TEST_DOWN_DSET_NAME   "CameraRoll"

int cmd_cr_check_status(int argc, const char* argv[]);
int cmd_cr_clear(int argc, const char* argv[]);
int cmd_cr_upload_photo(int argc, const char* argv[]);
int cmd_cr_upload_big_photo_delete(int argc, const char* argv[]);
int cmd_cr_upload_big_photo_slow(int argc, const char* argv[]);
int cmd_cr_rename_download_dir(int argc, const char* argv[]);
int cmd_cr_send_file_to_stream(int argc, const char* argv[]);
int cmd_cr_set_full_res_dl_dir(int argc, const char* argv[]);
int cmd_cr_set_low_res_dl_dir(int argc, const char* argv[]);
int cmd_cr_upload_enable(int argc, const char* argv[]);
int cmd_start_cr_up(int argc, const char* argv[]);
int cmd_start_cr_down(int argc, const char* argv[]);
int cmd_cr_upload_dirs_add(int argc, const char* argv[]);
int cmd_cr_upload_dirs_rm(int argc, const char* argv[]);
int cmd_cr_trigger_upload_dir(int argc, const char* argv[]);
int cmd_cr_set_thumb_dl_dir(int argc, const char* argv[]);
int cmd_cr_list_items(int argc, const char* argv[]);
int cmd_cr_global_delete_enable(int argc, const char* argv[]);

int cr_subscribe_up(bool android);
int cr_subscribe_down(const std::string& localDsetName,
                      u64 maxBytes, bool useMaxBytes,
                      u64 maxFiles, bool useMaxFiles);
int cr_unsubscribe_down();
int cr_check_status_test();
int cr_upload_enable_test(bool enable);
int cr_clear_test(bool clearUploadFolder, bool clearDownloadFolder);
int cr_upload_photo_test(u32 numPhotos,
                         const std::string& testPhotoPath,
                         const std::string& testPhotoName,
                         const std::string& testPhotoExt,
                         const std::string& tempFolder,
                         const std::string& srcPhotoDir,
                         int deleteAfterMs);
int cr_upload_big_photo_pulse(const std::string& testPhotoPath,
                              int timeMs,
                              int pulseSizeKb);
int cr_add_upload_dirs(std::vector<std::string>& uploadDirs);
int cr_rm_upload_dirs(std::vector<std::string>& uploadDirs);
int cr_get_upload_dir(int index, std::string& uploadDir);

#endif // CR_TEST_HPP_02172012
