#include "DatasetDB.hpp"

#include "TestMoveComponent.hpp"
#include "utils.hpp"

const std::string TestMoveComponent::dbpath = "testMoveComponent.db";

TestMoveComponent::TestMoveComponent(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestMoveComponent::~TestMoveComponent()
{
}

void TestMoveComponent::RunTests()
{
    try {
        u64 version = 32;
        VPLTime_t time = 456;

        // create root component
        version++; time++;
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);

        // error case - trying to move non-existing component
        version++; time++;
        testMoveComponent("a", "b", version, time, DATASETDB_ERR_UNKNOWN_COMPONENT);

        // moving to itself is allowed.  Just updates version and modtime of parent. 
        version++; time++;
        testCreateComponent("self", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentLastModifyTime("", DATASETDB_OK, time);
        testGetComponentLastModifyTime("self", DATASETDB_OK, time);

        version++; time++;
        testMoveComponent("self", "self", version, time, DATASETDB_OK);

        // simple case: file in root
        version++; time++;
        testCreateComponent("a", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        const VPLTime_t time_a = time;

        version++; time++;
        testMoveComponent("a", "b", version, time, DATASETDB_OK);
        testGetComponentVersion("a", DATASETDB_ERR_UNKNOWN_COMPONENT, 0);
        testGetComponentVersion("b", DATASETDB_OK, version);
        // Move will not change the time on the moved components.
        // It will change the time of the parents only.
        testGetComponentCreateTime("b", DATASETDB_OK, time_a);
        testGetComponentLastModifyTime("b", DATASETDB_OK, time_a);
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentLastModifyTime("", DATASETDB_OK, time);

        // nested case
        version++; time++;
        testCreateComponent("p", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        testCreateComponent("p/q", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        testCreateComponent("p/q/r", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        testCreateComponent("p/q/r/s", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        const VPLTime_t time_pqrs = time;
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("p", DATASETDB_OK, version);
        testGetComponentVersion("p/q", DATASETDB_OK, version);
        testGetComponentVersion("p/q/r", DATASETDB_OK, version);
        testGetComponentVersion("p/q/r/s", DATASETDB_OK, version);
        testGetComponentLastModifyTime("", DATASETDB_OK, time);
        testGetComponentLastModifyTime("p", DATASETDB_OK, time);
        testGetComponentLastModifyTime("p/q", DATASETDB_OK, time);
        testGetComponentLastModifyTime("p/q/r", DATASETDB_OK, time_pqrs);
        testGetComponentLastModifyTime("p/q/r/s", DATASETDB_OK, time_pqrs);
	u64 move_setup_version = version;

        version++; time++;
        testMoveComponent("p/q/r", "p/t", version, time, DATASETDB_OK);
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("p", DATASETDB_OK, version);
        testGetComponentVersion("p/q", DATASETDB_OK, version);
        testGetComponentVersion("p/t", DATASETDB_OK, version);
#ifdef ORIGINAL_VERSION_NUMBER_SCHEME
        testGetComponentVersion("p/t/s", DATASETDB_OK, version);
#else
        testGetComponentVersion("p/t/s", DATASETDB_OK, move_setup_version);
#endif
        testGetComponentLastModifyTime("", DATASETDB_OK, time_pqrs);
        testGetComponentLastModifyTime("p", DATASETDB_OK, time);
        testGetComponentLastModifyTime("p/q", DATASETDB_OK, time);
        testGetComponentLastModifyTime("p/t", DATASETDB_OK, time_pqrs);
        testGetComponentLastModifyTime("p/t/s", DATASETDB_OK, time_pqrs);

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestMoveComponent::PrintTestResult()
{
    printTestResult("MoveComponent", testPassed);
}
