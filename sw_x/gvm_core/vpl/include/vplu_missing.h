/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */
#ifndef __VPLU_MISSING_H__
#define __VPLU_MISSING_H__

//============================================================================
/// @file
/// VPL utility (VPLU) to provide standard functions, types, and definitions
/// to platforms that are missing them.
/// After including this file, all platforms should have at least the following:
/// <pre>
/// bool
/// false
/// ssize_t
/// STDERR_FILENO
/// STDIN_FILENO
/// STDOUT_FILENO
/// size_t strnlen(const char *s, size_t maxlen);
/// true
/// va_copy
/// </pre>
//============================================================================

#include "vpl_plat.h"
#include "vplu__missing.h"

#endif // include guard
