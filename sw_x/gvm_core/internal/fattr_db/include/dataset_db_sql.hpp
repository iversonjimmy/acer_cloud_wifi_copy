#ifndef __DATASET_DB_SQL_HPP__
#define __DATASET_DB_SQL_HPP__

// As things stand there is only one type of metadata, the signature.
// Therefore a single table is used.
// Generic metadata will require another table.
//
// TODO - index names
//
// Note: parentID is not necessary, as the tree is implicit in the full paths.
// However, quickly determining cumulative directory sizes does not seem to
// be efficient using only the paths.  Aside from this, it might 
// require a custom function to do the string processing (glob might do).
static const char addTablesSQL[] = {
   "BEGIN TRANSACTION;\
    CREATE TABLE fileAttributes \
    ( \
        fileID INTEGER PRIMARY KEY, \
        parentID INTEGER, \
        name VARCHAR UNIQUE, \
    	size INTEGER, \
        atime INTEGER, \
    	mtime INTEGER, \
    	ctime INTEGER, \
    	isDir INTEGER, \
        versionChanged INTEGER, \
        signature BLOB \
    ); \
    COMMIT;"
};

static const char haveTablesSQL[] = {
    "SELECT name FROM sqlite_master \
    WHERE type IN ('table','view') AND name='fileAttributes';"
};

static const char startTransactionSQL[] = {
    "BEGIN IMMEDIATE;"
};

static const char abandonTransactionSQL[] = {
    "ROLLBACK;"
};

static const char commitTransactionSQL[] = {
    "COMMIT;"
};

static const char updateStatsSQL[] = {
    "UPDATE fileAttributes \
    SET size = ?1, \
        atime = ?2, \
        mtime = ?3, \
        ctime = ?4, \
        isDir = ?5, \
        versionChanged = ?6 \
    WHERE name = ?7;"
};

// TODO -- investigate more efficient query options 
static const char updateDirSQL[] = {
    "UPDATE fileAttributes \
    SET isDir = 1, \
        versionChanged = ?1, \
        size = (SELECT SUM(size) FROM fileAttributes WHERE \
            parentID = (SELECT fileID FROM fileAttributes WHERE name = ?2)) \
    WHERE name = ?2;"
};

// A straight INSERT OR REPLACE will alter ancestor primary keys,
// potentially orphaning child entries.
static const char setParentSQL[] = {
    "INSERT OR REPLACE INTO fileAttributes \
    (name, parentID, fileID) \
    VALUES (?1, (SELECT fileID FROM fileAttributes WHERE rowid = ?2), \
    (SELECT fileID FROM fileAttributes WHERE name = ?1));"
};

// 0 parentID implies a file/directory at the root
static const char setRootParentSQL[] = {
    "INSERT OR REPLACE INTO fileAttributes \
    (name, parentID, fileID) \
    VALUES (?1, 0, \
    (SELECT fileID FROM fileAttributes WHERE name = ?1));"
};

static const char getStatsSQL[] = {
    "SELECT size, atime, mtime, ctime, isDir, versionChanged \
    FROM fileAttributes \
    WHERE name = ?1;"
};

static const char deleteStatsSQL[] = {
    "DELETE FROM fileAttributes WHERE name LIKE ?1;" 
};

static const char checkDirSQL[] = {
    "SELECT isDir, fileID FROM fileAttributes WHERE name = ?1;" 
};

static const char dirContentsSQL[] = {
    "SELECT size, atime, mtime, ctime, isDir, versionChanged, name \
    FROM fileAttributes WHERE parentID = ?1;" 
};

static const char setMetadataSQL[] = {
    "UPDATE fileAttributes \
    SET signature = ?1 \
    WHERE name = ?2;"
};

static const char getMetadataSQL[] = {
    "SELECT signature \
    FROM fileAttributes \
    WHERE name = ?1;"
};

#endif

