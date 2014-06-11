
#include <string>
#include <log.h>

#include "thread.hpp"

namespace thread
{
    void sleep (int milliseconds)
    {
        VPLThread_Sleep (milliseconds * 1000);
    }

    static VPLThread_return_t adapter (VPLThread_arg_t argument)
    {
        reinterpret_cast<thread *> (argument)->run ();
        return 0;
    }

    thread::thread (std::string str, bool detachable) :
        name (str), detach (detachable)
    {
        VPLThread_attr_t attributes;
        VPLThread_AttrInit (&attributes);
        VPLThread_AttrSetDetachState (&attributes, detach);

        bool proceed = (error = VPLThread_Create 
                (this, &adapter, VPL_AS_THREAD_FUNC_ARG (this), &attributes, name.c_str())) == VPL_OK;

        if (!proceed)
            LOG_ERROR("[!!] could not create thread %s (%d)", name.c_str(), error);
    }

    thread::~thread ()
    {
        if (!detach)
        {
            bool proceed = (error = VPLThread_Join (this, 0)) == VPL_OK;

            if (!proceed)
                LOG_ERROR("[!!] could not join thread %s (%d)", name.c_str(), error);
        }
    }

    thread::operator bool () const 
    { 
        return error == VPL_OK; 
    }

    mutex::mutex (std::string str) :
            name (str)
    { 
        bool proceed = (error = VPLMutex_Init (this)) == VPL_OK;

        if (!proceed)
            LOG_ERROR("[!!] could not create mutex %s (%d)", name.c_str(), error);
    }

    mutex::~mutex ()
    {
        bool proceed = (error = VPLMutex_Destroy (this)) == VPL_OK;

        if (!proceed)
            LOG_ERROR("[!!] could not destroy mutex %s (%d)", name.c_str(), error);
    }

    mutex::operator bool () const 
    { 
        return error == VPL_OK; 
    }

    bool mutex::lock () 
    {
        return (error = VPLMutex_Lock (this)) == VPL_OK; 
    }

    bool mutex::unlock () 
    { 
        return (error = VPLMutex_Unlock (this)) == VPL_OK; 
    }
}
