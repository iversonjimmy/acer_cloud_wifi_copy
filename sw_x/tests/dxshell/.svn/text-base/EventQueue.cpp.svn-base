#include "EventQueue.hpp"
#include <log.h>
#include <vplu_format.h>

EventQueue::EventQueue() : queueHandle(0)
{
    int rv = 0;

    ccd::EventsCreateQueueInput req;
    ccd::EventsCreateQueueOutput res;
    rv = CCDIEventsCreateQueue(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIEventsCreateQueue failed: %d", rv);
    }
    else {
        queueHandle = res.queue_handle();
        LOG_ALWAYS("Event queue handle "FMTu64" created.", queueHandle);
    }
}

EventQueue::~EventQueue()
{
    int rv = 0;

    ccd::EventsDestroyQueueInput req;
    req.set_queue_handle(queueHandle);
    rv = CCDIEventsDestroyQueue(req);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIEventsDestroyQueue failed: %d", rv);
    }
    else{
        LOG_ALWAYS("Event queue handle "FMTu64" destroyed.", queueHandle);
    }

    queueHandle = 0;  // hopefully invalid value
}

int EventQueue::visit(u32 maxCount, s32 timeout,
                      int (*visitor)(const ccd::CcdiEvent &event, void *_ctx),
                      void *_ctx)
{
    int rv = 0;

    ccd::EventsDequeueInput req;
    ccd::EventsDequeueOutput res;
    req.set_queue_handle(queueHandle);
    if (maxCount > 0) {
        req.set_max_count(maxCount);
    }
    if (timeout > 0) {
        req.set_timeout(timeout);
    }
    rv = CCDIEventsDequeue(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIEventsDequeue failed: %d", rv);
        goto end;
    }

    LOG_INFO("%d events found", res.events_size());
    for (int i = 0; i < res.events_size(); i++) {
        visitor(res.events(i), _ctx);
    }

 end:
    return rv;
}

int EventQueue::dump(u32 maxCount, s32 timeout)
{
    int rv = 0;

    ccd::EventsDequeueInput req;
    ccd::EventsDequeueOutput res;
    req.set_queue_handle(queueHandle);
    if (maxCount > 0) {
        req.set_max_count(maxCount);
    }
    if (timeout > 0) {
        req.set_timeout(timeout);
    }
    rv = CCDIEventsDequeue(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIEventsDequeue failed: %d", rv);
        goto end;
    }

    LOG_INFO("%d events found", res.events_size());
    for (int i = 0; i < res.events_size(); i++) {
        const ccd::CcdiEvent &event = res.events(i);
        LOG_INFO("%s", event.DebugString().c_str());
    }

 end:
    return rv;
}
