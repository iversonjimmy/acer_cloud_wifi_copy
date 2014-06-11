#include "DatasetDB.hpp"

#include "TestDBConstraint.hpp"
#include "utils.hpp"

const std::string TestDBConstraint::dbpath = "testDBConstraint.db";

TestDBConstraint::TestDBConstraint(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestDBConstraint::~TestDBConstraint()
{
}

static int exec_callback(void *param, int ncols, char *value[], char *name[])
{
    int *counter = (int*)param;
    counter++;
    return 0;
}

void TestDBConstraint::RunTests()
{
    try {
        int rv;
        char *errmsg = NULL;

        // populate with some data
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, name_upper, ctime, mtime, type, version, perm) VALUES (1, 'a', 'A', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, ctime, mtime, type, version, perm) VALUES (1, 'a', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, name_upper, ctime, mtime, type, version, perm) VALUES (2, 'b', 'B', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, ctime, mtime, type, version, perm) VALUES (2, 'b', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO content (id, path) VALUES (1, 'path-a')", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into content table. Error %d", rv);
        }

        // make sure NOT NULL constraint over component.name is working
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, name_upper, ctime, mtime, type, version, perm) VALUES (NULL, 'AA', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, ctime, mtime, type, version, perm) VALUES (NULL, 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over component.name_upper is working
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, name_upper, ctime, mtime, type, version, perm) VALUES ('aa', NULL, 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
            if (rv != SQLITE_CONSTRAINT) {
                fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
            }
        }

        // make sure NOT NULL constraint over component.ctime is working
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, name_upper, ctime, mtime, type, version, perm) VALUES ('aa', 'AA', NULL, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, ctime, mtime, type, version, perm) VALUES ('aa', NULL, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over component.mtime is working
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, name_upper, ctime, mtime, type, version, perm) VALUES ('aa', 'AA', 0, NULL, 0, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, ctime, mtime, type, version, perm) VALUES ('aa', 0, NULL, 0, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over component.type is working
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, name_upper, ctime, mtime, type, version, perm) VALUES ('aa', 'AA', 0, 0, NULL, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, ctime, mtime, type, version, perm) VALUES ('aa', 0, 0, NULL, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over component.version is working
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, name_upper, ctime, mtime, type, version, perm) VALUES ('aa', 'AA', 0, 0, 0, NULL, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, ctime, mtime, type, version, perm) VALUES ('aa', 0, 0, 0, NULL, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over component.perm is working
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, name_upper, ctime, mtime, type, version, perm) VALUES ('aa', 'AA', 0, 0, 0, 0, NULL)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, ctime, mtime, type, version, perm) VALUES ('aa', 0, 0, 0, 0, NULL)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure uniqueness constraint over component.(name_upper, trash_id) is working
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, name_upper, ctime, mtime, type, version, perm) VALUES (1011, 'c-u-n-tid-a', 'C-U-N-TID-A', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, ctime, mtime, type, version, perm) VALUES (1011, 'c-u-n-tid-a', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, name_upper, ctime, mtime, type, version, perm) VALUES (1012, 'c-u-n-tid-b', 'C-U-N-TID-B', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, ctime, mtime, type, version, perm) VALUES (1012, 'c-u-n-tid-b', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (id, version, idx, dtime, component_id, size) VALUES (1013, 1013, 0, 0, 1011, 0)", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "UPDATE component SET trash_id=1013 WHERE id=1011 OR id=1012", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }

        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, name_upper, ctime, mtime, type, version, perm) VALUES (1021, 'c-u-n-tid-a', 'C-U-N-TID-A', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, ctime, mtime, type, version, perm) VALUES (1021, 'c-u-n-tid-a', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, name_upper, ctime, mtime, type, version, perm) VALUES (1022, 'c-u-n-tid-b', 'C-U-N-TID-B', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, ctime, mtime, type, version, perm) VALUES (1022, 'c-u-n-tid-b', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (id, version, idx, dtime, component_id, size) VALUES (1023, 1023, 0, 0, 1021, 0)", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "UPDATE component SET trash_id=1023 WHERE id=1021 OR id=1022", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }

        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, name_upper, ctime, mtime, type, version, perm) VALUES (1031, 'c-u-n-tid-a', 'C-U-N-TID-A', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (id, name, ctime, mtime, type, version, perm) VALUES (1031, 'c-u-n-tid-a', 0, 0, 0, 0, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "UPDATE component SET trash_id=1023 WHERE id=1031", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure foreign key constraint over component.content_id is working 
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, name_upper, content_id, type) VALUES ('aa', 'AA', 33, 0)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, content_id, type) VALUES ('aa', 33, 0)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure foreign key constraint over component.trash_id is working
        if (this->db_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, name_upper, content_id, type, trash_id) VALUES ('aa', 'AA', 1, 0, 33)", NULL, NULL, &errmsg);
        } else {
            rv = sqlite3_exec(getSqlite(), "INSERT INTO component (name, content_id, type, trash_id) VALUES ('aa', 1, 0, 33)", NULL, NULL, &errmsg);
        }
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over metadata.component_id is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO metadata (component_id, type) VALUES (NULL, 0)", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over metadata.type is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO metadata (component_id, type) VALUES (1, NULL)", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure foreign key constraint over metadata.component_id is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO metadata (component_id, type) VALUES (33, 0)", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure uniqueness constraint over metadata.(component_id, type) is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO metadata (component_id, type) VALUES (1, 10)", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into metadata table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO metadata (component_id, type) VALUES (1, 11)", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into metadata table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO metadata (component_id, type) VALUES (2, 10)", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into metadata table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO metadata (component_id, type) VALUES (2, 11)", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into metadata table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO metadata (component_id, type) VALUES (1, 10)", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure deletion of component_id==1 cascades to deleting related entries in metadata
        rv = sqlite3_exec(getSqlite(), "DELETE FROM component WHERE id=1", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into component table. Error %d", rv);
        }
        int counter = 0;
        rv = sqlite3_exec(getSqlite(), "SELECT * FROM metadata WHERE component_id=2", exec_callback, &counter, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to select from metadata table. Error %d", rv);
        }
        if (counter != 0) {
            fatal("Deletion of component entry didn't cascade to deleting metadata entries");
        }

        // make sure uniqueness constraint over content.path is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO content (id, path) VALUES (2, 'path-a')", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over trash.version is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (version, idx, dtime, component_id) VALUES (NULL, 0, 0, 2)", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over trash.idx is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (version, idx, dtime, component_id) VALUES (0, NULL, 0, 2)", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over trash.dtime is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (version, idx, dtime, component_id) VALUES (0, 0, NULL, 2)", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure NOT NULL constraint over trash.component_id is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (version, idx, dtime, component_id) VALUES (0, 0, 0, NULL)", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure foreign key constraint over trash.component_id is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (version, idx, dtime, component_id) VALUES (0, 0, 0, 33)", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        // make sure uniqueness constraint over trash.(version, idx) is working
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (version, idx, dtime, component_id, size) VALUES (1, 10, 0, 2, 1)", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into trash table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (version, idx, dtime, component_id, size) VALUES (1, 11, 0, 2, 1)", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into trash table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (version, idx, dtime, component_id, size) VALUES (2, 10, 0, 2, 1)", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into trash table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (version, idx, dtime, component_id, size) VALUES (2, 11, 0, 2, 1)", NULL, NULL, &errmsg);
        if (rv != SQLITE_OK) {
            fatal("Failed to insert into trash table. Error %d", rv);
        }
        rv = sqlite3_exec(getSqlite(), "INSERT INTO trash (version, idx, dtime, component_id, size) VALUES (1, 10, 0, 2, 1)", NULL, NULL, &errmsg);
        if (rv != SQLITE_CONSTRAINT) {
            fatal("Expected SQLITE_CONSTRAINT error, got %d", rv);
        }

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestDBConstraint::PrintTestResult()
{
    printTestResult("DBConstraint", testPassed);
}
