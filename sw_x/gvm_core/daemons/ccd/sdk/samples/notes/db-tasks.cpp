
#include <iostream>
#include <algorithm>
#include <iterator>

#include "conv.hpp"
#include "db.hpp"
#include "db-tasks.hpp"

namespace db
{
    namespace task
    {
        open_database::open_database (db::object &db) :
            db::task::worker (db) {}

        bool open_database::operator() ()
        {
            database.open ("./notes.db", "./temp/sqlite");
            
            bool proceed = !database.has_error();

            if (!proceed)
                error << "cannot open database";

            return proceed;
        }

        create_note_table::create_note_table (db::object &db) :
            db::task::worker (db) {}

        bool create_note_table::operator() ()
        {
            std::string sql = "CREATE TABLE IF NOT EXISTS note " \
                               "(" \
                               "guid INTEGER PRIMARY KEY, " \
                               "title TEXT NOT NULL, " \
                               "location TEXT NOT NULL, " \
                               "body TEXT NOT NULL" \
                               ");";

            db::statement stmt (database, sql);

            bool proceed = !stmt.has_error () &&
                stmt.step().is_done();

            if (!proceed)
                error << sqlite3_errmsg (database.context);

            return proceed;
        }

        drop_note_table::drop_note_table (db::object &db) :
            db::task::worker (db) {}

        bool drop_note_table::operator() ()
        {
            std::string sql = "DROP TABLE IF EXISTS note;";

            db::statement stmt (database, sql);

            bool proceed = !stmt.has_error () &&
                stmt.step().is_done();

            if (!proceed)
                error << sqlite3_errmsg (database.context);

            return proceed;
        }

        insert_note::insert_note (db::object &db, uint64_t g, std::string t, std::string l, std::string b) :
            db::task::worker (db), guid (g), title (t), location (l), body (b) {}

        bool insert_note::operator() ()
        {
            bool proceed = guid && !title.empty() && !location.empty() && !body.empty();

            if (!proceed)
                error << "missing or bad arguments";

            if (proceed)
            {
                std::string sql = "INSERT INTO note (guid, title, location, body) VALUES (?1, ?2, ?3, ?4);";

                db::statement stmt (database, sql);

                proceed = !stmt.has_error () &&
                    stmt.bind (1, guid) &&
                    stmt.bind (2, title) &&
                    stmt.bind (3, location) &&
                    stmt.bind (4, body) &&
                    stmt.step().is_done();

                if (!proceed)
                    error << sqlite3_errmsg (database.context);
            }

            return proceed;
        }

        update_note::update_note (db::object &db, uint64_t g, std::string t, std::string l, std::string b) :
            db::task::worker (db), guid (g), title (t), location (l), body (b) {}

        bool update_note::operator() ()
        {
            bool proceed = guid && !title.empty() && !location.empty() && !body.empty();

            if (!proceed)
                error << "missing or bad arguments";

            if (proceed)
            {
                std::string sql = "UPDATE note SET title = ?2, location = ?3, body = ?4 WHERE guid = ?1";

                db::statement stmt (database, sql);

                proceed = !stmt.has_error () &&
                    stmt.bind (1, guid) &&
                    stmt.bind (2, title) &&
                    stmt.bind (3, location) &&
                    stmt.bind (4, body) &&
                    stmt.step().is_done();

                if (!proceed)
                    error << sqlite3_errmsg (database.context);
            }

            return proceed;
        }

        delete_note::delete_note (db::object &db, uint64_t g) :
            db::task::worker (db), guid (g) {}

        bool delete_note::operator() ()
        {
            bool proceed = guid != 0;

            if (!proceed)
                error << "missing or bad arguments";

            if (proceed)
            {
                std::string sql = "DELETE FROM note WHERE guid = ?1";

                db::statement stmt (database, sql);

                proceed = !stmt.has_error () &&
                    stmt.bind (1, guid) &&
                    stmt.step().is_done();

                if (!proceed)
                    error << sqlite3_errmsg (database.context);
            }

            return proceed;
        }

        find_note::find_note (db::object &db, uint64_t g) :
            db::task::worker (db), guid (g) {}

        find_note::find_note (db::object &db, std::string k, std::string v) :
            db::task::worker (db), guid (0), key (k), value (v) {}

        bool find_note::operator() ()
        {
            bool proceed = (!key.empty() && !value.empty()) || guid != 0;

            if (!proceed)
                error << "missing or bad arguments";

            if (proceed)
            {
                bool has_search_key = !key.empty();

                if (!has_search_key)
                    key = "guid";

                // NOTE: Sqlite3 does *not* support binding of column names
                std::string sql = "SELECT DISTINCT * FROM note WHERE " + key + " = ?1;";

                db::statement stmt (database, sql);
                proceed = !stmt.has_error (); 
                
                if (!has_search_key)
                    stmt.bind (1, guid), guid = 0;
                else
                    stmt.bind (1, value);

                if (proceed)
                {
                    db::row next;

                    while ((next = stmt.step ()) &&
                            next.read (0, guid) &&
                            next.read (1, title) &&
                            next.read (2, location) &&
                            next.read (3, body));

                   proceed = next.is_done ();
                }

                if (!proceed)
                    error << sqlite3_errmsg (database.context);
            }

            return proceed;
        }

        get_guid_set::get_guid_set (db::object &db) :
            db::task::worker (db) {}

        bool get_guid_set::operator() ()
        {
            db::statement stmt (database, "SELECT guid FROM note;");

            bool proceed = !stmt.has_error ();

            if (proceed)
            {
                db::row next;
                uint64_t guid;

                while ((next = stmt.step ()) && next.read (0, guid))
                    guids.insert (guid);

                proceed = next.is_done ();
            }

            if (!proceed)
                error << sqlite3_errmsg (database.context);

            return proceed;
        }

        dump_notes::dump_notes (db::object &db) :
            db::task::worker (db) {}

        bool dump_notes::operator() ()
        {
            db::statement stmt (database, "SELECT * FROM note;");

            bool proceed = !stmt.has_error ();

            if (proceed)
            {
                db::row next;
                std::string title, location, text;
                uint64_t guid;

                while ((next = stmt.step ()) &&
                        next.read (0, guid) &&
                        next.read (1, title) &&
                        next.read (2, location) &&
                        next.read (3, text))
                    std::cout
                        << " guid: " << guid
                        << " title: " << title
                        << " location: " << location
                        << " body: " << text
                        << std::endl;

                proceed = next.is_done ();
            }

            if (!proceed)
                error << sqlite3_errmsg (database.context);

            return proceed;
        }
        
        merge_sync_data::merge_sync_data (db::object &db) :
            db::task::worker (db) {}

        bool merge_sync_data::operator() ()
        {
            bool proceed = true;

            std::set<uint64_t> file_guids;
            std::vector< std::map<std::string, std::string> >::const_iterator file = sync_data.begin();
            std::vector< std::map<std::string, std::string> >::const_iterator fileend = sync_data.end();

            for (; proceed && file != fileend; ++file)
            {
                proceed = 
                    file->count ("guid") &&
                    file->count ("title") && 
                    file->count ("location") && 
                    file->count ("body");

                if (!proceed)
                    error << "missing fields from sync data";

                if (proceed)
                {
                    uint64_t guid = hex_str_to<uint64_t> (file->find("guid")->second);

                    std::string title    (file->find("title")->second);
                    std::string location (file->find("location")->second);
                    std::string body     (file->find("body")->second);

                    file_guids.insert (guid);

                    find_note found (database, guid);
                    proceed = found ();
                        
                    if (!proceed)
                        error << "unable to find note " << guid;

                    bool exists = proceed && found.guid != 0;
                    
                    if (exists)
                    {
                        delete_note deleted (database, guid);
                        proceed = deleted ();
                        
                        if (!proceed)
                            error << "unable to delete note " << guid;
                    }

                    if (proceed)
                    {
                        insert_note inserted (database, guid, title, location, body);
                        proceed = inserted ();

                        if (!proceed)
                            error << "unable to insert note " << guid;
                    }
                }
            }

            get_guid_set db_guids (database);
            proceed = db_guids ();

            std::set<uint64_t> stale_guids;
            std::set_difference (db_guids.guids.begin(), db_guids.guids.end(), file_guids.begin(), file_guids.end(), 
                    std::inserter (stale_guids, stale_guids.begin()));

            std::set<uint64_t>::const_iterator iter = stale_guids.begin();
            std::set<uint64_t>::const_iterator end = stale_guids.end();

            for (; proceed && iter != end; ++iter)
            {
                delete_note deleted (database, *iter);
                proceed = deleted ();
            }

            return proceed;
        }
    }
}
