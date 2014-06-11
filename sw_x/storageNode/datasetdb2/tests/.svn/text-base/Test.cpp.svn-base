#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include "DatasetDB.hpp"

#include "Test.hpp"
#include "utils.hpp"

Test::Test(const std::string &dbpath, u32 options)
{
    std::string ext_path;

    unlink(dbpath.c_str());

    // WAL journaling includes additional database files.
    // that need to be removed between runs
    ext_path = dbpath + "-wal";
    unlink(ext_path.c_str());
    ext_path = dbpath + "-shm";
    unlink(ext_path.c_str());

    db = new DatasetDB();
    int rv = db->OpenDB(dbpath, options);
    if (rv != DATASETDB_OK)
	fatal("OpenDB failed: %d", rv);

    db_options = options;
}

Test::~Test()
{
    int rv = db->CloseDB();
    if (rv != DATASETDB_OK)
	fatal("CloseDB failed: %d", rv);
    delete db;
}

void Test::printTestResult(const std::string &name, bool passed)
{
    std::cout << "TC_RESULT=" << (passed ? "PASS" : "FAIL") << " ;;; TC_NAME=" << name << std::endl;
}

void Test::testGetDatasetFullVersion(u64 expected_version)
{
    int rv;
    u64 version;
    rv = db->GetDatasetFullVersion(version);
    if (rv != DATASETDB_OK)
	fatal("GetDatasetFullVersion failed: %d", rv);
    if (version != expected_version)
	fatal("Unexpected version %lld, expected %lld", version, expected_version);
}

void Test::testSetDatasetFullVersion(u64 version)
{
    int rv;
    rv = db->SetDatasetFullVersion(version);
    if (rv != DATASETDB_OK)
	fatal("SetDatasetFullVersion failed: %d", rv);
    testGetDatasetFullVersion(version);
}

void Test::testCreateComponent(const std::string &name,
			       int is_directory,
			       u64 version,
			       VPLTime_t time,
			       int expectedErrorCode)
{
    int rv = db->TestAndCreateComponent(name, is_directory, /*perm*/0, version, time);
    if (rv != expectedErrorCode) {
        std::cout << "FAIL: TestAndCreateComponent(" 
                  << name << ","
                  << is_directory << ","
                  << 0 << ","
                  << version << ","
                  << time << ")"
                  << std::endl;
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    }
}

void Test::testExistComponent(const std::string &name,
			      int expectedErrorCode)
{
    int rv = db->TestExistComponent(name);
    if (rv != expectedErrorCode) {
        fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    }
}

void Test::testGetComponentType(const std::string &name,
                                int expectedErrorCode,
                                int expectedType)
{
    int type;
    int rv = db->GetComponentType(name, type);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
	if (type != expectedType) 
	    fatal("Unexpected component type %d, expected %d", type, expectedType);
    }
}

void Test::testGetComponentPath(const std::string &name,
                                int expectedErrorCode,
                                const std::string &expectedPath)
{
    std::string path;
    int rv = db->GetComponentPath(name, path);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
	if (path != expectedPath) 
	    fatal("Unexpected content path %s, expected %s", path.c_str(), expectedPath.c_str());
    }
}

void Test::testSetComponentPath(const std::string &name,
                                const std::string &path,
                                u64 version, 
                                VPLTime_t time,
                                int expectedErrorCode)
{
    int rv = db->SetComponentPath(name, path, /*perm*/0, version, time);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK)
	testGetComponentPath(name, DATASETDB_OK, path);
}

void Test::testGetComponentSize(const std::string &name, 
				int expectedErrorCode,
				u64 expectedSize)
{
    u64 size;
    int rv = db->GetComponentSize(name, size);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
	if (size != expectedSize)
	    fatal("Unexpected size %lld, expected %lld", size, expectedSize);
    }
}

void Test::testSetComponentSize(const std::string &name, 
				u64 size,
                                u64 version,
				VPLTime_t time,
				int expectedErrorCode)
{
    int rv = db->SetComponentSize(name, size, version, time);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK)
	testGetComponentSize(name, DATASETDB_OK, size);
}

void Test::testGetComponentCreateTime(const std::string &name, 
				      int expectedErrorCode,
				      VPLTime_t expectedCreateTime)
{
    VPLTime_t ctime;
    int rv = db->GetComponentCreateTime(name, ctime);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
	if (ctime != expectedCreateTime)
	    fatal("Unexpected ctime %lld, expected %lld", ctime, expectedCreateTime);
    }
}

void Test::testGetComponentLastModifyTime(const std::string &name, 
					  int expectedErrorCode,
					  VPLTime_t expectedLastModifyTime)
{
    VPLTime_t mtime;
    int rv = db->GetComponentLastModifyTime(name, mtime);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
	if (mtime != expectedLastModifyTime)
	    fatal("Unexpected mtime of component %s: got %lld, expected %lld", name.c_str(), mtime, expectedLastModifyTime);
    }
}

void Test::testSetComponentLastModifyTime(const std::string &name, 
                                          u64 version,
					  VPLTime_t mtime,
					  int expectedErrorCode)
{
    int rv = db->SetComponentLastModifyTime(name, version, mtime);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK)
	testGetComponentLastModifyTime(name, DATASETDB_OK, mtime);
}

void Test::testGetComponentVersion(const std::string &name, 
					  int expectedErrorCode,
					  u64 expectedVersion)
{
    u64 version;
    int rv = db->GetComponentVersion(name, version);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
	if (version != expectedVersion)
	    fatal("Unexpected version on component %s: got %lld, expected %lld", name.c_str(), version, expectedVersion);
    }
}

void Test::testSetComponentVersion(const std::string &name, 
				   u64 version,
				   VPLTime_t time,
				   int expectedErrorCode)
{
    int rv = db->SetComponentVersion(name, /*perm*/0, version, time);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK)
	testGetComponentVersion(name, DATASETDB_OK, version);
}

void Test::testGetComponentPermission(const std::string &name, 
                                      int expectedErrorCode,
                                      u32 expectedPerm)
{
    u32 perm;
    int rv = db->GetComponentPermission(name, perm);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
	if (perm != expectedPerm)
	    fatal("Unexpected perm on component %s: got %d, expected %d", name.c_str(), perm, expectedPerm);
    }
}

void Test::testSetComponentPermission(const std::string &name, 
                                      u32 perm,
                                      u64 version,
                                      int expectedErrorCode)
{
    int rv = db->SetComponentPermission(name, perm, version);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK)
	testGetComponentPermission(name, DATASETDB_OK, perm);
}

void Test::testDeleteComponent(const std::string &name,
                               u64 version, 
                               VPLTime_t time,
                               int expectedErrorCode)
{
    int rv = db->DeleteComponent(name, version, time);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testMoveComponent(const std::string &old_name,
                             const std::string &new_name,
                             u64 version,
                             VPLTime_t time,
                             int expectedErrorCode)
{
    int rv = db->MoveComponent(old_name, new_name, /*perm*/0, version, time);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testListComponents(const std::string &name,
                              std::vector<std::string> &names,
                              int expectedErrorCode)
{
    int rv = db->ListComponents(name, names);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testListComponentsInfo(const std::string &name,
                                  std::vector<ComponentInfo> &components_info,
                                  int expectedErrorCode)
{
    int rv = db->ListComponentsInfo(name, components_info);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testGetContentRefCount(const std::string &path,
                                  int expectedErrorCode,
                                  int expectedRefCount)
{
    int count;
    int rv = db->GetContentRefCount(path, count);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
        if (count != expectedRefCount)
            fatal("Unexpected count %d, expected %d", count, expectedRefCount);
    }
}

void Test::testAllocateContentPath(int expectedErrorCode,
                                   std::string &path)
{
    int rv = db->AllocateContentPath(path);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testDeleteContent(const std::string &path,
                             int expectedErrorCode)
{
    int rv = db->DeleteContentComponentByLocation(path);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testBeginTransaction(int expectedErrorCode)
{
    int rv = db->BeginTransaction();
    if (rv != expectedErrorCode)
	fatal("Unexpected error returned for BEGIN: got %d, expected %d", rv, expectedErrorCode);
}

void Test::testCommitTransaction(int expectedErrorCode)
{
    int rv = db->CommitTransaction();
    if (rv != expectedErrorCode)
	fatal("Unexpected error returned for COMMIT: got %d, expected %d", rv, expectedErrorCode);
}

void Test::testRollbackTransaction(int expectedErrorCode)
{
    int rv = db->RollbackTransaction();
    if (rv != expectedErrorCode)
	fatal("Unexpected error returned for ROLLBACK: got %d, expected %d", rv, expectedErrorCode);
}

