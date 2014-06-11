//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef _DX_COMMON_H_
#define _DX_COMMON_H_

#include <vpl_plat.h>
#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <vplex_file.h>
#include <vplex_http2.hpp>
#include <vpl_fs.h>
#include <vplu_types.h>
#include <vpl_socket.h>

#include "TargetDevice.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include "log.h"

#define CCDCONF_INFRADOMAIN_KEY "infraDomain"
#define DEFAULT_INFRA_DOMAIN "cloud.acer.com"
#define CCDCONF_USERGROUP_KEY "userGroup"
#define CCDCONF_ENABLE_ARCHIVE_DOWNLOAD "mediaMetadataSyncDownloadFromArchiveDevice"
#define CCDCONF_ENABLE_UPLOAD_VIRTUAL_SYNC "mediaMetadataUploadVirtualSync"
#define CCDCONF_CLEARFI_MODE "clearfiMode"
#define CCDCONF_ENABLE_TS "enableTs"
#define DEFAULT_USER_GROUP ""

#ifdef WIN32
#define DIR_DELIM "\\"
#else
#define DIR_DELIM "/"
#endif

#define CCD_CONF_TMPL_FILE      "ccd.conf.tmpl"
#define DX_REMOTE_IP_ENV        "DX_REMOTE_IP"
#define DX_REMOTE_PORT_ENV      "DX_REMOTE_PORT"
#define DX_OS_USER_ID_ENV       "DX_OS_USER_ID"
#define DX_VALGRIND_SUPP_FILE_ENV  "DX_VALGRIND_SUPP_FILE"

#define DX_REMOTE_DEFAULT_PORT  24000

#define DX_REMOTE_POLL_TIME_OUT  300

//////////////////////////////////////////////
////////// dxshell canned file data //////////
// TODO: Should change names to more descriptive file sizes (ie. "7mb") rather
// than arbitrary "small" and "large".
// For test.jpg (default)
#define PICSTREAM_TEST_CLIP_FILE_NAME  "test"
#define PICSTREAM_TEST_CLIP_LARGE_FILE_NAME "test_large"
#define PICSTREAM_TEST_CLIP_LARGE_FILE_EXT  "jpg"
// Reusing test.jpg
#define METADATA_PHOTO_TEST_CLIP_FILE  PICSTREAM_TEST_CLIP_FILE_NAME".jpg"
#define METADATA_PHOTO_TEST_CLIP_NAME  "TestPhoto"
#define METADATA_PHOTO_TEST_THUMB_FILE "test_thumb.jpg"
// Let's stream metadata
#define STREAMING_TEST_CLIP_NAME METADATA_PHOTO_TEST_CLIP_NAME
// Reusing picstream largefile
#define RF_TEST_LARGE_FILE PICSTREAM_TEST_CLIP_LARGE_FILE_NAME"."PICSTREAM_TEST_CLIP_LARGE_FILE_EXT

#define CLOUDDOC_DOCX_FILE "CloudDoc.docx"
#define CLOUDDOC_DOCX_JPG_FILE "CloudDoc.docx.jpg"
#define CLOUDDOC_DOCX_FILE2 "CloudDoc2.docx"
#define CLOUDDOC_DOCX_JPG_FILE2 "CloudDoc2.docx.jpg"
#define CLOUDDOC_DOCX_DUMMY "CloudDocTestDummy"
#define CLOUDDOC_DOCX_100M_DUMMY "CloudDocTest100MDummy"
////////// dxshell canned file data //////////
//////////////////////////////////////////////

/// Any files placed in this path (under the current directory) will be preserved by the
/// log collection steps, enabling later analysis.
#define DX_FAILURE_DIRECTORY "dxshellFailureDir"

extern int testInstanceNum;


//// High level diagnostic functions ////
int msa_diag(const std::string &test_clip_path,
             const std::string &test_clip_name,
             const std::string &test_thumb_name_path,
             bool useLongPath,
             bool incrementTimestamp,
             bool addOnly,
             u32 num_music_albums, u32 num_music_tracks_per_album,
             u32 num_photo_albums, u32 num_photos_per_album,
             bool using_remote_agent);
int msaGetMetadataUploadPath(std::string& uploadPath_out);
int msaDiagDeleteCatalog(TargetDevice *target = NULL);

typedef void (*LOGPrintFn)(LOGLevel level, const char* file, int line, const char* function, const char* format, ...);

int msa_add_cloud_music(const std::string& musicPath);

int msa_add_cloud_photo(const std::string& photoPath,
                        int intervalSec,
                        const std::string& albumName,
                        const std::string& thumbnailPath);

int msa_delete_object(media_metadata::CatalogType_t catType,
                      const std::string& collectionId,
                      const std::string& objectId);

int msa_delete_collection(media_metadata::CatalogType_t catType,
                          const std::string& collectionId);
bool isEqualFileContents(const char* lhsPath, const char* rhsPath);

int swup_fetch_all(const std::string& strGuids,
                   const std::string& strDestdir,
                   const std::string& strAppVer,
                   const bool just_check,
                   const bool polled,
                   const bool android_guids,
                   const bool refresh);
int swup_diag(int guid_set, std::string strDestdir);
int get_cr_photo_count(const std::string& directory, u32& numPhotos_out);
int get_cr_photo_count_cloudnode(const std::string& directory, u32& numPhotos_out);

int start_ccd_in_client_subdir();

// Sub commands dispatch function (Sub commands are quick command used for operation diagnostic purpose)
int proc_subcmd(const std::string &testroot, int argc, const char* argv[]);

// First level subcommands
int set_domain(int argc, const char* argv[]);
int set_group(int argc, const char* argv[]);
int set_sync_mode_upload(int argc, const char* argv[]);
int set_sync_mode_download(int argc, const char* argv[]);
int start_ccd(int argc, const char* argv[]);
#ifdef VPL_PLAT_IS_WIN_DESKTOP_MODE
int start_ccd_by_srv(int argc, const char* argv[]);
int stop_ccd_by_srv(int argc, const char* argv[]);
#endif
int stop_ccd_hard(int argc, const char* argv[]);
int stop_ccd_soft(int argc, const char* argv[]);
int start_cloudpc(int argc, const char* argv[]);
int stop_cloudpc(int argc, const char* argv[]);
int update_psn(int argc, const char* argv[]);
int start_client(int argc, const char* argv[]);
int stop_client(int argc, const char* argv[]);
int check_metadata(int argc, const char* argv[]);
int check_metadata2(int argc, const char* argv[]);
/// If the target device is CloudPC, calls msa_diag (commit collections to MSA, then delete one).
/// If the target device is not CloudPC, calls mca_diag
int check_metadata_generic(int argc, const char* argv[], bool using_remote_agent);
int mca_count_metadata_files_cmd(int argc, const char* argv[]);
int setup_check_metadata(int argc, const char* argv[]);
int mca_list_metadata(int argc, const char* argv[]);
int mca_list_allphotos(int argc, const char* argv[]);
int mca_migrate_thumb_cmd(int argc, const char* argv[]);
int msa_delete_catalog(int argc, const char* argv[]);
int check_streaming(int argc, const char* argv[]);
int check_streaming2(int argc, const char* argv[]);
int check_streaming_generic(int argc, const char* argv[], bool using_remote_agent);
int check_clouddoc(int argc, const char* argv[]);
int dump_events(int argc, const char* argv[]);
int download_updates(int argc, const char* argv[]);
int time_stream_download(int argc, const char* argv[]);
int wake_sleeping_devices(int argc, const char* argv[]);
int list_devices(int argc, const char* argv[]);
int list_user_storage(int argc, const char* argv[]);
int enable_cloudpc(int argc, const char* argv[]);
int report_lan_devices(int argc, const char* argv[]);
int list_lan_devices(int argc, const char* argv[]);
int probe_lan_devices(int argc, const char* argv[]);
int get_storage_node_ports(int argc, const char* argv[]);
int list_storage_node_datasets(int argc, const char* argv[]);
int report_network_connected(int argc, const char* argv[]);

int cloudmedia_commands(int argc, const char* argv[]);
int mca_dispatch(int argc, const char* argv[]);
int photo_share_dispatch(int argc, const char* argv[]);
int picstream_dispatch(int argc, const char* argv[]);
int power_dispatch(int argc, const char* argv[]);
int vcs_dispatch(int argc, const char* argv[]);
int vcspic_dispatch(int argc, const char* argv[]);
int ts_test(int argc, const char* argv[]);

int dispatch_fstest_cmd_with_response(int argc, const char* argv[], std::string& response);

int dispatch_clouddochttp_cmd_with_response(int argc, const char* argv[], std::string& response);
int dispatch_picstreamhttp_cmd_with_response(int argc, const char* argv[], std::string& response);

int http_get(int argc, const char* argv[]);

int postpone_sleep(int argc, const char* argv[]);

int userlogin_dispatch(int argc, const char* argv[]);

std::string getDxshellTempFolder();

int config_autotest(int argc, const char* argv[]);

int config_device(int argc, const char* argv[]);

int launch_android_cc_service();

int stop_android_cc_service();

int launch_android_dx_remote_agent();

int stop_android_dx_remote_agent();

int restart_android_dx_remote_agent();

int get_ccd_log_from_android();

int clean_ccd_log_on_android();

int check_android_net_status();  

int launch_dx_remote_agent_app(const char *alias);

int stop_dx_remote_agent_app(const char *alias);

int restart_dx_remote_agent_ios_app(const char *alias);

int remote_agent_poll(const char *alias);

int set_target_machine(const char *machine_alias);

int set_target_machine(const char *machine_alias, VPLNet_addr_t &ip_addr, VPLNet_port_t &port_num);

std::string get_target_machine(void);

int set_target(const char *machine_alias);

int stop_remoteagent(int argc, const char* argv[]);

int start_remoteagent(int argc, const char* argv[]);

int restart_remoteagent_app(int argc, const char* argv[]);

int enable_in_memory_logging(int argc, const char* argv[]);

int disable_in_memory_logging(int argc, const char* argv[]);

int flush_in_memory_logs(int argc, const char* argv[]);

#endif // #ifndef _DX_COMMON_H_
