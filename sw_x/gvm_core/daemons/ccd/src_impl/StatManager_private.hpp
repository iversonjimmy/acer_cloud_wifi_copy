#ifndef __STAT_MANAGER_PRIVATE_HPP__
#define __STAT_MANAGER_PRIVATE_HPP__

#include <vpl_types.h>
#include <vpl_th.h>
#include <vpl_thread.h>
#include "ccdi_rpc.pb.h"

#include <map>

class StatManager {

public:
    StatManager();
    ~StatManager();

    int Start(u64 userId);
    int Stop();
    int AddEventBegin(const std::string& app_id, const std::string& event_id);
    int AddEventEnd(const std::string& app_id, const std::string& event_id);
    int NotifyConnectionChange();

private:
    /// IMPORTANT: To avoid deadlock, you must always acquire StatManager::mutex *before*
    ///   the CCD Cache lock.  Note that Cache*() functions implicitly acquire the
    ///   Cache lock within their implementations.
    VPLMutex_t mutex;
    VPLCond_t cond;
    bool thread_spawned;
    bool exit_thread;

    u64 userId;
    std::string accountId;
    
    int event_count_limit;
    u32 report_every_n_seconds;
    u32 retry_every_n_seconds;
    u32 fast_retry_every_n_seconds;
    u32 report_batch_size;
    u32 retry_count;
    std::string redirect_url;

    int spawnStatManagerThread();
    int signalStatManagerThreadToStop();
    static VPLTHREAD_FN_DECL statManagerThreadMain(void *arg);
    void statManagerThreadMain();
    int reportToServer(const std::list<ccd::CachedStatEvent> &events);
    int updateReportedAndWaitList();

    /// Maps eventId -> appId -> unreported CachedStatEvent.
    std::map<std::string, std::map<std::string, ccd::CachedStatEvent> > statEventMap;

    /// Maps eventId -> reported CachedStatEvent.
    /// - CachedStatEvent::start_time_ms records the last time that we reported to the infra.
    ///   This time is from VPLTime_ToMillisec(VPLTime_GetTime()).
    /// - CachedStatEvent::event_id records the event_id.
    /// - Other fields are not populated.
    std::map<std::string, ccd::CachedStatEvent> reportedEventMap;


};

#endif  //  __STAT_MANAGER_PRIVATE_HPP__
