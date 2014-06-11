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
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, /*version*/1, /*time*/12345, DATASETDB_OK);

        // test rollback
        testBeginTransaction(DATASETDB_OK);
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, /*version*/2, /*time*/12346, DATASETDB_OK);
        testRollbackTransaction(DATASETDB_OK);
        testGetComponentVersion("", DATASETDB_OK, /*version*/1);

        // test commit
        testBeginTransaction(DATASETDB_OK);
        testSetComponentVersion("", /*version*/2, /*time*/12347, DATASETDB_OK);
        testCommitTransaction(DATASETDB_OK);
        testGetComponentVersion("", DATASETDB_OK, /*version*/2);

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
