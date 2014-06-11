#include "DatasetDB.hpp"

#include "TestListComponents.hpp"
#include "utils.hpp"

const std::string TestListComponents::dbpath = "testListComponents.db";

TestListComponents::TestListComponents(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestListComponents::~TestListComponents()
{
}

void TestListComponents::RunTests()
{
    try {
        u64 version = 32;
        VPLTime_t time = 456;

        // create root component
        version++; time++;
        testCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);

        std::vector<std::string> names;
        std::vector<ComponentInfo> components_info;

        // error case: non-existing component
        testListComponents("a/b", names, DATASETDB_ERR_UNKNOWN_COMPONENT);

        testListComponentsInfo("a/b", components_info, DATASETDB_ERR_UNKNOWN_COMPONENT);
        // populate with some components
        version++; time++;
        testCreateComponent("a", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        testCreateComponent("a/b", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        testCreateComponent("a/b/c", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        testCreateComponent("a/b/c/d", DATASETDB_COMPONENT_TYPE_DIRECTORY, version, time, DATASETDB_OK);
        testCreateComponent("a/b/f1", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testSetComponentPath("a/b/f1", "f1_location", version, time, DATASETDB_OK);
        testCreateComponent("a/b/f2", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testSetComponentPath("a/b/f2", "f2_location", version, time, DATASETDB_OK);
        testCreateComponent("a/b/f3", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testSetComponentPath("a/b/f3", "f3_location", version, time, DATASETDB_OK);
        testCreateComponent("a/b/c/f4", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testSetComponentPath("a/b/c/f4", "f4_location", version, time, DATASETDB_OK);
        testCreateComponent("a/b/c/d/f5", DATASETDB_COMPONENT_TYPE_FILE, version, time, DATASETDB_OK);
        testSetComponentPath("a/b/c/d/f5", "f5_location", version, time, DATASETDB_OK);

        testListComponents("a/b", names, DATASETDB_OK);
        if (names.size() != 4) {
            fatal("ListComponents() returned unexpected number of components %d, expected 4", names.size());
        }

        testListComponentsInfo("a/b", components_info, DATASETDB_OK);
        if (components_info.size() != 4) {
            fatal("ListComponentsInfo() returned unexpected number of components %d, expected 4", components_info.size());
        }
        bool found_f1 = false, found_f2 = false, found_f3 = false, found_c = false;
        std::vector<std::string>::const_iterator it;
        for (it = names.begin(); it != names.end(); it++) {
            if (*it == "f1") found_f1 = true;
            else if (*it == "f2") found_f2 = true;
            else if (*it == "f3") found_f3 = true;
            else if (*it == "c") found_c = true;
        }
        if (!found_f1 || !found_f2 || !found_f3 || !found_c) {
            fatal("ListComponents() returned unexpected list of components");
        }
        
        found_f1 = false;
        found_f2 = false;
        found_f3 = false;
        found_c = false;

        bool found_f1_location = false, found_f2_location = false, found_f3_location = false;
        std::vector<ComponentInfo>::const_iterator info_it;
        for (info_it = components_info.begin(); info_it != components_info.end(); info_it++) {
            if ((*info_it).name == "f1") found_f1 = true;
            else if ((*info_it).name == "f2") found_f2 = true;
            else if ((*info_it).name == "f3") found_f3 = true;
            else if ((*info_it).name == "c") found_c = true;

            if ((*info_it).path == "f1_location") found_f1_location = true;
            else if ((*info_it).path == "f2_location") found_f2_location = true;
            else if ((*info_it).path == "f3_location") found_f3_location = true;
        }
        if (!found_f1 || !found_f2 || !found_f3 || !found_c) {
            fatal("ListComponentsInfo() returned unexpected list of components");
        }

        testListComponents("", names, DATASETDB_OK);
        if (names.size() != 1) {
            fatal("ListComponents() returned unexpected number of components %d, expected 1", names.size());
        }

        testListComponentsInfo("", components_info, DATASETDB_OK);
        if (components_info.size() != 1) {
            fatal("ListComponentsInfo() returned unexpected number of components %d, expected 1", components_info.size());
        }

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestListComponents::PrintTestResult()
{
    printTestResult("ListComponents", testPassed);
}
