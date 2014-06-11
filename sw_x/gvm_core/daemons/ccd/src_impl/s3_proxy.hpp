//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.


#ifndef __S3_PROXY_H__
#define __S3_PROXY_H__

#include <vpl_types.h>
#include <iostream>
#include <string.h>
#include <sstream>

#include <vplex_file.h>
#include <vplex_http2.hpp>

#include "stream_transaction.hpp"

int S3_getFile(stream_transaction* transaction, std::string bucket, std::string filename);

/// We don't want this to get mixed up with other putFile functions.
/// At the very least, error handling convention is different.
/// For SyncUp, a positive error means drop the job.
int S3_putFileSyncUp(const std::string &serviceName,
                     const std::string &comppath,
                     u64 modifyTime,
                     u64 userId,
                     u64 datasetId,
                     bool baserev_valid,
                     u64 baserev,
                     u64 compid,
                     const std::string &contentType,
                     const std::string &srcPath,
                     VPLHttp2 *http2,
                     /*OUT*/ std::string &vcsresponse);

int S3_getFile2HttpStreamAndFile(stream_transaction* transaction, std::string bucket, std::string filename);

#endif //__S3_PROXY_H__
