#include <cstdlib>
#include <iostream>
#include <unistd.h>

#include "DatasetDB.hpp"

#include "Test.hpp"
#include "utils.hpp"

Test::Test(const std::string &dbpath, u32 options)
{
    std::string ext_path;
    bool fsck_needed = false;

    unlink(dbpath.c_str());

    // WAL journaling includes additional database files.
    // that need to be removed between runs
    ext_path = dbpath + "-wal";
    unlink(ext_path.c_str());
    ext_path = dbpath + "-shm";
    unlink(ext_path.c_str());

    db = new DatasetDB();
    int rv = db->OpenDB(dbpath, fsck_needed, options);
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

void Test::createComponent(const std::string &name,
                           u64 version,
                           VPLTime_t time)
{
    int rv = db->createComponentsToRootIfMissing(name, /*perm*/0, version, time);
    if (rv != DATASETDB_OK)
        fatal("Failed to create component (%s, %llu, %llu): %d", name.c_str(), version, time, rv);
}

// make sure there is non-deleted component with the given name
void Test::confirmComponentVisible(const std::string &name)
{
    int rv;
    sqlite3_stmt *stmt = NULL;

    rv = sqlite3_prepare_v2(db->db, "SELECT 1 FROM component WHERE name=:name AND trash_id IS NULL", -1, &stmt, NULL);
    if (rv != SQLITE_OK)
        fatal("sqlite_prepare_v2 error: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));

    rv = sqlite3_bind_text(stmt, 1, name.c_str(), -1, NULL);
    if (rv != SQLITE_OK)
        fatal("sqlite_bind_text error: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));

    rv = sqlite3_step(stmt);
    if (rv != SQLITE_OK && rv != SQLITE_ROW && rv != SQLITE_DONE) 
        fatal("sqlite_step failed: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));
    if (rv != SQLITE_ROW)
        fatal("component %s is missing or deleted", name.c_str());

    rv = sqlite3_finalize(stmt);
    if (rv != SQLITE_OK)
        fatal("sqlite_finalize failed: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));
}

// make sure that there is at least one trashed component with the given name
// and no non-trashed component with the given name
void Test::confirmComponentTrashed(const std::string &name)
{
    int rv;
    sqlite3_stmt *stmt = NULL;

    rv = sqlite3_prepare_v2(db->db, "SELECT 1 FROM component WHERE name=:name AND trash_id IS NULL", -1, &stmt, NULL);
    if (rv != SQLITE_OK)
        fatal("sqlite_prepare_v2 error: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));

    rv = sqlite3_bind_text(stmt, 1, name.c_str(), -1, NULL);
    if (rv != SQLITE_OK)
        fatal("sqlite_bind_text error: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));

    rv = sqlite3_step(stmt);
    if (rv != SQLITE_OK && rv != SQLITE_ROW && rv != SQLITE_DONE) 
        fatal("sqlite_step failed: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));
    if (rv != SQLITE_DONE)
        fatal("component %s exists undeleted", name.c_str());

    rv = sqlite3_finalize(stmt);
    if (rv != SQLITE_OK)
        fatal("sqlite_finalize failed: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));

    rv = sqlite3_prepare_v2(db->db, "SELECT 1 FROM component WHERE name=:name AND trash_id IS NOT NULL", -1, &stmt, NULL);
    if (rv != SQLITE_OK)
        fatal("sqlite_prepare_v2 error: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));

    rv = sqlite3_bind_text(stmt, 1, name.c_str(), -1, NULL);
    if (rv != SQLITE_OK)
        fatal("sqlite_bind_text error: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));

    rv = sqlite3_step(stmt);
    if (rv != SQLITE_OK && rv != SQLITE_ROW && rv != SQLITE_DONE) 
        fatal("sqlite_step failed: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));
    if (rv != SQLITE_ROW)
        fatal("component %s not in trash bin", name.c_str());

    rv = sqlite3_finalize(stmt);
    if (rv != SQLITE_OK)
        fatal("sqlite_finalize failed: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));
}

// component is not known to the component table
void Test::confirmComponentUnknown(const std::string &name)
{
    int rv;
    sqlite3_stmt *stmt = NULL;

    rv = sqlite3_prepare_v2(db->db, "SELECT 1 FROM component WHERE name=:name", -1, &stmt, NULL);
    if (rv != SQLITE_OK)
        fatal("sqlite_prepare_v2 error: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));

    // bind :name
    rv = sqlite3_bind_text(stmt, 1, name.c_str(), -1, NULL);
    if (rv != SQLITE_OK)
        fatal("sqlite_bind_text error: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));

    rv = sqlite3_step(stmt);
    if (rv != SQLITE_OK && rv != SQLITE_ROW && rv != SQLITE_DONE) 
        fatal("sqlite_step failed: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));
    if (rv != SQLITE_DONE)
        fatal("component %s not known in component table", name.c_str());

    rv = sqlite3_finalize(stmt);
    if (rv != SQLITE_OK)
        fatal("sqlite_finalize failed: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));
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

void Test::testGetDatasetFullMergeVersion(u64 expected_version)
{
    int rv;
    u64 version;
    rv = db->GetDatasetFullMergeVersion(version);
    if (rv != DATASETDB_OK)
	fatal("GetDatasetFullMergeVersion failed: %d", rv);
    if (version != expected_version)
	fatal("Unexpected version %lld, expected %lld", version, expected_version);
}

void Test::testSetDatasetFullMergeVersion(u64 version)
{
    int rv;
    rv = db->SetDatasetFullMergeVersion(version);
    if (rv != DATASETDB_OK)
	fatal("SetDatasetFullMergeVersion failed: %d", rv);
    testGetDatasetFullMergeVersion(version);
}

void Test::testGetDatasetSchemaVersion(u64 expected_version)
{
    int rv;
    u64 version;
    rv = db->GetDatasetSchemaVersion(version);
    if (rv != DATASETDB_OK)
	fatal("GetDatasetSchemaVersion failed: %d", rv);
    if (version != expected_version)
	fatal("Unexpected version %lld, expected %lld", version, expected_version);
}

void Test::testCreateComponent(const std::string &name,
			       int is_directory,
			       u64 version,
			       VPLTime_t time,
			       int expectedErrorCode)
{
    int rv = db->TestAndCreateComponent(name, is_directory, /*perm*/0, version, time);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
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

void Test::testGetComponentPath(u64 version, u32 index,
                                const std::string &name,
                                int expectedErrorCode,
                                const std::string &expectedPath)
{
    std::string path;
    int rv = db->GetComponentPath(version, index, name, path);
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

void Test::testSetComponentSizeIfLarger(const std::string &name, 
					u64 size,
					u64 version,
					VPLTime_t time,
					int expectedErrorCode)
{
    int rv = db->SetComponentSizeIfLarger(name, size, version, time);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testGetComponentHash(const std::string &name, 
				int expectedErrorCode,
				const std::string &expectedHash)
{
    std::string hash;
    int rv = db->GetComponentHash(name, hash);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
	if (hash != expectedHash)
	    fatal("Unexpected hash");
    }
}

void Test::testSetComponentHash(const std::string &name, 
				const std::string &hash,
				int expectedErrorCode)
{
    int rv = db->SetComponentHash(name, hash);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK)
	testGetComponentHash(name, DATASETDB_OK, hash);
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

void Test::testGetComponentMetadata(const std::string &name, 
				    int type,
				    int expectedErrorCode,
				    const std::string &expectedValue)
{
    std::string value;
    int rv = db->GetComponentMetadata(name, type, value);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
	if (value != expectedValue)
	    fatal("Unexpected value");
    }
}

void Test::testGetComponentAllMetadata(const std::string &name, 
                                       std::vector<std::pair<int, std::string> > &metadata,
                                       int expectedErrorCode)
{
    std::string value;
    int rv = db->GetComponentAllMetadata(name, metadata);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testSetComponentMetadata(const std::string &name, 
				    int type,
				    const std::string &value,
				    int expectedErrorCode)
{
    int rv = db->SetComponentMetadata(name, type, value);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK)
	testGetComponentMetadata(name, type, DATASETDB_OK, value);
}

void Test::testDeleteComponentMetadata(const std::string &name, 
                                       int type,
                                       int expectedErrorCode)
{
    int rv = db->DeleteComponentMetadata(name, type);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
        std::string dummy;
	testGetComponentMetadata(name, type, DATASETDB_ERR_UNKNOWN_METADATA, dummy);
    }
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

void Test::testTrashComponent(const std::string &name,
                              u64 version, u32 index,
                              VPLTime_t time,
                              int expectedErrorCode)
{
    int rv = db->TrashComponent(name, version, index, time);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testRestoreTrash(u64 trash_version, u32 index,
                            const std::string &name,
                            u64 version,
                            VPLTime_t time,
                            int expectedErrorCode)
{
    int rv = db->RestoreTrash(trash_version, index, name, /*perm*/0, version, time);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
        TrashAttrs trashAttrs;
        rv = db->getTrashByVersionIndex(version, index, trashAttrs);
        if (rv == DATASETDB_OK)
            fatal("Trash entry still exists after restoring trash");
        else if (rv != DATASETDB_ERR_UNKNOWN_TRASH)
            fatal("Failed to get trash entry: %d", rv);
    }
}

void Test::testDeleteTrash(u64 version, u32 index,
                            int expectedErrorCode)
{
    int rv = db->DeleteTrash(version, index);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
        TrashAttrs trashAttrs;
        rv = db->getTrashByVersionIndex(version, index, trashAttrs);
        if (rv == DATASETDB_OK)
            fatal("Trash entry still exists after deleting trash");
        else if (rv != DATASETDB_ERR_UNKNOWN_TRASH)
            fatal("Failed to get trash entry: %d", rv);
    }
}

void Test::testDeleteAllTrash(int expectedErrorCode)
{
    int rv = db->DeleteAllTrash();
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);

    if (rv == DATASETDB_OK) {
        sqlite3_stmt *stmt = NULL;
        rv = sqlite3_prepare_v2(db->db, "SELECT 1 FROM trash", -1, &stmt, NULL);
        if (rv != SQLITE_OK)
            fatal("sqlite_prepare_v2 error: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));
        rv = sqlite3_step(stmt);
        if (rv != SQLITE_OK && rv != SQLITE_ROW && rv != SQLITE_DONE) 
            fatal("sqlite_step failed: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));
        if (rv != SQLITE_DONE)
            fatal("trash table is not empty");
        rv = sqlite3_finalize(stmt);
        if (rv != SQLITE_OK)
            fatal("sqlite_finalize failed: %d, %s", rv, (const char*)sqlite3_errmsg(db->db));
    }
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

void Test::testCopyComponent(const std::string &old_name,
                             const std::string &new_name,
                             u64 version,
                             VPLTime_t time,
                             int expectedErrorCode)
{
    int rv = db->CopyComponent(old_name, new_name, /*perm*/0, version, time);
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

void Test::testGetContentPathBySizeHash(u64 size,
					const std::string &hash,
					int expectedErrorCode,
					const std::string &expectedPath)
{
    std::string path;
    int rv = db->GetContentPathBySizeHash(size, hash, path);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
        if (path != expectedPath)
            fatal("Unexpected path %s, expected %s", path.c_str(), expectedPath.c_str());
    }
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

void Test::testGetContentHash(const std::string &path, 
				int expectedErrorCode,
				const std::string &expectedHash)
{
    std::string hash;
    int rv = db->GetContentHash(path, hash);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK) {
	if (hash != expectedHash)
	    fatal("Unexpected hash");
    }
}

void Test::testSetContentHash(const std::string &path, 
                              const std::string &hash,
                              int expectedErrorCode)
{
    int rv = db->SetContentHash(path, hash);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    if (rv == DATASETDB_OK)
	testGetContentHash(path, DATASETDB_OK, hash);
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
    int rv = db->DeleteContent(path);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testListUnrefContents(int expectedErrorCode,
                                 std::vector<std::string> &paths)
{
    int rv = db->ListUnrefContents(paths);
    if (rv != expectedErrorCode)
	fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
}

void Test::testCreateComponentAncestryList(const std::string &name,
					   int componentType,
					   bool includeComponent,
					   int expectedErrorCode,
					   std::vector<std::pair<std::string, int> > &expectedNames)
{
    std::vector<std::pair<std::string, int> > names;
    int rv = db->createComponentAncestryList(name, componentType, includeComponent, names);
    if (rv != expectedErrorCode) {
        fatal("Unexpected error code %d, expected %d", rv, expectedErrorCode);
    }
    if (names.size() != expectedNames.size()) {
        fatal("Unexpected number of names %d, expected %d", names.size(), expectedNames.size());
    }
    for (size_t i = 0; i < names.size(); i++) {
	if (names[i] != expectedNames[i]) {
            fatal("Unexpected name <%s,%d>, expected <%s,%d>", names[i].first.c_str(), names[i].second, expectedNames[i].first.c_str(), expectedNames[i].second);
        }
    }
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

