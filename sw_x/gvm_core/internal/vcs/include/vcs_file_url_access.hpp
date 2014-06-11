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

#ifndef __VCS_FILE_URL_ACCESS_HPP_03_12_2014__
#define __VCS_FILE_URL_ACCESS_HPP_03_12_2014__

#include <vplu_types.h>
#include <vplu_common.h>
#include "vcs_util.hpp"

#include <string>

/// Each instance of #VCSFileUrlOperation can perform at most 1 operation (Get/Put/Delete).
/// Attempting to reuse the same #VCSFileUrlOperation instance results in an error.
class VCSFileUrlOperation
{
public:
    virtual ~VCSFileUrlOperation() {}

    /// Download the file.
    virtual int GetFile(const std::string& dest_file_abs_path) = 0;

    /// Upload the file.
    virtual int PutFile(const std::string& src_file_abs_path) = 0;

    /// Upload the file and get this file's hash number
    virtual int PutFile(const std::string& src_file_abs_path, std::string& hash__out) = 0;

    /// Delete the file.
    /// NOTE: This is currently only intended for staging area cleanup.
    /// (To remove a file from a dataset, be sure to call VCS DELETE, not this function!)
    virtual int DeleteFile() = 0;

    /// Aborts this operation.
    /// @note Cancel() is "sticky"; if this is called before a {Get,Put,Delete}File() operation and
    ///     then {Get,Put,Delete}File() is called later, {Get,Put,Delete}File() will immediately
    ///     return #VPL_ERR_CANCELED.
    virtual int Cancel() = 0;

protected:
    VCSFileUrlOperation(){};
private:
    VPL_DISABLE_COPY_AND_ASSIGN(VCSFileUrlOperation);
};

/// This is the interface that the SyncConfig implementation will use to access
/// the file from the "access URL" provided by VcsAccessInfo.
class VCSFileUrlAccess
{
public:
    virtual ~VCSFileUrlAccess() {}

    /// Create a #VCSFileUrlOperation (to transfer 1 file).
    /// @param err_code__out Returns the error code.  Here are some specific cases:
    ///     - other Treat as "transient" or "permanent" depending on #SyncConfigImpl::isTransientError().
    ///     .
    /// @return The newly created #VCSFileUrlOperation object, or NULL if there was an error (check
    ///     \a err_code__out to find out the error code).
    /// @note You must eventually call destroyOperation() to avoid leaking resources.
    virtual VCSFileUrlOperation* createOperation(
            const VcsAccessInfo& access_info,
            bool verbose_log,
            int& err_code__out) = 0;

    virtual void destroyOperation(VCSFileUrlOperation* vcs_file_url_operation) = 0;

protected:
    VCSFileUrlAccess() {};
private:
    VPL_DISABLE_COPY_AND_ASSIGN(VCSFileUrlAccess);
};

#endif // __VCS_FILE_URL_ACCESS_HPP_03_12_2014__
