#ifndef COMMAND_HPP__
#define COMMAND_HPP__

#include <sstream>
#include <string>
#include <vector>
#include <map>

#include "event.hpp"

namespace db
{
    struct object;
}

namespace command
{
    struct user_interface : public event::command::handler
    {
        db::object &database;
        std::string const &sync_path;

        std::map<std::string, void (user_interface::*)(event::queue, std::vector<std::string> const &)> table;

        user_interface (db::object &db, std::string const &path);

        bool operator() (event::queue enqueue, event::command const &event);

        void enqueue_sleep (event::queue enqueue, std::vector<std::string> const &args);
        void enqueue_sync (event::queue enqueue, std::vector<std::string> const &args);
        void enqueue_dump_notes (event::queue enqueue, std::vector<std::string> const &args);
        void enqueue_find_note (event::queue enqueue, std::vector<std::string> const &args);
        void enqueue_create_note (event::queue enqueue, std::vector<std::string> const &args);
        void enqueue_update_note (event::queue enqueue, std::vector<std::string> const &args);
        void enqueue_delete_note (event::queue enqueue, std::vector<std::string> const &args);
        void enqueue_attach_media (event::queue enqueue, std::vector<std::string> const &args);
        void enqueue_remove_media (event::queue enqueue, std::vector<std::string> const &args);

        bool expect_exactly (size_t const N, std::vector<std::string> const &args, std::stringstream &msg);
        bool expect_at_least (size_t const N, std::vector<std::string> const &args, std::stringstream &msg);
    };
}

#endif
