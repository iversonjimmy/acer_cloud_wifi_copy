#include "DatasetDB.hpp"

#include "TestComponentPath.hpp"
#include "utils.hpp"

const std::string TestComponentPath::dbpath = "testComponentPath.db";

TestComponentPath::TestComponentPath(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestComponentPath::~TestComponentPath()
{
}

void TestComponentPath::RunTests()
{
    try {
        u64 version = 32;
        VPLTime_t time = 12345;
        //int rv;

        // create root component
        version++; time++;
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);

        // trying to get the location of a non-existing component returns an error
        testGetComponentPath("comp_a", DATASETDB_ERR_UNKNOWN_COMPONENT, "");

        // trying to get path of component without a content
        version++; time++;
        testCreateComponent("comp_a", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);

        // simple success case
        //version++; time++;
        testSetComponentPath("comp_a", "path_a", version, time, DATASETDB_OK);
        //testGetComponentCreateTime("comp_a", DATASETDB_OK, ctime_a);
        //testGetComponentLastModifyTime("comp_a", DATASETDB_OK, ctime_a);
        //testGetComponentVersion("", DATASETDB_OK, version);
        //testGetComponentVersion("comp_a", DATASETDB_OK, version);
        testGetComponentPath("comp_a", DATASETDB_OK, "path_a");

        // nested case
        version++; time++;
        testCreateComponent("d/", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        testCreateComponent("d/e", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        testCreateComponent("d/e/f", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testSetComponentPath("d/e/f", "path_def", version, time, DATASETDB_OK);

#if 0
        ComponentInfo componentInfo;
        rv = db->GetComponentInfo("comp_a", componentInfo);
        if (rv != DATASETDB_OK) {
            fatal("GetComponentInfo() returned error %d", rv);
        }
        if (componentInfo.name != "comp_a" ||
            componentInfo.ctime != ctime_a ||
            componentInfo.mtime != ctime_a ||
            componentInfo.type != DATASETDB_COMPONENT_TYPE_FILE ||
            componentInfo.version != version ||
            componentInfo.path != "path_a1") {
            fatal("GetComponentInfo() returned unexpected info");
        }

        // change the content 
        version++; time++;
        testSetComponentPath("comp_a", "path_a2", version, time, DATASETDB_OK);
        testSetComponentLastModifyTime("comp_a", version, time, DATASETDB_OK);
        testGetComponentCreateTime("comp_a", DATASETDB_OK, ctime_a);
        testGetComponentLastModifyTime("comp_a", DATASETDB_OK, time);
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("comp_a", DATASETDB_OK, version);

        // new component with new content
        version++; time++;
        testSetComponentPath("comp_b", "path_b1", version, time, DATASETDB_OK);
        const VPLTime_t ctime_b = time;
        testGetComponentCreateTime("comp_b", DATASETDB_OK, ctime_b);
        testGetComponentLastModifyTime("comp_b", DATASETDB_OK, time);

        // change the content of b
        version++; time++;
        testSetComponentPath("comp_b", "path_a1", version, time, DATASETDB_OK);
        testSetComponentLastModifyTime("comp_b", version, time, DATASETDB_OK);
        testGetComponentCreateTime("comp_b", DATASETDB_OK, ctime_b);
        testGetComponentLastModifyTime("comp_b", DATASETDB_OK, time);

        version++; time++;
        std::string path1, path2;
        testAllocateContentPath(DATASETDB_OK, path1);
        testAllocateContentPath(DATASETDB_OK, path2);
        if (path1 == path2) {
            fatal("Two calls to AllocateContentPath() returned same path");
        }

        // test GetStorageRefCount
        testGetContentRefCount("path_x", DATASETDB_ERR_UNKNOWN_CONTENT, 0);

        // TODO: need test case for content path that's in the DB but no component references it

        version++; time++;
        testSetComponentPath("comp_y1", "path_y", version, time, DATASETDB_OK);
        version++; time++;
        testSetComponentPath("comp_y2", "path_y", version, time, DATASETDB_OK);
        testGetContentRefCount("path_y", DATASETDB_OK, 2);

        // trying to delete a content entry that is still in use results in an error
        testDeleteContent("path_y", DATASETDB_ERR_SQLITE_CONSTRAINT);
#endif

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestComponentPath::PrintTestResult()
{
    printTestResult("ComponentPath", testPassed);
}
