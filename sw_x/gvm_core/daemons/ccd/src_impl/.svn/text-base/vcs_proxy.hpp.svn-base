/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef __VCS_PROXY_H__
#define __VCS_PROXY_H__

#include <vpl_types.h>
#include <string>
#include <vector>
#include <vplex_file.h>
#include "stream_transaction.hpp"

#include <CloudDocMgr.hpp>
#include <vssi_error.h>

int VCSGetCredentials(u64 uid, u64 &sessionHandle_out, string &serviceTicket_out);
int VCSpostUpload(DocSNGTicket* ticket, std::string&, std::string, VPLFile_offset_t);
int VCSpostUpload(const std::string &serviceName, const std::string &comppath, u64 modifyTime, u64 userId, u64 datasetId, bool baserev_valid, u64 baserev, u64 compid,
                  std::string &vcsresponse, std::string locationUri, VPLFile_offset_t filesize);
int VCSuploadPreview(DocSNGTicket* ticket, std::string metadata);

int VCSreadFile(stream_transaction *transaction);
int VCSreadFolder(stream_transaction *transaction);
int VCSdeleteFile(stream_transaction *transaction);
int VCSdeleteFile(u64 uid, std::string sessionHandle, std::string serviceTicket, const std::string &userReq, std::string &response, int &httpResponse);
int VCSmoveFile(stream_transaction *transaction);
int VCSmoveFile(u64 uid, std::string sessionHandle, std::string serviceTicket, const std::string &userReq, std::string &response, int &httpResponse);
int VCSdownloadPreview(stream_transaction *transaction);
int VCSgetDatasetChanges(u64 userid, std::string sessionHandle, std::string serviceTicket, u64 datasetid, u64 changeSince, int max, bool includeDeleted, int &httpResponse, std::string &response);
int VCSdummy();

int VCSreadFile_helper(u64 uid, std::string sessionHandle, std::string serviceTicket, u64 deviceId, const std::string &uri, std::string &response);
int VCSreadFile_helper(u64 uid, u64 datasetId, std::string sessionHandle, std::string serviceTicket, u64 deviceId, const std::string &uri, const char *serviceNameOverride,
                       /*OUT*/ std::string &response);

int VCSreadFolder_helper(u64 uid, std::string sessionHandle, std::string serviceTicket, u64 deviceId, const std::string &uri, std::string &response);
int VCSreadFolder_helper(u64 uid, u64 datasetId, std::string sessionHandle, std::string serviceTicket, u64 deviceId, const std::string &uri, const char *serviceNameOverride,
                       /*OUT*/ std::string &response);

int VCSgetAccessInfo(u64 userid, const std::string &sessionHandle, const std::string &serviceTicket, const std::string &method, const std::string &userReq, const std::string &contentType,
                     /*OUT*/ std::string &accessurl, std::vector<std::string> &header, std::string &locationname);

// DEPRECATE: assumes clouddoc dataset
int VCSgetAccessInfo(u64 userid, const std::string &sessionHandle, const std::string &serviceTicket, const std::string &method, const std::string &userReq, const std::string &contentType,
                     /*OUT*/ std::string &accessurl, std::vector<std::string> &headerName, std::vector<std::string> &headerValue, std::string &locationname);

int VCSgetAccessInfo(u64 userid, u64 datasetid, const std::string &sessionHandle, const std::string &serviceTicket, const std::string &method, const std::string &userReq, const std::string &contentType,
                     /*OUT*/ std::string &accessurl, std::vector<std::string> &headerName, std::vector<std::string> &headerValue, std::string &locationname);
int VCSgetAccessInfo(u64 userid, u64 datasetid, const std::string &sessionHandle, const std::string &serviceTicket, const std::string &method, const std::string &userReq, const std::string &contentType, const std::string &categoryName, const std::string &protocolVersion,
                     /*OUT*/ std::string &accessurl, std::vector<std::string> &headerName, std::vector<std::string> &headerValue, std::string &locationname);

VPLTHREAD_FN_DECL VcsCloudDoc_HandleTicket(void *vpTicket);

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
static VPLThread_return_t computeSHA1(void* arg);
#endif

int VCSgetPreview(stream_transaction *transaction, u64 datasetId);

#endif //__VCS_PROXY_H__
