/*
 *  Copyright 2011 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND 
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef STORAGE_NODE__VSS_UTIL_HPP__
#define STORAGE_NODE__VSS_UTIL_HPP__

/// @file
/// Virtual Storage Service Utilities
/// Utility functions for miscellaneous common tasks.

#include <string>

/// In-place decode of base64 string to unencoded string.
void decode64(std::string& data);

/// More flexible alternative. Allocates out_buf for decoded string.
int decode64(const char* in_buf, size_t in_len, char** out_buf, size_t* out_len, bool oneline);

#endif // include guard
