//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "gvm_file_utils.hpp"

#include "vpl_fs.h"
#include "vplex_file.h"

#include <log.h>

std::string Util_CleanupPath(const char* path)
{
    // NOTE: Please don't do any logging within this function; we use this
    //   function to compute the path to pass into the logging module initialization.

    std::string result;
    bool lastWasSlash = false;
    for (; *path != '\0'; path++) {
        char curr = *path;
        if (curr == '/' || curr == '\\') {
            if (!lastWasSlash) {
                result.append("/");
                lastWasSlash = true;
            }
        } else {
            result.append(1, curr);
            lastWasSlash = false;
        }
    }
    // Trim trailing slashes.
    while ((result.size() > 0) && result[result.size()-1] == '/') {
        result.resize(result.size()-1);
    }
    return result;
}

static void trimTrailingSlashesSelf(std::string& path)
{
    // Remove trailing
    while(path.size() > 0 && path.rfind('/') == path.size()-1) {
        path = path.substr(0, path.size()-1);
    }
}
std::string Util_trimTrailingSlashes(const std::string& path)
{
    std::string result = path;
    trimTrailingSlashesSelf(result);
    return result;
}
void Util_trimTrailingSlashes(const std::string& path_in,
                              std::string& path_out)
{
    path_out = path_in;
    trimTrailingSlashesSelf(path_out);
}

static void trimLeadingSlashesSelf(std::string& path)
{
    // Remove leading
    while(path.find('/') == 0) {
        path = path.substr(1);
    }
}
std::string Util_trimLeadingSlashes(const std::string& path)
{
    std::string result = path;
    trimLeadingSlashesSelf(result);
    return result;
}
void Util_trimLeadingSlashes(const std::string& path_in,
                             std::string& path_out)
{
    // Remove leading
    path_out = path_in;
    trimLeadingSlashesSelf(path_out);
}

std::string Util_trimSlashes(const std::string& path)
{
    std::string result = path;
    trimTrailingSlashesSelf(result);
    trimLeadingSlashesSelf(result);
    return result;
}
void Util_trimSlashes(const std::string& path_in,
                      std::string& path_out)
{
    Util_trimTrailingSlashes(path_in, path_out);
    Util_trimLeadingSlashes(path_out, path_out);
}

void Util_appendToAbsPath(const std::string& path,
                          const std::string& name,
                          std::string& absPath_out)
{
    std::string tempName;
    std::string tempPath;
    Util_trimSlashes(name, tempName);
    Util_trimTrailingSlashes(path, tempPath);
    if(tempName == "") {
        absPath_out = tempPath;
    } else if(tempPath == "") {
        absPath_out = tempName;
    } else {
        absPath_out = tempPath+"/"+tempName;
    }
}

std::string Util_getParent(const std::string& path)
{
    std::string parent_out;
    ssize_t index = static_cast<ssize_t>(path.rfind('/'));

#ifdef WIN32
    // Actually we don't expect any '\\' values in the path because it should
    // have been cleaned in ccdiimpl.  Can be removed, but keeping this here
    // just in case.
    ssize_t index2 = static_cast<ssize_t>(path.rfind('\\'));
    if(index2 > index) {
        index = index2;
    }
#endif

    if(index < 0) {
        parent_out = "";
    } else {
        parent_out = path.substr(0, index);
    }
    std::string parent_out_noTrailSlash;
    Util_trimTrailingSlashes(parent_out,
                             parent_out_noTrailSlash);
    return parent_out_noTrailSlash;
}

std::string Util_getChild(const std::string& path)
{
    std::string child_out;
    ssize_t index = static_cast<ssize_t>(path.rfind('/'));

#ifdef WIN32
    // Actually we don't expect any '\\' values in the path because it should
    // have been cleaned in ccdiimpl.  Can be removed, but keeping this here
    // just in case.
    ssize_t index2 = static_cast<ssize_t>(path.rfind('\\'));
    if(index2 > index) {
        index = index2;
    }
#endif
    index++;  // increment one past the '/' character.
    if(index >= (int)path.size()) {
        // child_out is null when the final character is '/'
    } else if(index < 0) {
        child_out = path;
    } else {
        child_out = path.substr(index, path.size()-index);
    }

    return child_out;
}

// ie.  If filename is example.txt, ext_out will be txt.
std::string Util_getFileExtension(const std::string& filename)
{
    const int extensionMinInclusive = 3;
    const int extensionMaxInclusive = 4;
    std::size_t extIndex = filename.find_last_of(".") + 1;
    if (extIndex != std::string::npos && extIndex < filename.size()) {
        std::string myExt = filename.substr(extIndex);
        if (myExt.size() < extensionMinInclusive ||
            myExt.size() > extensionMaxInclusive)
        {
            LOG_WARN("Invalid extension(%s)", filename.c_str());
        } else
        {   // success, extension found
            return myExt;
        }
    }
    return std::string("");
}

