//
// Note Prototype main
//
//               Copyright Acer Cloud Technologies 2013
//

#include <iostream>
#include <log.h>

#include "db.hpp"
#include "sync.hpp"
#include "file.hpp"
#include "task.hpp"
#include "event.hpp"
#include "thread.hpp"
#include "command.hpp"
#include "conv.hpp"

#include "db-tasks.hpp"
#include "sync-tasks.hpp"
#include "file-tasks.hpp"

bool is_active = false;

struct exit_on_sleep : public event::sleep::handler
{
    bool operator() (event::queue enqueue, event::sleep const &event) 
    { 
        is_active = false;
        return true; 
    }
};

struct exit_on_panic : public event::panic::handler
{
    bool operator() (event::queue enqueue, event::panic const &event) 
    { 
        std::cout << "[!!] panic: " << event.message.str() << std::endl;;

        is_active = false;
        return true; 
    }
};

struct print_on_warn : public event::warn::handler
{
    bool operator() (event::queue enqueue, event::warn const &event) 
    { 
        std::cout << ">> warning: " << event.message.str() << std::endl;;
        
        return true; 
    }
};
struct enqueue_jobs : public event::job::handler
{
    task::processor &processor;
    enqueue_jobs (task::processor &proc) : processor (proc) {}

    bool operator() (event::queue enqueue, event::job const &event) 
    { 
        std::vector<task::worker *>::const_iterator iter = event.work.begin ();
        std::vector<task::worker *>::const_iterator end = event.work.end ();

        for (; iter != end; ++iter)
            processor.enqueue (*iter);

        return true;
    }
};

struct setup_database_on_wake : public event::wake::handler
{
    db::object &database;
    setup_database_on_wake (db::object &db) : database (db) {}

    bool operator() (event::queue enqueue, event::wake const &event)
    {
        event::job *job = new event::job;
        job->work.push_back (new db::task::open_database (database));
        job->work.push_back (new db::task::create_note_table (database));
        enqueue (job);

        return true;
    }
};

struct setup_ccd_on_wake : public event::wake::handler
{
    sync::info &sync_info;
    setup_ccd_on_wake (sync::info &info) : sync_info (info) {}

    bool operator() (event::queue enqueue, event::wake const &event)
    {
        event::job *job = new event::job;
        job->work.push_back (new sync::task::update_app_state (sync_info));
        job->work.push_back (new sync::task::read_sys_state (sync_info));
        job->work.push_back (new sync::task::update_sync_settings (sync_info));
        job->work.push_back (new sync::task::read_sync_state (sync_info));
        enqueue (job);

        return true;
    }
};

struct index_path_on_synced : public event::synced::handler
{
    db::object &database;
    std::string const &sync_path;

    index_path_on_synced (db::object &db, std::string const &path) : 
        database (db), sync_path (path) {}

    bool operator() (event::queue enqueue, event::synced const &event)
    {
        event::job *job = new event::job;
        db::task::merge_sync_data *merge = new db::task::merge_sync_data (database);
        file::task::parse_sync_path *parse = new file::task::parse_sync_path (sync_path, merge->sync_data);

        job->work.push_back (parse);
        job->work.push_back (merge);
        enqueue (job);

        return true;
    }
};

struct command_line_thread : public thread::thread
{
    event::processor &processor;
    command_line_thread (event::processor &proc) : 
        thread ("command line front-end", true), 
        processor (proc) {}

    void run ()
    {
        std::string command;

        while (is_active &&
               std::cout << "> " && 
               std::cin >> command)
        {
            std::string argument, remaining;
            std::vector<std::string> arguments;
            std::getline (std::cin, remaining);
            std::stringstream stream (remaining);

            while (std::getline (stream, argument, (stream.peek () == '"')? (char) stream.get () : ' '))
                if (!argument.empty()) arguments.push_back (argument);

            processor.enqueue (new event::command (command, arguments));
        }
    }
};


struct ccd_notification_thread : public thread::thread
{
    sync::event::source source;
    event::processor &processor;

    ccd_notification_thread (event::processor &proc) : 
        thread ("sync notifications from ccd", true), 
        processor (proc) {}

    void run ()
    {
        while (is_active)
            source.forward (processor);
    }
};

int main (int argc, char **argv)
{
    LOGInit (argv[0], 0);
    VPL_Init();

    is_active = true;
    
    db::object database;
    sync::info sync_info;

    task::processor tasks;
    event::processor events;

    events.default_handler (event::sleep_type, new exit_on_sleep);
    events.default_handler (event::panic_type, new exit_on_panic);
    events.default_handler (event::warn_type, new print_on_warn);
    
    events.listen (event::wake_type, new setup_database_on_wake (database));
    events.listen (event::wake_type, new setup_ccd_on_wake (sync_info));
    events.listen (event::synced_type, new index_path_on_synced (database, sync_info.path));
    events.listen (event::job_type, new enqueue_jobs (tasks));

    events.listen (event::command_type, new command::user_interface (database, sync_info.path));

    command_line_thread input (events);
    ccd_notification_thread notify (events);

    events.enqueue (new event::wake);
    events.enqueue (new event::synced);

    while (is_active)
    {
        events.dispatch ();

        if (!tasks.execute ())
        {
            events.enqueue (new event::warn (tasks.log.errors.str ()));
            tasks.log.errors.clear();
        }
    }

    VPL_Quit();

    return 0;
}
