#include "DatasetDB.hpp"

#include "TestTrashComponent.hpp"
#include "utils.hpp"

const std::string TestTrashComponent::dbpath = "testTrashComponent.db";

TestTrashComponent::TestTrashComponent(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestTrashComponent::~TestTrashComponent()
{
}

void TestTrashComponent::RunTests()
{
    try {
        u64 version = 33;
        VPLTime_t time = 3321;
        DatasetDBError dberr;
        ComponentInfo componentInfo;
        std::vector<std::pair<u64, u32> > trashvec;

        // error case: trashing non-existing component
        testTrashComponent("comp_a", version, /*index*/0, time, DATASETDB_ERR_UNKNOWN_COMPONENT);

        // success case: trash file component
        version++; time++;
        createComponent("comp_a", version, time);
        testSetComponentPath("comp_a", "path_a", version, time, DATASETDB_OK);
        testSetComponentSize("comp_a", /*size*/17, version, time, DATASETDB_OK);
        version++; time++;
        const u64 version_a = version;
        const u32 index_a = 0;
        testTrashComponent("comp_a", version_a, index_a, time, DATASETDB_OK);
        confirmComponentTrashed("comp_a");
        testGetComponentVersion("", DATASETDB_OK, version);
        dberr = db->GetComponentInfo(version_a, index_a, "comp_a", componentInfo);
        if (dberr != DATASETDB_OK) {
            fatal("GetComponentInfo() returned error %d", dberr);
        }
        dberr = db->ListTrash(trashvec);
        if (dberr != DATASETDB_OK) {
            fatal("ListTrash() returned error %d", dberr);
        }
        if (trashvec.size() != 1) {
            fatal("ListTrash() returned unexpected number %d of entries, expected 1", trashvec.size());
        }
        if (trashvec[0].first != version_a || trashvec[0].second != index_a) {
            fatal("ListTrash() returned unexpected entry");
        }
        testGetComponentPath(version_a, index_a, "comp_a", DATASETDB_OK, "path_a");

        // valid trash id but wrong component name
        testGetComponentPath(version_a, index_a, "comp_b", DATASETDB_ERR_UNKNOWN_COMPONENT, "");

        // success case: restore
        version++; time++;
        testRestoreTrash(version_a, index_a, "comp_a", version, time, DATASETDB_OK);
        confirmComponentVisible("comp_a");
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("comp_a", DATASETDB_OK, version);

        // success case: trash file component
        version++; time++;
        createComponent("comp_b", version, time);
        testSetComponentPath("comp_b", "path_b", version, time, DATASETDB_OK);
        testSetComponentSize("comp_b", /*size*/33, version, time, DATASETDB_OK);
        version++; time++;
        const u64 version_b = version;
        const u32 index_b = 0;
        testTrashComponent("comp_b", version_b, index_b, time, DATASETDB_OK);
        confirmComponentTrashed("comp_b");
        testGetComponentVersion("", DATASETDB_OK, version);

        // success case: delete
        version++; time++;
        testDeleteTrash(version_b, index_b, DATASETDB_OK);

        // error case: cannot restore from deleted trash record
        version++; time++;
        testRestoreTrash(version_b, index_b, "comp_b", version, time, DATASETDB_ERR_UNKNOWN_TRASH);

        // success case: trash file component
        version++; time++;
        createComponent("comp_c", version, time);
        testSetComponentPath("comp_c", "path_c", version, time, DATASETDB_OK);
        testSetComponentSize("comp_c", /*size*/23, version, time, DATASETDB_OK);
        version++; time++;
        const u64 version_c = version;
        const u32 index_c = 0;
        testTrashComponent("comp_c", version_c, index_c, time, DATASETDB_OK);
        confirmComponentTrashed("comp_c");
        testGetComponentVersion("", DATASETDB_OK, version);

        // success case: delete all trash
        version++; time++;
        testDeleteAllTrash(DATASETDB_OK);

        // success case: shared prefix 
        version++; time++;
        createComponent("abc123", version, time);
        testSetComponentPath("abc123", "abc123", version, time, DATASETDB_OK);
        testSetComponentSize("abc123", /*size*/21, version, time, DATASETDB_OK);
        version++; time++;
        createComponent("abc/def/gh", version, time);
        testSetComponentPath("abc/def/gh", "abc/def/gh", version, time, DATASETDB_OK);
        testSetComponentSize("abc/def/gh", /*size*/21, version, time, DATASETDB_OK);
        version++; time++;
        const u64 version_abc_def = version;
        const u32 index_abc_def = 2;
        testTrashComponent("abc/def", version_abc_def, index_abc_def, time, DATASETDB_OK);
        confirmComponentVisible("abc123");
        confirmComponentTrashed("abc/def");
        confirmComponentTrashed("abc/def/gh");

        // success case: restore
        version++; time++;
        testRestoreTrash(version_abc_def, index_abc_def, "abc/def", version, time, DATASETDB_OK);
        confirmComponentVisible("abc/def");
        confirmComponentVisible("abc/def/gh");

        // success case: trash then restore to new name
        version++; time++;
        createComponent("d1/d2/f1", version, time);
        testSetComponentPath("d1/d2/f1", "d1/d2/f1", version, time, DATASETDB_OK);
        testSetComponentSize("d1/d2/f1", /*size*/22, version, time, DATASETDB_OK);
        version++; time++;
        const u64 version_d1_d2 = version;
        const u32 index_d1_d2 = 1;
        testTrashComponent("d1/d2", version_d1_d2, index_d1_d2, time, DATASETDB_OK);
        confirmComponentTrashed("d1/d2");
        confirmComponentTrashed("d1/d2/f1");

        version++; time++;
        testRestoreTrash(version_d1_d2, index_d1_d2, "d3", version, time, DATASETDB_OK);
        confirmComponentVisible("d3");
        confirmComponentVisible("d3/f1");
        confirmComponentUnknown("d1/d2");  // make sure it doesn't exist under old name
        confirmComponentUnknown("d1/d2/f1");  // make sure it doesn't exist under old name
        testGetComponentVersion("", DATASETDB_OK, version);
        testGetComponentVersion("d3", DATASETDB_OK, version);
        testGetComponentVersion("d3/f1", DATASETDB_OK, version);
        testGetComponentVersion("d1", DATASETDB_OK, version_d1_d2);

        // success case: make sure we can trash the whole tree
        version++; time++;
        testTrashComponent("", version, 0, time, DATASETDB_OK);
        confirmComponentTrashed("");
        confirmComponentTrashed("abc123");
        confirmComponentTrashed("abc/def");
        confirmComponentTrashed("abc/def/gh");
        confirmComponentTrashed("d1");
        confirmComponentTrashed("d3");
        confirmComponentTrashed("d3/f1");

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestTrashComponent::PrintTestResult()
{
    printTestResult("TrashComponent", testPassed);
}
