#include "DatasetDB.hpp"

#include "TestCreateComponentAncestryList.hpp"
#include "utils.hpp"

const std::string TestCreateComponentAncestryList::dbpath = "none.db";

TestCreateComponentAncestryList::TestCreateComponentAncestryList(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestCreateComponentAncestryList::~TestCreateComponentAncestryList()
{
}

void TestCreateComponentAncestryList::RunTests()
{
    try {
        std::vector<std::pair<std::string, int> > expectedNames;

        // error cases

        expectedNames.clear();
        testCreateComponentAncestryList("", DATASETDB_COMPONENT_TYPE_FILE, true, DATASETDB_ERR_BAD_REQUEST, /*dummy*/expectedNames);

        // success cases

        expectedNames.clear();
        testCreateComponentAncestryList("", DATASETDB_COMPONENT_TYPE_DIRECTORY, false, DATASETDB_OK, expectedNames);

        expectedNames.clear();
        expectedNames.push_back(std::make_pair<std::string, int>("", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        testCreateComponentAncestryList("", DATASETDB_COMPONENT_TYPE_DIRECTORY, true, DATASETDB_OK, expectedNames);

        expectedNames.clear();
        expectedNames.push_back(std::make_pair<std::string, int>("", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        expectedNames.push_back(std::make_pair<std::string, int>("a", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        expectedNames.push_back(std::make_pair<std::string, int>("a/b", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        expectedNames.push_back(std::make_pair<std::string, int>("a/b/c", DATASETDB_COMPONENT_TYPE_FILE));
        testCreateComponentAncestryList("a/b/c", DATASETDB_COMPONENT_TYPE_FILE, true, DATASETDB_OK, expectedNames);

        expectedNames.clear();
        expectedNames.push_back(std::make_pair<std::string, int>("", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        expectedNames.push_back(std::make_pair<std::string, int>("a", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        expectedNames.push_back(std::make_pair<std::string, int>("a/b", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        expectedNames.push_back(std::make_pair<std::string, int>("a/b/c", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        testCreateComponentAncestryList("a/b/c", DATASETDB_COMPONENT_TYPE_DIRECTORY, true, DATASETDB_OK, expectedNames);

        expectedNames.clear();
        expectedNames.push_back(std::make_pair<std::string, int>("", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        expectedNames.push_back(std::make_pair<std::string, int>("a", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        expectedNames.push_back(std::make_pair<std::string, int>("a/b", DATASETDB_COMPONENT_TYPE_DIRECTORY));
        testCreateComponentAncestryList("a/b/c", DATASETDB_COMPONENT_TYPE_FILE, false, DATASETDB_OK, expectedNames);

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestCreateComponentAncestryList::PrintTestResult()
{
    printTestResult("CreateComponentAncestryList", testPassed);
}
