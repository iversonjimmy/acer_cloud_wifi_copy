
#include <log.h>

#include "sync.hpp"

namespace sync
{
    namespace event
    {
        source::source (int32_t timeout) : 
            handle_ (0), timeout_ (timeout)
        {
            ccd::EventsCreateQueueOutput response;
            ccd::EventsCreateQueueInput request;

            error_ = CCDIEventsCreateQueue (request, response);

            if (error_ == CCD_OK)
                handle_ = response.queue_handle ();
            else
                LOG_ERROR("[!!] could not create events queue (%d)", error_);
        }

        source::~source ()
        {
            if (handle_)
            {
                ccd::EventsDestroyQueueInput request;
                request.set_queue_handle (handle_);

                error_ = CCDIEventsDestroyQueue (request);

                if (error_ != CCD_OK)
                    LOG_ERROR("[!!] could not destroy events queue (%d)", error_);
            }
        }

        source::operator bool () const
        {
            return handle_ && error_ == CCD_OK;
        }
        
        void source::forward (::event::processor &processor)
        {
            ccd::EventsDequeueOutput response;
            ccd::EventsDequeueInput request;

            request.set_queue_handle (handle_);
            request.set_timeout (timeout_);

            error_ = CCDIEventsDequeue (request, response);
            bool proceed = error_ == CCD_OK;

            for (int i=0; proceed && i < response.events_size (); ++i)
            {
                ccd::CcdiEvent const &event = response.events (i);

                if (event.has_sync_feature_status_change())
                {
                    ccd::EventSyncFeatureStatusChange const &change = event.sync_feature_status_change();

                    if (change.feature() == ccd::SYNC_FEATURE_NOTES)
                    {
                        ccd::FeatureSyncStateSummary const &summary = change.status();

                        switch (summary.status())
                        {
                            case ccd::CCD_FEATURE_STATE_IN_SYNC:
                                processor.enqueue (new ::event::synced);
                                break;

                            case ccd::CCD_FEATURE_STATE_SYNCING:
                                LOG_INFO("[..] start syncing.");
                                break;

                            case ccd::CCD_FEATURE_STATE_OUT_OF_SYNC:
                                LOG_INFO("[??] out of sync.");
                                break;

                            default:
                                LOG_ERROR("[!!] unknown feature status change (%d)", summary.status());
                        }
                    }
                }
            }
        }
    }
}
