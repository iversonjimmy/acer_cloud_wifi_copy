#include <iostream>
#include <cstdlib>
#include <cstring>
#include "dataset_db.hpp"

using namespace std;

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

struct stats_record_t {
    char name[256];
    u64 size;
    u64 atime;
    u64 mtime;
    u64 ctime;
    bool isDir;
    u64 versionChanged;
};

u64 getSize(dataset_db& ddb, const char *name);

u64 getSize(dataset_db& ddb, const char *name)
{
    u64 size, atime, mtime, ctime, versionChanged;
    bool isDir;
    if (ddb.getStats(name,
                    size,
                    atime,
                    mtime,
                    ctime,
                    isDir,
                    versionChanged) != DDB_SUCCESS) {
            cout << "Failed to get file stats\n";
            exit(1);
    }

    return size;
}

int main()
{
    unsigned i;
    dataset_db ddb, ddb1;
    u64 size, atime, mtime, ctime, versionChanged;
    bool isDir;
    vector<ddb_stats> contents;
    std::vector<ddb_metadata> md;

    if (ddb.open("testFiles.db") != DDB_SUCCESS) {
        cout << "Failed to open database\n";
        exit(1);
    }

    if (ddb1.open("testFiles.db") != DDB_SUCCESS) {
        cout << "Failed to open database\n";
        exit(1);
    }

    // Check file stats can be set/retrieved correctly 
    stats_record_t stats_tests[3] = {
        {
            "some/directory/names/testname",
            10ULL,
            1000000000000ULL,
            2000000000000ULL,
            3000000000000ULL,
            false,
            5ULL
        },
        {
            "some/testname2",
            100ULL,
            4000000000000ULL,
            5000000000000ULL,
            6000000000000ULL,
            false,
            6ULL
        },
        {
            "rootname",
            42424242ULL,
            6100000000000ULL,
            7200000000000ULL,
            8300000000000ULL,
            true,
            8ULL
        }
    };

    if (ddb.startTransaction() != DDB_SUCCESS) {
        cout << "Failed to start transaction\n";
    }

    for (unsigned i = 0; i < ARRAY_SIZE(stats_tests); ++i) {
        stats_record_t &rec = stats_tests[i];
        if (ddb.setStats(rec.name, rec.size, rec.atime,
                        rec.mtime, rec.ctime,
                        rec.isDir, rec.versionChanged) != DDB_SUCCESS) {
            cout << "Failed to set file stats\n";
            exit(1);
        }
    }       

    // Check the uncommitted transaction is not yet visible.
    // Note: It will be visible if the same connection is used for the
    // read as was used for the write.
    if (ddb1.getStats(stats_tests[0].name, size, atime, mtime,
                    ctime, isDir,
                    versionChanged) != DDB_NOT_FOUND) {
        cout << "File was not determined to be absent 5\n";
        exit(1);
    }
    ddb1.close();

    if (ddb.commitTransaction() != DDB_SUCCESS) {
        cout << "Failed to commit transaction\n";
    }

    for (i = 0; i < ARRAY_SIZE(stats_tests); ++i) {
        stats_record_t &rec = stats_tests[i];
        if (ddb.getStats(stats_tests[i].name, size, atime,
                    mtime, ctime, isDir,
                    versionChanged) != DDB_SUCCESS) {
            cout << "Failed to get file stats\n";
            exit(1);
        }

        if (((rec.isDir && size) || (!rec.isDir && size != rec.size)) ||
            atime != rec.atime || mtime != rec.mtime ||
            ctime != rec.ctime || isDir != rec.isDir ||
            versionChanged != rec.versionChanged) {
            cout << "Failed: getStats data didn't match setStats data for test";
            cout << i << "\n";
        }
    }

    // Check cumulative directory sizes
    if (getSize(ddb, "some") != 110) {
        cout << "Incorrect directory size\n";
        exit(1);
    }
    if (getSize(ddb, "some/directory") != 10) {
        cout << "Incorrect directory size\n";
        exit(1);
    }
    if (getSize(ddb, "some/directory/names") != 10) {
        cout << "Incorrect directory size\n";
        exit(1);
    }

    // Try getting a non-existent directory's contents
    if (ddb.getDirectoryContents("nonexistent/directory",
            contents) != DDB_NOT_FOUND) {
        cout << "Non-existent directory contents check failed\n";
        exit(1);
    }

    // Check we can't query a file's contents
    if (ddb.getDirectoryContents("some/directory/names/testname",
            contents) != DDB_NOT_DIRECTORY) {
        cout << "Failed to get directory contents\n";
        exit(1);
    }

    // Check directory contents
    if (ddb.getDirectoryContents("some", contents) != DDB_SUCCESS) {
        cout << "Failed to get directory contents\n";
        exit(1);
    }
    if (contents.size() != 2) {
        cout << "getDirectoryContents return wrong number of records\n";
        exit(1);
    } 
    if (contents[0].name != string("some/directory") ||
        contents[1].name != string("some/testname2")) {
        cout << "getDirectoryContents returned wrong names\n";
        exit(1);
    }

    // Test metadata set/get
    const char testMetadata[] = "testmetadata";
    const int testMetadataSize = sizeof(testMetadata);

    if (ddb.setMetadata("some/testname2", ~0, testMetadata,
                        testMetadataSize) != DDB_BAD_METADATA_TYPE) {
        cout << "Unsupported metadata test failed\n";
        exit(1);
    }

    if (ddb.setMetadata("some/testname2", DDB_METADATA_SIGNATURE, testMetadata,
                        ~0) != DDB_BAD_METADATA_SIZE) {
        cout << "Metadata size test failed\n";
        exit(1);
    }

    if (ddb.setMetadata("some/testname2", DDB_METADATA_SIGNATURE, testMetadata,
                        testMetadataSize) != DDB_SUCCESS) {
        cout << "Metadata set failed\n";
        exit(1);
    }
    if (ddb.getMetadata("some/testname2", md) != DDB_SUCCESS) {
        cout << "Metadata get failed\n";
        exit(1);
    }
    if (md.size() != 1) {
        cout << "Unexpected number of metadata records\n";
        exit(1);
    }
    if (md[0].len != sizeof(testMetadata)) {
        cout << "Wrong metadata size\n";
        exit(1);
    }
    if (memcmp(md[0].val, testMetadata, md[0].len)) {
        cout << "Metadata mismatch\n";
        exit(1);
    }

    // Test missing metadata case
    if (ddb.getMetadata("some", md) != DDB_SUCCESS) {
        cout << "Metadata get failed\n";
        exit(1);
    }
    if (md.size()) {
        cout << "Unexpected metadata records\n";
        exit(1);
    }    

    // Test metadata truncation 
    if (ddb.setMetadata("some/testname2", DDB_METADATA_SIGNATURE, testMetadata,
                        0) != DDB_SUCCESS) {
        cout << "Metadata set failed\n";
        exit(1);
    }
    if (ddb.getMetadata("some/testname2", md) != DDB_SUCCESS) {
        cout << "Metadata get failed\n";
        exit(1);
    }
    if (md.size()) {
        cout << "Unexpected metadata records\n";
        exit(1);
    }

    // Test metadata deletion    
    if (ddb.setMetadata("some/testname2", DDB_METADATA_SIGNATURE, testMetadata,
                        testMetadataSize) != DDB_SUCCESS) {
        cout << "Metadata set failed\n";
        exit(1);
    }
    if (ddb.deleteMetadata("some/testname2") != DDB_SUCCESS) {
        cout << "Metadata delete failed\n";
        exit(1);
    }
    if (ddb.getMetadata("some/testname2", md) != DDB_SUCCESS) {
        cout << "Metadata get failed\n";
        exit(1);
    }
    if (md.size()) {
        cout << "Unexpected metadata records\n";
        exit(1);
    }    
 
    // Check rolled back transaction
    if (ddb.startTransaction() != DDB_SUCCESS) {
        cout << "Failed to start transaction\n";
    }
    if (ddb.setStats(string("nonexistent/name"),
                        0ULL, 0ULL, 0ULL, 0ULL, false, 0ULL) != DDB_SUCCESS) {
        cout << "Failed to add file\n";
        exit(1);
    }
    if (ddb.abandonTransaction() != DDB_SUCCESS) {
        cout << "Failed to abandon transaction\n";
    }
               
    if (ddb.getStats("nonexistent/name",
                    size, atime, mtime, ctime, isDir,
                    versionChanged) != DDB_NOT_FOUND) {
        cout << "getStats on non-existent file didn't return DDB_NOT_FOUND\n";
        exit(1);
    }

    // Delete a sub-tree 
    if (ddb.deleteStats("some/directory") != DDB_SUCCESS) {
        cout << "Failed to delete directory\n";
        exit(1);
    }

    // Check we only have the right entries now
    if (ddb.getStats("some/directory",
                    size, atime, mtime, ctime, isDir,
                    versionChanged) != DDB_NOT_FOUND) {
        cout << "File was not determined to be absent 1\n";
        exit(1);
    }

    if (ddb.getStats("some/directory/names",
                    size, atime, mtime, ctime, isDir,
                    versionChanged) != DDB_NOT_FOUND) {
        cout << "File was not determined to be absent 2\n";
        exit(1);
    }

    if (ddb.getStats("some/directory/names/testname",
                    size, atime, mtime, ctime, isDir,
                    versionChanged) != DDB_NOT_FOUND) {
        cout << "File was not determined to be absent 3\n";
        exit(1);
    }

    if (ddb.getStats("some/testname2",
                    size, atime, mtime, ctime, isDir,
                    versionChanged) != DDB_SUCCESS) {
        cout << "File was not present\n";
        exit(1);
    }
 
    if (ddb.getStats("some",
                    size, atime, mtime, ctime, isDir,
                    versionChanged) != DDB_SUCCESS) {
        cout << "File was not present\n";
        exit(1);
    }

    // Delete the remaining entries
    if (ddb.deleteStats("some") != DDB_SUCCESS) {
        cout << "Failed to delete directory\n";
        exit(1);
    }

    if (ddb.deleteStats("rootname") != DDB_SUCCESS) {
        cout << "Failed to delete directory\n";
        exit(1);
    }

    ddb.close();
    cout << "Succeeded\n";
    return 0;
}
