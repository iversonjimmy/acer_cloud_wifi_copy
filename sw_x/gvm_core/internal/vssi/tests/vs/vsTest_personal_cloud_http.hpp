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

#ifndef __VSTEST_PERSONAL_CLOUD_HTTP_HPP__
#define __VSTEST_PERSONAL_CLOUD_HTTP_HPP__

#include <vplu_types.h>
#include "vplex_vs_directory.h"
#include "vpl_socket.h"

#include <string>

/// Test access to read/write dataset via HTTP interface.
int test_http_dataset_access(const vplex::vsDirectory::DatasetDetail& dataset,
                             u64 uid, u64 handle, const std::string& ticket);

#endif // include guard
