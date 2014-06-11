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

    void testGetDatasetFullVersion(u64 expectedVersion);
    void testSetDatasetFullVersion(u64 version);

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

    void testDeleteComponent(const std::string &name,
                            u64 version, 
                            VPLTime_t time,
                            int expectedErrorCode);

    void testMoveComponent(const std::string &old_name,
                           const std::string &new_name,
                           u64 version,
                           VPLTime_t time,
                           int expectedErrorCode);

    void testListComponents(const std::string &name,
                            std::vector<std::string> &names,
                            int expectedErrorCode);

    void testListComponentsInfo(const std::string &name,
                                std::vector<ComponentInfo> &components_info,
                                int expectedErrorCode);

    void testGetContentRefCount(const std::string &path,
                                int expectedErrorCode,
                                int expectedRefCount);
    void testGetComponentPathByLocation(const std::string location,
                                        int expectedErrorCode,
                                        const std::string &expectedPath);
    void testAllocateContentPath(int expectedErrorCode,
                                 std::string &path);
    void testDeleteContent(const std::string &path,
                           int expectedErrorCode);
    void testListUnrefContents(int expectedErrorCode,
                               std::vector<std::string> &paths);

    void testBeginTransaction(int expectedErrorCode);
    void testCommitTransaction(int expectedErrorCode);
    void testRollbackTransaction(int expectedErrorCode);

private:
    Test();  // hide it so that Test(const std::string&) must be called
};

#endif
