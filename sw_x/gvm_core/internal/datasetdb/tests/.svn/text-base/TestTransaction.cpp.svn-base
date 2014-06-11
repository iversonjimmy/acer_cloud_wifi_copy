#include "DatasetDB.hpp"

#include "TestTransaction.hpp"
#include "utils.hpp"

const std::string TestTransaction::dbpath = "testTransaction.db";

TestTransaction::TestTransaction(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestTransaction::~TestTransaction()
{
}

void TestTransaction::RunTests()
{
    try {
        // COMMIT without BEGIN is an error
        testCommitTransaction(DATASETDB_ERR_SQLITE_ERROR);

        // ROLLBACK without BEGIN is an error
        testRollbackTransaction(DATASETDB_ERR_SQLITE_ERROR);

        // BEGIN-COMMIT empty transaction
        testBeginTransaction(DATASETDB_OK);
        testCommitTransaction(DATASETDB_OK);

        // BEGIN-ROLLBACK empty transaction
        testBeginTransaction(DATASETDB_OK);
        testRollbackTransaction(DATASETDB_OK);

        // set dataset version for testing
        testSetDatasetFullVersion(1);

        // test rollback
        testBeginTransaction(DATASETDB_OK);
        testSetDatasetFullVersion(2);
        testRollbackTransaction(DATASETDB_OK);
        testGetDatasetFullVersion(1);

        // test commit
        testBeginTransaction(DATASETDB_OK);
        testSetDatasetFullVersion(2);
        testCommitTransaction(DATASETDB_OK);
        testGetDatasetFullVersion(2);

        // BEGIN-BEGIN is an error
        testBeginTransaction(DATASETDB_OK);
        testBeginTransaction(DATASETDB_ERR_SQLITE_ERROR);

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestTransaction::PrintTestResult()
{
    printTestResult("Transaction", testPassed);
}
