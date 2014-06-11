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

        std::vector<std::string> components;

        // error case: non-existing component
        testListComponents("a/b", components, DATASETDB_ERR_UNKNOWN_COMPONENT);

        // populate with some components
        version++; time++;
        createComponent("a/b/f1", version, time);
        createComponent("a/b/f2", version, time);
        createComponent("a/b/f3", version, time);
        createComponent("a/b/c/f4", version, time);
        createComponent("a/b/c/d/f5", version, time);

        testListComponents("a/b", components, DATASETDB_OK);
        if (components.size() != 4) {
            fatal("ListComponents() returned unexpected number of components %d, expected 4", components.size());
        }

        bool found_f1 = false, found_f2 = false, found_f3 = false, found_c = false;
        std::vector<std::string>::const_iterator it;
        for (it = components.begin(); it != components.end(); it++) {
            if (*it == "f1") found_f1 = true;
            else if (*it == "f2") found_f2 = true;
            else if (*it == "f3") found_f3 = true;
            else if (*it == "c") found_c = true;
        }
        if (!found_f1 || !found_f2 || !found_f3 || !found_c) {
            fatal("ListComponents() returned unexpected list of components");
        }

        testListComponents("", components, DATASETDB_OK);
        if (components.size() != 1) {
            fatal("ListComponents() returned unexpected number of components %d, expected 1", components.size());
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
