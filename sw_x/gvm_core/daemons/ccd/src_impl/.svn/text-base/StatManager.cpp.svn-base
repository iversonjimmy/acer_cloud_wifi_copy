#include "StatManager.hpp"

#include "cache.h"
#include "StatManager_private.hpp"
#include "virtual_device.hpp"
#include <vplex_http_util.hpp>
#include "config.h"
#include <vplex_math.h>
#include "JsonHelper.hpp"
#include <vplex_http2.hpp>

#include <gvm_errors.h>
#include <vplu_mutex_autolock.hpp>
#include <log.h>
#include <scopeguard.hpp>

#include <time.h>

#define STAT_MANAGER_RETRY_TIMER        3600    //seconds
#define STAT_MANAGER_RETRY_COUNT        2
#define STAT_MANAGER_FAST_RETRY_TIMER   900     //seconds
#define STAT_MANAGER_FAST_RETRY_COUNT   1
#define STAT_MANAGER_BATCH_SIZE         100
#define STAT_MANAGER_REPORT_TIMER       86400   //seconds

typedef std::map<std::string, ccd::CachedStatEvent> InnerMap;
typedef std::map<std::string, InnerMap> OuterMap;

StatManager::StatManager()
:   thread_spawned(false),
    exit_thread(false),
    userId(0),
    event_count_limit(100),
    report_every_n_seconds(STAT_MANAGER_REPORT_TIMER),
    retry_every_n_seconds(STAT_MANAGER_RETRY_TIMER),
    fast_retry_every_n_seconds(STAT_MANAGER_FAST_RETRY_TIMER),
    report_batch_size(STAT_MANAGER_BATCH_SIZE),
    retry_count(0)
{
    int err = VPLMutex_Init(&mutex);
    if (err) {
        LOG_ERROR("Failed to initialize mutex: %d", err);
    }
    err  = VPLCond_Init(&cond);
    if (err) {
        LOG_ERROR("Failed to initialize condvar: %d", err);
    }

    if(__ccdConfig.statMgrReportInterval > 0){
        report_every_n_seconds = __ccdConfig.statMgrReportInterval;
    }
    if(__ccdConfig.statMgrMaxEvent > 0){
        event_count_limit = __ccdConfig.statMgrMaxEvent;
    }
    LOG_INFO("report_every_n_seconds: %d, event_count_limit: %d",
                report_every_n_seconds, event_count_limit);

    if(retry_every_n_seconds > report_every_n_seconds){
        // This is currently expected during tests (we reduce the report time, but
        // can't change the retry time).
        // As a result, we won't test the retry logic.
        LOG_INFO("retry time %d (sec) is larger than report time %d (sec)",
                    retry_every_n_seconds, report_every_n_seconds);
    }
}

StatManager::~StatManager()
{
    int err = VPLMutex_Destroy(&mutex);
    if (err) {
        LOG_ERROR("Failed to destroy mutex: %d", err);
    }
    err = VPLCond_Destroy(&cond);
    if (err) {
        LOG_ERROR("Failed to destroy condvar: %d", err);
    }

}

int StatManager::Start(u64 userId)
{
    int rv = CCD_OK;

    MutexAutoLock lock(&mutex);

    if (thread_spawned) {
        LOG_ERROR("StatManager obj already started");
        return CCD_ERROR_ALREADY_INIT;
    }

    this->userId = userId;

    //load reportedEventMap from CCD Storage
    {
        CacheAutoLock autoLock;
        int err = autoLock.LockForRead();
        if (err < 0) {
            LOG_ERROR("Failed to obtain lock, %d", err);
            rv = err;
            goto end;
        }

        CachePlayer* user = cache_getUserByUserId(userId);
        if (user == NULL) {
            LOG_ERROR("userId "FMT_VPLUser_Id_t" not signed in", userId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto end;
        }

        accountId = user->_cachedData.summary().account_id();

        LOG_INFO("stat_event_list_size: %d", user->_cachedData.details().stat_event_list_size());

        // Load reported events from the CCD Cache.
        for (int i = 0; i < user->_cachedData.details().stat_event_list_size(); i++) {
            const ccd::CachedStatEvent& curr =
                user->_cachedData.details().stat_event_list(i);

            reportedEventMap[curr.event_id()] = curr;
            LOG_INFO("load %s,"FMTu64, curr.event_id().c_str(), curr.start_time_ms());
        }

        // Load unreported events from the CCD Cache.
        for (int i = 0; i < user->_cachedData.details().stat_event_wait_list_size(); i++) {
            const ccd::CachedStatEvent& curr =
                user->_cachedData.details().stat_event_wait_list(i);

            statEventMap[curr.event_id()][curr.app_id()] = curr;
        }
        LOG_INFO("stat_event_wait_list_size: %d", user->_cachedData.details().stat_event_wait_list_size());
        user->_cachedData.mutable_details()->clear_stat_event_wait_list();
    }

    rv = spawnStatManagerThread();
    if (rv) {
        LOG_ERROR("Failed to spawn StatManager thread: %d", rv);
        goto end;
    }

end:
    return rv;
}

// TODO: return value is always CCD_OK; consider changing return type to void.
int StatManager::updateReportedAndWaitList()
{
    MutexAutoLock lock(&mutex);

    //If exit_thread flag is set, the Stop() function should already save the information
    if (exit_thread) {
        return CCD_OK;
    }

    //Write reportedEventMap and statEventMap to CCD Storage
    do{
        CacheAutoLock autoLock;
        int err = autoLock.LockForWrite();
        if (err < 0) {
            LOG_ERROR("Failed to obtain lock");
            break;
        }

        CachePlayer* user = cache_getUserByUserId(userId);
        if (user == NULL) {
            LOG_ERROR("userId "FMT_VPLUser_Id_t" not signed in", userId);
            break;
        }

        // Store reported events to the CCD Cache.
        std::map<std::string, ccd::CachedStatEvent>::iterator it;
        for (it=reportedEventMap.begin(); it!=reportedEventMap.end(); it++) {
            ccd::CachedStatEvent &event_reported = user->getOrCreateStatEvent((it->second).event_id());
            LOG_INFO("save %s,"FMTu64, (it->second).event_id().c_str(), (it->second).start_time_ms());
            event_reported.set_start_time_ms((it->second).start_time_ms());
        }

        LOG_INFO("stat_event_list_size: %d", user->_cachedData.details().stat_event_list_size());

        // Store unreported events to the CCD Cache.
        {
            user->_cachedData.mutable_details()->clear_stat_event_wait_list();
            OuterMap& outerMap = statEventMap;
            for(OuterMap::iterator it = outerMap.begin(); it != outerMap.end(); it++){
                InnerMap& innerMap = it->second;
                for(InnerMap::iterator it2 = innerMap.begin(); it2 != innerMap.end(); it2++) {
                    const ccd::CachedStatEvent &event = it2->second;
                    ccd::CachedStatEvent* newData = user->_cachedData.mutable_details()->add_stat_event_wait_list();
                    *newData = event;
                }
            }
            LOG_INFO("stat_event_wait_list_size: %d", user->_cachedData.details().stat_event_wait_list_size());
        }

        //FIXME: we can optimize the write if we check the data is different or not
        err = user->writeCachedData(false);
        if(err < 0){
            LOG_ERROR("writeCachedData failed, %d", err);
            break;
        }
    }while(0);

    return CCD_OK;
}

int StatManager::Stop()
{
    int rv = CCD_OK;

    MutexAutoLock lock(&mutex);
    LOG_INFO("enter %s", __func__);

    updateReportedAndWaitList();
    statEventMap.clear();

    {
        rv = signalStatManagerThreadToStop();
        if (rv) {
            LOG_ERROR("Failed to signal thread to stop: %d", rv);
            goto end;
        }
    }

end:
    return rv;

}

int StatManager::AddEventBegin(const std::string& app_id, const std::string& event_id)
{
    int rv = CCD_OK;

    MutexAutoLock lock(&mutex);


    {
        ccd::CachedStatEvent &event = statEventMap[event_id][app_id];
        if(event.has_event_id()){
            event.set_event_count(event.event_count()+1);
            event.set_end_time_ms(VPLTime_ToMillisec(VPLTime_GetTime()));
            LOG_INFO("event count increased: %d, %s, %s", event.event_count(), event.app_id().c_str(), event.event_id().c_str());
        }else{
            event.set_app_id(app_id);
            event.set_event_id(event_id);
            event.set_start_time_ms(VPLTime_ToMillisec(VPLTime_GetTime()));
            event.set_end_time_ms(VPLTime_ToMillisec(VPLTime_GetTime()));
            event.set_event_count(1);
            LOG_INFO("New event added: %d, %s, %s", event.event_count(), event.app_id().c_str(), event.event_id().c_str());
        }
    }

    //Add initial random reportedEventMap value if not exist
    {
        std::map<std::string, ccd::CachedStatEvent>::iterator it;
        it = reportedEventMap.find(event_id);
        if(it == reportedEventMap.end()){
            VPLTime_t last_report_time = VPLTime_GetTime();
            LOG_INFO("    Current time(ms): "FMTu64, VPLTime_ToMillisec(last_report_time));
            if (report_every_n_seconds != 0) {
                // Subtract a random time in the range [0, report_every_n_seconds).
                last_report_time -= VPLTime_FromSec(VPLMath_Rand() % report_every_n_seconds);
            }
            LOG_INFO("Last report time(ms): "FMTu64, VPLTime_ToMillisec(last_report_time));
            reportedEventMap[event_id].set_event_id(event_id);
            reportedEventMap[event_id].set_start_time_ms(VPLTime_ToMillisec(last_report_time));
        }
    }

    updateReportedAndWaitList();

    {
        int err = VPLCond_Signal(&cond);
        if (err) {
            LOG_ERROR("Unexpected error signaling condvar: %d", err);
            // don't override value of "rv"
        }
    }

    return rv;
}

int StatManager::AddEventEnd(const std::string& app_id, const std::string& event_id)
{
    int rv = CCD_OK;

    MutexAutoLock lock(&mutex);


    return rv;
}

int StatManager::NotifyConnectionChange()
{
    LOG_INFO("Notification received");

    MutexAutoLock lock(&mutex);

    int rv = VPLCond_Signal(&cond);
    if (rv) {
        LOG_ERROR("Failed to signal condvar: %d", rv);
    }
    return rv;
}

int StatManager::spawnStatManagerThread()
{
    int rv = CCD_OK;
    MutexAutoLock lock(&mutex);

    if (!thread_spawned) {
        rv = Util_SpawnThread(statManagerThreadMain, (void*)this,
                UTIL_DEFAULT_THREAD_STACK_SIZE, VPL_FALSE, NULL);
        if (rv != 0) {
            LOG_ERROR("Failed to spawn statManager thread: %d", rv);
            return rv;
        }
        thread_spawned = true;
    }

    return rv;
}

int StatManager::signalStatManagerThreadToStop()
{
    // Stop() expects us to return 0 when thread_spawned is false.
    int rv = 0;

    MutexAutoLock lock(&mutex);
    LOG_INFO("enter %s", __func__);

    if (thread_spawned) {
        exit_thread = true;

        rv = VPLCond_Signal(&cond);
        if (rv != VPL_OK) {
            LOG_ERROR("Unexpected error signaling condvar: %d", rv);
            return rv;
        }
        LOG_INFO("signaled StatManager thread to stop");
    }

    return rv;
}

VPLTHREAD_FN_DECL StatManager::statManagerThreadMain(void *arg)
{
    StatManager *sm = (StatManager*)arg;
    if (sm) {
        sm->statManagerThreadMain();

        delete sm;
        LOG_DEBUG("StatManager obj destroyed");
    }
    return VPLTHREAD_RETURN_VALUE;
}

static int compare_event_time(const ccd::CachedStatEvent &event, const ccd::CachedStatEvent &event_reported, u64 diff_ms)
{
    if(event.start_time_ms() > event_reported.start_time_ms()+diff_ms)
        return 1;
    else if(event.start_time_ms() == event_reported.start_time_ms()+diff_ms)
        return 0;
    else
        return -1;
}

static int compare_event_time(const ccd::CachedStatEvent &event_reported, u64 diff_ms)
{
    ccd::CachedStatEvent event;
    event.set_start_time_ms(VPLTime_ToMillisec(VPLTime_GetTime()));
    return compare_event_time(event, event_reported, diff_ms);
}

int StatManager::reportToServer(const std::list<ccd::CachedStatEvent> &events)
{
    int rv = 0;

    std::string event_info;
    std::ostringstream oss;

    oss << "{";
    oss << "\"userId\":\"" << userId << "\",";
    oss << "\"deviceId\":\"" << VirtualDevice_GetDeviceId() << "\",";
    oss << "\"events\": [";
    bool first = true;
    std::list<ccd::CachedStatEvent>::const_iterator it;
    for(it=events.begin(); it!= events.end(); it++){
        const ccd::CachedStatEvent &event = *it;
        if(first){
            first = false;
        }else{
            oss << ",";
        }
        oss << "{ \"appId\":\"" << event.app_id() << "\",";
        oss << "\"eventId\":\"" << event.event_id() << "\",";
        oss << "\"startTime\":\"" << event.start_time_ms() << "\",";
        oss << "\"endTime\":\"" << event.end_time_ms() << "\",";
        oss << "\"eventCount\":\"" << event.event_count() << "\",";
        if(event.has_limit_reached() && event.limit_reached()){
            oss << "\"limitReached\":\"" << "true" << "\",";
        }else{
            oss << "\"limitReached\":\"" << "false" << "\",";
        }
        oss << "\"attributes\":";
        oss << "{";
        //FIXME: use event_info
        oss << "\"info\":\"info\"";
        oss << "} }";
    }
    oss << "] }";

    event_info = VPLHttp_UrlEncoding(oss.str());

    oss.str("");
    oss << "storeEvents?";
    oss << "&serviceIdParam=OPS";
    oss << "&accountId=" << accountId;
    oss << "&eventInfo=" << event_info;

    std::string req = oss.str();

    //Get http info
    ccd::GetInfraHttpInfoOutput response;
    {
        ccd::GetInfraHttpInfoInput request;
        request.set_user_id(userId);
        request.set_service(ccd::INFRA_HTTP_SERVICE_OPS);
        request.set_secure(true);

        rv = CCDIGetInfraHttpInfo(request, response);
        if (rv < 0) {
            LOG_ERROR("CCDIGetInfraHttpInfo("FMTu64") failed: %d", userId, rv);
            return rv;
        }
    }

    //send http request to OPS
    VPLHttp2 handle;
    std::string opsResponse;
    {
        std::string url;

        oss.str("");
        if(redirect_url.empty()){
            oss << response.url_prefix() << "/ops/json/" << req;
        }else{
            oss << redirect_url << req;
        }

        url = oss.str();

        handle.SetUri(url);
        handle.SetTimeout(VPLTime_FromSec(30));
        handle.AddRequestHeader("sessionHandle", response.session_handle());
        handle.AddRequestHeader("serviceTicket", response.service_ticket());
        
        if (__ccdConfig.debugInfraHttp)
            handle.SetDebug(1);

        rv = handle.Post("", opsResponse);
        
    }

    //set to default value first, in case any error happen
    {
        redirect_url.clear();
        if(__ccdConfig.statMgrReportInterval > 0){
            report_every_n_seconds = __ccdConfig.statMgrReportInterval;
        }else{
            report_every_n_seconds = STAT_MANAGER_REPORT_TIMER;
        }
    }

    if(rv == VPL_OK && handle.GetStatusCode() == 200){
        //check return json from server
        std::string redirectUrl;
        u64 nextReportTime;
        cJSON2* json_response = cJSON2_Parse(opsResponse.c_str());
        if(json_response != NULL){
            ON_BLOCK_EXIT(cJSON2_Delete, json_response);    
            if (!JSON_getString(json_response, "redirectUrl", redirectUrl)) {
                if(!redirectUrl.empty()){
                    redirect_url = redirectUrl;
                    LOG_INFO("Found redirectUrl: %s", redirectUrl.c_str());
                }
            }
            if(!JSON_getInt64(json_response, "nextReportTime", nextReportTime)){
                if(nextReportTime > 0){
                    report_every_n_seconds = (u32)VPLTime_ToSec(VPLTime_FromMinutes(nextReportTime));
                    LOG_INFO("Found nextReportTime: "FMTu64" min, %d sec", nextReportTime, report_every_n_seconds);
                }
            }
        }

        LOG_INFO("Reported to server, events="FMTu_size_t, events.size());
    }else{
        LOG_INFO("Report to server failed %d, %d, events="FMTu_size_t,
                rv, handle.GetStatusCode(), events.size());
        rv = CCD_ERROR_HTTP_STATUS;
    }

    return rv;
}

void StatManager::statManagerThreadMain()
{
    int err = 0;
    VPLTime_t timeToWait = VPLTime_FromSec(report_every_n_seconds);

    LOG_DEBUG("StatManager thread started");

    while(!exit_thread) {

        bool needUpdateDiskCache = false;

        //Get the events need to report
        std::list<ccd::CachedStatEvent> eventsToReport;
        {
            //FIXME: Do we need lock cache here?
            MutexAutoLock lock(&mutex);
            if(exit_thread)
                break;

            //FIXME: Should just report all events(for all type) if time is up, reportedEvent should not be a map
            OuterMap& outerMap = statEventMap;
            for(OuterMap::iterator it = outerMap.begin(); it != outerMap.end(); it++){
                const std::string& eventId = it->first;
                InnerMap& innerMap = it->second;
                for(InnerMap::iterator it2 = innerMap.begin(); it2 != innerMap.end(); it2++) {
                    //check previous report time, if timeout is reached, report all the list
                    ccd::CachedStatEvent &event_reported = reportedEventMap[eventId];
                    if(compare_event_time(event_reported, VPLTime_ToMillisec(VPLTime_FromSec(report_every_n_seconds))) >= 0){
                        ccd::CachedStatEvent &event = it2->second;
                        eventsToReport.push_back(event);

                        timeToWait = VPLTime_FromSec(report_every_n_seconds);
                    }
                }
            }
            LOG_INFO(FMTu_size_t" events ready to report", eventsToReport.size());
        }

        {
            //report events
            u32 eventCount = eventsToReport.size();
            std::list<ccd::CachedStatEvent>::iterator itFirst = eventsToReport.begin();
            std::list<ccd::CachedStatEvent>::iterator itLast  = eventsToReport.begin();
            
            while(eventCount > 0){
                
                if(eventCount > report_batch_size){
                    itFirst = itLast;
                    advance(itLast, report_batch_size);
                    eventCount -= report_batch_size;
                }else{
                    itFirst = itLast;
                    itLast = eventsToReport.end();
                    eventCount = 0;
                }
                std::list<ccd::CachedStatEvent> batchEvents(itFirst, itLast);
                
                LOG_INFO(FMTu_size_t" batchEvents ready to report", batchEvents.size());
                int rv = reportToServer(batchEvents);
                if(rv == CCD_OK){
                    retry_count = 0;
                    timeToWait = VPLTime_FromSec(report_every_n_seconds);
                }else{
                    retry_count++;
                    //set fast_retry_every_n_seconds for next retry
                    timeToWait = VPLTime_FromSec(fast_retry_every_n_seconds);
                    eventCount = 0;
                    //Keep events as reported event list, so removed unreported ones
                    eventsToReport.erase(itFirst, eventsToReport.end());
                    break;
                }
            }
        }

        {
            MutexAutoLock lock(&mutex);
            if(exit_thread)
                break;

            //Update last reported time to now for reported event_id
            //Only update this when all report succeed.
            //If something failed, just keep last report time untouched for those failed events
            if(retry_count == 0){
                std::list<ccd::CachedStatEvent>::const_iterator it;
                for(it = eventsToReport.begin(); it != eventsToReport.end(); it++){
                    const ccd::CachedStatEvent &eventReported = *it;
                    reportedEventMap[eventReported.event_id()].set_event_id(eventReported.event_id());
                    reportedEventMap[eventReported.event_id()].set_start_time_ms(VPLTime_ToMillisec(VPLTime_GetTime()));
                    needUpdateDiskCache = true;
                }
            }

            //retry failed, check FAST_RETRY_COUNT and RETRY_COUNT
            if(retry_count > STAT_MANAGER_FAST_RETRY_COUNT){
                if(retry_count > STAT_MANAGER_RETRY_COUNT){
                    retry_count = 0;
                    timeToWait = VPLTime_FromSec(report_every_n_seconds);
                }else{
                    timeToWait = VPLTime_FromSec(retry_every_n_seconds);
                }
            }

            //clean up reported event, which are the remaining items in events list
            std::list<ccd::CachedStatEvent>::const_iterator it;
            for(it = eventsToReport.begin(); it != eventsToReport.end(); it++){
                const ccd::CachedStatEvent &eventReported = *it;
                InnerMap::iterator it2 = statEventMap[eventReported.event_id()].find(eventReported.app_id());
                if(it2 != statEventMap[eventReported.event_id()].end()){
                    const ccd::CachedStatEvent &event = it2->second;
                    //FIXME: some event may be added during reporting, need to check event_count
                    LOG_INFO("remove event %s,%s,"FMTu64" count:%d", 
                            event.app_id().c_str(), event.event_id().c_str(), 
                            event.start_time_ms(), event.event_count());
                    statEventMap[eventReported.event_id()].erase(it2);
                    needUpdateDiskCache = true;
                }else{
                    LOG_ERROR("Cannot find event in statEventMap, should not happen!");
                }
            }

            //clear all reported events from events list
            eventsToReport.clear();
        }

        if(needUpdateDiskCache){
            updateReportedAndWaitList();
            needUpdateDiskCache = false;
        }

        {
            MutexAutoLock lock(&mutex);

            if(exit_thread)
                break;

            VPLTime_t timeOut = VPLTime_GetTimeStamp() + timeToWait;
            LOG_INFO("Wait "FMTu64" sec to report", VPLTime_ToSec(timeToWait));
            err = VPLCond_TimedWait(&cond, &mutex, timeToWait);

            if(exit_thread)
                break;

            timeToWait = VPLTime_DiffClamp(timeOut, VPLTime_GetTimeStamp());

            if (err != 0 && err != VPL_ERR_TIMEOUT) {
                LOG_ERROR("Unexpected error from VPLCond_TimedWait: %d", err);
                //Break the main while loop and then exit the thread function
                break;
            }
            LOG_DEBUG("woke up - due to %s", err ? "timeout" : "signal");
        }

        {
            if(err == VPL_ERR_TIMEOUT){
                MutexAutoLock lock(&mutex);

                if(exit_thread)
                    break;

                timeToWait = VPLTime_FromSec(report_every_n_seconds);
            }
        }
    }

    LOG_INFO("StatManager thread exiting");

    thread_spawned = false;
}

//----------------------------------------------------------
class OneStatMgr {

public:
    OneStatMgr():obj(NULL) {
        VPLMutex_Init(&mutex);
    }
    ~OneStatMgr() {
        VPLMutex_Destroy(&mutex);
    }
    StatManager *obj;
    VPLMutex_t mutex;

private:

};

static OneStatMgr oneStatMgr;


int StatMgr_Start(u64 userId)
{
    int rv = CCD_OK;

    MutexAutoLock lock(&oneStatMgr.mutex);

    if (oneStatMgr.obj != NULL) {
        LOG_WARN("StatManager already started");
        rv = CCD_ERROR_ALREADY_INIT;
        goto end;
    }
    oneStatMgr.obj = new StatManager();

    rv = oneStatMgr.obj->Start(userId);

    if(rv != CCD_OK){
        LOG_ERROR("Failed to start StatManager: %d", rv);
        delete oneStatMgr.obj;
        oneStatMgr.obj = NULL;
        goto end;
    }

end:
    return rv;
}

int StatMgr_Stop()
{
    int rv = CCD_OK;

    MutexAutoLock lock(&oneStatMgr.mutex);
    LOG_INFO("enter %s", __func__);

    if (!oneStatMgr.obj) {
        LOG_WARN("StatMgr not available yet - ignore");
        rv = CCD_ERROR_NOT_INIT;
        goto end;
    }

    rv = oneStatMgr.obj->Stop();
    if(rv != CCD_OK){
        goto end;
    }
    
end:
    //Just set null here, obj will be deleted by the thread
    oneStatMgr.obj = NULL;
    return rv;
}

int StatMgr_AddEventBegin(const std::string& app_id, const std::string& event_id)
{
    int rv = CCD_OK;

    MutexAutoLock lock(&oneStatMgr.mutex);

    if (!oneStatMgr.obj) {
        LOG_WARN("StatMgr not available yet - ignore");
        rv = CCD_ERROR_NOT_INIT;
        goto end;
    }

    rv = oneStatMgr.obj->AddEventBegin(app_id, event_id);
    if (rv) {
        LOG_ERROR("Fail to add event begin %s,%s: %d", app_id.c_str(), event_id.c_str(), rv);
        goto end;
    }

end:
    return rv;
}

int StatMgr_AddEventEnd(const std::string& app_id, const std::string& event_id)
{
    int rv = CCD_OK;

    MutexAutoLock lock(&oneStatMgr.mutex);

    if (!oneStatMgr.obj) {
        LOG_WARN("StatMgr not available yet - ignore");
        rv = CCD_ERROR_NOT_INIT;
        goto end;
    }

    rv = oneStatMgr.obj->AddEventEnd(app_id, event_id);
    if (rv) {
        LOG_ERROR("Fail to add event end %s,%s: %d", app_id.c_str(), event_id.c_str(), rv);
        goto end;
    }

end:
    return rv;
}

int StatMgr_NotifyConnectionChange()
{
    int result = 0;
    int err = 0;

    MutexAutoLock lock(&oneStatMgr.mutex);

    if (!oneStatMgr.obj) {
        LOG_WARN("StatMgr not available yet - ignore");
        result = CCD_ERROR_NOT_INIT;
        goto end;
    }

    err = oneStatMgr.obj->NotifyConnectionChange();
    if (err) {
        LOG_ERROR("Failed to notify StatMgr of connection change: %d", err);
        result = err;
        goto end;
    }

 end:
    return result;
}
