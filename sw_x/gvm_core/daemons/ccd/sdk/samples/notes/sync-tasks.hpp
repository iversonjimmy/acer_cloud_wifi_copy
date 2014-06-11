#ifndef SYNC_TASKS_HPP__
#define SYNC_TASKS_HPP__

#include "task.hpp"

namespace sync
{
    struct info;

    namespace task
    {
        struct worker : public ::task::worker
        {
            sync::info &sync_info;
            worker (sync::info &sync_info);
        };

        struct update_app_state : public sync::task::worker
        {
            update_app_state (sync::info &sync_info);
            bool operator() ();
        };

        struct read_sys_state : public sync::task::worker
        {
            read_sys_state (sync::info &sync_info);
            bool operator() ();
        };

        struct update_sync_settings : public sync::task::worker
        {
            update_sync_settings (sync::info &sync_info);
            bool operator() ();
        };

        struct read_sync_state : public sync::task::worker
        {
            read_sync_state (sync::info &sync_info);
            bool operator() ();
        };
    }
}

#endif
