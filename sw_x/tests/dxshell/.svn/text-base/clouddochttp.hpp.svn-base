//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.

#ifndef __CLOUDDOCHTTP_H__
#define __CLOUDDOCHTTP_H__

#include <vplu_types.h>
#include "cJSON2.h"

struct InfraHttpInfo
{
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
};

typedef int (*clouddoc_subcmd_fn)(int argc, const char *argv[], std::string& response);

int dispatch_clouddochttp_cmd(int argc, const char* argv[]);

int clouddochttp_upload(u64 userId, const std::string &docname, const std::string &doc, const std::string &preview, const std::string &component_id, int revision, std::string& response);

int clouddochttp_list_requests(u64 userId, std::string& response);

int clouddochttp_get_progress(u64 userId, const std::string &requesthandle, std::string& response);

int clouddochttp_cancel_request(u64 userId, const std::string &requesthandle, std::string& response);

int clouddochttp_list_docs(u64 userId, const std::string &pagination, std::string& response);

int clouddochttp_get_metadata(u64 userId, const std::string &docname, const std::string &component_id, const std::string &revision, std::string& response);

int clouddochttp_download(u64 userId, const std::string &docname, const std::string &component_id, const std::string &revision, const std::string &outfilename, const std::string &rangeHeader, std::string& response);

int clouddochttp_download_preview(u64 userId, const std::string &docname, const std::string &component_id, const std::string &revision, const std::string &outfilename, std::string& response);

int clouddochttp_delete(u64 userId, const std::string &docname, const std::string &component_id, const std::string &revision, std::string& response);

int clouddochttp_move(u64 userId, const std::string &docname, const std::string &component_id, const std::string &revision, const std::string &newdocname, std::string& response);

int clouddochttp_delete_async(u64 userId, const std::string &docname, const std::string &component_id, const std::string &revision, std::string& response);

int clouddochttp_move_async(u64 userId, const std::string &docname, const std::string &component_id, const std::string &revision, const std::string &newdocname, std::string& response);

int clouddochttp_dataset_changes(u64 userId, const std::string &datasetId, const std::string &pagination, std::string& response);

int clouddochttp_check_conflict(u64 userId, const std::string &docname, const std::string &component_id, std::string& response);

int clouddochttp_check_copyback(u64 userId, const std::string &docname, const std::string &component_id, std::string& response);

std::string urlEncodingLoose(const std::string &sIn);

std::string createParameter(std::map<std::string, std::string> parameters);

std::string urlEncoding(std::string &sIn);

//functions for JSON parsing
int JSON_getJSONObject(cJSON2* node, const char* attributeName, cJSON2** value);
int JSON_getInt64(cJSON2* node, const char* attributeName, u64 &value);
int JSON_getString(cJSON2* node, const char* attributeName, std::string& value);

#endif // __CLOUDDOCHTTP_H__
