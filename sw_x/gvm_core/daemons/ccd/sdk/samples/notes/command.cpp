
#include "command.hpp"
#include "db-tasks.hpp"
#include "file-tasks.hpp"

namespace command 
{
    user_interface::user_interface (db::object &db, std::string const &path) : 
        database (db), sync_path (path)
    {
        using std::make_pair;

        table.insert (make_pair ("quit", &user_interface::enqueue_sleep));
        table.insert (make_pair ("sync", &user_interface::enqueue_sync));

        table.insert (make_pair ("dump-notes", &user_interface::enqueue_dump_notes));
        table.insert (make_pair ("find-note", &user_interface::enqueue_find_note));

        table.insert (make_pair ("create-note", &user_interface::enqueue_create_note));
        table.insert (make_pair ("update-note", &user_interface::enqueue_update_note));
        table.insert (make_pair ("delete-note", &user_interface::enqueue_delete_note));
        table.insert (make_pair ("attach-media", &user_interface::enqueue_attach_media));
        table.insert (make_pair ("remove-media", &user_interface::enqueue_remove_media));
    }

    bool user_interface::operator() (event::queue enqueue, event::command const &event) 
    { 
        bool handled = false;

        if (table.count (event.name))
            (this->*table[event.name]) (enqueue, event.arguments);
        else
            enqueue (new event::warn ("unable to find command: " + event.name));

        return !handled; 
    }

    void user_interface::enqueue_sleep (event::queue enqueue, std::vector<std::string> const &args)
    {
        enqueue (new event::sleep);
    }

    void user_interface::enqueue_sync (event::queue enqueue, std::vector<std::string> const &args)
    {
        enqueue (new event::synced);
    }

    void user_interface::enqueue_dump_notes (event::queue enqueue, std::vector<std::string> const &args)
    {
        event::base *event = 0;
        std::stringstream msg;

        if (expect_exactly (0, args, msg))
            event = new event::job (new db::task::dump_notes (database));
        else
            event = new event::warn (msg.str ());

        enqueue (event);
    }

    void user_interface::enqueue_find_note (event::queue enqueue, std::vector<std::string> const &args)
    {
        event::base *event = 0;
        std::stringstream msg;

        if (expect_exactly (2, args, msg))
        {
            std::string key (args[0]), value (args[1]);

            event = new event::job (new db::task::find_note (database, key, value));
        }
        else
            event = new event::warn (msg.str ());

        enqueue (event);
    }

    void user_interface::enqueue_create_note (event::queue enqueue, std::vector<std::string> const &args)
    {
        event::base *event = 0;
        std::stringstream msg;

        if (expect_at_least (3, args, msg))
        {
            std::vector<std::string> fields (args);

            event = new event::job (new file::task::create_note (sync_path, fields));
        }
        else
            event = new event::warn (msg.str ());

        enqueue (event);
        enqueue (new event::synced);
    }

    void user_interface::enqueue_update_note (event::queue enqueue, std::vector<std::string> const &args)
    {
        event::base *event = 0;
        std::stringstream msg;

        if (expect_at_least (2, args, msg))
        {
            std::string guid (args[0]);
            std::vector<std::string> fields (args.begin()+1, args.end());

            event = new event::job (new file::task::update_note (sync_path, guid, fields));
        }
        else
            event = new event::warn (msg.str ());

        enqueue (event);
        enqueue (new event::synced);
    }

    void user_interface::enqueue_delete_note (event::queue enqueue, std::vector<std::string> const &args)
    {
        event::base *event = 0;
        std::stringstream msg;

        if (expect_exactly (1, args, msg))
        {
            std::string guid (args[0]);

            event = new event::job (new file::task::delete_note (sync_path, guid));
        }
        else
            event = new event::warn (msg.str ());

        enqueue (event);
        enqueue (new event::synced);
    }

    void user_interface::enqueue_attach_media (event::queue enqueue, std::vector<std::string> const &args)
    {
        event::base *event = 0;
        std::stringstream msg;

        if (expect_at_least (2, args, msg))
        {
            std::string note_guid (args[0]), media_path (args[1]), 
                media_type ((args.size() > 2)? args[2] : "");

            event = new event::job (new file::task::attach_media (sync_path, note_guid, media_path, media_type));
        }
        else
            event = new event::warn (msg.str ());

        enqueue (event);
        enqueue (new event::synced);
    }

    void user_interface::enqueue_remove_media (event::queue enqueue, std::vector<std::string> const &args)
    {
        event::base *event = 0;
        std::stringstream msg;

        if (expect_exactly (1, args, msg))
        {
            std::string media_guid (args[0]);

            event = new event::job (new file::task::remove_media (sync_path, media_guid));
        }
        else
            event = new event::warn (msg.str ());

        enqueue (event);
        enqueue (new event::synced);
    }

    bool user_interface::expect_exactly (size_t const N, std::vector<std::string> const &args, std::stringstream &msg)
    {
        bool expected = args.size() == N;
        if (!expected) msg << "command: expected exactly " << N << " arguments (got " << args.size() << ")\n";
        return expected;
    }

    bool user_interface::expect_at_least (size_t const N, std::vector<std::string> const &args, std::stringstream &msg)
    {
        bool expected = args.size() >= N;
        if (!expected) msg << "command: expected at least " << N << " arguments (got " << args.size() << ")\n";
        return expected;
    }
}

