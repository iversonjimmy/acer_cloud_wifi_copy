//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.

#ifndef __PICSTREAMHTTP_H__
#define __PICSTREAMHTTP_H__

#include <vplu_types.h>

typedef int (*picstream_subcmd_fn)(int argc, const char *argv[], std::string& response);

int dispatch_picstreamhttp_cmd(int argc, const char* argv[]);

int picstream_get_image(u64 userId, const std::string &title, const std::string &component_id, const  std::string type, const std::string path, std::string& response);

int picstream_get_fileinfo(u64 userId, const std::string &title, const std::string &albumname, const std::string type, std::string& response);

int picstream_delete(u64 userId, const std::string &title, const std::string &component_id, const std::string &revision, std::string& response);

#endif // __CLOUDDOCHTTP_H__
