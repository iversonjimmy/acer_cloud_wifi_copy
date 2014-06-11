//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_ERROR_H__
#define __VPL_ERROR_H__

//============================================================================
/// @file
/// Virtual Platform Layer error code definitions.
/// Please see @ref VPLErrors.
//============================================================================

#include "vpl__error.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLErrors VPL Error Codes
///
/// These are the definitions of possible error codes for VPL API functions.
///
/// #VPL_OK is code 0, indicating unconditional success.
/// All other VPL_ERR_* codes will be in the range -9000 through -9999.
/// Sub-ranges are defined for sub-modules.
/// <br>
/// When a VPL API function returns an error code, please see that function's
/// documentation for a more specific explanation of the error.
///@{

#define VPL_ERR_MIN_VALUE -9599
#define VPL_ERR_MAX_VALUE -9000

///
/// Error codes for VPL API functions.
/// @copydetails VPLErrors
///
enum VPLError_t {

    //------------------------------------------------------------------------
    // General errors.
    // Range: -9000 through -9299
    //------------------------------------------------------------------------

    /// OK return. Complete success.
    VPL_OK = 0,

    /// No memory.
    VPL_ERR_NOMEM      = -9000,

    /// Operation not permitted.
    VPL_ERR_PERM       = -9001,

    /// Interrupted system call.
    VPL_ERR_INTR       = -9002,

    /// I/O error.
    VPL_ERR_IO         = -9003,

    /// Bad file descriptor, socket handle, or other handle.
    VPL_ERR_BADF       = -9004,

    /// Not ready, try again later.
    VPL_ERR_AGAIN      = -9005,

    /// Resource busy.
    VPL_ERR_BUSY       = -9006,

    /// Invalid function parameter values.
    VPL_ERR_INVALID    = -9007,

    /// Deadlock would occur.
    VPL_ERR_DEADLK     = -9008,

    /// A limited resource has reached its maximum limit.
    VPL_ERR_MAX        = -9009,

    /// (Socket) Connection reset by peer.
    VPL_ERR_CONNRESET  = -9010,

    /// (Socket) Not connection-mode, and no peer address is set.
    VPL_ERR_DESTADDRREQ = -9011,

    /// An invalid pointer was provided.
    VPL_ERR_FAULT      = -9012,

    /// Connection mode socket already connected to a different recipient.
    VPL_ERR_ISCONN     = -9013,

    /// Socket must send messages atomically, and size makes this impossible.
    VPL_ERR_MSGSIZE    = -9014,

    /// Network interface outbound queue full.
    VPL_ERR_NOBUFS     = -9015,

    /// Socket not connected and no target given.
    VPL_ERR_NOTCONN    = -9016,

    /// Socket operation performed on something not a socket.
    VPL_ERR_NOTSOCK    = -9017,

    /// Operation flags inappropriate to the socket type.
    VPL_ERR_OPNOTSUPPORTED = -9018,

    /// Local end of the socket has shut down.
    VPL_ERR_PIPE       = -9019,

    /// Given address is already in use.
    VPL_ERR_ADDRINUSE  = -9020,

    /// A non-existent or non-local interface was requested.
    VPL_ERR_ADDRNOTAVAIL = -9021,

    /// Too many symbolic names encountered in name.
    VPL_ERR_LOOP       = -9022,

    /// Symbolic name too long.
    VPL_ERR_NAMETOOLONG = -9023,

    /// File does not exist.
    VPL_ERR_NOENT      = -9024,

    /// Component of path is not a directory.
    VPL_ERR_NOTDIR     = -9025,

    /// Socket or writable file on read-only file system.
    VPL_ERR_ROFS       = -9026,

    /// Operation is disabled.
    VPL_ERR_DISABLED   = -9027,

    /// Host name not valid for operation.
    VPL_ERR_HOSTNAME   = -9028,

    /// Could not create socket for operation.
    VPL_ERR_SOCKET     = -9029,

    /// Operation is not implemented.
    VPL_ERR_NOOP       = -9030,

    /// Connection terminated locally, e.g. due to timeout.
    VPL_ERR_CONNABORT  = -9031,

    /// Network or remote host unreachable.
    VPL_ERR_UNREACH = -9032,

    /// Operation encountered a dead network.
    VPL_ERR_NETDOWN = -9033,

    /// Remote host has reset and dropped connection.
    VPL_ERR_NETRESET = -9034,

    /// An internal system buffer initialization failed.
    VPL_ERR_SYSTEM_BUFFER_INIT = -9035,

    /// Path given to mount a content has bad format.
    VPL_ERR_VDISK_BAD_PATH = -9036,

    /// Mount failed.
    VPL_ERR_VDISK_MNT = -9037,

    /// Unmount failed.
    VPL_ERR_VDISK_UMNT = -9038,

    /// Connection was refused for some reason.
    VPL_ERR_CONNREFUSED = -9039,

    /// Improper concurrent or reentrant use of a VPL structure.
    VPL_ERR_CONCURRENT = -9040,

    /// Not enough space for operation.
    VPL_ERR_NOSPC = -9041,

    /// Container not empty.
    VPL_ERR_NOTEMPTY = -9042,

    /// Item already exists.
    VPL_ERR_EXIST = -9043,

    /// Not system supported.
    VPL_ERR_NOSYS = -9044,

    /// No available device.
    VPL_ERR_NODEV = -9045,

    /// Text file busy.
    VPL_ERR_TXTBSY = -9046,
    
    /// There is no user signed-in for the specified player index.
    VPL_ERR_NO_USER = -9047,

    /// The specified #VPLUser_Id_t is not signed-in.
    VPL_ERR_NOT_SIGNED_IN = -9048,

    /// The user is currently running in offline mode, but the requested operation requires
    /// a connection to the server.
    VPL_ERR_OFFLINE_MODE = -9049,

    /// The specified achievement index is unknown for the current title.
    VPL_ERR_BAD_ACHIEVE_INDEX = -9051,
    
    /// Achievement progress values can never be decreased.
    VPL_ERR_BAD_ACHIEVE_PROGRESS = -9052,

    VPL_ERR_NO_TITLE_DATA = -9053,

    /// The thread is detached, thus it is not joinable.
    VPL_ERR_THREAD_DETACHED = -9054,

    /// Could not connect to a named socket; the other process may not be running.
    VPL_ERR_NAMED_SOCKET_NOT_EXIST = -9055,

    /// Operation on a non-blocking socket is in progress.
    VPL_ERR_INPROGRESS = -9056,

    /// Cross volume rename is not supported.
    VPL_ERR_CROSS_DEVICE_LINK = -9057,

    /// Shortcut has arguments and implies not file/folder.
    VPL_ERR_SHORTCUT_INVALID = -9058,

    // ** (Add new VPL errors here.) **

    /// The operation was canceled by another API call.
    // NOTE: duplicated in the Java code (so do not change the value).
    VPL_ERR_CANCELED = -9087,

    /// Subsystem was stopped or has never been started.
    VPL_ERR_NOT_RUNNING = -9088,

    /// Subsystem was already initialized.  Note that subsystem initialization is
    /// generally <b>not</b> thread-safe.
    VPL_ERR_IS_INIT    = -9089,

    /// Subsystem must be initialized prior to use.
    VPL_ERR_NOT_INIT   = -9090,

    /// Access denied.
    VPL_ERR_ACCESS     = -9091,

    /// Operation timed out.
    VPL_ERR_TIMEOUT    = -9096,

    /// The object is already in the requested state.
    /// It may be reasonable to treat this as success in some cases.
    VPL_ERR_ALREADY    = -9097,

    /// Unspecified failure.
    /// You may receive additional details if you registered a #VPL_DebugCallback_t via
    /// #VPL_RegisterDebugCallback().
    // NOTE: duplicated in the Java code (so do not change the value).
    VPL_ERR_FAIL       = -9099,

    //% Reserved for VPL expansion
    //% Range: -9100 through -9299
    //------------------------------------------------------------------------
    // VPLDisplay error codes.
    // Range: -9300 through -9399
    //------------------------------------------------------------------------

    /// Context related error.
    VPL_ERR_DISPLAY_CONTEXT = -9300,

    /// Translated GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
    VPL_ERR_DISPLAY_FBO_INCOMPLETE_ATTACHMENT = -9301,

    /// Translated GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
    VPL_ERR_DISPLAY_FBO_INCOMPLETE_DRAW_BUFFER = -9302,

    /// Translated GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
    VPL_ERR_DISPLAY_FBO_INCOMPLETE_LAYER_TARGETS = -9303,

    /// Translated GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
    VPL_ERR_DISPLAY_FBO_INCOMPLETE_MISSING_ATTACHMENT = -9304,

    /// Translated GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
    VPL_ERR_DISPLAY_FBO_INCOMPLETE_MULTISAMPLE = -9305,

    /// Translated GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
    VPL_ERR_DISPLAY_FBO_INCOMPLETE_READ_BUFFER = -9306,

    /// Translated GL_FRAMEBUFFER_INVALID_OPERATION
    VPL_ERR_DISPLAY_FBO_INVALID_OPERATION = -9307,

    /// Translated GL_FRAMEBUFFER_UNDEFINED
    VPL_ERR_DISPLAY_FBO_UNDEFINED = -9308,

    /// Translated GL_FRAMEBUFFER_UNSUPPORTED
    VPL_ERR_DISPLAY_FBO_UNSUPPORTED = -9309,

    /// Display format related error.
    VPL_ERR_DISPLAY_FORMAT = -9310,

    /// Fragment shader compilation error.
    VPL_ERR_DISPLAY_FRAGMENT_SHADER = -9311,

    /// Translated GL_INVALID_ENUM
    VPL_ERR_DISPLAY_GL_INVALID_ENUM = -9312,

    /// Translated GL_INVALID_VALUE
    VPL_ERR_DISPLAY_GL_INVALID_VALUE = -9313,

    /// Translated GL_INVALID_OPERATION
    VPL_ERR_DISPLAY_GL_INVALID_OPERATION = -9314,

    /// Translated GL_OUT_OF_MEMORY
    VPL_ERR_DISPLAY_GL_OUT_OF_MEMORY = -9315,

    /// Indirect rendering context created.
    VPL_ERR_DISPLAY_INDIRECT_CONTEXT = -9317,

    /// Shader linker error.
    VPL_ERR_DISPLAY_LINK_SHADER = -9318,

    /// Notification could not be initialized.
    VPL_ERR_DISPLAY_NOTIFICATION = -9319,

    /// Vertex shader compilation error.
    VPL_ERR_DISPLAY_VERTEX_SHADER = -9320,

    /// Window creation failed.
    VPL_ERR_DISPLAY_WINDOW = -9321,

    /// Vertical sync initialization failed.
    VPL_ERR_DISPLAY_VSYNC = -9322,

    //------------------------------------------------------------------------
    // VPLInput error codes.
    // Range: -9400 through -9499
    //------------------------------------------------------------------------

    /// Gamepad rumble duration out of bounds.
    VPL_ERR_INPUT_INVALID_DURATION = -9400,

    /// Gamepad rumbler index out of bounds.
    VPL_ERR_INPUT_INVALID_RUMBLER = -9401,

    /// Gamepad rumble speed out of bounds.
    VPL_ERR_INPUT_INVALID_SPEED = -9402,

    /// Input device is not connected.
    VPL_ERR_INPUT_NOT_CONNECTED = -9403,


    //------------------------------------------------------------------------
    // VPLAudio error codes.
    // Range: -9500 through -9599
    //------------------------------------------------------------------------

    /// Unsupported audio format.
    VPL_ERR_AUDIO_UNSUPPORTED = -9500,

    /// Audio device configuration failure.
    VPL_ERR_AUDIO_SYSTEM_INIT = -9501,

    /// Audio buffer write operation fail or is not supported by the device.
    VPL_ERR_AUDIO_WRITE = -9502,

    /// Audio buffer read operation fail or is not supported by the device.
    VPL_ERR_AUDIO_READ = -9503,

    /// Audio buffer full.
    VPL_ERR_AUDIO_BUFFER_FULL = -9504,

    /// Audio buffer empty.
    VPL_ERR_AUDIO_BUFFER_EMPTY = -9505,

    /// Audio stream is stopped.
    VPL_ERR_AUDIO_STREAM_STOPPED = -9506,

    /// Invalid audio device.
    VPL_ERR_AUDIO_INVALID_DEVICE = -9507,

    /// Audio device is not connected.
    VPL_ERR_AUDIO_NOT_CONNECTED = -9508,

    /// Audio/video synchronization mechanism initialization failure.
    VPL_ERR_AVSYNC_INIT = -9550,


    //%-----------------------------------------------------------------------
    //% VPLex-specific errors (see vplex_error.h).
    //% Range: -9600 through -9999
    //%-----------------------------------------------------------------------
};

///@}

#ifdef __cplusplus
}
#endif

#endif // include guard
