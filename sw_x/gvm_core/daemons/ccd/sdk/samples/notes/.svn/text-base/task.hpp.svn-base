#ifndef TASK_HPP__
#define TASK_HPP__

#include <string>
#include <sstream>
#include <deque>

namespace task
{
    struct worker
    {
        std::string name;
        std::stringstream error;
        virtual ~worker () {}
        virtual bool operator() () = 0;
    };

    struct log
    {
        std::stringstream info;
        std::stringstream warnings;
        std::stringstream errors;
    };

    class processor
    {
        public:
            processor ();
            ~processor ();

            void enqueue (task::worker *task);
            bool execute ();

        public:
            task::log log;

        private:
            std::deque<task::worker *>  workers_;
    };
}

#endif
