#include "DatasetDB.hpp"

#include "TestComponentMetadata.hpp"
#include "utils.hpp"

const std::string TestComponentMetadata::dbpath = "testComponentMetadata.db";

TestComponentMetadata::TestComponentMetadata(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestComponentMetadata::~TestComponentMetadata()
{
}

void TestComponentMetadata::RunTests()
{
    try {
        std::string value1 = "12345678901234567890";
        std::string value2 = "98765432109876543210";

        ComponentInfo info;
        std::vector<std::pair<int, std::string> > metadata;
        DatasetDBError dberr;

        testGetComponentMetadata("comp_a", 0, DATASETDB_ERR_UNKNOWN_COMPONENT, "");

        testSetComponentPath("comp_a", "path_a", 2, 26, DATASETDB_OK);
        testGetComponentMetadata("comp_a", 0, DATASETDB_ERR_UNKNOWN_METADATA, "");

        testSetComponentMetadata("comp_a", 0, value1, DATASETDB_OK);
        testGetComponentMetadata("comp_a", 0, DATASETDB_OK, value1);

        testSetComponentMetadata("comp_a", 0, value2, DATASETDB_OK);
        testGetComponentMetadata("comp_a", 0, DATASETDB_OK, value2);

        testDeleteComponentMetadata("comp_a", 0, DATASETDB_OK);

        testDeleteComponentMetadata("comp_a", 0, DATASETDB_ERR_UNKNOWN_METADATA);

        testGetComponentAllMetadata("comp_a", metadata, DATASETDB_OK);
        if (metadata.size() != 0) {
            fatal("Expected number of metadata %d, expected 0", metadata.size());
        }

        testSetComponentMetadata("comp_a", 1, value1, DATASETDB_OK);
        testSetComponentMetadata("comp_a", 2, value2, DATASETDB_OK);
        testGetComponentAllMetadata("comp_a", metadata, DATASETDB_OK);
        if (metadata.size() != 2) {
            fatal("Expected number of metadata %d, expected 2", metadata.size());
        }

        bool found_1 = false, found_2 = false;
        std::vector<std::pair<int, std::string> >::const_iterator it;
        for (it = metadata.begin(); it != metadata.end(); it++) {
            if (it->first == 1 && it->second == value1) found_1 = true;
            else if (it->first == 2 && it->second == value2) found_2 = true;
        }
        if (!found_1 || !found_2) {
            fatal("Some metadata missing");
        }

        dberr = db->GetComponentInfo("comp_a", info);
        if (dberr != DATASETDB_OK) {
            fatal("GetComponentInfo() returned error %d", dberr);
        }
        found_1 = false; found_2 = false;
        for (it = info.metadata.begin(); it != info.metadata.end(); it++) {
            if (it->first == 1 && it->second == value1) found_1 = true;
            else if (it->first == 2 && it->second == value2) found_2 = true;
        }
        if (!found_1 || !found_2) {
            fatal("Some metadata missing");
        }

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestComponentMetadata::PrintTestResult()
{
    printTestResult("ComponentMetadata", testPassed);
}
