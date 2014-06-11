//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_LIMITS_H__
#define __VPL_LIMITS_H__

//============================================================================
/// @file
/// Virtual Platform Layer resource limits.
/// Please see @ref VPLLimits.
//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLLimits VPL Resource Limits
///
/// These declarations define the resource limits of the games operating
/// above the VPL.
///
///@{

/// Max virtual memory (address space) is 768 MB = 768 * 1024 * 1024.
#define VPLLIMIT_AS (768*1024*1024)

/// Max RAM is same as max virtual memory size.
#define VPLLIMIT_MEMLOCK (VPLLIMIT_AS)

/// Max posix thread priority is 99.
#define VPLLIMIT_RTPRIO (99)

/// Max 64 thread limit.
#define VPLLIMIT_NPROC (64)

/// Max 1023 open file descriptors (one less than VPLLIMIT_NOFILE).
#define VPLLIMIT_NOFILE (1024)

/// Max stack size is 2 MB = 2 * 1024 * 1024.
#define VPLLIMIT_STACK (2*1024*1024)

///@}

#ifdef __cplusplus
}
#endif

#endif // include guard
