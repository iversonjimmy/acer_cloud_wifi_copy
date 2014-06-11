#ifndef THREAD_HPP__
#define THREAD_HPP__

#include <vpl_th.h>
#include <deque>

namespace thread
{
    namespace detail
    {
        class noncopyable
        {
            public:
                noncopyable () {}
            private:
                noncopyable (noncopyable const &);
                void operator= (noncopyable const &);
        };
    }

    void sleep (int milliseconds);

    struct thread : public VPLThread_t, detail::noncopyable
    {
        std::string name;
        int error;
        bool detach;

        thread (std::string str = "thread", bool detachable = false);
        ~thread ();

        operator bool () const;

        virtual void run () = 0;
    };

    struct mutex : public VPLMutex_t, detail::noncopyable
    {
        std::string name;
        int error;

        mutex (std::string str = "mutex");
        ~mutex ();

        operator bool () const;

        bool lock ();
        bool unlock ();
    };

    template <typename Mutex>
    struct lock : detail::noncopyable
    {
        Mutex &mutex;
        lock (Mutex &m) : mutex (m) { mutex.lock (); }
        ~lock () { mutex.unlock (); }
    };

    template <typename Type>
    class queue
    {
        public:
            typedef typename std::deque<Type>::const_iterator const_iterator;

            mutex &get_mutex () const
            {
                return mutex_;
            }

            const_iterator begin () const
            {
                return data_.begin ();
            }

            const_iterator end () const
            {
                return data_.end ();
            }

            size_t size () const 
            { 
                lock<mutex> lck (mutex_);
                return data_.size (); 
            }

            bool empty () const 
            { 
                lock<mutex> lck (mutex_);
                return data_.empty (); 
            }

            Type pop ()
            {
                lock<mutex> lck (mutex_);
                Type front = data_.front (); 
                data_.pop_front ();
                return front;
            }

            void push (Type value)
            {
                lock<mutex> lck (mutex_);
                data_.push_back (value);
            }

        private:
            mutable mutex mutex_;
            std::deque<Type> data_;

    };
}

#endif
