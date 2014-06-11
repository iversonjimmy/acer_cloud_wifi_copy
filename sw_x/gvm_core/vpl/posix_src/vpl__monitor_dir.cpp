/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */
#ifdef IOS

#include <sys/types.h>
#include "vpl_fs.h"

int VPLFS_MonitorInit(void)
{
    return -1;
}

int VPLFS_MonitorDestroy(void)
{
    return -1;
}

int VPLFS_MonitorDir(const char* local_dir,
                     int num_events_internal,
                     VPLFS_MonitorCallback cb,
                     VPLFS_MonitorHandle* handle_out)
{
    return -1;
}

int VPLFS_MonitorDirStop(VPLFS_MonitorHandle handle)
{
    return -1;
}

#else

#include "vpl_fs.h"
#include "vpl_lazy_init.h"
#include "vpl_time.h"
#include "vpl_th.h"
#include "vpl_fs.h"
#include "vplu_debug.h"
#include "vplu_format.h"

#include <deque>
#include <errno.h>
#include <fcntl.h>
#include <map>
#include <string>
#include <vector>
#include <sys/inotify.h>

#define SYNC_INOTIFY_STOP 'S'
#define SYNC_INOTIFY_REFRESH 'R'
#define SYNC_INOTIFY_STACK_SIZE (128 * 1024)

struct fdset_info {
    fd_set fdset;
    int maxfd;
};

typedef std::map<int, std::string> WdDirectoryMap; // pair <fd, relativeDir>

const int MAX_EVENT_SIZE = sizeof(struct inotify_event)+FILENAME_MAX;

struct SyncInotifyState {
    int inotify_fd;
    std::string baseDir;
    VPLFS_MonitorCallback cb;
    WdDirectoryMap wdDirectoryMap;
    char tempBuf[MAX_EVENT_SIZE];
    SyncInotifyState()
    :  inotify_fd(-1)
    {}
};

struct SyncCallbackInfo {
    std::string name;
    VPLFS_MonitorEventType action;

    std::string moveTo;
    u32 moveToCookie;
};

struct SyncInotifyWaitQ {
//    int inotify_fd; // not needed, first in FdWaitMap
    VPLTime_t createTime;
    bool overflow;
    bool unmount;
    VPLFS_MonitorCallback cb;
    std::deque<SyncCallbackInfo> callbackInfo;
};

typedef std::map<int, SyncInotifyState*> FdInotifyMap;
typedef std::map<int, SyncInotifyWaitQ> FdWaitMap;

static FdInotifyMap g_fdInotifyMap;
static FdWaitMap g_fdWaitMap;

static u32 g_isInitCount = 0;
static VPLLazyInitMutex_t g_vpl_file_monitor_api = VPLLAZYINITMUTEX_INIT;
static VPLLazyInitMutex_t g_sync_inotify_mutex = VPLLAZYINITMUTEX_INIT;
static int g_sync_inotify_pipe[2] = {-1, -1};
static VPLThread_t g_sync_inotify_handler_thread;

const static u32 WATCHED_INOTIFY_EVENTS = IN_CREATE | IN_DELETE |
                                          IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO |
                                          IN_DELETE_SELF;
// This wait time is only to put remove/add events together into a move (if the
// add takes a while)
// All other events should be happen as soon as possible.
// De-duplication (combining several add events into one event) is not done.
const static VPLTime_t WAIT_TIME = VPLTIME_FROM_MILLISEC(50);

static int vs_sync_inotify_init(SyncInotifyState* syncInotifyState,
                                const char* local_dir,
                                VPLFS_MonitorCallback cb);
static int recur_add_watch_then_map(int inotify_fd,
                                    const std::string& basePath,
                                    const std::string& relPath,
                                    WdDirectoryMap* wdDirectoryMap);
static void vs_sync_inotify_remove_all();
static void* vs_sync_inotify_handler(void* unused);

// copied from vsd_util.c
static int init_notify_pipe(int pipefds[2])
{
    int i;
    int rc = pipe(pipefds);
    int rv = -1;

    if(rc != 0) {
        int err = errno;
        VPL_REPORT_WARN("Failed to create notify pipe: %s(%d)",
                        strerror(err), err);
        goto exit;
    }

    for(i = 0; i < 2; i++) {
        int flags = fcntl(pipefds[i], F_GETFL);
        if (flags == -1) {
            int err = errno;
            VPL_REPORT_WARN("Failed to get flags for end %d of pipe: %s(%d)",
                            i, strerror(err), err);
            goto close_pipe;
        }
        rc = fcntl(pipefds[i], F_SETFL, flags | O_NONBLOCK);
        if(rc == -1) {
            int err = errno;
            VPL_REPORT_WARN("Failed to set nonblocking flag for end %d of pipe: %s(%d)",
                            i, strerror(err), err);
            goto close_pipe;
        }
    }

    rv = 0;
    goto exit;
 close_pipe:
    close(pipefds[0]);
    close(pipefds[1]);
 exit:
    return rv;
}

int VPLFS_MonitorInit(void)
{
    int rv = 0;
    VPLThread_attr_t thread_attr;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
    if(g_isInitCount != 0) {
        g_isInitCount++;
        VPL_REPORT_INFO("Already init with refcount %d", g_isInitCount);
        goto already_init;
    }

    rv = VPLThread_AttrInit(&thread_attr);
    if(rv != VPL_OK) {
        VPL_REPORT_WARN("Failed to initialize thread attributes.");
        goto fail_attr_init;
    }

    // Create the notifications pipe.
    rv = init_notify_pipe(g_sync_inotify_pipe);
    if(rv != 0) {
        VPL_REPORT_WARN("Failed to initialize notification pipe.");
        goto fail_pipe;
    }

    VPLThread_AttrSetStackSize(&thread_attr, SYNC_INOTIFY_STACK_SIZE);
    VPL_REPORT_INFO("Starting sync inotify handler thread with stack size %d.",
                    SYNC_INOTIFY_STACK_SIZE);

    rv = VPLThread_Create(&g_sync_inotify_handler_thread,
                          vs_sync_inotify_handler, NULL, &thread_attr, "vs_SyncInotify_handler");
    if(rv != VPL_OK) {
        VPL_REPORT_WARN("Failed to init sync_inotify handler thread: %s(%d).",
                        strerror(rv), rv);
        goto fail_thread;
    }
    g_isInitCount++;
    VPL_REPORT_INFO("Initialized FileMonitor.");

    goto done;
 fail_thread:
    close(g_sync_inotify_pipe[0]);
    close(g_sync_inotify_pipe[1]);
 fail_pipe:
    VPLThread_AttrDestroy(&thread_attr);
 fail_attr_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
    return rv;

 done:
    VPLThread_AttrDestroy(&thread_attr);
 already_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
    return rv;
}

int VPLFS_MonitorDestroy(void)
{
    char stop = SYNC_INOTIFY_STOP;
    int rv = 0;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
    g_isInitCount--;
    if(g_isInitCount != 0) {
        VPL_REPORT_INFO("Still init with refcount %d", g_isInitCount);
        goto exit;
    }
    VPL_REPORT_INFO("FileMonitor told to stop. Shutting down.");

    if(write(g_sync_inotify_pipe[1], &stop, 1) != 1) {
        int err = errno;
        VPL_REPORT_WARN("FileMonitor stop failed: %s(%d)",
                        strerror(err), err);
        rv = -1;
        goto exit;
    }

    VPLThread_Join(&g_sync_inotify_handler_thread, NULL);

    close(g_sync_inotify_pipe[0]);
    close(g_sync_inotify_pipe[1]);
 exit:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
    return 0;
}

static bool isBeginWithPeriod(const std::string& filename)
{
    if(filename.size()>0 && filename[0]=='.') {
        return true;
    }
    if(filename.find("/.") != std::string::npos) {
        return true;
    }
    return false;
}

// Recursively add watches to the intotify file descriptor.
// If the map is not null, insert the watch/directory pair into the map.
static int recur_add_watch_then_map(int inotify_fd,
                                    const std::string& basePath,
                                    const std::string& relPath,
                                    WdDirectoryMap* wdDirectoryMap)
{
    int rv = 0;
    std::deque<std::string> dirRelLocalPathQ;
    dirRelLocalPathQ.push_back(std::string("/"));

    while(!dirRelLocalPathQ.empty()) {
        VPLFS_dir_t dp;
        VPLFS_dirent_t dirp;
        std::string currDir(dirRelLocalPathQ.front());
        dirRelLocalPathQ.pop_front();
        std::string fullPath = basePath+relPath+currDir;
        if(VPLFS_Opendir(fullPath.c_str(), &dp) != VPL_OK) {
            VPL_REPORT_WARN("Error(%d,%s) opening (%s,%s,%s)",
                            errno, strerror(errno),
                            basePath.c_str(), relPath.c_str(), currDir.c_str());
            continue;
        }

        int wd = inotify_add_watch(inotify_fd,
                                   fullPath.c_str(),
                                   WATCHED_INOTIFY_EVENTS);
        if(wd < 0) {
            VPL_REPORT_WARN("Adding watch failed: fd:%d, (%d, %s), dir:%s",
                            inotify_fd, errno, strerror(errno),
                            fullPath.c_str());
            rv = -1;
            // Continue on, there might be other valid directories
        }else{
            if(wdDirectoryMap) {
                std::pair<WdDirectoryMap::iterator, bool> retC =
                        wdDirectoryMap->insert(
                                std::pair<int, std::string>(wd, relPath+currDir));
                if(retC.second == false) {
                    VPL_REPORT_WARN("Watch insertion not successful: fd:%d, wd:%d, dir:%s",
                                    inotify_fd, wd, fullPath.c_str());
                    rv = -1;
                }
            }
        }

        while (VPLFS_Readdir(&dp, &dirp) == VPL_OK) {
            std::string dirent(dirp.filename);
            if(isBeginWithPeriod(dirent))
            {   // Optimization, not syncing hidden directory
                continue;
            }
            VPLFS_stat_t statBuf;
            if (VPLFS_Stat((fullPath+dirent).c_str(), &statBuf) != VPL_OK) {
                VPL_REPORT_WARN("Error(%d,%s) using stat on (%s,%s,%s)",
                                errno, strerror(errno),
                                relPath.c_str(), currDir.c_str(), dirent.c_str());
                rv = -1;
                continue;
            }

            if((statBuf.type == VPLFS_TYPE_DIR) && (dirent=="." || dirent=="..")) {
                continue;
            }

            if(statBuf.type == VPLFS_TYPE_FILE) {
                // No need to have inotify on files, only directories
                continue;
            }

            dirRelLocalPathQ.push_back(currDir+dirent+std::string("/"));
        }
        VPLFS_Closedir(&dp);
    }
    return rv;
}

static int recur_rm_watch_then_map(int inotify_fd,
                                   const std::string& relPath,
                                   WdDirectoryMap& wdDirectoryMap)
{
    int rv = 0;
    std::string toRemoveSlash(relPath);
    toRemoveSlash.append("/");

    WdDirectoryMap::iterator mapIter = wdDirectoryMap.begin();
    while(mapIter != wdDirectoryMap.end()) {
        std::string itDir = (*mapIter).second;
        if(itDir.find(toRemoveSlash)==0) {
            VPL_REPORT_INFO("Moved from: Removing fd:%d, watch:%d, path:%s",
                            inotify_fd, (*mapIter).first, (*mapIter).second.c_str());
            int rc = inotify_rm_watch(inotify_fd, (*mapIter).first);
            if(rc != 0){
                int err = errno;
                VPL_REPORT_WARN("Removing watch failed: fd:%d, wd:%d, watchDir:%s (%d, %s)",
                                inotify_fd, (*mapIter).first, (*mapIter).second.c_str(),
                                err, strerror(err));
                rv = -1;
            }
            WdDirectoryMap::iterator toDelete = mapIter;
            ++mapIter;
            wdDirectoryMap.erase(toDelete); // Will invalidate current iterator
        }else{
            ++mapIter;
        }
    }
    return rv;
}

static void trimLeadingSlash(const std::string& path_in,
                             std::string& path_out)
{
    path_out = path_in;
    while(path_out.size()>0 && path_out[0] == '/') {
        path_out = path_out.substr(1, path_out.size()-1);
    }
}

static void handleNotifications(const fd_set& fdset,
                                VPLTime_t currTime,
                                VPLTime_t& remainingTime)
{
    FdInotifyMap::const_iterator mapIter;
    FdInotifyMap::const_iterator endIter;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));

    mapIter = g_fdInotifyMap.begin();
    for(; mapIter != g_fdInotifyMap.end(); ++mapIter) {
        if(!FD_ISSET((*mapIter).first, &fdset)) {
            continue;
        }

        FdWaitMap::iterator entry = g_fdWaitMap.find((*mapIter).first);
        if(entry == g_fdWaitMap.end()) {
            SyncInotifyWaitQ toInsert;
            toInsert.createTime = currTime;
            toInsert.overflow = false;
            toInsert.unmount = false;
            toInsert.cb = (*mapIter).second->cb;
            std::pair<FdWaitMap::iterator, bool> inserted =
                g_fdWaitMap.insert(std::pair<int, SyncInotifyWaitQ>(
                                    (*mapIter).first, toInsert));
            if(inserted.second == false) {
                VPL_REPORT_WARN("Error inserting fd:%d", (*mapIter).first);
                continue;
            }
            entry = inserted.first;
        }

        ssize_t bytesRead;
        int alreadyRead = 0;
        do {
            // WARNING:  Need to grab a whole event, otherwise read would rather return
            // -1 rather than a partial event.
            bytesRead = read((*mapIter).first,
                             (*mapIter).second->tempBuf+alreadyRead,
                             MAX_EVENT_SIZE-alreadyRead);
            if(bytesRead < 0) {
                int err = errno;
                VPL_REPORT_WARN("Read fd error: fd:%d, bytesRead:"FMT_ssize_t", alreadyRead:%d sizes:%d,%d (%d, %s)",
                                (*mapIter).first, bytesRead, alreadyRead, sizeof(struct inotify_event), FILENAME_MAX,
                                err, strerror(err));
                break;
            }

            if(bytesRead == 0){  // Done, no more events to handle.
                if(alreadyRead > 0) {
                    VPL_REPORT_WARN("Only partial request received. Abandoning.  alreadyRead:%d",
                                    alreadyRead);
                }
                break;
            }

            alreadyRead += bytesRead;
            if(alreadyRead < sizeof(struct inotify_event) ) {
                VPL_REPORT_WARN("Read bytes %d not equal to %d on fd: %d",
                                (int)bytesRead, sizeof(struct inotify_event), (*mapIter).first);
                continue;
            }

            // Parse events
            ssize_t consumedBytes = 0;
            while(alreadyRead-consumedBytes >= sizeof(struct inotify_event)) {
                struct inotify_event* event =
                        (struct inotify_event*) ((*mapIter).second->tempBuf + consumedBytes);
                if(event->len+sizeof(struct inotify_event) > alreadyRead-consumedBytes) {
                    // First pass, not enough bytes for complete event.
                    VPL_REPORT_INFO("Partial event received: len:%d, structSize:%d, "
                                    "alreadyRead:%d, consumedBytes:"FMT_ssize_t,
                                     event->len, sizeof(struct inotify_event),
                                     alreadyRead, consumedBytes);
                    break;
                }
                consumedBytes += event->len+sizeof(struct inotify_event);

                // General events
                if(event->mask & IN_Q_OVERFLOW) {
                    VPL_REPORT_INFO("Overflow received: fd:%d, %s",
                                    (*mapIter).first, event->name);
                    (*entry).second.overflow = true;
                    continue;
                }
                if(event->mask & IN_UNMOUNT) {
                    VPL_REPORT_INFO("Unmount occurred: fd:%d, %s",
                                    (*mapIter).first, event->name);
                    (*entry).second.unmount = true;
                    continue;
                }
                if(event->mask & IN_IGNORED) {
                    // Something happened to make a watch disappear.
                    // Remove watch (should be already removed)
                    WdDirectoryMap::iterator found = (*mapIter).second->wdDirectoryMap.find(event->wd);
                    if(found != (*mapIter).second->wdDirectoryMap.end()) {
                        VPL_REPORT_WARN("Unexpected delete event: fd:%d,wd:%d,"
                                        "wdMap:%s, name:%s.  Recovering.",
                                        (*mapIter).first, (*found).first,
                                        (*found).second.c_str(), event->name);
                        // Remove the watch map, it has already been removed.
                        (*mapIter).second->wdDirectoryMap.erase(found);
                    }
                    continue;
                }

                WdDirectoryMap::iterator wdDirMap = (*mapIter).second->wdDirectoryMap.find(event->wd);
                if(wdDirMap == (*mapIter).second->wdDirectoryMap.end()) {
                    // This shouldn't happen
                    VPL_REPORT_WARN("wd map does not exist. fd:%d, wd:%d, mask:%x,"
                                    "LEGEND: IN_CREATE:%x,IN_DELETE:%x,IN_MODIFY:%x,"
                                    "IN_MOVED_FROM:%x,IN_MOVED_TO:%x, IN_DELETE_SELF:%x",
                                    (*mapIter).first, event->wd, event->mask,
                                    IN_CREATE,IN_DELETE,IN_MODIFY,IN_MOVED_FROM,
                                    IN_MOVED_TO,IN_DELETE_SELF);
                    continue;
                }

                std::string refreshDir = (*wdDirMap).second;
                std::string refreshDirNoLeadingtSlash;
                trimLeadingSlash((*wdDirMap).second, refreshDirNoLeadingtSlash);
                std::string displayName = refreshDirNoLeadingtSlash+std::string(event->name);

                ///// Event handling.
                // a) Remove or add watches and get the watch descriptor (wd)
                // b) Appropriately update the wd_2_directory map.  (remove or insert)
                // c) Add a task to refresh the parent directory
                if(event->mask & IN_DELETE_SELF) {  // Remove watch
                    // delete a)
                    // Watches automatically deleted when directory is removed,
                    // no need to remove again.
                        //int rc = inotify_rm_watch((*mapIter).second->inotify_fd, event->wd);
                        //if(rc != 0) {
                        //    int err = errno;
                        //    LOG_ERROR("Remove watch failed: fd:%d wd:%d, %s, (%d, %s)",
                        //    (*mapIter).second->inotify_fd, event->wd,
                        //    refreshDir.c_str(), err, strerror(err));
                        //}
                    // delete b)
                    (*mapIter).second->wdDirectoryMap.erase(event->wd);
                    //VPL_REPORT_INFO("Removed wd:%d, %s, mask(%x)",
                    //                event->wd, refreshDir.c_str(), event->mask);
                    // delete c) will happen in separate event IN_ISDIR && IN_DELETE
                }else if((event->mask & IN_ISDIR) && (event->mask & IN_DELETE)) {
                    // delete c)
                    //VPL_REPORT_INFO("refresh delete wd:%d, %s due to delete(%x) in %s",
                    //                event->wd, refreshDir.c_str(), event->mask, event->name);
                    SyncCallbackInfo cbInfo;
                    cbInfo.name = displayName;
                    cbInfo.action = VPLFS_MonitorEvent_FILE_REMOVED;
                    (*entry).second.callbackInfo.push_back(cbInfo);
                }else if((event->mask & IN_ISDIR) && (event->mask & IN_MOVED_TO)) {
                    // move_to a) Add watches
                    // move_to b) Add to wd_2_directory map
                    std::string event_name = event->name;
                    if(isBeginWithPeriod(event_name))
                    {   // Optimization, not syncing hidden directory
                        continue;
                    }
                    int rc = recur_add_watch_then_map((*mapIter).second->inotify_fd,
                                                      (*mapIter).second->baseDir,
                                                      refreshDir+event_name,
                                                      &((*mapIter).second->wdDirectoryMap));
                    if(rc != 0) {
                        VPL_REPORT_WARN("failed recur_add_watch_then_map");
                    }
                    // move_to c)
                    bool matched = false;
                    if((*entry).second.callbackInfo.size()>0) {
                        SyncCallbackInfo &lastCbInfo = (*entry).second.callbackInfo.back();
                        if(lastCbInfo.action == VPLFS_MonitorEvent_FILE_RENAMED &&
                           lastCbInfo.moveToCookie == event->cookie) {
                            lastCbInfo.moveTo = displayName;
                            matched = true;
                        }
                    }
                    if(!matched) {
                        SyncCallbackInfo cbInfo;
                        cbInfo.name = displayName;
                        cbInfo.action = VPLFS_MonitorEvent_FILE_ADDED;
                        (*entry).second.callbackInfo.push_back(cbInfo);
                    }
                    //VPL_REPORT_INFO("dir Move_to event: fd:%d, wd:%d, dir:%s, mask:(%x)",
                    //                (*mapIter).second->inotify_fd, event->wd,
                    //                (refreshDir+std::string(event->name)).c_str(), event->mask);
                }else if((event->mask & IN_ISDIR) && (event->mask & IN_MOVED_FROM)) {
                    // move_from is special, because we don't know what subdirectories were
                    //   moved.  Do an inefficient lookup through map for now.
                    // move_from b) look up directories to remove from map
                    // move_from a) remove watches
                    int rc = recur_rm_watch_then_map((*mapIter).second->inotify_fd,
                                                     refreshDir+std::string(event->name),
                                                     (*mapIter).second->wdDirectoryMap);
                    if(rc != 0) {
                        VPL_REPORT_WARN("failed recur_rm_watch_then_map");
                    }
                    // move_from c)
                    SyncCallbackInfo cbInfo;
                    cbInfo.name = displayName;
                    cbInfo.action = VPLFS_MonitorEvent_FILE_RENAMED;
                    cbInfo.moveToCookie = event->cookie;
                    (*entry).second.callbackInfo.push_back(cbInfo);
                    //VPL_REPORT_INFO("dir Move_from event: fd:%d, wd:%d, dir:%s, mask:(%x)",
                    //                (*mapIter).second->inotify_fd, event->wd,
                    //                (refreshDir+std::string(event->name)).c_str(), event->mask);
                }else if((event->mask & IN_ISDIR) && (event->mask & IN_CREATE)) {
                    // create_dir a)
                    // create_dir b)
                    std::string event_name = event->name;
                    if(isBeginWithPeriod(event_name))
                    {   // Optimization, not syncing hidden directory
                        continue;
                    }
                    int rc = recur_add_watch_then_map((*mapIter).second->inotify_fd,
                                                      (*mapIter).second->baseDir,
                                                      refreshDir+event_name,
                                                      &((*mapIter).second->wdDirectoryMap));
                    if(rc != 0) {
                        VPL_REPORT_WARN("failed recur_add_watch_then_map");
                    }
                    // create_dir c)
                    SyncCallbackInfo cbInfo;
                    cbInfo.name = displayName;
                    cbInfo.action = VPLFS_MonitorEvent_FILE_ADDED;
                    (*entry).second.callbackInfo.push_back(cbInfo);
                    //VPL_REPORT_INFO("dir create event: fd:%d, dir:%s, mask:(%x)",
                    //                (*mapIter).second->inotify_fd,
                    //                (refreshDir+std::string(event->name)).c_str(), event->mask);
                } else {
                    // Directory did not change, only file changed.
                    // No need for steps (a) and (b)
                    // File change c)
                    SyncCallbackInfo cbInfo;
                    switch ( event->mask ) {
                    case IN_CREATE:
                        cbInfo.action = VPLFS_MonitorEvent_FILE_ADDED;
                        cbInfo.name = displayName;
                        (*entry).second.callbackInfo.push_back(cbInfo);
                        break;
                    case IN_DELETE:
                        cbInfo.action = VPLFS_MonitorEvent_FILE_REMOVED;
                        cbInfo.name = displayName;
                        (*entry).second.callbackInfo.push_back(cbInfo);
                        break;
                    case IN_MODIFY:
                        cbInfo.action = VPLFS_MonitorEvent_FILE_MODIFIED;
                        cbInfo.name = displayName;
                        (*entry).second.callbackInfo.push_back(cbInfo);
                        break;
                    case IN_MOVED_FROM:
                        cbInfo.action = VPLFS_MonitorEvent_FILE_RENAMED;
                        cbInfo.name = displayName;
                        cbInfo.moveToCookie = event->cookie;
                        (*entry).second.callbackInfo.push_back(cbInfo);
                        break;
                    case IN_MOVED_TO:
                        {
                            bool matched = false;
                            if((*entry).second.callbackInfo.size()>0) {
                                SyncCallbackInfo &lastCbInfo = (*entry).second.callbackInfo.back();
                                if(lastCbInfo.action == VPLFS_MonitorEvent_FILE_RENAMED &&
                                   lastCbInfo.moveToCookie == event->cookie) {
                                    lastCbInfo.moveTo = displayName;
                                    matched = true;
                                }
                            }
                            if(!matched) {
                                SyncCallbackInfo cbInfo;
                                cbInfo.name = displayName;
                                cbInfo.action = VPLFS_MonitorEvent_FILE_ADDED;
                                (*entry).second.callbackInfo.push_back(cbInfo);
                            }
                            break;
                        }
                    }
                    //VPL_REPORT_INFO("file changed: %s, refreshing: %s. fd:%d, wd:%d, mask(%x)",
                    //                event->name, refreshDir.c_str(),
                    //                (*mapIter).second->inotify_fd, event->wd, event->mask);
                }
            }
            alreadyRead -= consumedBytes;
            memcpy((*mapIter).second->tempBuf,
                   (*mapIter).second->tempBuf+consumedBytes,
                   alreadyRead);

        } while(alreadyRead != 0);  // Should be this: while(bytesRead > 0);  // Only stop when bytes read is empty; however,
                                    // android does not support non-blocking fd function, init_inotify1(IN_NONBLOCK)
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));

    // Process wait Q
    FdWaitMap::iterator fdWait_it = g_fdWaitMap.begin();
    remainingTime = WAIT_TIME;
    while(fdWait_it != g_fdWaitMap.end()) {
        SyncInotifyWaitQ &waitQ = (*fdWait_it).second;
        VPLTime_t difference = currTime - waitQ.createTime;
        bool eventGenerated = false;
        bool workDone = true;;
        while(workDone) {
            workDone = false;
            std::vector<std::string> keepUntilCallback;
            // Wait time has elapsed, issue call to callback!
            const u32 MAX_EVENTS = 32;
            VPLFS_MonitorEvent cbEvents[MAX_EVENTS];
            int cbEventSize = 0;
            std::vector<SyncCallbackInfo>::size_type numCallbacks = waitQ.callbackInfo.size();

            if(!waitQ.overflow && !waitQ.unmount)
            {
                if(numCallbacks == 0) {
                    // no work to do
                    break;
                }
                u32 cbEventIndex = 0;
                while(cbEventIndex<numCallbacks &&
                      cbEventIndex < MAX_EVENTS &&
                      !waitQ.callbackInfo.empty())
                {
                    SyncCallbackInfo &callBackInfo = waitQ.callbackInfo.front();
                    cbEvents[cbEventIndex].action = callBackInfo.action;
                    keepUntilCallback.push_back(callBackInfo.name);
                    cbEvents[cbEventIndex].filename = keepUntilCallback.back().c_str();
                    cbEvents[cbEventIndex].nextEntryOffsetBytes = sizeof(VPLFS_MonitorEvent);
                    cbEvents[cbEventIndex].moveTo = "";
                    if(callBackInfo.action == VPLFS_MonitorEvent_FILE_RENAMED) {
                        if (callBackInfo.moveTo != "") {
                            keepUntilCallback.push_back(callBackInfo.moveTo);
                            cbEvents[cbEventIndex].moveTo = keepUntilCallback.back().c_str();
                        }else{

                            if(waitQ.callbackInfo.size()>1 ||    // additional events, match skipped
                               difference >= WAIT_TIME)
                            {
                                // Never matched, was moved out.
                                cbEvents[cbEventIndex].action = VPLFS_MonitorEvent_FILE_REMOVED;
                            }else{
                                break;
                            }

                        }
                    }
                    ++cbEventIndex;
                    waitQ.callbackInfo.pop_front();
                    eventGenerated = true;
                    workDone = true;
                }
                if(cbEventIndex > 0) {
                    cbEventSize = sizeof(VPLFS_MonitorEvent)*cbEventIndex;
                    cbEvents[cbEventIndex-1].nextEntryOffsetBytes = 0;
                }
            } else {
                numCallbacks = 0;
                waitQ.callbackInfo.clear();
                waitQ.overflow = true;
            }

            int error = 0;
            if(waitQ.overflow) {
                error = VPLFS_MonitorCB_OVERFLOW;
            }
            if(waitQ.unmount){
                error = VPLFS_MonitorCB_UNMOUNT;
            }

            waitQ.cb(fdWait_it->first,
                     cbEvents,
                     cbEventSize,
                     error);
        }

        FdWaitMap::iterator toDelete = fdWait_it;
        ++fdWait_it;
        if(toDelete->second.callbackInfo.size()==0) {
            g_fdWaitMap.erase(toDelete);  // current iterator is invalid after erase
        }else if(eventGenerated) {
            waitQ.createTime = currTime;
        }
    }

    if(g_fdWaitMap.size() == 0) {
        remainingTime = VPLTIME_INVALID;
    }
}

static void* vs_sync_inotify_handler(void* unused)
{
    int rc;
    fd_set fdset;
    struct fdset_info master_fdset;
    struct timeval timeout;
    VPLTime_t wait_queue_delay = VPLTIME_INVALID;

    FD_ZERO(&master_fdset.fdset);
    master_fdset.maxfd = 0;
    FD_ZERO(&fdset);

    FD_SET(g_sync_inotify_pipe[0], &master_fdset.fdset);
    master_fdset.maxfd = g_sync_inotify_pipe[0];

    while(1) {
        fdset = master_fdset.fdset;
        rc = select(master_fdset.maxfd + 1, &fdset, NULL, NULL,
                    (wait_queue_delay == VPLTIME_INVALID) ? NULL : &timeout);
        if(rc == -1) {
            int err = errno;

            if(err == EBADF) {
                VPL_REPORT_INFO("Connection must have just closed. Got EBADF. Wait for rebuild signal on pipe.");
                FD_ZERO(&master_fdset.fdset);
                FD_SET(g_sync_inotify_pipe[0], &master_fdset.fdset);
                master_fdset.maxfd = g_sync_inotify_pipe[0];
                continue;
            } else if(err == EINTR) {
                // Debug gprof profiling would interrupt the select call, simply try again
                continue;
            } else {
                VPL_REPORT_FATAL("Select call error: %s(%d).",
                                 strerror(err), err);
                return NULL;
            }
        }

        handleNotifications(fdset, VPLTime_GetTime(), wait_queue_delay);
        if(wait_queue_delay != VPLTIME_INVALID){
            timeout.tv_sec = VPLTIME_TO_SEC(wait_queue_delay);
            timeout.tv_usec = wait_queue_delay - VPLTIME_FROM_SEC(timeout.tv_sec);
        }

        // Check notify pipe for orders
        // Quit loop on stop order
        // Anything else, just eat it.
        char cmd = '\0';
        int err = 0;
        do {
//            LOG_DEBUG("Reading sync_inotify pipe...");
            rc = read(g_sync_inotify_pipe[0], &cmd, 1);
            if(rc == -1) err = errno;
//            LOG_DEBUG("Read sync_inotify pipe. rc:%d(%d), cmd:%c",
//                      rc, err, cmd);
            if(rc == 0) {
                // no more messages at EOF.
                break;
            }
            else if(rc == 1) {
                // Got a message.
                FdInotifyMap::const_iterator mapIter;

                switch(cmd) {
                case SYNC_INOTIFY_STOP:
                    VPL_REPORT_INFO("Sync inotify stop. time to go...");
                    vs_sync_inotify_remove_all();
                    return NULL;
                    break;

                case SYNC_INOTIFY_REFRESH:
                    // handle updated sockets after pipe is empty.
                    FD_ZERO(&master_fdset.fdset);
                    FD_SET(g_sync_inotify_pipe[0], &master_fdset.fdset);
                    master_fdset.maxfd = g_sync_inotify_pipe[0];

                    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));
                    mapIter = g_fdInotifyMap.begin();
                    for(; mapIter != g_fdInotifyMap.end(); ++mapIter) {
                        FD_SET((*mapIter).first, &master_fdset.fdset);
                        if((*mapIter).first > master_fdset.maxfd) {
                            master_fdset.maxfd = (*mapIter).first;
                        }
                    }
                    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));
                    break;

                default:
                    // Handle outbound requests after pipe is empty.
                    break;
                }
            }
            else {
                // Got an error.
                if(err == EAGAIN || err == EINTR) {
                    // no big deal
                    break;
                }
                else {
                    VPL_REPORT_WARN("Got error %s(%d) reading notify pipe:%d.",
                                    strerror(err), err, rc);
                    vs_sync_inotify_remove_all();
                    return NULL;
                }
            }
        } while(rc == 1);
    }
    return NULL;
}

int VPLFS_MonitorDir(const char* local_dir,
                     int num_events_internal,
                     VPLFS_MonitorCallback cb,
                     VPLFS_MonitorHandle* handle_out)
{
    int rc;
    int rv=0;
    char msg;
    SyncInotifyState* syncInotifyState;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
    if(g_isInitCount == 0) {
        VPL_REPORT_FATAL("FileMonitor not initialized.");
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
        return -2;
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));

    syncInotifyState = new SyncInotifyState();
    rc = vs_sync_inotify_init(syncInotifyState, local_dir, cb);
    if(rc != 0) {
        VPL_REPORT_WARN("Failed inotify init for local_dir: %s",
                        local_dir);
        goto fail_inotify_init;
    }

    {
        std::pair<FdInotifyMap::iterator, bool> retC =
                g_fdInotifyMap.insert(std::pair<int, SyncInotifyState*>(
                                        syncInotifyState->inotify_fd,
                                        syncInotifyState));
        if(retC.second == false) {
            VPL_REPORT_WARN("Failed inserting syncInotifyState, fd:%d",
                            syncInotifyState->inotify_fd);
            goto fail_inotify_insert;
        }
    }
    *handle_out = syncInotifyState->inotify_fd;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));

    msg = SYNC_INOTIFY_REFRESH;
    if(write(g_sync_inotify_pipe[1], &msg, 1) != 1) {
        int err = errno;
        VPL_REPORT_WARN("FD Refresh failed in add: %s(%d)", strerror(err), err);
    }
    goto done;

 fail_inotify_insert:
 fail_inotify_init:
    delete syncInotifyState;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));
    rv = -1;

 done:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
    return rv;
}

//int vs_sync_inotify_remove(u64 user_id,
//                           u64 dataset_id)
int VPLFS_MonitorDirStop(VPLFS_MonitorHandle handle)
{
    int rv = -1;
    char msg;
    FdInotifyMap::iterator mapIter;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
    if(g_isInitCount == 0) {
        VPL_REPORT_FATAL("FileMonitor not initialized.");
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
        return -2;
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));

    int inotifyFd = (int)handle;
    mapIter = g_fdInotifyMap.find(inotifyFd);
    if(mapIter != g_fdInotifyMap.end()) {
        const SyncInotifyState* syncInotifyState = (*mapIter).second;
        close((*mapIter).first);
        delete syncInotifyState;
        FdInotifyMap::iterator toDelete = mapIter;
        g_fdInotifyMap.erase(toDelete); // Will invalidate current iterator
        rv = 0;
        VPL_REPORT_INFO("Removed inotify for user:%d", inotifyFd);
    }

    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));

    msg = SYNC_INOTIFY_REFRESH;
    if(write(g_sync_inotify_pipe[1], &msg, 1) != 1) {
        int err = errno;
        VPL_REPORT_WARN("FD Refresh failed in remove: %s(%d)", strerror(err), err);
    }

    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_vpl_file_monitor_api));
    return rv;
}

static void vs_sync_inotify_remove_all()
{
    FdInotifyMap::iterator mapIter;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));
    mapIter = g_fdInotifyMap.begin();
    for(; mapIter != g_fdInotifyMap.end(); ++mapIter) {
        const SyncInotifyState* syncInotifyState = (*mapIter).second;
        close((*mapIter).first);
        delete syncInotifyState;
    }
    g_fdInotifyMap.clear();
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_sync_inotify_mutex));
}

static int vs_sync_inotify_init(SyncInotifyState* syncInotifyState,
                                const char* local_dir,
                                VPLFS_MonitorCallback cb)
{
    if(syncInotifyState->inotify_fd != -1) {
        close(syncInotifyState->inotify_fd);
        syncInotifyState->inotify_fd = -1;
    }

    syncInotifyState->inotify_fd = inotify_init();
    if(syncInotifyState->inotify_fd < 0) {
        VPL_REPORT_WARN("Error creating inotify_fd for %s, %d, %s",
                        local_dir,
                        errno, strerror(errno));
        syncInotifyState->inotify_fd = -1;
        return -1;
    }
    syncInotifyState->baseDir.assign(local_dir);
    syncInotifyState->cb = cb;

    int rv = 0;
    int rc = recur_add_watch_then_map(syncInotifyState->inotify_fd,
                                      syncInotifyState->baseDir,
                                      std::string(""),
                                      &(syncInotifyState->wdDirectoryMap));
    if(rc != 0) {
        VPL_REPORT_WARN("Errors adding watches for local_dir:%s", local_dir);
        rv = -1;
    }

    if(syncInotifyState->wdDirectoryMap.size() == 0) {
        VPL_REPORT_WARN("No watches were created for %s", local_dir);
        rv = -1;
    }

    return rv;
}
#endif
