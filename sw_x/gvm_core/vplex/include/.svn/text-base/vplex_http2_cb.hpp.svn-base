#ifndef __VPLEX_HTTP2_CB_HPP__
#define __VPLEX_HTTP2_CB_HPP__

#include <vplu_types.h>

class VPLHttp2;

/// @return the number of bytes consumed by the callback.  If this is not equal to \a size,
///     it is considered an error, and the transfer will be aborted.
typedef s32 (*VPLHttp2_RecvCb)(VPLHttp2 *http, void *ctx, const char *buf, u32 size);

/// @return the number of bytes placed in the buffer by the callback function.
/// 0 means EOF.
/// <0 means error.
typedef s32 (*VPLHttp2_SendCb)(VPLHttp2 *http, void *ctx, char *buf, u32 size);

/// @param total Total number of bytes scheduled for transfer.
/// @param soFar Number of bytes transferred so far.
typedef void (*VPLHttp2_ProgressCb)(VPLHttp2 *http, void *ctx, u64 total, u64 soFar);

#endif // __VPLEX_HTTP2_CB_HPP__
