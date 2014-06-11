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

//============================================================================
/// @file
/// Definitions from the old UCF (User Cache File) library that are still used.
//============================================================================

#ifndef __UCF_H__
#define __UCF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <vplu_types.h>

#define UCF_SESSION_SECRET_LENGTH   20

typedef struct {
    u32                 size;
    char*               data;
} UCFBuffer;

#ifdef __cplusplus
}
#endif

#endif // include guard
