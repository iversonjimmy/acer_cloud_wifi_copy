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

        // trying to move non-existing component
        version++; time++;
        testMoveComponent("a", "b", version, time, DATASETDB_ERR_UNKNOWN_COMPONENT);

        // moving to itself is an error
        version++; time++;
        createComponent("self", version, time);
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentLastModifyTime("", DATASETDB_OK, time);
        testGetComponentLastModifyTime("self", DATASETDB_OK, time);

        version++; time++;
        testMoveComponent("self", "self", version, time, DATASETDB_ERR_BAD_REQUEST);

        // simple case: file in root
        version++; time++;
        createComponent("a", version, time);
        const VPLTime_t time_a = time;

        version++; time++;
        testMoveComponent("a", "b", version, time, DATASETDB_OK);
        confirmComponentVisible("b");
        confirmComponentUnknown("a");
        // Move will not change the time on the moved components.
        // It will change the time of the parents only.
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentLastModifyTime("", DATASETDB_OK, time);
        testGetComponentLastModifyTime("b", DATASETDB_OK, time_a);
        testGetComponentCreateTime("b", DATASETDB_OK, time_a);

        // nested case
        version++; time++;
        createComponent("p/q/r/s", version, time);
        const VPLTime_t time_pqrs = time;
        confirmComponentVisible("p");
        confirmComponentVisible("p/q");
        confirmComponentVisible("p/q/r");
        confirmComponentVisible("p/q/r/s");
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

        version++; time++;
        testMoveComponent("p/q/r", "p/t", version, time, DATASETDB_OK);
        confirmComponentVisible("p/t");
        confirmComponentVisible("p/t/s");
        confirmComponentUnknown("p/q/r");
        confirmComponentUnknown("p/q/r/s");
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("p", DATASETDB_OK, version);
        testGetComponentVersion("p/q", DATASETDB_OK, version);
        testGetComponentVersion("p/t", DATASETDB_OK, version);
        testGetComponentVersion("p/t/s", DATASETDB_OK, version);
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
