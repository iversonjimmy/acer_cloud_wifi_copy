//
//  Copyright 2012 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "SyncConfigDb.hpp"
#include "SyncConfigDbSchemaV02.hpp"
#include "SyncConfig.hpp"

#include "gvm_file_utils.hpp"
#include "gvm_rm_utils.hpp"

#include <string>
#include <log.h>

int main(int argc, char** argv)
{
    int rv = 0;
    int rc;
    std::string dbpath = "./db_migrate_test";
    std::string db_tmp_delete = "./tmp_delete";
    const SyncDbFamily syncDbFamily(SYNC_DB_FAMILY_DOWNLOAD);
    const u64 userId = 2;
    const u64 datasetId = 3;
    const std::string syncConfigId("syncConfigId");

    rc = Util_CreateDir(db_tmp_delete.c_str());
    if (rc != 0) {
        LOG_ERROR("Util_CreateDir(%s):%d", db_tmp_delete.c_str(), rc);
        rv = rc;
        return rv;
    }

    rc = Util_rmRecursive(dbpath, db_tmp_delete);
    if (rc != 0) {
        LOG_WARN("Util_rmRecursive(%s, %s):%d.  Continuing.",
                 dbpath.c_str(), db_tmp_delete.c_str(), rc);
    }

    rc = Util_CreateDir(dbpath.c_str());
    if (rc != 0) {
        LOG_ERROR("Util_CreateDir(%s):%d", dbpath.c_str(), rc);
        rv = rc;
        return rv;
    }

    {
        SyncConfigDbSchemaV02 oldDb(dbpath, syncDbFamily, userId, datasetId, syncConfigId);
        rc = oldDb.openDb();
        if(rc != 0) {
            LOG_ERROR("oldDb.openDb():%d", rc);
            rv = rc;
            return rv;
        }

        const u64 deviceId = 5;
        rc = oldDb.admin_set(deviceId, userId, datasetId, syncConfigId, 27);
        if(rc != 0) {
            LOG_ERROR("admin_set:%d", rc);
            rv = rc;
            return rv;
        }

        // TODO: Add some actual data but for now this is enough.

        rc = oldDb.closeDb();
        if(rc != 0) {
            LOG_ERROR("closeDb():%d", rc);
            rv = rc;
            return rv;
        }
    }

    {
        SyncConfigDb currDb(dbpath, syncDbFamily, userId, datasetId, syncConfigId);
        rc = currDb.openDb();
        if(rc != 0) {
            LOG_ERROR("currDb.openDb():%d", rc);
            rv = rc;
            return rv;
        }

        rc = currDb.closeDb();
        if(rc != 0) {
            LOG_ERROR("closeDb():%d", rc);
            rv = rc;
            return rv;
        }
    }

    return 0;
}
