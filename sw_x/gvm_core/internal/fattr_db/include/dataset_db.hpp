#ifndef __DATASET_DB_HPP__
#define __DATASET_DB_HPP__

#include <string>
#include <sqlite3.h>
#include <vector>
#include <vplu_types.h>

// Result codes
enum {
    DDB_SUCCESS             = 0,
    DDB_DB_OPEN_FAILED      = -1,   // Failed to initialize db connection
    DDB_QUERY_FAILED        = -2,   // SQL query failed
    DDB_DB_ALREADY_OPEN     = -3,   // Tried to open the db more than once
    DDB_DB_NOT_OPEN         = -4,   // Tried to execute a query before opening
    DDB_COMPILE_FAILED      = -5,   // SQL precompilation failed
    DDB_NOT_FOUND           = -6,   // File/dir was not found
    DDB_NOT_DIRECTORY       = -7,   // Directory operation performed on file
    DDB_BAD_METADATA_TYPE   = -8,   // Unsupported metadata type
    DDB_BAD_METADATA_SIZE   = -9    // Metadata was too large or had a -ve size
};

// Metadata types
enum {
    DDB_METADATA_SIGNATURE = 0
};

// Returned by getDirectoryContents
struct ddb_stats {
    std::string name;
    u64 size;
    u64 atime;
    u64 mtime;
    u64 ctime;
    bool isDir;
    u64 versionChanged;
};

// Maximum metadata size
enum {
    DDB_MAX_METADATA_SIZE = 256
};

// Returned by getMetadata
struct ddb_metadata {
    int type;
    int len;
    char val[DDB_MAX_METADATA_SIZE];
};

class dataset_db
{
   enum {
        STMT_UPDATE_STATS = 0,
        STMT_SET_PARENT = 1,
        STMT_SET_ROOT_PARENT = 2,
        STMT_UPDATE_DIR = 3,
        STMT_GET_STATS = 4,
        STMT_DELETE_STATS = 5,
        STMT_CHECK_DIR = 6,
        STMT_DIR_CONTENTS = 7,
        STMT_SET_METADATA = 8,
        STMT_GET_METADATA = 9,
        STMT_COUNT = 10
    };

    sqlite3 *db_handle;
    sqlite3_stmt *stmts[STMT_COUNT];

    int haveTables(bool& tables_present);
    int execResultlessQuery(const char *sql);
    int stepResetResultlessStmt(sqlite3_stmt *stmt);
    int getDirID(const std::string& name, u64& id);

public:

    dataset_db();
    ~dataset_db();
    void cleanUp();

    int open(const std::string& file_name);
    void close();

    int startTransaction(); 
    int abandonTransaction();
    int commitTransaction();

    int setStats(const std::string& name,
                        u64 size,
                        u64 atime,
                        u64 mtime,
                        u64 ctime,
                        bool isDir,
                        u64 versionChanged);
    int getStats(const std::string& name,
                        u64& size,
                        u64& atime,
                        u64& mtime,
                        u64& ctime,
                        bool& isDir,
                        u64& versionChanged);
    int deleteStats(const std::string& name);
    int getDirectoryContents(const std::string& dir_name,
                             std::vector<ddb_stats>& recs);

    int setMetadata(const std::string& name,
                    int type, const void *data, int len);
    int getMetadata(const std::string& name,
                    std::vector<ddb_metadata>& recs);
    int deleteMetadata(const std::string& name);
};

#endif

