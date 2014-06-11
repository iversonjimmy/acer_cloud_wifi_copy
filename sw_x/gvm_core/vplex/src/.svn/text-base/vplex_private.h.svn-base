//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_PRIVATE_H__
#define __VPLEX_PRIVATE_H__

/** @file
 * Common definitions private to the VPLex library implementation
 */

#include <vplex_trace.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VPL_LIB_LOG_ALWAYS(subgroup, ...) \
    VPLTRACE_LOG_ALWAYS(VPLTRACE_GRP_VPL, subgroup, __VA_ARGS__)
#define VPL_LIB_LOG_ERR(subgroup, ...) \
    VPLTRACE_LOG_ERR(VPLTRACE_GRP_VPL, subgroup, __VA_ARGS__)
#define VPL_LIB_LOG_WARN(subgroup, ...) \
    VPLTRACE_LOG_WARN(VPLTRACE_GRP_VPL, subgroup, __VA_ARGS__)
#define VPL_LIB_LOG_INFO(subgroup, ...) \
    VPLTRACE_LOG_INFO(VPLTRACE_GRP_VPL, subgroup, __VA_ARGS__)
#define VPL_LIB_LOG_FINE(subgroup, ...) \
    VPLTRACE_LOG_FINE(VPLTRACE_GRP_VPL, subgroup, __VA_ARGS__)
#define VPL_LIB_LOG_FINER(subgroup, ...) \
    VPLTRACE_LOG_FINER(VPLTRACE_GRP_VPL, subgroup, __VA_ARGS__)
#define VPL_LIB_LOG_FINEST(subgroup, ...) \
    VPLTRACE_LOG_FINEST(VPLTRACE_GRP_VPL, subgroup, __VA_ARGS__)
#define VPL_LIB_LOG(level, subgroup, ...) \
    VPLTRACE_LOG(level, VPLTRACE_GRP_VPL, subgroup, __VA_ARGS__)

#define VPL_LIB_DUMP_BUF_ERR(subgroup, buf, len) \
    VPLTRACE_DUMP_BUF_ERR(VPLTRACE_GRP_VPL, subgroup, buf, len)
#define VPL_LIB_DUMP_BUF_WARN(subgroup, buf, len) \
    VPLTRACE_DUMP_BUF_WARN(VPLTRACE_GRP_VPL, subgroup, buf, len)
#define VPL_LIB_DUMP_BUF_INFO(subgroup, buf, len) \
    VPLTRACE_DUMP_BUF_INFO(VPLTRACE_GRP_VPL, subgroup, buf, len)
#define VPL_LIB_DUMP_BUF_FINE(subgroup, buf, len) \
    VPLTRACE_DUMP_BUF_FINE(VPLTRACE_GRP_VPL, subgroup, buf, len)
#define VPL_LIB_DUMP_BUF_FINER(subgroup, buf, len) \
    VPLTRACE_DUMP_BUF_FINER(VPLTRACE_GRP_VPL, subgroup, buf, len)
#define VPL_LIB_DUMP_BUF_FINEST(subgroup, buf, len) \
    VPLTRACE_DUMP_BUF_FINEST(VPLTRACE_GRP_VPL, subgroup, buf, len)
#define VPL_LIB_DUMP_BUF(level, subgroup, buf, len) \
    VPLTRACE_DUMP_BUF(level, VPLTRACE_GRP_VPL, subgroup, buf, len)

/**
 * Logs the buffer, stripping out known-to-be-sensitive information as well as unprintable characters.
 * @param label Label the log with "send", "send header", "receive", etc.
 * @param buf
 * @param len
 */
void VPL_LogHttpBuffer(const char* label, const void* buf, size_t len);

#ifdef  __cplusplus
}
#endif

#endif // include guard
