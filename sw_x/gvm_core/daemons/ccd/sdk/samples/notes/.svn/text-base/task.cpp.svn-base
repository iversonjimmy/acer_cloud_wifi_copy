
#include "task.hpp"

namespace task
{
    processor::processor () {}

    processor::~processor ()
    {
        std::deque<task::worker *>::const_iterator iter = workers_.begin ();
        std::deque<task::worker *>::const_iterator end = workers_.end ();

        for (; iter != end; ++iter)
            delete *iter;
    }

    bool processor::execute ()
    {
        bool success = true;

        while (success && !workers_.empty ())
        {
            std::auto_ptr<task::worker> task (workers_.front()); 

            success = (*task.get ()) ();

            if (!success)
                log.errors 
                    << (task->name.size()? task->name : "task") << " : " 
                    << task->error.str() << "; ";
            
            workers_.pop_front();
        }
        
        return success;
    }

    void processor::enqueue (worker *task)
    {
        workers_.push_back (task);
    }
}
