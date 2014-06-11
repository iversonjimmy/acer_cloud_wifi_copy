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
#include "SyncConfigDb.hpp"

#include "gvm_errors.h"
#include "gvm_file_utils.h"
#include "gvm_rm_utils.hpp"
#include "gvm_thread_utils.h"
#include "util_open_db_handle.hpp"
#include "db_util_access_macros.hpp"
#include "db_util_check_macros.hpp"
#include "vpl_thread.h"
#include "vplu_mutex_autolock.hpp"
#include <log.h>

//================== EXPERIMENTAL/EXPLORATORY ===========================
// This is not a formal test.  This is only built, but not run by
// buildbot.
// Tests here are not checked for correctness, and manual
// examination of logs and perhaps even making changes in test source
// are required.  The purpose of this was to experiment and explore
// behavior of sqlite calls as documented online and verify
// understanding.  If desired, these can be converted into formal
// tests with some effort (for the multithread cases, a lot of effort),
// but the value for doing this currently isn't there.

#define CHECK_TEST_ERR(numTest, numErr, err, str)    \
    do{ numTest++;                                   \
        if(err != 0) {                               \
          LOG_ERROR("TestErr(%s:%d)", str, err);      \
          numErr++;                                  \
        }                                            \
    }while(false)

#define CHECK_TEST_EXPR(numTest, numErr, expr)       \
    do{ numTest++;                                   \
        if((expr) == false) {                        \
          LOG_ERROR("FailCheck:%s", #expr);          \
          numErr++;                                  \
        }                                            \
    }while(false)

static void adminTableTest(SyncConfigDb* myDb,
                           u32& numTest,
                           u32& numErr)
{
    int rc;
    SCRow_admin entry;

    rc = myDb->admin_get(entry);
    CHECK_TEST_ERR(numTest,numErr,rc, "admin_get");
    CHECK_TEST_EXPR(numTest,numErr, entry.row_id==1);
    CHECK_TEST_EXPR(numTest,numErr, entry.schema_version!=0);
    CHECK_TEST_EXPR(numTest,numErr, !entry.dataset_id_exists);
    CHECK_TEST_EXPR(numTest,numErr, !entry.device_id_exists);
    CHECK_TEST_EXPR(numTest,numErr, !entry.user_id_exists);
    CHECK_TEST_EXPR(numTest,numErr, !entry.sync_config_id_exists);
    CHECK_TEST_EXPR(numTest,numErr, !entry.last_opened_timestamp_exists);

    rc = myDb->admin_set(20, 21, 22, "twenty-three", 24);
    CHECK_TEST_ERR(numTest,numErr,rc, "admin_set");

    rc = myDb->admin_get(entry);
    CHECK_TEST_ERR(numTest,numErr,rc, "admin_get");
    CHECK_TEST_EXPR(numTest,numErr, entry.row_id==1);
    CHECK_TEST_EXPR(numTest,numErr, entry.schema_version!=0);
    CHECK_TEST_EXPR(numTest,numErr, entry.dataset_id_exists);
    CHECK_TEST_EXPR(numTest,numErr, entry.dataset_id==22);
    CHECK_TEST_EXPR(numTest,numErr, entry.device_id_exists);
    CHECK_TEST_EXPR(numTest,numErr, entry.device_id==20);
    CHECK_TEST_EXPR(numTest,numErr, entry.user_id_exists);
    CHECK_TEST_EXPR(numTest,numErr, entry.user_id==21);
    CHECK_TEST_EXPR(numTest,numErr, entry.sync_config_id_exists);
    CHECK_TEST_EXPR(numTest,numErr, entry.sync_config_id=="twenty-three");
    CHECK_TEST_EXPR(numTest,numErr, entry.last_opened_timestamp_exists);
    CHECK_TEST_EXPR(numTest,numErr, entry.last_opened_timestamp==24);

}

static void needDownloadScanTableTest(SyncConfigDb* myDb,
                                      u32& numTest,
                                      u32& numErr)
{
    int rc;
    u64 maxRowId=0;
    SCRow_needDownloadScan entry;

    // Expect not found.
    rc = myDb->needDownloadScan_get(entry);
    CHECK_TEST_EXPR(numTest,numErr,rc==SYNC_AGENT_DB_ERR_ROW_NOT_FOUND);

    rc = myDb->needDownloadScan_getMaxRowId(maxRowId);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_getMaxRowId");
    CHECK_TEST_EXPR(numTest,numErr, maxRowId==0);

    // Add an entry (A)
    rc = myDb->needDownloadScan_add("entry_A", 10);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_add");

    rc = myDb->needDownloadScan_getMaxRowId(maxRowId);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_getMaxRowId");
    CHECK_TEST_EXPR(numTest,numErr, maxRowId==1);

    // Add another entry
    rc = myDb->needDownloadScan_add("entry_B", 15);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_add");

    rc = myDb->needDownloadScan_getMaxRowId(maxRowId);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_getMaxRowId");
    CHECK_TEST_EXPR(numTest,numErr, maxRowId==2);

    // Get and check entry_A (FIFO)
    rc = myDb->needDownloadScan_get(entry);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_get");
    CHECK_TEST_EXPR(numTest,numErr,entry.row_id==1);
    CHECK_TEST_EXPR(numTest,numErr,entry.dir_path=="entry_A");
    CHECK_TEST_EXPR(numTest,numErr,entry.comp_id==10);
    CHECK_TEST_EXPR(numTest,numErr,entry.err_count==0);

    // No errors should, expect not found.
    rc = myDb->needDownloadScan_getErr(SQLITE3_MAX_ROW_ID, entry);
    CHECK_TEST_EXPR(numTest,numErr,rc==SYNC_AGENT_DB_ERR_ROW_NOT_FOUND);

    // Try and incErr a rowId that does not exist.
    rc = myDb->needDownloadScan_incErr(50);
    CHECK_TEST_EXPR(numTest,numErr,rc==SYNC_AGENT_DB_ERR_ROW_NOT_FOUND);

    // IncErr for row 1.  row2 should be valid, row3 should be the new err.
    rc = myDb->needDownloadScan_incErr(1);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_incErr");

    // Should be the valid row2
    rc = myDb->needDownloadScan_get(entry);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_get");
    CHECK_TEST_EXPR(numTest,numErr,entry.row_id==2);
    CHECK_TEST_EXPR(numTest,numErr,entry.dir_path=="entry_B");
    CHECK_TEST_EXPR(numTest,numErr,entry.comp_id==15);
    CHECK_TEST_EXPR(numTest,numErr,entry.err_count==0);

    rc = myDb->needDownloadScan_getMaxRowId(maxRowId);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_getMaxRowId");
    CHECK_TEST_EXPR(numTest,numErr, maxRowId==3);

    // No error entries expected to be found
    rc = myDb->needDownloadScan_getErr(2, entry);
    CHECK_TEST_EXPR(numTest,numErr,rc==SYNC_AGENT_DB_ERR_ROW_NOT_FOUND);

    // No errors expected.
    rc = myDb->needDownloadScan_getErr(3, entry);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_getErr");
    CHECK_TEST_EXPR(numTest,numErr,entry.row_id==3);
    CHECK_TEST_EXPR(numTest,numErr,entry.dir_path=="entry_A");
    CHECK_TEST_EXPR(numTest,numErr,entry.comp_id==10);
    CHECK_TEST_EXPR(numTest,numErr,entry.err_count==1);

    // Increment the error again.
    rc = myDb->needDownloadScan_incErr(3);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_incErr");

    // No errors expected.
    rc = myDb->needDownloadScan_getErr(5, entry);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_getErr");
    CHECK_TEST_EXPR(numTest,numErr,entry.row_id==4);
    CHECK_TEST_EXPR(numTest,numErr,entry.dir_path=="entry_A");
    CHECK_TEST_EXPR(numTest,numErr,entry.comp_id==10);
    CHECK_TEST_EXPR(numTest,numErr,entry.err_count==2);

    // Should return an error
    rc = myDb->needDownloadScan_remove(10);
    CHECK_TEST_EXPR(numTest,numErr,rc==SYNC_AGENT_DB_ERR_ROW_NOT_FOUND);

    // Remove the valid entry
    rc = myDb->needDownloadScan_remove(2);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_remove");

    rc = myDb->needDownloadScan_get(entry);
    CHECK_TEST_EXPR(numTest,numErr,rc==SYNC_AGENT_DB_ERR_ROW_NOT_FOUND);

    // No errors expected.
    rc = myDb->needDownloadScan_getErr(5, entry);
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_getErr");
    CHECK_TEST_EXPR(numTest,numErr,entry.row_id==4);
    CHECK_TEST_EXPR(numTest,numErr,entry.dir_path=="entry_A");
    CHECK_TEST_EXPR(numTest,numErr,entry.comp_id==10);
    CHECK_TEST_EXPR(numTest,numErr,entry.err_count==2);

    // All entries removed
    rc = myDb->needDownloadScan_clear();
    CHECK_TEST_ERR(numTest,numErr,rc, "needDownloadScan_clear");

    // Remaining error entry should be gone.
    rc = myDb->needDownloadScan_getErr(5, entry);
    CHECK_TEST_EXPR(numTest,numErr,rc==SYNC_AGENT_DB_ERR_ROW_NOT_FOUND);
}

static const char SQL_CREATE_TEST_TABLE[] =
// The admin table will only ever have 1 row with id==1 (ADMIN_ROW_ID).
"CREATE TABLE IF NOT EXISTS test_table ("
        "row_id INTEGER PRIMARY KEY,"
        "test_num INTEGER NOT NULL);";

static const char SQL_INSERT_TEST_ENTRY[] =
"INSERT INTO test_table (row_id, test_num) VALUES (1, 778);";

static const char SQL_QUERY_TEST[] =
"SELECT test_num FROM test_table WHERE row_id=1;";

static int testReadOnlySqlite3WalDb()
{
    int rc;
    int rv = 0;
    std::string toDeleteTmp = "./to_delete";
    std::string testReadOnlyWalDb = "./testReadOnlyWal.db";
    sqlite3 *dbHandle=NULL;
    char* errmsg;

    rc = Util_rmRecursive(testReadOnlyWalDb, toDeleteTmp);
    if(rc != 0) {
        LOG_ERROR("Cannot setup test: Util_rm_dash_rf(%s):%d",
                  testReadOnlyWalDb.c_str(), rc);
        return rc;
    }

    // Not checking error code.  setup
    Util_InitSqliteTempDir("/tmp");

    // Create DB for test, inserting a single dummy value, then closing the DB.
    // Test will check to see if this db can be opened read-only without issue.
    rc = Util_OpenDbHandle(testReadOnlyWalDb,
                           true,
                           true,
                           /*OUT*/ &dbHandle);
    if(rc != 0) {
        LOG_ERROR("DB creation failed(%s):%d", testReadOnlyWalDb.c_str(), rc);
        return rc;
    }

    rc = sqlite3_exec(dbHandle, SQL_CREATE_TEST_TABLE, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create tables: %d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        goto test_fail;
    }

    rc = sqlite3_exec(dbHandle, SQL_INSERT_TEST_ENTRY, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create tables: %d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        goto test_fail;
    }

    rc = sqlite3_close(dbHandle);
    if(rc != SQLITE_OK) {
        LOG_ERROR("CloseHandle failed:%d", rc);
        rv = rc;
        goto end;
    }
    dbHandle = NULL;

    //  See if this DB can be opened READ_ONLY
    rc = Util_OpenDbHandle(testReadOnlyWalDb,
                           false,
                           false,
                           /*OUT*/ &dbHandle);
    if(rc != 0) {
        LOG_ERROR("DB open read-only failed(%s):%d", testReadOnlyWalDb.c_str(), rc);
        rv = rc;
        goto end;
    }

    {   // Make sure the row in the table can be read.
        sqlite3_stmt *stmt = NULL;
        u64 testValue;
        int argIndex = 0;

        rc = sqlite3_prepare_v2(dbHandle, SQL_QUERY_TEST, -1, &stmt, NULL);
        DB_UTIL_CHECK_PREPARE(rc, rv, dbHandle, test_fail_query);

        rc = sqlite3_step(stmt);
        DB_UTIL_CHECK_STEP(rc, rv, dbHandle, test_fail_query);
        DB_UTIL_CHECK_ROW_EXIST(rc, rv, dbhandle, test_fail_query);

        DB_UTIL_GET_SQLITE_U64(stmt,
                               testValue,
                               argIndex++, rv, test_fail_query);

        LOG_ALWAYS("Retrieved test query number:%d", (int)testValue);

 test_fail_query:
        sqlite3_finalize(stmt);
    }

 test_fail:
    rc = sqlite3_close(dbHandle);
    if(rc != SQLITE_OK) {
        LOG_ERROR("CloseHandle failed:%d", rc);
    }
 end:
    return rv;
}

#ifdef MULTITHREAD

static const char SQL_INSERT_TEST_ENTRY_2[] =
"INSERT INTO test_table (row_id, test_num) VALUES (2, 888);";

static const char SQL_INCREMENT_TEST_ENTRY_2[] =
        "UPDATE test_table "
        "SET test_num=(SELECT test_num FROM test_table WHERE row_id=2)+1 "
        "WHERE row_id=2";

static const char SQL_INSERT_TEST_ENTRY_3[] =
"INSERT INTO test_table (row_id, test_num) VALUES (3, 999);";

struct ThreadContext {
    VPLMutex_t mutex;
    std::string dbName;
    sqlite3* orig_dbHandle;
};



static VPLTHREAD_FN_DECL testThreadRoutine(void* ctx)
{
    ThreadContext* threadCtx = (ThreadContext*)ctx;
    {
        // Locking to make sure primary thread got to the correct point
        MutexAutoLock(&threadCtx->mutex);
        LOG_ALWAYS("Thread starting.");
    }

    int rc;
    sqlite3 * dbHandle = NULL;
    char* errmsg;
    rc = Util_OpenDbHandle(threadCtx->dbName,
                           true,
                           false,
                           /*OUT*/ &dbHandle);
    if(rc != 0) {
        LOG_ERROR("Util_OpenDbHandle(%s):%d", threadCtx->dbName.c_str(), rc);
        goto end;
    }
    LOG_ALWAYS("(1) Thread dbHandle opened");

    rc = sqlite3_exec(dbHandle, SQL_INCREMENT_TEST_ENTRY_2, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create tables: %d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        goto end;
    }
    LOG_ALWAYS("(2) INSERTED ENTRY");

 end:
    LOG_INFO("Exiting thread Done:%p", threadCtx);
    if(dbHandle != NULL) {
        rc = sqlite3_close(dbHandle);
        if(rc != SQLITE_OK) {
            LOG_ERROR("CloseHandle failed:%d", rc);
        }
    }
    return VPLTHREAD_RETURN_VALUE;
}

static int testMultiThreadAccessDb()
{
    int rc;
    int rv = 0;
    std::string toDeleteTmp = "./to_delete";
    std::string testMultiThreadDb = "./testMultiThreadAccess.db";
    sqlite3 *dbHandle=NULL;
    char* errmsg;
    ThreadContext threadCtx;
    VPLDetachableThreadHandle_t thread;

    rc = Util_rmRecursive(testMultiThreadDb, toDeleteTmp);
    if(rc != 0) {
        LOG_ERROR("Cannot setup test: Util_rm_dash_rf(%s):%d",
                  testMultiThreadDb.c_str(), rc);
        return rc;
    }

    // Not checking error code.  setup
    Util_InitSqliteTempDir("/tmp");

    // Create DB for test, inserting a single dummy value, then closing the DB.
    // Test will check to see if this db can be opened read-only without issue.
    rc = Util_OpenDbHandle(testMultiThreadDb,
                           true,
                           true,
                           /*OUT*/ &dbHandle);
    if(rc != 0) {
        LOG_ERROR("DB creation failed(%s):%d", testMultiThreadDb.c_str(), rc);
        return rc;
    }

    rc = VPLMutex_Init(&threadCtx.mutex);
    if(rc != 0) {
        LOG_ERROR("VPLMutex_Init:%d", rc);
        goto test_fail;
    }
    threadCtx.dbName = testMultiThreadDb;
    threadCtx.orig_dbHandle = dbHandle;

    rc = sqlite3_exec(dbHandle, SQL_CREATE_TEST_TABLE, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create tables: %d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        goto test_fail;
    }

    rc = sqlite3_exec(dbHandle, SQL_INSERT_TEST_ENTRY, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create tables: %d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        goto test_fail;
    }

    rc = sqlite3_exec(dbHandle, SQL_INSERT_TEST_ENTRY_2, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create tables: %d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        goto test_fail;
    }

    {
        MutexAutoLock lock(&threadCtx.mutex);
        // Initialize thread
        rc = Util_SpawnThread(testThreadRoutine,
                              &threadCtx,
                              UTIL_DEFAULT_THREAD_STACK_SIZE,
                              VPL_FALSE,
                              &thread);
        if(rc != 0) {
            LOG_ERROR("Util_SpawnThread:%d", rc);
            rv = rc;
            goto test_fail;
        }

        rc = sqlite3_exec(dbHandle, "BEGIN", NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            LOG_ERROR("Failed to create tables: %d, %s", rc, errmsg);
            sqlite3_free(errmsg);
            rv = rc;
            goto test_fail;
        }

        rc = sqlite3_exec(dbHandle, SQL_INCREMENT_TEST_ENTRY_2, NULL, NULL, &errmsg);
        if (rc != SQLITE_OK) {
            LOG_ERROR("Failed to create tables: %d, %s", rc, errmsg);
            sqlite3_free(errmsg);
            rv = rc;
            goto test_fail;
        }
    }

    //// SLEEP, we want to see what the other thread can do.
    LOG_ALWAYS("Main thread sleeping for 10 sec");
    VPLThread_Sleep(VPLTime_FromSec(10));
    LOG_ALWAYS("Main thread waking up");

    rc = sqlite3_exec(dbHandle, "COMMIT", NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create tables: %d, %s", rc, errmsg);
        sqlite3_free(errmsg);
        rv = rc;
        goto test_fail;
    }

    //// SLEEP, we want to see what the other thread can do.
    LOG_ALWAYS("Main thread sleeping for 20 sec");
    VPLThread_Sleep(VPLTime_FromSec(20));
    LOG_ALWAYS("Main thread waking up");

    rc = sqlite3_close(dbHandle);
    if(rc != SQLITE_OK) {
        LOG_ERROR("CloseHandle failed:%d", rc);
        rv = rc;
        goto end;
    }
    dbHandle = NULL;
//
//    //  See if this DB can be opened READ_ONLY
//    rc = Util_OpenDbHandle(testMultiThreadDb,
//                           false,
//                           false,
//                           /*OUT*/ &dbHandle);
//    if(rc != 0) {
//        LOG_ERROR("DB open read-only failed(%s):%d", testMultiThreadDb.c_str(), rc);
//        rv = rc;
//        goto end;
//    }
//
//    {   // Make sure the row in the table can be read.
//        sqlite3_stmt *stmt = NULL;
//        u64 testValue;
//        int argIndex = 0;
//
//        rc = sqlite3_prepare_v2(dbHandle, SQL_QUERY_TEST, -1, &stmt, NULL);
//        DB_UTIL_CHECK_PREPARE(rc, rv, dbHandle, test_fail_query);
//
//        rc = sqlite3_step(stmt);
//        DB_UTIL_CHECK_STEP(rc, rv, dbHandle, test_fail_query);
//        DB_UTIL_CHECK_ROW_EXIST(rc, rv, dbhandle, test_fail_query);
//
//        DB_UTIL_GET_SQLITE_U64(stmt,
//                               testValue,
//                               argIndex++, rv, test_fail_query);
//
//        LOG_ALWAYS("Retrieved test query number:%d", (int)testValue);
//
// test_fail_query:
//        sqlite3_finalize(stmt);
//    }

 test_fail:
    rc = VPLMutex_Destroy(&threadCtx.mutex);
    if(rc != 0) {
        LOG_ERROR("VPLMutex_Destroy:%d", rc);
    }

    rc = sqlite3_close(dbHandle);
    if(rc != SQLITE_OK) {
        LOG_ERROR("CloseHandle failed:%d", rc);
    }
 end:
    return rv;
}

#endif

int main(int argc, char** argv)
{
    std::string workingDir = "./tmpScwDb";
    std::string toDeleteTmp = "./to_delete";
    u32 numErrors = 0;
    u32 numAdminTableTests = 0;
    u32 numAdminTableErrs = 0;
    u32 numNeedDownloadScanTests=0;
    u32 numNeedDownloadScanErrs=0;
    u32 numReadOnlyWalError = 0;

    int rc;

    rc = Util_rmRecursive(workingDir, toDeleteTmp);
    if(rc != 0) {
        LOG_ERROR("Cannot setup test: Util_rm_dash_rf(%s):%d",
                  workingDir.c_str(), rc);
        return rc;
    }

    rc = Util_CreatePath(workingDir.c_str(), true);
    if(rc != 0) {
        LOG_ERROR("Cannot setup test: Util_CreatePath(%s):%d",
                  workingDir.c_str(), rc);
        return rc;
    }

    SyncConfigDb* myDb = new SyncConfigDb(workingDir,
                                          SYNC_DB_FAMILY_TWO_WAY,
                                          1,
                                          2,
                                          std::string("three"));

    rc = myDb->openDb();
    if(rc != 0) {
        LOG_ERROR("openDb:%d", rc);
        return rc;
    }

    adminTableTest(myDb,
                   numAdminTableTests,
                   numAdminTableErrs);
    needDownloadScanTableTest(myDb,
                              numNeedDownloadScanTests,
                              numNeedDownloadScanErrs);

    rc = myDb->closeDb();
    if(rc != 0) {
        LOG_ERROR("closeDb:%d", rc);
        numErrors++;
    }

    LOG_ALWAYS("Begin Read-Only WAL test");
    rc = testReadOnlySqlite3WalDb();
    if(rc == 0) {
        LOG_ALWAYS("Read-Only WAL test successful!");
    }else{
        LOG_ERROR("Read-Only WAL test FAIL");
        numReadOnlyWalError++;
    }

#ifdef MULTITHREAD
    LOG_ALWAYS("Test multi-thread DB access.");
    rc = testMultiThreadAccessDb();
    if(rc == 0) {
        LOG_ALWAYS("multi-thread DB access successful!");
    }else{
        LOG_ERROR("multiple writers FAIL");
    }
#endif

    LOG_ALWAYS("\n\n"
    "=====================================\n"
    "=========== TEST REPORT =============\n"
    "  AdminTable                Tests:%d  Errs:%d\n"
    "  NeedDownloadScanTable     Tests:%d  Errs:%d\n"
    "  Wal-ReadOnly Test         Tests:1   Errs:%d\n",
    numAdminTableTests, numAdminTableErrs,
    numNeedDownloadScanTests, numNeedDownloadScanErrs,
    numReadOnlyWalError);
    LOG_ALWAYS("Other Db Errors:%d", numErrors);
    return 0;
}


