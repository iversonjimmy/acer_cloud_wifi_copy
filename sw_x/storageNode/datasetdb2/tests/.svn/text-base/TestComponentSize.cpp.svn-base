#include "DatasetDB.hpp"

#include "TestComponentSize.hpp"
#include "utils.hpp"

const std::string TestComponentSize::dbpath = "testComponentSize.db";

TestComponentSize::TestComponentSize(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestComponentSize::~TestComponentSize()
{
}

void TestComponentSize::RunTests()
{
    try {
        u64 version = 23;
        VPLTime_t time = 12233;

        // create root component
        version++; time++;
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);

        // error case: non-existing component
        testGetComponentSize("comp_a", DATASETDB_ERR_UNKNOWN_COMPONENT, 0);

        // error case: trying to get size of component without content
        version++; time++;
        testCreateComponent("comp_a", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testGetComponentSize("comp_a", DATASETDB_OK, 0);

        // error case: trying to set size of component without content
        version++; time++;
        testSetComponentSize("comp_a", /*size*/123, version, time, DATASETDB_OK);

        // case: initial setting of size
        version++; time++;
        testSetComponentPath("comp_a", "path_a", version, time, DATASETDB_OK);
        testSetComponentSize("comp_a", /*size*/123, version, time, DATASETDB_OK);
        testGetComponentSize("comp_a", DATASETDB_OK, /*size*/123);

        // case: change size
        version++; time++;
        testSetComponentSize("comp_a", /*size*/96, version, time, DATASETDB_OK);
        testGetComponentSize("comp_a", DATASETDB_OK, /*size*/96);

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestComponentSize::PrintTestResult()
{
    printTestResult("ComponentSize", testPassed);
}
