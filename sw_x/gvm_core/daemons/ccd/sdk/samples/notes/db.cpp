
#include <log.h>
#include <util_open_db_handle.hpp>

#include "db.hpp"

namespace db
{
    object::object () : 
        error (SQLITE_OK), context (0) 
    {}

    object::object (std::string dbpath, std::string tmppath) : 
        error (SQLITE_OK), context (0) 
    {
        open (dbpath, tmppath); 
    }

    object::~object () 
    { 
        close (); 
    }

    void object::open (std::string dbpath, std::string tmppath)
    {
        static bool static_temp_dir_set = false;

        if (!static_temp_dir_set)
        {
            static_temp_dir_set = true;
            error = Util_InitSqliteTempDir(tmppath);
        }

        if (error == SQLITE_OK)
            error = Util_OpenDbHandle (dbpath, true, true, &context);

        if (error != SQLITE_OK)
            LOG_ERROR("database open error (%s)", sqlite3_errstr (error));
    }

    void object::close ()
    {
        if (context)
        {
            error = sqlite3_close_v2 (context);
            if (error != SQLITE_OK)
                LOG_ERROR("database close error (%s)", sqlite3_errstr (error));
        }
    }

    object::operator bool () const
    {
        return error == SQLITE_OK;
    }

    bool object::has_error () const
    {
        return error != SQLITE_OK;
    }

    bool object::is_open () const
    {
        return context != 0;
    }

    row::row () :
        error (SQLITE_DONE)
    {
    }

    row::row (sqlite3_stmt *statement)
    {
        context = statement;
        error = sqlite3_step (context);
    }

    int row::column_count ()
    {
        return sqlite3_column_count (context);
    }

    int row::column_type (int column)
    {
        return sqlite3_column_type (context, column);
    }

    bool row::has_column (int column)
    {
        return column >= 0 && column < column_count ();
    }

    row::operator bool () const
    {
        return error == SQLITE_ROW;
    }

    bool row::has_error () const
    {
        return error != SQLITE_ROW && error != SQLITE_DONE;
    }

    bool row::is_done () const
    {
        return error == SQLITE_DONE;
    }

    statement::statement (object &db, std::string sql)
    {
        error = sqlite3_prepare_v2 (db.context, sql.c_str(), sql.size()+1, &context, 0);
        if (error != SQLITE_OK)
            LOG_ERROR("statement prepare error (%s) for %s", sqlite3_errstr (error), sql.c_str());
    }

    statement::~statement ()
    {
        error = sqlite3_finalize (context);
        if (error != SQLITE_OK)
            LOG_ERROR("statement finalize error (%s)", sqlite3_errstr (error));
    }

    row statement::step ()
    {
        return row (context);
    }

    void statement::reset ()
    {
        error = sqlite3_reset (context);
    }

    statement::operator bool () const
    {
        return error == SQLITE_OK;
    }

    bool statement::has_error () const
    {
        return error != SQLITE_OK;
    }
}
