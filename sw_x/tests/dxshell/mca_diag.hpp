//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef MCA_DIAG_HPP_05_28_2013
#define MCA_DIAG_HPP_05_28_2013

#include <ccdi.hpp>
#include <vpl_plat.h>
#include <vplu_types.h>
#include <string>

#define MUSIC_ALBUM0_TRACK0_NEW_TITLE "MusicAlbum0Track0_NewTitle"
#define PHOTO_ALBUM0_PHOTO0_NEW_TITLE "PhotoAlbum0Photo0_NewTitle"

extern const char* MCA_STR;

int mca_diag(const std::string &test_clip_name,
             std::string &test_url,
             std::string &test_thumb_url,
             bool checkCloudpc,
             bool checkClient,
             bool using_remote_agent);
int mca_get_metadata_dl_path(std::string& downloadPath_out);
int mca_count_metadata_files(u64& allFiles_out,
                             u64& index_out,
                             u64& photoFile_out);

/// Blocks until at least one Media Server exists for the user's account (or until retryTimeout).
/// Returns an error if more than one Media Server is detected.
/// If \a requireMediaServerOnline is true, it will also block until the media server is
/// reported as ONLINE.
int mca_get_media_servers(ccd::ListLinkedDevicesOutput& mediaServerDevice_out,
                          int retryTimeout,
                          bool requireMediaServerOnline);

int mca_list_metadata();
int mca_dump_metadata(const std::string strDumpFileFolder);
int mca_summary_metadata();
int mca_get_photo_object_id(const std::string &title, std::string &objectId);
int mca_get_photo_object_num(u32& photoAlbumNum, u32& photoNum);
int mca_get_music_object_id(const std::string &title, std::string &objectId);
int mca_get_music_object_num(u32& musicAlbumNum, u32& trackNum);
int mca_migrate_thumb(const std::string& dstDir);
int mca_stop_thumb_sync_cmd(int argc, const char* argv[]);
int mca_resume_thumb_sync_cmd(int argc, const char* argv[]);
int mca_status_cmd(int argc, const char* argv[]);

#endif /* MCA_DIAG_HPP_05_28_2013 */
