//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL__ERROR_H__
#define __VPL__ERROR_H__
#ifdef __cplusplus
extern "C" {
#endif

/// Translate an error code from system errno to VPL_ERR_*.
int VPLError_XlatErrno(int err_code);

#ifdef __cplusplus
}
#endif
#endif // include guard
