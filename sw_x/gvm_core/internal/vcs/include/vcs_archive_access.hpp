//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef __VCS_ARCHIVE_ACCESS_HPP__
#define __VCS_ARCHIVE_ACCESS_HPP__

#include <vplu_types.h>
#include <vplu_common.h>

#include <string>

/// Each instance of #VCSArchiveOperation can perform at most 1 file transfer.
/// Attempting to reuse the same #VCSArchiveOperation instance results in an error.
class VCSArchiveOperation
{
public:
    virtual ~VCSArchiveOperation() {}

    /// Download the component from the Archive Storage Device and store it in the local
    /// filesystem at \a destFileAbsPath.  Blocks until the operation completes or until Cancel()
    /// is called from another thread.
    /// @param datasetRelPath Required for items within the media metadata dataset.  Optional otherwise.
    /// @return some possible values:
    ///     - #VPL_ERR_CANCELED Cancel() was called.
    ///     - #VPL_ERR_ALREADY Reuse detected (programmer error).
    ///     - #CCD_ERROR_TRANSIENT
    ///     - #CCD_ERROR_HTTP_STATUS
    ///     - #CCD_ERROR_BAD_SERVER_RESPONSE
    ///     - other Problem writing to \a dest_file_abs_path.
    ///     - other Problem contacting the Archive Storage Device.
    ///     .
    virtual int GetFile(
            u64 component_id,
            u64 revision,
            const std::string& dataset_rel_path,
            const std::string& dest_file_abs_path) = 0;

    /// Aborts this operation.
    /// @note Cancel() is "sticky"; if this is called before GetFile() and then GetFile() is
    ///     called later, GetFile() will immediately return #VPL_ERR_CANCELED.
    virtual int Cancel() = 0;

protected:
    VCSArchiveOperation() {};
private:
    VPL_DISABLE_COPY_AND_ASSIGN(VCSArchiveOperation);
};

/// The interface that the SyncConfig implementation will use to access the Archive Storage
/// Device for a particular SyncConfig's dataset.
class VCSArchiveAccess
{
public:
    virtual ~VCSArchiveAccess() {}

    /// Create a #VCSArchiveOperation (to transfer 1 file).
    /// @param err_code__out Returns the error code.  Here are some specific cases:
    ///     - #CCD_ERROR_ARCHIVE_DEVICE_OFFLINE Treat as "semi-transient"; SyncConfig should try again
    ///           after #ReportArchiveStorageDeviceAvailability is called, but not before then.
    ///     - other Treat as "transient" or "permanent" depending on #SyncConfigImpl::isTransientError().
    ///     .
    /// @return The newly created #VCSArchiveOperation object, or NULL if there was an error (check
    ///     \a err_code__out to find out the error code).
    /// @note You must eventually call destroyOperation() to avoid leaking resources.
    virtual VCSArchiveOperation* createOperation(int& err_code__out) = 0;

    /// Release the resources associated with a #VCSArchiveOperation.
    virtual void destroyOperation(VCSArchiveOperation* vcs_archive_access) = 0;

protected:
    VCSArchiveAccess() {};
private:
    VPL_DISABLE_COPY_AND_ASSIGN(VCSArchiveAccess);
};

#endif // include guard

// ========================================================
// ==================== EXAMPLE USAGE =====================
//    int exampleUsage(u64 compId,
//            u64 rev,
//            const std::string& datsetRelPath,
//            const std::string& tempDownloadPath,
//            VCSArchiveAccess& archiveAccess)
//    {
//        int rv;
//
//        VCSArchiveOperation* archiveOperation = archiveAccess.createOperation(/*out*/rv);
//        if (archiveOperation == NULL) {
//            LOG_WARN("createOperation failed: %d", rv);
//            return rv;
//        }
//
//        // Make it visible for another thread to call Cancel();
//        registerForAsyncCancel(archiveOperation);
//
//        // Perform the download (blocking).
//        rv = archiveOperation->GetFile(compId, rev, datsetRelPath, tempDownloadPath);
//
//        // Operation complete; cleanup.
//        unregisterForAsyncCancel(archiveOperation);
//        archiveAccess.destroyOperation(archiveOperation);
//
//        // Check error code from GetFile.
//        if (rv != 0) {
//            LOG_WARN("GetFile failed: %d", rv);
//        }
//        return rv;
//    }
// ==================== EXAMPLE USAGE =====================
// ========================================================
