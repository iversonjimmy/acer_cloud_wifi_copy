#include "DatasetDB.hpp"

#include "TestCopyComponent.hpp"
#include "utils.hpp"

const std::string TestCopyComponent::dbpath = "testCopyComponent.db";

TestCopyComponent::TestCopyComponent(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestCopyComponent::~TestCopyComponent()
{
}

void TestCopyComponent::RunTests()
{
    try {
        u64 version = 22;
        VPLTime_t time = 11223;

        // error case: trying to copy non-existing component
        version++; time++;
        testCopyComponent("a", "b", version, time, DATASETDB_ERR_UNKNOWN_COMPONENT);

        // error case: moving to itself
        version++; time++;
        createComponent("self", version, time);
        version++; time++;
        testCopyComponent("self", "self", version, time, DATASETDB_ERR_BAD_REQUEST);

        // simple case: file in root
        version++; time++;
        createComponent("a", version, time);
        const VPLTime_t time_a = time;
        testSetComponentPath("a", "path_a", version, time, DATASETDB_OK);
        testSetComponentMetadata("a", 3, "three", DATASETDB_OK);
        version++; time++;
        testCopyComponent("a", "b", version, time, DATASETDB_OK);
        confirmComponentVisible("a");
        confirmComponentVisible("b");
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("b", DATASETDB_OK, version);
        testGetComponentLastModifyTime("", DATASETDB_OK, time);
        testGetComponentLastModifyTime("b", DATASETDB_OK, time_a);
        testGetComponentCreateTime("b", DATASETDB_OK, time_a);
        testGetComponentPath("b", DATASETDB_OK, "path_a");
        testGetComponentMetadata("b", 3, DATASETDB_OK, "three");

        // another simple case
        version++; time++;
        testCopyComponent("a", "d1/d2/aa", version, time, DATASETDB_OK);
        testGetComponentMetadata("d1/d2/aa", 3, DATASETDB_OK, "three");

        // nested case
        version++; time++;
        createComponent("p/q/r/s", version, time);
        const VPLTime_t time_pqrs = time;
        const u64 version_pqrs = version;
        testSetComponentPath("p/q/r/s", "path_s", version, time, DATASETDB_OK);
        testSetComponentMetadata("p/q/r", 5, "p/q/r", DATASETDB_OK);
        testSetComponentMetadata("p/q/r/s", 6, "p/q/r/s", DATASETDB_OK);
        version++; time++;
        testCopyComponent("p/q/r", "p/t", version, time, DATASETDB_OK);
        confirmComponentVisible("p/q/r");
        confirmComponentVisible("p/q/r/s");
        confirmComponentVisible("p/t");
        confirmComponentVisible("p/t/s");
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("p", DATASETDB_OK, version);
        testGetComponentVersion("p/q", DATASETDB_OK, version_pqrs);
        testGetComponentVersion("p/q/r", DATASETDB_OK, version_pqrs);
        testGetComponentVersion("p/q/r/s", DATASETDB_OK, version_pqrs);
        testGetComponentVersion("p/t", DATASETDB_OK, version);
        testGetComponentVersion("p/t/s", DATASETDB_OK, version);
        testGetComponentLastModifyTime("", DATASETDB_OK, time_pqrs);
        testGetComponentLastModifyTime("p", DATASETDB_OK, time);
        testGetComponentLastModifyTime("p/q", DATASETDB_OK, time_pqrs);
        testGetComponentLastModifyTime("p/q/r", DATASETDB_OK, time_pqrs);
        testGetComponentLastModifyTime("p/q/r/s", DATASETDB_OK, time_pqrs);
        testGetComponentLastModifyTime("p/t", DATASETDB_OK, time_pqrs);
        testGetComponentLastModifyTime("p/t/s", DATASETDB_OK, time_pqrs);
        testGetComponentPath("p/t/s", DATASETDB_OK, "path_s");
        testGetComponentMetadata("p/q/r", 5, DATASETDB_OK, "p/q/r");
        testGetComponentMetadata("p/q/r/s", 6, DATASETDB_OK, "p/q/r/s");

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestCopyComponent::PrintTestResult()
{
    printTestResult("CopyComponent", testPassed);
}
