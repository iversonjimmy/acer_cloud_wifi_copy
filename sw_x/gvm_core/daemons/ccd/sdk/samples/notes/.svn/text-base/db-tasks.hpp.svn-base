#ifndef DB_TASKS_HPP__
#define DB_TASKS_HPP__

#include <string>
#include <vector>
#include <set>
#include <map>

#include "task.hpp"

namespace db
{
    struct object;

    namespace task
    {
        struct worker : public ::task::worker
        {
            db::object &database;
            worker (db::object &db) : database (db) {}
        };

        struct open_database : public db::task::worker
        {
            open_database (db::object &db);
            bool operator() ();
        };

        struct create_note_table : public db::task::worker
        {
            create_note_table (db::object &db);
            bool operator() ();
        };

        struct drop_note_table : public db::task::worker
        {
            drop_note_table (db::object &db);
            bool operator() ();
        };

        struct insert_note : public db::task::worker
        {
            uint64_t guid;
            std::string title, location, body;

            insert_note (db::object &db, uint64_t guid, std::string title, std::string location, std::string body);
            bool operator() ();
        };

        struct update_note : public db::task::worker
        {
            uint64_t guid;
            std::string title, location, body;

            update_note (db::object &db, uint64_t guid, std::string title, std::string location, std::string body);
            bool operator() ();
        };

        struct delete_note : public db::task::worker
        {
            uint64_t guid;
            delete_note (db::object &db, uint64_t guid);
            bool operator() ();
        };

        struct find_note : public db::task::worker
        {
            uint64_t guid;
            std::string title, location, body;
            std::string key, value;

            find_note (db::object &db, uint64_t guid);
            find_note (db::object &db, std::string key, std::string value);
            bool operator() ();
        };

        struct get_guid_set : public db::task::worker
        {
            std::set<uint64_t> guids;
            get_guid_set (db::object &db);
            bool operator() ();
        };

        struct dump_notes : public db::task::worker
        {
            dump_notes (db::object &db);
            bool operator() ();
        };

        struct merge_sync_data : public db::task::worker
        {
            std::vector< std::map<std::string, std::string> > sync_data;
            merge_sync_data (db::object &db);
            bool operator() ();
        };
    }
}

#endif
