#include "DatasetDB.hpp"

#include "TestBigDB.hpp"
#include "utils.hpp"

#include <iostream>

const std::string TestBigDB::dbpath = "testBigDB.db";

TestBigDB::TestBigDB(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestBigDB::~TestBigDB()
{
}

void TestBigDB::RunTests()
{
    try {
        u64 version = 10;
        VPLTime_t time = 12345;
        int i, j, k;

        // create root component
        version++; time++;
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);

        testBeginTransaction(DATASETDB_OK);
        for (i = 0; i < 10; i++) {
            char dirpath1[32];
            sprintf(dirpath1, "d%03d", i + 100);
            version++; time++;
            testCreateComponent(dirpath1, DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
            for (j = 0; j < 100; j++) {
                char dirpath2[32];
                sprintf(dirpath2, "d%03d/d%03d", i + 100, j + 200);
                version++; time++;
                testCreateComponent(dirpath2, DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
                for (k = 0; k < 100; k++) {
                    char filepath[32];
                    sprintf(filepath, "d%03d/d%03d/f%03d", i + 100, j + 200, k + 300);
                    version++; time++;
                    testCreateComponent(filepath, DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
                    testSetComponentPath(filepath, filepath, version, time, DATASETDB_OK);
                }
            }
        }
        testCommitTransaction(DATASETDB_OK);

        VPLTime_t time_start, time_end;
        std::string parentpath = "d102/d238";
        std::vector<std::string> names;
        time_start = VPLTime_GetTimeStamp();
        testListComponents(parentpath, names, DATASETDB_OK);
        time_end = VPLTime_GetTimeStamp();
        std::cout << "ListComponents took " << time_end - time_start << "usec" << std::endl;

        time_start = VPLTime_GetTimeStamp();
        for (i = 0; i < names.size(); i++) {
            std::string path = parentpath + "/" + names[i];
            testGetComponentPath(path, DATASETDB_OK, path);
        }
        time_end = VPLTime_GetTimeStamp();
        std::cout << "sum GetComponentPath took " << time_end - time_start << "usec" << std::endl;

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestBigDB::PrintTestResult()
{
    printTestResult("BigDB", testPassed);
}
