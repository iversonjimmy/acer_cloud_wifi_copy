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

#ifndef __VCS_ARCHIVE_ACCESS_CCD_HPP__
#define __VCS_ARCHIVE_ACCESS_CCD_HPP__

#include "vcs_archive_access.hpp"
#include "vpl_user.h"

/// Implementation of #VCSArchiveAccess for accessing the Archive Storage Device
/// for the Media Metadata dataset from within CCD.
/// An instance of this class can safely be used concurrently from multiple threads.
class VCSArchiveAccessCcdMediaMetadataImpl : public VCSArchiveAccess
{
public:
    VCSArchiveAccessCcdMediaMetadataImpl() {}

    virtual ~VCSArchiveAccessCcdMediaMetadataImpl() {}

    virtual VCSArchiveOperation* createOperation(int& err_code__out);

    virtual void destroyOperation(VCSArchiveOperation* vcs_archive_access);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(VCSArchiveAccessCcdMediaMetadataImpl);
};

int GetMediaServerArchiveStorage(VPLUser_Id_t& userId_out,
                                 u64& mediaServerDeviceId_out,
                                 u64& mediaMetadataDatasetId_out,
                                 bool& mediaServerIsArchiveStorageCapable_out,
                                 bool& mediaServerIsOnline_out);

#endif // include guard
