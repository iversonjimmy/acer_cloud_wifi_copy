#ifndef DB_HPP__
#define DB_HPP__

#include <string>
#include <sqlite3.h>
#include <vpl_types.h>

namespace db
{
    struct object
    {
        int error;
        sqlite3 *context;

        object ();
        object (std::string dbpath, std::string tmppath);
        ~object ();

        void open (std::string dbpath, std::string tmppath);
        void close ();

        operator bool () const;
        bool has_error () const;
        bool is_open () const;
    };

    struct row
    {
        int error;
        sqlite3_stmt *context;

        row ();
        row (sqlite3_stmt *statement);

        int column_count ();
        int column_type (int column);
        bool has_column (int column);

        template <typename T>
        bool read (int column, T &value);

        template <>
        bool read (int column, int &value)
        {
            bool readable = context && has_column (column) && !has_error ();
            if (readable) value = sqlite3_column_int (context, column);
            return readable;
        }

        template <>
        bool read (int column, unsigned int &value)
        {
            bool readable = context && has_column (column) && !has_error ();
            if (readable) value = sqlite3_column_int (context, column);
            return readable;
        }

        template <>
        bool read (int column, int64_t &value)
        {
            bool readable = context && has_column (column) && !has_error ();
            if (readable) value = sqlite3_column_int64 (context, column);
            return readable;
        }

        template <>
        bool read (int column, uint64_t &value)
        {
            bool readable = context && has_column (column) && !has_error ();
            if (readable) value = sqlite3_column_int64 (context, column);
            return readable;
        }

        template <>
        bool read (int column, double &value)
        {
            bool readable = context && has_column (column) && !has_error ();
            if (readable) value = sqlite3_column_double (context, column);
            return readable;
        }

        template <>
        bool read (int column, std::string &value)
        {
            bool readable = context && has_column (column) && !has_error ();
            if (readable) value = (const char *) sqlite3_column_text (context, column);
            return readable;
        }

        operator bool () const;
        bool has_error () const;
        bool is_done () const;
    };

    struct statement
    {
        int error;
        sqlite3_stmt *context;

        statement (object &db, std::string sql);
        ~statement ();

        template <typename T>
        bool bind (int index, T value);

        template <>
        bool bind (int index, int value)
        {
            bool bindable = context != 0;
            if (bindable) error = sqlite3_bind_int (context, index, value);
            return bindable && error == SQLITE_OK;
        }

        template <>
        bool bind (int index, unsigned int value)
        {
            bool bindable = context != 0;
            if (bindable) error = sqlite3_bind_int (context, index, value);
            return bindable && error == SQLITE_OK;
        }

        template <>
        bool bind (int index, int64_t value)
        {
            bool bindable = context != 0;
            if (bindable) error = sqlite3_bind_int64 (context, index, value);
            return bindable && error == SQLITE_OK;
        }

        template <>
        bool bind (int index, uint64_t value)
        {
            bool bindable = context != 0;
            if (bindable) error = sqlite3_bind_int64 (context, index, value);
            return bindable && error == SQLITE_OK;
        }

        template <>
        bool bind (int index, double value)
        {
            bool bindable = context != 0;
            if (bindable) error = sqlite3_bind_double (context, index, value);
            return bindable && error == SQLITE_OK;
        }

        template <>
        bool bind (int index, std::string value)
        {
            bool bindable = context != 0;
            if (bindable) error = sqlite3_bind_text (context, index, value.c_str(), value.size(), SQLITE_TRANSIENT);
            return bindable && error == SQLITE_OK;
        }

        db::row step ();
        void reset ();

        operator bool () const;
        bool has_error () const;
    };
}

#endif
