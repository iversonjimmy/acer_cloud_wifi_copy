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
        testGetComponentLastModifyTime("comp_a", DATASETDB_ERR_UNKNOWN_COMPONENT, 0);

        testCreateComponent("comp_a", DATASETDB_COMPONENT_TYPE_FILE, 1, 31, DATASETDB_OK);
        testGetComponentCreateTime("comp_a", DATASETDB_OK, 31);

        testSetComponentLastModifyTime("comp_a", 3, 32, DATASETDB_OK);
        testGetComponentCreateTime("comp_a", DATASETDB_OK, 31);

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
