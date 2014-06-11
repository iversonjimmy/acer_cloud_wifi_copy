/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */

#ifndef __DXSHELL_MINIDMS_TEST_H__
#define __DXSHELL_MINIDMS_TEST_H__

#include <vplu_types.h>
#include <string>
#include "media_metadata_types.pb.h"

typedef int (*minidms_subcmd_fn)(int argc, const char *argv[], std::string& response);

int dispatch_minidms_test_cmd(int argc, const char* argv[]);

int minidms_protocolinfo(u64 userId, std::string mimetype, std::string& response);
int minidms_samplemetadata(u64 userId, std::string protocolInfo, std::string contentUrl, std::string albumartUrl, std::string& response);
int minidms_msa_contenturl(u64 userId, std::string collectionId, std::string objectId, media_metadata::CatalogType_t type, std::string& response);
int minidms_add_pinitem(u64 userId, std::string filepath, std::string source_device_id, std::string object_id, std::string& response);
int minidms_remove_pinitem(u64 userId, std::string filepath, std::string& response);
int minidms_get_pinitem(u64 userId, std::string filepath, std::string& response);
int minidms_deviceinfo(u64 userId, std::string device_id, std::string& response);

#endif
