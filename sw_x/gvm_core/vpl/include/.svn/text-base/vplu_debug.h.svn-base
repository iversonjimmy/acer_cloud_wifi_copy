//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLU_DEBUG_H__
#define __VPLU_DEBUG_H__

//============================================================================
/// @file
/// VPL utility (VPLU) operations for debugging VPL.
//============================================================================

#include "vpl_plat.h"
#include "vpl_thread.h"
#include "vplu_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {

    /// Information only; does not indicate a problem.
    VPLDEBUG_LEVEL_INFO = 6,

    /// Current operation failed.
    VPLDEBUG_LEVEL_WARN = 3,

    /// VPL encountered a serious problem.  Subsequent operations are likely to fail as well.
    VPLDEBUG_LEVEL_FATAL = 1,

} VPL_DebugLevel_t;

typedef struct {
    
    /// Null-terminated string containing the debug message.
    /// Note that it will generally not include a newline.
    /// The string is only valid within the #VPL_DebugCallback_t, so be sure
    /// to make a copy if you need to access it outside of the callback.
    const char* msg;
    
    /// ID of the thread that generated the message.
    VPLThread_t threadId;
    
    /// Name of the file generating the debug message.
    const char* file;
    
    /// Line number within the file.
    int line;
    
    /// Severity of the message.
    VPL_DebugLevel_t level;

} VPL_DebugMsg_t;

/// Process a debug message.  Everything pointed to by @a data is only valid
/// within the callback, so be sure to make a copy if you need to access any of
/// if outside of the callback.
typedef void (*VPL_DebugCallback_t)(const VPL_DebugMsg_t* data);

/// Register a debug callback, to be called whenever VPL has information
/// to report.
void VPL_RegisterDebugCallback(VPL_DebugCallback_t callback);

/// Report the message to the callback that was previously registered with
/// #VPL_RegisterDebugCallback().  If no callback has been registered, this
/// call has no effect.
void VPL_ReportMsg(VPL_DebugLevel_t level, const char* file, int line,
        const char* msg, ...) ATTRIBUTE_PRINTF(4, 5);

/// Macro to call #VPL_ReportMsg(), automatically filling in the current
/// filename and line number.
#define VPL_REPORT_MSG(level_, ...)  VPL_ReportMsg((level_), __FILE__, __LINE__, __VA_ARGS__)

#define VPL_REPORT_INFO(...)  VPL_ReportMsg(VPLDEBUG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define VPL_REPORT_WARN(...)  VPL_ReportMsg(VPLDEBUG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define VPL_REPORT_FATAL(...) VPL_ReportMsg(VPLDEBUG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#define VPL_ERR_CODES \
    ROW(VPL_OK) \
    ROW(VPL_ERR_NOMEM) \
    ROW(VPL_ERR_PERM) \
    ROW(VPL_ERR_INTR) \
    ROW(VPL_ERR_IO) \
    ROW(VPL_ERR_BADF) \
    ROW(VPL_ERR_AGAIN) \
    ROW(VPL_ERR_BUSY) \
    ROW(VPL_ERR_INVALID) \
    ROW(VPL_ERR_DEADLK) \
    ROW(VPL_ERR_MAX) \
    ROW(VPL_ERR_CONNRESET) \
    ROW(VPL_ERR_DESTADDRREQ) \
    ROW(VPL_ERR_FAULT) \
    ROW(VPL_ERR_ISCONN) \
    ROW(VPL_ERR_MSGSIZE) \
    ROW(VPL_ERR_NOBUFS) \
    ROW(VPL_ERR_NOTCONN) \
    ROW(VPL_ERR_NOTSOCK) \
    ROW(VPL_ERR_OPNOTSUPPORTED) \
    ROW(VPL_ERR_PIPE) \
    ROW(VPL_ERR_ADDRINUSE) \
    ROW(VPL_ERR_ADDRNOTAVAIL) \
    ROW(VPL_ERR_LOOP) \
    ROW(VPL_ERR_NAMETOOLONG) \
    ROW(VPL_ERR_NOENT) \
    ROW(VPL_ERR_NOTDIR) \
    ROW(VPL_ERR_ROFS) \
    ROW(VPL_ERR_DISABLED) \
    ROW(VPL_ERR_HOSTNAME) \
    ROW(VPL_ERR_SOCKET) \
    ROW(VPL_ERR_NOOP) \
    ROW(VPL_ERR_CONNABORT) \
    ROW(VPL_ERR_UNREACH) \
    ROW(VPL_ERR_NETDOWN) \
    ROW(VPL_ERR_NETRESET) \
    ROW(VPL_ERR_SYSTEM_BUFFER_INIT) \
    ROW(VPL_ERR_VDISK_BAD_PATH) \
    ROW(VPL_ERR_VDISK_MNT) \
    ROW(VPL_ERR_VDISK_UMNT) \
    ROW(VPL_ERR_CONNREFUSED) \
    ROW(VPL_ERR_CONCURRENT) \
    ROW(VPL_ERR_NOSPC) \
    ROW(VPL_ERR_NOTEMPTY) \
    ROW(VPL_ERR_EXIST) \
    ROW(VPL_ERR_NOSYS) \
    ROW(VPL_ERR_NODEV) \
    ROW(VPL_ERR_TXTBSY) \
    ROW(VPL_ERR_NO_USER) \
    ROW(VPL_ERR_NOT_SIGNED_IN) \
    ROW(VPL_ERR_OFFLINE_MODE) \
    ROW(VPL_ERR_BAD_ACHIEVE_INDEX) \
    ROW(VPL_ERR_BAD_ACHIEVE_PROGRESS) \
    ROW(VPL_ERR_NO_TITLE_DATA) \
    ROW(VPL_ERR_THREAD_DETACHED) \
    ROW(VPL_ERR_NAMED_SOCKET_NOT_EXIST) \
    ROW(VPL_ERR_INPROGRESS) \
    ROW(VPL_ERR_CROSS_DEVICE_LINK) \
    ROW(VPL_ERR_SHORTCUT_INVALID) \
    ROW(VPL_ERR_CANCELED) \
    ROW(VPL_ERR_NOT_RUNNING) \
    ROW(VPL_ERR_IS_INIT) \
    ROW(VPL_ERR_NOT_INIT) \
    ROW(VPL_ERR_ACCESS) \
    ROW(VPL_ERR_TIMEOUT) \
    ROW(VPL_ERR_ALREADY) \
    ROW(VPL_ERR_FAIL) \
    ROW(VPL_ERR_DISPLAY_CONTEXT) \
    ROW(VPL_ERR_DISPLAY_FBO_INCOMPLETE_ATTACHMENT) \
    ROW(VPL_ERR_DISPLAY_FBO_INCOMPLETE_DRAW_BUFFER) \
    ROW(VPL_ERR_DISPLAY_FBO_INCOMPLETE_LAYER_TARGETS) \
    ROW(VPL_ERR_DISPLAY_FBO_INCOMPLETE_MISSING_ATTACHMENT) \
    ROW(VPL_ERR_DISPLAY_FBO_INCOMPLETE_MULTISAMPLE) \
    ROW(VPL_ERR_DISPLAY_FBO_INCOMPLETE_READ_BUFFER) \
    ROW(VPL_ERR_DISPLAY_FBO_INVALID_OPERATION) \
    ROW(VPL_ERR_DISPLAY_FBO_UNDEFINED) \
    ROW(VPL_ERR_DISPLAY_FBO_UNSUPPORTED) \
    ROW(VPL_ERR_DISPLAY_FORMAT) \
    ROW(VPL_ERR_DISPLAY_FRAGMENT_SHADER) \
    ROW(VPL_ERR_DISPLAY_GL_INVALID_ENUM) \
    ROW(VPL_ERR_DISPLAY_GL_INVALID_VALUE) \
    ROW(VPL_ERR_DISPLAY_GL_INVALID_OPERATION) \
    ROW(VPL_ERR_DISPLAY_GL_OUT_OF_MEMORY) \
    ROW(VPL_ERR_DISPLAY_INDIRECT_CONTEXT) \
    ROW(VPL_ERR_DISPLAY_LINK_SHADER) \
    ROW(VPL_ERR_DISPLAY_NOTIFICATION) \
    ROW(VPL_ERR_DISPLAY_VERTEX_SHADER) \
    ROW(VPL_ERR_DISPLAY_WINDOW) \
    ROW(VPL_ERR_DISPLAY_VSYNC) \
    ROW(VPL_ERR_INPUT_INVALID_DURATION) \
    ROW(VPL_ERR_INPUT_INVALID_RUMBLER) \
    ROW(VPL_ERR_INPUT_INVALID_SPEED) \
    ROW(VPL_ERR_INPUT_NOT_CONNECTED) \
    ROW(VPL_ERR_AUDIO_UNSUPPORTED) \
    ROW(VPL_ERR_AUDIO_SYSTEM_INIT) \
    ROW(VPL_ERR_AUDIO_WRITE) \
    ROW(VPL_ERR_AUDIO_READ) \
    ROW(VPL_ERR_AUDIO_BUFFER_FULL) \
    ROW(VPL_ERR_AUDIO_BUFFER_EMPTY) \
    ROW(VPL_ERR_AUDIO_STREAM_STOPPED) \
    ROW(VPL_ERR_AUDIO_INVALID_DEVICE) \
    ROW(VPL_ERR_AUDIO_NOT_CONNECTED) \
    ROW(VPL_ERR_AVSYNC_INIT)

#ifdef __cplusplus
}
#endif

#endif // include guard
