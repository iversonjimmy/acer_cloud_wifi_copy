#ifndef EVENT_HPP__
#define EVENT_HPP__

#include <sstream>
#include <string>
#include <vector>

#include "thread.hpp"

namespace task
{
    struct worker;
};

namespace event
{
    class processor;

    enum event_type
    {
        job_type,
        wake_type,
        sleep_type,
        warn_type,
        panic_type,
        login_type,
        logout_type,
        syncing_type,
        synced_type,
        command_type,
        num_event_type
    };

    struct base 
    {
        struct handler { virtual ~handler () {} };

        event_type type;
        base (event_type t) : type (t) {}
        virtual ~base () {}
    };

    struct queue
    {
        processor *events;
        queue (processor *p);
        void operator() (event::base const *e);
    };

    struct listener
    {
        base::handler *handler;

        listener ();
        listener (base::handler *h);

        template <typename CastEvent>
        bool invoke (event::queue queue, event::base::handler *basehandler, event::base const *baseevent) const
        {
            CastEvent const *event = static_cast<CastEvent const *> (baseevent);
            CastEvent::handler *handler = static_cast<CastEvent::handler *> (basehandler);

            return handler && event && ((*handler) (queue, *event));
        }

        bool operator() (event::queue queue, event::base const *event) const;
    };

    struct source
    {
        virtual void forward (event::processor &processor) = 0;
    };

    struct job : public event::base 
    {
        struct handler : public event::base::handler 
        { 
            virtual bool operator() (event::queue queue, job const &event) = 0; 
        };

        job () : event::base (job_type) {}
        job (task::worker *task) : event::base (job_type) { work.push_back (task); }

        std::vector<task::worker *> work;
    };

    struct wake : public event::base
    {
        struct handler : public event::base::handler 
        { 
            virtual bool operator() (event::queue queue, wake const &event) = 0; 
        };

        wake () : event::base (wake_type) {}
    };

    struct sleep : public event::base 
    {
        struct handler : public event::base::handler 
        { 
            virtual bool operator() (event::queue queue, sleep const &event) = 0; 
        };

        sleep () : event::base (sleep_type) {}
    };

    struct warn : public event::base 
    {
        struct handler : public event::base::handler 
        { 
            virtual bool operator() (event::queue queue, warn const &event) = 0; 
        };

        warn (std::string msg = "") : event::base (warn_type), message (msg) {}

        std::stringstream message;
    };

    struct panic : public event::base 
    {
        struct handler : public event::base::handler 
        { 
            virtual bool operator() (event::queue queue, panic const &event) = 0; 
        };

        panic (std::string msg = "") : event::base (panic_type), message (msg) {}

        std::stringstream message;
    };

    struct login : public event::base 
    {
        struct handler : public event::base::handler 
        { 
            virtual bool operator() (event::queue queue, login const &event) = 0; 
        };

        login () : event::base (login_type) {}
    };

    struct logout : public event::base 
    {
        struct handler : public event::base::handler 
        { 
            virtual bool operator() (event::queue queue, logout const &event) = 0; 
        };

        logout () : event::base (logout_type) {}
    };

    struct syncing : public event::base 
    {
        struct handler : public event::base::handler 
        { 
            virtual bool operator() (event::queue queue, syncing const &event) = 0; 
        };

        syncing () : event::base (syncing_type) {}
    };

    struct synced : public event::base 
    {
        struct handler : public event::base::handler 
        { 
            virtual bool operator() (event::queue queue, synced const &event) = 0; 
        };

        synced () : event::base (synced_type) {}
    };

    struct command : public event::base
    {
        struct handler : public event::base::handler 
        { 
            virtual bool operator() (event::queue queue, command const &event) = 0; 
        };

        command (std::string const &cmd, std::vector<std::string> const &args) : 
            event::base (command_type), name (cmd), arguments (args) {}

        std::string name;
        std::vector<std::string> arguments;
    };

    class processor
    {
        public:
            processor ();
            ~processor ();

            void enqueue (event::base const *event);
            void listen (event_type type, event::base::handler *handle);
            void default_handler (event_type type, event::base::handler *handle);
            void dispatch ();

            bool empty () const;

        private:
            thread::queue<event::base const *> events_;
            thread::queue<listener> listeners_[num_event_type];
            listener                defaults_[num_event_type];
    };
}

#endif
