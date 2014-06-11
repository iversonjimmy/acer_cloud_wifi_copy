
#include "event.hpp"

namespace event
{
    queue::queue (processor *p) : 
        events (p) {}

    void queue::operator() (event::base const *e) 
    { 
        events->enqueue (e); 
    }

    listener::listener () : 
        handler (0) {}

    listener::listener (event::base::handler *h) : 
        handler (h) {}

    bool listener::operator() (event::queue queue, event::base const *event) const
    {
        switch (event->type)
        {
            case job_type:      return invoke <job> (queue, handler, event);
            case wake_type:     return invoke <wake> (queue, handler, event);
            case sleep_type:    return invoke <sleep> (queue, handler, event);
            case warn_type:     return invoke <warn> (queue, handler, event);
            case panic_type:    return invoke <panic> (queue, handler, event);
            case login_type:    return invoke <login> (queue, handler, event);
            case logout_type:   return invoke <logout> (queue, handler, event);
            case syncing_type:  return invoke <syncing> (queue, handler, event);
            case synced_type:   return invoke <synced> (queue, handler, event);
            case command_type:  return invoke <command> (queue, handler, event);
            default:            return false;
        }
    }

    processor::processor () 
    {}

    processor::~processor ()
    {
        {
            thread::lock<thread::mutex> lock (events_.get_mutex ());
            thread::queue<event::base const *>::const_iterator iter = events_.begin ();
            thread::queue<event::base const *>::const_iterator end = events_.end ();

            for (; iter != end; ++iter)
                delete *iter;
        }

        for (int i=0; i < num_event_type; ++i)
        {
            thread::queue<listener> &list = listeners_[i];

            thread::lock<thread::mutex> lock (list.get_mutex ());
            thread::queue<listener>::const_iterator iter = list.begin ();
            thread::queue<listener>::const_iterator end = list.end ();

            for (; iter != end; ++iter)
                delete iter->handler;
        }
        
        for (int i=0; i < num_event_type; ++i)
            delete defaults_[i].handler;
    }

    void processor::dispatch ()
    {
        while (!events_.empty ())
        {
            std::auto_ptr<event::base const> event (events_.pop ());

            bool proceed = true;

            {
                thread::queue<listener> &list = listeners_[event->type];

                thread::lock <thread::mutex> lock (list.get_mutex ());
                thread::queue<listener>::const_iterator iter = list.begin ();
                thread::queue<listener>::const_iterator end = list.end ();

                for (; proceed && iter != end; ++iter)
                    proceed = (*iter) (event::queue (this), event.get ());
            }

            proceed = (defaults_[event->type]) (event::queue (this), event.get ());
        }
    }

    void processor::enqueue (event::base const *event)
    {
        events_.push (event);
    }

    void processor::listen (event_type type, event::base::handler *handle)
    {
        listeners_[type].push (handle);
    }

    void processor::default_handler (event_type type, event::base::handler *handle)
    {
        defaults_[type].handler = handle;
    }
            
    bool processor::empty () const
    {
        return events_.empty();
    }
}
