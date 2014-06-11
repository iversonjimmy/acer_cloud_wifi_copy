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
        // make sure version 0 is returned if no entry
        testGetDatasetFullVersion(0ULL);
        testGetDatasetFullMergeVersion(0ULL);

        // set full version of 5 and make sure it is returned
        testSetDatasetFullVersion(5ULL);

        // set full merge version to 7 and make sure it is returned
        testSetDatasetFullMergeVersion(7ULL);

        // make sure full version is still 5 (i.e., no interference from full merge version)
        testGetDatasetFullVersion(5ULL);

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

