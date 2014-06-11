/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

/**
 * vssts_error.h
 *
 * Virtual Storage Server interface: Error definitions
 */

#ifndef __VSSTS_ERROR_H__
#define __VSSTS_ERROR_H__

/// Result codes (-16100 to -16149)
enum {
    VSSI_SUCCESS  =      0,///< Command succeeded.
    VSSI_BADVER   = -16101,///< Protocol version not supported.
    VSSI_BADCMD   = -16102,///< Command code invalid.
    VSSI_NOTFOUND = -16103,///< File/directory not found.
    VSSI_OPENED   = -16104,///< Operation not valid for opened object.
    VSSI_PERM     = -16105,///< Permission denied for command.
    VSSI_ACCESS   = -16106,///< Access denied for command.
    VSSI_NOSPACE  = -16107,///< Not enough space to complete command.
    VSSI_CONFLICT = -16108,///< File version conflict occurred.
    VSSI_EOF      = -16109,///< End-of-file reached.
    VSSI_COMM     = -16110,///< Communication failure processing command.
    VSSI_NOMEM    = -16111,///< Not enough memory to process command.
    VSSI_INVALID  = -16112,///< Invalid command parameter.
    VSSI_HLIMIT   = -16113,///< Open file handle limit reached.
    VSSI_TYPE     = -16114,///< Object type does not match requested type.
    VSSI_BADXID   = -16115,///< Object-access request had a repeat or invalid XID.
    VSSI_BADSIG   = -16116,///< Command had a bad signature.
    VSSI_INIT     = -16117,///< Library not initialized.
    VSSI_CMDLIMIT = -16118,///< Outstanding command limit reached for the session.
                           ///< Try again later.
    VSSI_NOLOGIN  = -16119,///< Login session not found. Log in first.
    VSSI_ABORTED  = -16120,///< Command aborted by ending session.
    VSSI_ISDIR    = -16121,///< Read file attempted on a directory.
    VSSI_NOTDIR   = -16122,///< Read directory attempted on file, or part of path
                           ///< is a file.
    VSSI_NOCHANGE = -16123,///< Commit attempted with no changes.
    VSSI_BADOBJ   = -16124,///< Object identifier or handle invalid.
    VSSI_TIMEOUT  = -16125,///< Timeout occurred processing request.
    VSSI_NET_DOWN = -16126,///< Network interface down.
    VSSI_WRONG_CLUSTER = -16127,///< Attempt to access dataset not stored in this cluster.
    VSSI_AGAIN    = -16128,///< Try operation again later.
    VSSI_OLIMIT   = -16129,///< Open object limit reached. Try again later.
    VSSI_RESTORE  = -16130,///< Requested data must be restored from back-up.
                           ///< Unlock dataset if locked and retry.
    VSSI_LOCKED   = -16131,///< Operation conflicts with existing file lock.
    VSSI_RETRY    = -16132,///< Operation needs to be queued and retried (internal only).
    VSSI_BUSY     = -16133,///< Operation conflicts with an open file handle.
    VSSI_EXISTS   = -16134,///< Already exists (used when creating a file to indicate
                           ///< that the file already existed, so not really an error).
                           ///< Important: DO NOT USE for any real error, since this code
                           ///< is treated as success by the reply handling logic.
    VSSI_BADPATH  = -16135,///< Directory not found in a file pathname.
    VSSI_FEXISTS  = -16136,///< Existing file causes operation to fail (e.g. target of 
                           ///< rename).  This is a real error, as opposed to VSSI_EXISTS.
    VSSI_FAILED   = -16137, ///< Dataset is irrecoverably failed and unusable.
    VSSI_MAXCOMP  = -16138, ///< The number of child components is reaching the maximum value.
    VSSI_WAITLOCK = -16139, ///< Byte range lock request blocked by another lock (internal only)
};

#endif // include guard
