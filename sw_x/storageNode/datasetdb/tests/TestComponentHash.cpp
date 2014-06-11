#include "DatasetDB.hpp"

#include "TestComponentHash.hpp"
#include "utils.hpp"

const std::string TestComponentHash::dbpath = "testComponentHash.db";

TestComponentHash::TestComponentHash(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestComponentHash::~TestComponentHash()
{
}

void TestComponentHash::RunTests()
{
    try {
        u64 version = 23;
        VPLTime_t time = 12335;

        std::string hash1 = "12345678901234567890";
        std::string hash2 = "98765432109876543210";

        // trying to get the hash of a non-existing component returns an error
        testGetComponentHash("comp_a", DATASETDB_ERR_UNKNOWN_COMPONENT, "");

        // trying to get the content-path of a non-existing hash returns an error
        testGetContentPathBySizeHash(32, hash1, DATASETDB_ERR_UNKNOWN_CONTENT, "");

        // trying to get the content-path of an empty hash is an error
        testGetContentPathBySizeHash(32, "", DATASETDB_ERR_BAD_REQUEST, "");

        // trying to get the hash from a component without a content is an error
        version++; time++;
        createComponent("comp_a", version, time);
        testGetComponentHash("comp_a", DATASETDB_ERR_UNKNOWN_CONTENT, "");

        // trying to set a hash on a component that has no content is an error
        version++; time++;
        testSetComponentHash("comp_a", hash1, DATASETDB_ERR_UNKNOWN_CONTENT);

        // simplest working case
        version++; time++;
        testSetComponentPath("comp_a", "path_a", version, time, DATASETDB_OK);
        testSetComponentSize("comp_a", /*size*/32, version, time, DATASETDB_OK);
        testSetComponentHash("comp_a", hash1, DATASETDB_OK);
        testGetContentPathBySizeHash(32, hash1, DATASETDB_OK, "path_a");
        testGetContentPathBySizeHash(31, hash1, DATASETDB_ERR_UNKNOWN_CONTENT, "");
        testGetContentHash("path_a", DATASETDB_OK, hash1);

        // change the hash
        version++; time++;
        testSetComponentHash("comp_a", hash2, DATASETDB_OK);
        testGetContentPathBySizeHash(32, hash2, DATASETDB_OK, "path_a");
        testGetContentPathBySizeHash(32, hash1, DATASETDB_ERR_UNKNOWN_CONTENT, "");
        testGetContentHash("path_a", DATASETDB_OK, hash2);

        // change hash via content
        version++; time++;
        testSetContentHash("path_a", hash1, DATASETDB_OK);
        testGetComponentHash("comp_a", DATASETDB_OK, hash1);

        // delete component and try to get hash -> should result in an error
        version++; time++;
        testTrashComponent("comp_a", version, /*index*/0, time, DATASETDB_OK);
        testGetComponentHash("comp_a", DATASETDB_ERR_UNKNOWN_COMPONENT, "");

        // content is separate from whether the component is trashed or not
        testGetContentPathBySizeHash(32, hash1, DATASETDB_OK, "path_a");
        testGetContentPathBySizeHash(32, hash2, DATASETDB_ERR_UNKNOWN_CONTENT, "");

        // trying to get the hash of a directory component is an error
        version++; time++;
        createComponent("a/b/c", version, time);
        testGetComponentHash("a/b", DATASETDB_ERR_IS_DIRECTORY, "");

        // trying to set the hash of a directory component is an error
        version++; time++;
        testSetComponentHash("a/b", hash1, DATASETDB_ERR_IS_DIRECTORY);

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestComponentHash::PrintTestResult()
{
    printTestResult("ComponentHash", testPassed);
}
