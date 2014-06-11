#ifndef _EVENTQUEUE_HPP_
#define _EVENTQUEUE_HPP_

#include <ccdi.hpp>
#include <vplu_types.h>

class EventQueue {
public:
    EventQueue();
    ~EventQueue();
    int visit(u32 maxCount, s32 timeout,
              int (*visitor)(const ccd::CcdiEvent &event, void *_ctx),
              void *_ctx);
    int dump(u32 maxCount, s32 timeout);
    u64 getQueueHandle(){ return queueHandle; };
private:
    u64 queueHandle;
};

#endif // _EVENTQUEUE_HPP_
