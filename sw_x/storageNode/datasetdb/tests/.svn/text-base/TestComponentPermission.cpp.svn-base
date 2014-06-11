#include "DatasetDB.hpp"

#include "TestComponentPermission.hpp"
#include "utils.hpp"

const std::string TestComponentPermission::dbpath = "testComponentPermission.db";

TestComponentPermission::TestComponentPermission(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestComponentPermission::~TestComponentPermission()
{
}

void TestComponentPermission::RunTests()
{
    try {
        u64 version = 10;
        VPLTime_t time = 12345;
        u32 perm = 12;

        // error case: component does not exist
        testGetComponentPermission("comp_a", DATASETDB_ERR_UNKNOWN_COMPONENT, 0);
        testSetComponentPermission("comp_a", perm, version, DATASETDB_ERR_UNKNOWN_COMPONENT);

        // create a file component and set/get permission
        version++; time++;
        testCreateComponent("comp_a", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        version++; time++; perm++;
        testSetComponentPermission("comp_a", perm, version, DATASETDB_OK);
        testGetComponentPermission("comp_a", DATASETDB_OK, perm);
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("comp_a", DATASETDB_OK, version);

        // change the permission
        version++; time++; perm++;
        testSetComponentPermission("comp_a", perm, version, DATASETDB_OK);
        testGetComponentPermission("comp_a", DATASETDB_OK, perm);
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("comp_a", DATASETDB_OK, version);

        // nested case
        version++; time++;
        testCreateComponent("abc", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        version++; time++;
        testCreateComponent("abc/def", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        version++; time++;
        testCreateComponent("abc/def/gh", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        version++; time++; perm++;
        testSetComponentPermission("abc/def/gh", perm, version, DATASETDB_OK);
        testGetComponentPermission("abc/def/gh", DATASETDB_OK, perm);
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("abc", DATASETDB_OK, version);
        testGetComponentVersion("abc/def", DATASETDB_OK, version);
        testGetComponentVersion("abc/def/gh", DATASETDB_OK, version);

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestComponentPermission::PrintTestResult()
{
    printTestResult("ComponentPermission", testPassed);
}
