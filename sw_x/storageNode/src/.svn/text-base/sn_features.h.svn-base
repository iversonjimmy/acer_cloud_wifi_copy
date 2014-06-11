//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef __SN_FEATURE_H_01_16_2014__
#define __SN_FEATURE_H_01_16_2014__

#include "vpl_types.h"

///////////////////////////////////////
// The following defines use existence
#if defined(WIN32)
#define ENABLE_PHOTO_TRANSCODE 1
#elif defined(_LINUX) || defined(__linux__)
#define ENABLE_PHOTO_TRANSCODE 1
#elif defined(CLOUDNODE)
#define ENABLE_PHOTO_TRANSCODE 1
#endif // defined(WIN32)

///////////////////////////////////////
// The following defines use whether the define is value 1 or 0
#if defined(VPL_PLAT_IS_WIN_DESKTOP_MODE)
#define ENABLE_REMOTE_FILE_SEARCH 1
#else
#define ENABLE_REMOTE_FILE_SEARCH 0
#endif // defined(VPL_PLAT_IS_WIN_DESKTOP_MODE)


#endif // __SN_FEATURE_H_01_16_2014__
