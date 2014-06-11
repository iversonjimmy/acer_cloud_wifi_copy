#include "DatasetDB.hpp"

#include "TestCreateComponent.hpp"
#include "utils.hpp"

const std::string TestCreateComponent::dbpath = "testCreateComponent.db";

TestCreateComponent::TestCreateComponent(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestCreateComponent::~TestCreateComponent()
{
}

void TestCreateComponent::RunTests()
{
    try {
        u64 version = 32;
        VPLTime_t time = 12345;

        // error case: not possible to create root component as file
        version++; time++;
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_ERR_BAD_REQUEST);

        // success case: root component as directory
        version++; time++;
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        testGetComponentType("", DATASETDB_OK, DATASETDB_COMPONENT_TYPE_DIRECTORY);

        // success case: top-level component
        version++; time++;
        testCreateComponent("top", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        const u64 version_top = version;
        const VPLTime_t time_top = time;
        testGetComponentVersion("top", DATASETDB_OK, version_top);
        testGetComponentLastModifyTime("top", DATASETDB_OK, time_top);
        testGetComponentType("top", DATASETDB_OK, DATASETDB_COMPONENT_TYPE_FILE);
        testExistComponent("top", DATASETDB_OK);
        testExistComponent("top2", DATASETDB_ERR_UNKNOWN_COMPONENT);

        // asking to create a component with the same name and type is okay and is a nop
        version++; time++;
        testCreateComponent("top", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testGetComponentVersion("top", DATASETDB_OK, version_top);
        testGetComponentLastModifyTime("top", DATASETDB_OK, time_top);

        // asking to create a component with the same name but different type is an error
        version++; time++;
        testCreateComponent("top", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_ERR_WRONG_COMPONENT_TYPE);

        // make sure case sensitive
        if (!(db_options & DATASETDB_OPTION_CASE_INSENSITIVE)) {
            version++; time++;
            testCreateComponent("case", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
            version++; time++;
            testCreateComponent("CASE", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        }

        // nested component case
        // make sure intermediate components are created
        version++; time++;
        const VPLTime_t time_abc = time;
        testCreateComponent("a/b/c", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        confirmComponentVisible("a");
        confirmComponentVisible("a/b");
        confirmComponentVisible("a/b/c");

        version++; time++;
        testCreateComponent("a/b/c2", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("a", DATASETDB_OK, version);
        testGetComponentVersion("a/b", DATASETDB_OK, version);

        // delete component
        version++; time++;
        testDeleteComponent("a/b", version, time, DATASETDB_OK);
        confirmComponentUnknown("a/b");
        confirmComponentUnknown("a/b/c");
        confirmComponentUnknown("a/b/c2");
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("a", DATASETDB_OK, version);
        testGetComponentLastModifyTime("", DATASETDB_OK, time_abc);
        testGetComponentLastModifyTime("a", DATASETDB_OK, time);

        // delete root component
        version++; time++;
        testDeleteComponent("", version, time, DATASETDB_OK);
        confirmComponentUnknown("");
        confirmComponentUnknown("a");

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestCreateComponent::PrintTestResult()
{
    printTestResult("CreateComponent", testPassed);
}
