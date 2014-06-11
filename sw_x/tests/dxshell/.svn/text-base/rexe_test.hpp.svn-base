/*
 *  Copyright 2014 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */

#ifndef __DXSHELL_REMOTE_EXECUTABLE_TEST_H__
#define __DXSHELL_REMOTE_EXECUTABLE_TEST_H__

#include <vplu_types.h>
#include <string>

typedef int (*rexe_subcmd_fn)(int argc, const char *argv[], std::string& response);

int dispatch_rexe_test_cmd(int argc, const char* argv[]);

int rexe_register_executable(u64 user_id, std::string app_key, std::string executable_name, std::string absolute_path, u64 version_num, std::string& response);
int rexe_unregister_executable(u64 user_id, std::string app_key, std::string executable_name, std::string& response);
int rexe_list_registered_executable(u64 user_id, std::string app_key, std::string& response);
int rexe_execute(u64 user_id, std::string device_id, std::string app_key, std::string executable_name, std::string min_version_num, std::string request_body, std::string& response);

#endif
