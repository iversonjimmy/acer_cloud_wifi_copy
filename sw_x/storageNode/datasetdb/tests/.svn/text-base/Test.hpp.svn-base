#ifndef __TEST_HPP__
#define __TEST_HPP__

#include "DatasetDB.hpp"

class Test {
public:
    Test(const std::string &dbpath, u32 options);
    virtual ~Test();
    virtual void RunTests() = 0;
    virtual void PrintTestResult() = 0;
protected:
    DatasetDB *db;
    u32 db_options;

    void printTestResult(const std::string &name, bool passed);

    sqlite3 *getSqlite() { return db->db; }

    void createComponent(const std::string &component,
                         u64 version,
                         VPLTime_t time);

    void confirmComponentVisible(const std::string &name);
    void confirmComponentTrashed(const std::string &name);
    void confirmComponentUnknown(const std::string &name);

    void testGetDatasetFullVersion(u64 expectedVersion);
    void testSetDatasetFullVersion(u64 version);
    void testGetDatasetFullMergeVersion(u64 expectedVersion);
    void testSetDatasetFullMergeVersion(u64 version);
    void testGetDatasetSchemaVersion(u64 expectedVersion);

    void testCreateComponent(const std::string &name,
                             int is_directory,
                             u64 version,
                             VPLTime_t time,
                             int expectedErrorCode);

    void testExistComponent(const std::string &name,
			    int expectedErrorCode);

    void testGetComponentType(const std::string &name,
                              int expectedErrorCode,
                              int expectedType);

    void testGetComponentPath(const std::string &name,
                              int expectedErrorCode,
                              const std::string &expectedPath);
    void testGetComponentPath(u64 version, u32 index,
                              const std::string &name,
                              int expectedErrorCode,
                              const std::string &expectedPath);
    void testSetComponentPath(const std::string &name,
                              const std::string &path,
                              u64 version,
                              VPLTime_t time,
                              int expectedErrorCode);

    void testGetComponentSize(const std::string &name,
			      int expectedErrorCode,
			      u64 expectedSize);
    void testSetComponentSize(const std::string &name,
			      u64 size,
                              u64 version,
			      VPLTime_t time,
			      int expectedErrorCode);
    void testSetComponentSizeIfLarger(const std::string &name,
				      u64 size,
				      u64 version,
				      VPLTime_t time,
				      int expectedErrorCode);

    void testGetComponentHash(const std::string &name,
			      int expectedErrorCode,
			      const std::string &expectedHash);
    void testSetComponentHash(const std::string &name,
			      const std::string &hash,
			      int expectedErrorCode);

    void testGetComponentCreateTime(const std::string &name,
				    int expectedErrorCode,
				    VPLTime_t expectedCreateTime);
    void testGetComponentLastModifyTime(const std::string &name,
					int expectedErrorCode,
					VPLTime_t expectedLastModifyTime);
    void testSetComponentLastModifyTime(const std::string &name,
                                        u64 version,
					VPLTime_t mtime,
					int expectedErrorCode);

    void testGetComponentVersion(const std::string &name,
				 int expectedErrorCode,
				 u64 expectedVersion);
    void testSetComponentVersion(const std::string &name,
				 u64 version,
				 VPLTime_t time,
				 int expectedErrorCode);

    void testGetComponentPermission(const std::string &name,
                                    int expectedErrorCode,
                                    u32 expectedPerm);
    void testSetComponentPermission(const std::string &name,
                                    u32 perm,
                                    VPLTime_t time,
                                    int expectedErrorCode);

    void testGetComponentMetadata(const std::string &name,
				  int type,
				  int expectedErrorCode,
				  const std::string &expectedValue);
    void testGetComponentAllMetadata(const std::string &name,
                                     std::vector<std::pair<int, std::string> > &metadata,
                                     int expectedErrorCode);
    void testSetComponentMetadata(const std::string &name,
				  int type,
				  const std::string &value,
				  int expectedErrorCode);
    void testDeleteComponentMetadata(const std::string &name, 
                                     int type,
                                     int expectedErrorCode);

    void testDeleteComponent(const std::string &name,
                            u64 version, 
                            VPLTime_t time,
                            int expectedErrorCode);

    void testTrashComponent(const std::string &name,
                            u64 version, u32 index,
                            VPLTime_t time,
                            int expectedErrorCode);
    void testRestoreTrash(u64 trash_version, u32 index,
                          const std::string &name,
                          u64 version,
                          VPLTime_t time,
                          int expectedErrorCode);
    void testDeleteTrash(u64 version, u32 index,
                         int expectedErrorCode);
    void testDeleteAllTrash(int expectedErrorCode);

    void testMoveComponent(const std::string &old_name,
                           const std::string &new_name,
                           u64 version,
                           VPLTime_t time,
                           int expectedErrorCode);

    void testCopyComponent(const std::string &old_name,
                           const std::string &new_name,
                           u64 version,
                           VPLTime_t time,
                           int expectedErrorCode);

    void testListComponents(const std::string &name,
                            std::vector<std::string> &names,
                            int expectedErrorCode);

    void testGetContentPathBySizeHash(u64 size,
				      const std::string &hash,
				      int expectedErrorCode,
				      const std::string &expectedPath);
    void testGetContentRefCount(const std::string &path,
                                int expectedErrorCode,
                                int expectedRefCount);
    void testGetContentHash(const std::string &path,
                            int expectedErrorCode,
                            const std::string &expectedHash);
    void testSetContentHash(const std::string &path,
                            const std::string &hash,
                            int expectedErrorCode);
    void testAllocateContentPath(int expectedErrorCode,
                                 std::string &path);
    void testDeleteContent(const std::string &path,
                           int expectedErrorCode);
    void testListUnrefContents(int expectedErrorCode,
                               std::vector<std::string> &paths);

    void testCreateComponentAncestryList(const std::string &name,
					 int componentType,
					 bool includeComponent,
					 int expectedErrorCode,
					 std::vector<std::pair<std::string, int> > &expectedNames);

    void testBeginTransaction(int expectedErrorCode);
    void testCommitTransaction(int expectedErrorCode);
    void testRollbackTransaction(int expectedErrorCode);

private:
    Test();  // hide it so that Test(const std::string&) must be called
};

#endif
