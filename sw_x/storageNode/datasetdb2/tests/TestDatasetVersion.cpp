#include "DatasetDB.hpp"

#include "TestDatasetVersion.hpp"
#include "utils.hpp"

const std::string TestDatasetVersion::dbpath = "testDatasetVersion.db";

TestDatasetVersion::TestDatasetVersion(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestDatasetVersion::~TestDatasetVersion()
{
}

void TestDatasetVersion::RunTests()
{
    try {
        // create root component
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, /*version*/0, /*time*/12345, DATASETDB_OK);

        // make sure version 0 is returned if no entry
        testGetDatasetFullVersion(0ULL);

        // set full version of 5 and make sure it is returned
        testSetDatasetFullVersion(5ULL);

        // make sure it works with version number beyond s64 but within u64
        testSetDatasetFullVersion(1ULL << 63);

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestDatasetVersion::PrintTestResult()
{
    printTestResult("DatasetVersion", testPassed);
}

