//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

//============================================================================
/// @file
/// Utility functions that use C++.
//============================================================================

#ifndef _GVM_FILE_UTILS_HPP_
#define _GVM_FILE_UTILS_HPP_

#include "gvm_file_utils.h"

#include <string>

//@{
/// Removes excess slashes, and normalizes backslashes to forward slashes.
std::string Util_CleanupPath(const char* path);

static inline
std::string Util_CleanupPath(const std::string& path)
{
    return Util_CleanupPath(path.c_str());
}

std::string Util_trimTrailingSlashes(const std::string& path);

void Util_trimTrailingSlashes(const std::string& path_in,
                              std::string& path_out);

std::string Util_trimLeadingSlashes(const std::string& path);

void Util_trimLeadingSlashes(const std::string& path_in,
                             std::string& path_out);

/// Combination of #Util_trimTrailingSlashes() and #Util_trimLeadingSlashes().
std::string Util_trimSlashes(const std::string& path);

/// Combination of #Util_trimTrailingSlashes() and #Util_trimLeadingSlashes().
void Util_trimSlashes(const std::string& path_in,
                      std::string& path_out);

void Util_appendToAbsPath(const std::string& path,
                          const std::string& name,
                          std::string& absPath_out);

std::string Util_getParent(const std::string& path);

std::string Util_getChild(const std::string& path);

std::string Util_getFileExtension(const std::string& path);

/// Case insensitive compare for two UTF-8 strings.
/// @return 0 if \a str1 and \a str2 are the same,
///         -1 if \a str1 comes before \a str2,
///         1 if \a str2 comes before \a str1.
int Util_UTF8CaseNCmp(int str1_len, const char* str1, int str2_len, const char* str2);

/// Convert the UTF-8 string \a src to uppercase, storing the result in \a upper_out.
void Util_UTF8Upper(const char* src, std::string& upper_out);
std::string Util_UTF8Upper(const char* src);

#endif // include guard
