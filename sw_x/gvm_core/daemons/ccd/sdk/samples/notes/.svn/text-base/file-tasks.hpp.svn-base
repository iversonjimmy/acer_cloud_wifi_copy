#ifndef FILE_TASKS_HPP__
#define FILE_TASKS_HPP__

#include <string>
#include <vector>
#include <map>

#include "task.hpp"

namespace file
{
    namespace task
    {
        struct worker : public ::task::worker
        {
            std::string const &sync_path;
            worker (std::string const &path) : sync_path (path) {}
        };

        struct create_note : public file::task::worker
        {
            std::vector<std::string> note_fields;
            create_note (std::string const &path, std::vector<std::string> fields);

            bool operator() ();
        };

        struct update_note : public file::task::worker
        {
            std::string note_guid;
            std::vector<std::string> note_fields;
            update_note (std::string const &path, std::string guid, std::vector<std::string> fields);

            bool operator() ();
        };

        struct delete_note : public file::task::worker
        {
            std::string note_guid;
            delete_note (std::string const &path, std::string guid);

            bool operator() ();
        };
        
        struct attach_media : public file::task::worker
        {
            std::string note_guid, media_path, media_type;
            attach_media (std::string const &path, std::string guid, std::string media, std::string type = "");

            bool operator() ();
        };
        
        struct remove_media : public file::task::worker
        {
            std::string media_guid;
            remove_media (std::string const &path, std::string guid);

            bool operator() ();
        };

        struct parse_sync_path : public file::task::worker
        {
            std::vector< std::map<std::string, std::string> > &sync_data;
            parse_sync_path (std::string const &path, std::vector< std::map<std::string, std::string> > &data);

            bool operator() ();
        };
    }
}

#endif
