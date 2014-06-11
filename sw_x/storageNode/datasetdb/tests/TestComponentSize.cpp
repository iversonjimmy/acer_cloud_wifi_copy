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

        // error case: non-existing component
        testGetComponentSize("comp_a", DATASETDB_ERR_UNKNOWN_COMPONENT, 0);

        // error case: trying to get size of component without content
        version++; time++;
        createComponent("comp_a", version, time);
        testGetComponentSize("comp_a", DATASETDB_ERR_UNKNOWN_CONTENT, 0);

        // error case: trying to set size of component without content
        version++; time++;
        testSetComponentSize("comp_a", /*size*/123, version, time, DATASETDB_ERR_UNKNOWN_CONTENT);

        // case: initial setting of size
        version++; time++;
        testSetComponentPath("comp_a", "path_a", version, time, DATASETDB_OK);
        testSetComponentSize("comp_a", /*size*/123, version, time, DATASETDB_OK);
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("comp_a", DATASETDB_OK, version);

        // case: change size
        version++; time++;
        testSetComponentSize("comp_a", /*size*/96, version, time, DATASETDB_OK);

        // case: if-larger
        version++; time++;
        testSetComponentSizeIfLarger("comp_a", /*size*/ 128, version, time, DATASETDB_OK);
        testGetComponentSize("comp_a", DATASETDB_OK, /*size*/128);

        version++; time++;
        testSetComponentSizeIfLarger("comp_a", /*size*/ 108, version, time, DATASETDB_OK);
        testGetComponentSize("comp_a", DATASETDB_OK, /*size*/128);

        // case: nested
        version++; time++;
        const u64 size_c1 = 34;
        const u64 size_c2 = 54;
        testSetComponentPath("a/b/c1", "path_a_b_c1", version, time, DATASETDB_OK);
        testSetComponentSize("a/b/c1", size_c1, version, time, DATASETDB_OK);
        testSetComponentPath("a/b/c2", "path_a_b_c2", version, time, DATASETDB_OK);
        testSetComponentSize("a/b/c2", size_c2, version, time, DATASETDB_OK);

        // case: size of directory is sum of all files below it
        testGetComponentSize("a/b", DATASETDB_OK, size_c1 + size_c2);

        // case: empty dir return size 0
        version++; time++;
        testTrashComponent("a/b/c1", version, /*index*/0, time, DATASETDB_OK);
        testTrashComponent("a/b/c2", version, /*index*/1, time, DATASETDB_OK);
        testGetComponentSize("a/b", DATASETDB_OK, 0);

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
