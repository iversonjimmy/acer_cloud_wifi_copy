#include "DatasetDB.hpp"

#include "TestComponentMTime.hpp"
#include "utils.hpp"

const std::string TestComponentMTime::dbpath = "testComponentMTime.db";

TestComponentMTime::TestComponentMTime(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestComponentMTime::~TestComponentMTime()
{
}

void TestComponentMTime::RunTests()
{
    try {
        u64 version = 10;
        VPLTime_t time = 12345;

        // create root component
        version++; time++;
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);

        // error case - cannot get mtime from non-existent component
        testGetComponentLastModifyTime("comp_a", DATASETDB_ERR_UNKNOWN_COMPONENT, 0);

        // error case - cannot set mtime to non-existent component
        testSetComponentLastModifyTime("comp_a", version, time, DATASETDB_ERR_UNKNOWN_COMPONENT);

        // simple working cases
        version++; time++;
        const VPLTime_t compa_ctime = time;
        testCreateComponent("comp_a", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testGetComponentCreateTime("comp_a", DATASETDB_OK, compa_ctime);
        testGetComponentLastModifyTime("comp_a", DATASETDB_OK, time);

        version++; time++;
        testSetComponentLastModifyTime("comp_a", version, time, DATASETDB_OK);
        testGetComponentCreateTime("comp_a", DATASETDB_OK, compa_ctime);
        testGetComponentLastModifyTime("comp_a", DATASETDB_OK, time);

#ifdef later
        testSetComponentLastModifyTime("mtime/b/c/d", 4, 33, DATASETDB_ERR_UNKNOWN_COMPONENT);

        // nested directory case
        // make sure mtime affects only target of change operation.
        testCreateComponent("mtime/b/c/d", DATASETDB_COMPONENT_TYPE_FILE, 1, 33, DATASETDB_OK);
        testGetComponentCreateTime("mtime", DATASETDB_OK, 33);
        testGetComponentCreateTime("mtime/b", DATASETDB_OK, 33);
        testGetComponentCreateTime("mtime/b/c", DATASETDB_OK, 33);
        testGetComponentCreateTime("mtime/b/c/d", DATASETDB_OK, 33);
        testGetComponentLastModifyTime("", DATASETDB_OK, 33);
        testGetComponentLastModifyTime("mtime", DATASETDB_OK, 33);
        testGetComponentLastModifyTime("mtime/b", DATASETDB_OK, 33);
        testGetComponentLastModifyTime("mtime/b/c", DATASETDB_OK, 33);
        testGetComponentLastModifyTime("mtime/b/c/d", DATASETDB_OK, 33);

        testSetComponentLastModifyTime("mtime/b/c/d", 5, 34, DATASETDB_OK);
        testGetComponentLastModifyTime("", DATASETDB_OK, 33);
        testGetComponentLastModifyTime("mtime", DATASETDB_OK, 33);
        testGetComponentLastModifyTime("mtime/b", DATASETDB_OK, 33);
        testGetComponentLastModifyTime("mtime/b/c", DATASETDB_OK, 33);
        testGetComponentLastModifyTime("mtime/b/c/d", DATASETDB_OK, 34);
#endif

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestComponentMTime::PrintTestResult()
{
    printTestResult("ComponentMTime", testPassed);
}
