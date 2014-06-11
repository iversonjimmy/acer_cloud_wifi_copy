#ifndef SYNC_HPP__
#define SYNC_HPP__

#include <ccdi.hpp>
#include "event.hpp"

namespace task
{
    struct worker;
};

namespace sync
{
    struct info
    {
        std::string path;
        uint64_t user_id, device_id;
    };
    
    namespace event
    {
        class source : public ::event::source
        {
            public:
                source (int32_t timeout = -1);
                ~source ();

                operator bool () const;

                void forward (::event::processor &processor);

            private:
                CCDIError error_;
                uint64_t handle_;
                int32_t timeout_;
        };
    }
}

#endif
