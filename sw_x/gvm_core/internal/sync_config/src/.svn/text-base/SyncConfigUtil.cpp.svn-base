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
#include "SyncConfigUtil.hpp"
#include "vplex_assert.h"

SyncDbFamily getSyncDbFamily(SyncType syncType)
{
    switch(syncType)
    {
    case SYNC_TYPE_TWO_WAY:
    case SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE:
    case SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE_AND_USE_ACS:
        return SYNC_DB_FAMILY_TWO_WAY;

    case SYNC_TYPE_ONE_WAY_UPLOAD:
    case SYNC_TYPE_ONE_WAY_UPLOAD_PURE_VIRTUAL_SYNC:
    case SYNC_TYPE_ONE_WAY_UPLOAD_HYBRID_VIRTUAL_SYNC:
        return SYNC_DB_FAMILY_UPLOAD;

    case SYNC_TYPE_ONE_WAY_DOWNLOAD:
    case SYNC_TYPE_ONE_WAY_DOWNLOAD_PURE_VIRTUAL_SYNC:
        return SYNC_DB_FAMILY_DOWNLOAD;

    default:
        FAILED_ASSERT("Unrecognized type:%d", (int) syncType);
        return SyncDbFamily(-1);
    }
}
