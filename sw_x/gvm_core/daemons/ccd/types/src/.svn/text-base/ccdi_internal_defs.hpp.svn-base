//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __CCDI_INTERNAL_DEFS_HPP__
#define __CCDI_INTERNAL_DEFS_HPP__

//============================================================================
/// @file
/// This header defines constants used for serialization between libccdi and ccd.
/// It needs to be in a common location but nothing else should use it.
//============================================================================

#ifdef WIN32

/// Suggested name is "com.igware.ccdi.sock.<testInstanceNum>.<VPL_GetOSUserId()>".
/// <testInstanceNum> is to allow testing multiple instances as the same user on a single host; it
/// should always be 0 in production.
#define CCDI_PROTOBUF_SOCKET_NAME_FMT     "com.igware.ccdi.sock.%d.%s"
// VPL_GetOSUserId() can be up to 184 bytes, see http://stackoverflow.com/questions/1140528/what-is-the-maximum-length-of-a-sid-in-sddl-format
static const int CCDI_PROTOBUF_SOCKET_NAME_MAX_LENGTH = 225;

#else

/// Suggested name is "/tmp/com.igware.ccdi.sock.<testInstanceNum>.<VPL_GetOSUserId()>".
/// <testInstanceNum> is to allow testing multiple instances as the same user on a single host; it
/// should always be 0 in production.
#define CCDI_PROTOBUF_SOCKET_NAME_FMT     "/tmp/com.igware.ccdi.sock.%d.%s"
static const int CCDI_PROTOBUF_SOCKET_NAME_MAX_LENGTH = 100;

#endif

#endif // include guard
