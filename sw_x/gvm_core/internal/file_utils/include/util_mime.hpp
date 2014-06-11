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

#ifndef _UTIL_MIME_HPP_
#define _UTIL_MIME_HPP_

#include <vplex_util.hpp>
#include <map>
#include <string>

void Util_CreateVideoMimeMap(std::map<std::string, std::string, case_insensitive_less> &m);
void Util_CreateAudioMimeMap(std::map<std::string, std::string, case_insensitive_less> &m);
void Util_CreatePhotoMimeMap(std::map<std::string, std::string, case_insensitive_less> &m);

const std::string &Util_FindVideoMediaTypeFromExtension(const std::string &extension);
const std::string &Util_FindAudioMediaTypeFromExtension(const std::string &extension);
const std::string &Util_FindPhotoMediaTypeFromExtension(const std::string &extension);

#endif // guard
