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
        u64 version = 10;
        VPLTime_t time = 12345;

        // create root component
        version++; time++;
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);

        // error case - cannot get version from non-existent component
        testGetComponentVersion("comp_a", DATASETDB_ERR_UNKNOWN_COMPONENT, 0);

        // error case - cannot set version to non-existent component
        testSetComponentVersion("comp_a", version, time, DATASETDB_ERR_UNKNOWN_COMPONENT);

        // simple cases
        version++; time++;
        testCreateComponent("comp_a", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testSetComponentVersion("comp_a", version, time, DATASETDB_OK);
        testGetComponentVersion("comp_a", DATASETDB_OK, version);
        //testGetComponentCreateTime("comp_a", DATASETDB_OK, 31);

        version++; time++;
        testSetComponentVersion("comp_a", version, time, DATASETDB_OK);
        testGetComponentVersion("comp_a", DATASETDB_OK, version);
        //testGetComponentCreateTime("comp_a", DATASETDB_OK, 31);

#ifdef later
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
#endif

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
