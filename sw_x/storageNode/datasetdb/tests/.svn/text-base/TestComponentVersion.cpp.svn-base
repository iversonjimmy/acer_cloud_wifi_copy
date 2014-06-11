#include "DatasetDB.hpp"

#include "TestComponentVersion.hpp"
#include "utils.hpp"

const std::string TestComponentVersion::dbpath = "testComponentVersion.db";

TestComponentVersion::TestComponentVersion(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestComponentVersion::~TestComponentVersion()
{
}

void TestComponentVersion::RunTests()
{
    try {
        testGetComponentVersion("comp_a", DATASETDB_ERR_UNKNOWN_COMPONENT, 0);

        testSetComponentVersion("comp_a", 2, 31, DATASETDB_OK);
        testGetComponentVersion("comp_a", DATASETDB_OK, 2);
        testGetComponentCreateTime("comp_a", DATASETDB_OK, 31);

        testSetComponentVersion("comp_a", 5, 33, DATASETDB_OK);
        testGetComponentVersion("comp_a", DATASETDB_OK, 5);
        testGetComponentCreateTime("comp_a", DATASETDB_OK, 31);

        testSetComponentVersion("abc/def/gh", 7, 37, DATASETDB_OK);
        testGetComponentVersion("", DATASETDB_OK, 7);
        testGetComponentVersion("abc", DATASETDB_OK, 7);
        testGetComponentCreateTime("abc", DATASETDB_OK, 37);
        testGetComponentVersion("abc/def", DATASETDB_OK, 7);
        testGetComponentCreateTime("abc/def", DATASETDB_OK, 37);

        testSetComponentVersion("abc/def/jkl", 8, 39, DATASETDB_OK);
        testGetComponentVersion("", DATASETDB_OK, 8);
        testGetComponentVersion("abc", DATASETDB_OK, 8);
        testGetComponentCreateTime("abc", DATASETDB_OK, 37);
        testGetComponentVersion("abc/def", DATASETDB_OK, 8);
        testGetComponentCreateTime("abc/def", DATASETDB_OK, 37);

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestComponentVersion::PrintTestResult()
{
    printTestResult("ComponentVersion", testPassed);
}
