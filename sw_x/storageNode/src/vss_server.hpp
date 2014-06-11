/*
 *  Copyright 2014 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef STORAGE_NODE__VSS_SERVER_HPP__
#define STORAGE_NODE__VSS_SERVER_HPP__

//#define PERF_STATS 1

/// @file
/// VSS server class
/// A single VSS server needs to be instantiated, configured and started to
/// perform VSS duties.

#include "vpl_socket.h"
#include "vpl_th.h"

#include <string>
#include <vector>
#include <set>
#include <queue>
#include <list>
#include <sstream>

// forward declaration
class vss_server;
class vss_req_proc_ctx;

#include "sn_features.h"
#include "vss_cmdproc.hpp"
#include "vss_query.hpp"
#include "vss_client.hpp"
#include "vss_session.hpp"
#include "vss_comm.h"
#include "dataset.hpp"
#include "media_metadata_errors.hpp"
#include "strm_http.hpp"
#include "vss_p2p_client.hpp"
#include "ccdi.hpp"
#include <LocalInfo.hpp>

enum {
    STRM_DIRECT,
    STRM_PROXY,
    STRM_P2P
};
#define DEFAULT_TEST_FILENAME           "test.mov"
#define DEFAULT_MEDIA_TYPE              1

#ifdef PERF_STATS
struct vss_stat_s {
    u32         count;
    VPLTime_t   time_cum;
    VPLTime_t   time_min;
    VPLTime_t   time_max;
};
typedef struct vss_stat_s vss_stat_t;
#endif // PERF_STATS

class vss_req_proc_ctx {
public:
    vss_client* client;
    vss_session* session;
    u32         vssts_id;
    vss_server*  server;
#ifdef PERF_STATS
    VPLTime_t time_start;
    VPLTime_t time_wait;
#endif // PERF_STATS
    char header[VSS_HEADER_SIZE];
    char* body;

    vss_req_proc_ctx(vss_client* client);
    ~vss_req_proc_ctx();
private:
    VPL_DISABLE_COPY_AND_ASSIGN(vss_req_proc_ctx);
};

struct worker_state {
    VPLThread_t thread;
    std::string name;

    worker_state() :
        thread(VPLTHREAD_INVALID)
    {};
        
};

#ifndef VPL_PIPE_AS_SOCKET_OK
// simple thread-safe queue
template<class T>
class STSQ
{
public:
    STSQ() { VPLMutex_Init(&mutex); }
    ~STSQ() { VPLMutex_Destroy(&mutex); }
    bool empty() const { 
        VPLMutex_Lock(&mutex);
        bool isEmpty = q.empty();
        VPLMutex_Unlock(&mutex);
        return isEmpty;
    }
    void enqueue(T cmd) { 
        VPLMutex_Lock(&mutex);
        q.push(cmd); 
        VPLMutex_Unlock(&mutex);
    }
    T dequeue() { 
        VPLMutex_Lock(&mutex);
        T cmd = q.front(); 
        q.pop(); 
        VPLMutex_Unlock(&mutex);
        return cmd;
    }
private:
    VPL_DISABLE_COPY_AND_ASSIGN(STSQ);
    std::queue<T> q;
    mutable VPLMutex_t mutex;
};
#endif // !VPL_PIPE_AS_SOCKET_OK

class vss_server
{
public:
    // Cluster ID (device ID);
    u64 clusterId;

    vss_server();
    ~vss_server();

    enum {
        DIRECT_CLIENT_INACTIVE_TIMEOUT_SEC = 30,
        PROXY_CLIENT_INACTIVE_TIMEOUT_SEC = 300,
        SESSION_VALID_INTERVAL_SEC = 60,
        SESSION_INACTIVE_TIMEOUT_SEC = 300,
        DATASET_INACTIVE_TIMEOUT_SEC = 300,
        ADDRESS_REPORT_INTERVAL_SEC = 15,
        POSTPONE_SLEEP_INTERVAL_SEC = 15,
        DATASET_STAT_UPDATE_INTERVAL_SEC = 30,
        OBJECT_INACTIVE_TIMEOUT_SEC = 150,
        OBJECT_INACTIVE_TIMEOUT_RETRY_SEC = 30,
        VSSTS_SERVER_READ_TIMEOUT_SEC = 300,    /* > Object inactive timeout */
    };

    /// Media Server Agent for Streaming callback - retrieves the media object metadata
    /// for an incoming media request.
    typedef MMError (*msaGetObjectMetadataFunc_t)(
            const media_metadata::GetObjectMetadataInput& input,
            const std::string &collectionId,
            media_metadata::GetObjectMetadataOutput& output,
            void* callbackContext);

    /// Set config information for this server instance.
    void set_server_config(const vplex::vsDirectory::SessionInfo& session,
                           const std::string& sessionSecret,
                           u64 userId,
                           u64 clusterId,
                           const char* rootDir,
                           const char* serverServicePortRange,
                           const char* infraDomain,
                           const char* vsdsHostname,
                           const char* tagEditProgramPath,
                           const char* remotefileTempFolder,
                           u16 vsdsPort,
                           msaGetObjectMetadataFunc_t msaGetObjectMetadataCb,
                           void* msaCallbackContext,
                           int enableTs,
                           Ts2::LocalInfo *localInfo);

    const std::string& getStorageRoot() const;

    // Get snapshot of server state and runtime statistics.
    void getServerStats(std::string& data_out);

    /// Start a configured server in a new thread, plus a pool of @a worker_threads worker threads.
    /// Returns success once the new threads are created.
    int start(u32 worker_threads = 1);
    
    /// Request the server to stop.
    void stop();
    
    /// Server task queue: Add work to be done.
    void addTask(void (*task)(void*), void* params);
    /// Worker function to handle next waiting task.
    void doWork();

    // Deal with incoming connections.
    void connectHandler(int sockfd);

    /// A client has changed status (need to send response, disconnected, etc.)
    void changeNotice();

    vss_session* get_session(u64 handle);

    void release_session(vss_session* session);

    /// Notify server that network interface has changed.
    /// @param local_addr Specifies the local address that is now connected to the infrastructure,
    ///     or #VPLNET_ADDR_INVALID if we are not connected.
    void notifyNetworkChange(VPLNet_addr_t local_addr);

    /// Is the network interface on a trusted network?
    /// If so, clients may request lowered security for connections to reduce computation overhead.
    /// If not, all client traffic must use high-security settings.
    bool isTrustedNetwork();

    /// Called in the event that cloudPC is going to suspend.
    void disconnectAllClients();

    /// Client has a new request pending.
    void notifyReqReceived(vss_req_proc_ctx* req);
    /// Request processing by worker thread.
    void processRequest(vss_req_proc_ctx* context, bool do_verify);

    /// Client request has been deferred and needs to be restarted
    void requeueRequest(vss_req_proc_ctx* req);

    /// Receive an event notification.
    bool receiveNotification(const void* data, u32 size);

    /// Make a proxy connection.
    /// @param req Proxy request buffer. Self-describes size.
    /// @param session User session that validated the request.
    void makeProxyConnection(const char* req, vss_session* session);

    /// Provide notice to server that P2P client has connected.
    /// Switch index for client in the p2p_clients table.
    void noticeP2PConnected(vss_p2p_client* p2p_client);

    /// Make a P2P connection.
    /// @param sockfd Connected socket to the client
    /// @param session Client session used to authenticate connection
    /// @param client_type Type of client (proxy client types)
    /// @param client_device_id Device ID of client.
    /// @param resp Authentication response for the client
    void makeP2PConnection(VPLSocket_t sockfd, VPLSocket_addr_t sockaddr, vss_session* session,
                           u64 client_device_id, u8 client_type, char* resp);

    int getDataset(u64 uid, u64 did, dataset*& dataset_out);
    void removeDataset(dataset_id id, dataset* old_dataset);

    /// Close all unused files across datasets.
    void close_unused_files();

    /// Get the abs path and file type to the test media file (for streaming).
    void get_test_media_file(std::string& testFile, int *type);

    inline msaGetObjectMetadataFunc_t getMsaGetObjectMetadataCb() {
        return msaGetObjectMetadataCb;
    }

    inline void* getMsaCallbackContext() {
        return msaCallbackContext;
    }

    int findDatasetDetail(u64 uid, u64 did, vplex::vsDirectory::DatasetDetail& detail)
    {
        return query.findDatasetDetail(uid, did, detail);
    }

    /// Get VSSI and CLEARFI port numbers, in host byte order.
    void getPortNumbers(VPLNet_port_t *clientPort, VPLNet_port_t *secureClearfiPort);

#if defined(PERF_STATS)
    void update_stat(VPLTime_t time_start, vss_stat_t& stat);
#endif // PERF_STATS
    bool  handle_command(vss_req_proc_ctx* context);

    // Add dataset stats update to send to VSDS.
    void add_dataset_stat_update(const dataset_id& id,
                                 u64 size, u64 version);
    // Process pending updates, sending stats to VSDS.
    void process_stat_updates();

    // Send a dataset changed  event to the object.
    void send_notify_event(vss_object * obj, 
                           VSSI_NotifyMask event_type,
                           char* data = NULL,
                           size_t data_length = 0);

    inline std::string getTagEditProgramPath() {
        return tagEditProgramPath;
    }

    inline std::string getRemoteFileTempFolder() {
        return remotefileTempFolder;
    }

    int updateRemoteFileAccessControlDir(const u64& user_id, 
                                        const u64& dataset_id,
                                        const ccd::RemoteFileAccessControlDirSpec &dir, 
                                        bool add);

    int getRemoteFileAccessControlDir(ccd::RemoteFileAccessControlDirs &rfaclDirs);

    bool isDeviceLinked(u64 user_id, u64 device_id);

    // TS Integration: temporary until we get this working on all platforms.
    bool isTsEnabled() const { return enableTs != 0; }
    int  getEnableTs() const { return enableTs; }

    /// This info is used by the "rf" handler to create the alias mappings.
    /// \p datasetId and \p syncFolderPath are ignored when \p enable is false.
    /// @param datasetId The specific Syncbox dataset that is associated with the local Archive Storage Device.
    int setSyncboxArchiveStorageParam(u64 datasetId, bool enable, const std::string& syncFolderPath);
    int updateSyncboxArchiveStorageDataset(u64 datasetId);

    bool isSyncboxArchiveStorage() const { return syncboxArchiveStorageEnabled; }

    /// The specific Syncbox dataset that is associated with the local Archive Storage Device.
    u64  getSyncboxArchiveStorageDatasetId() const { return syncboxArchiveStorageDatasetId; }

    std::string getSyncboxArchiveStorageStagingAreaPath(u64 datasetId) const {
         std::ostringstream oss;
         oss << getStorageRoot() << "users/" << userId << "/syncbox_staging/" << datasetId;
         return oss.str();
    }

    /// @return If false, the Syncbox feature is not enabled and the output parameters will not be valid.
    bool getSyncboxSyncFeatureInfo(u64& datasetId_out, std::string& syncFolderPath_out) { 
        if (!syncboxArchiveStorageEnabled) {
            return false;
        } else {
            datasetId_out = syncboxArchiveStorageDatasetId;
            syncFolderPath_out = syncboxSyncFeaturePath;
            return true;
        }
    }

private:
    VPL_DISABLE_COPY_AND_ASSIGN(vss_server);

#ifdef VPL_PIPE_AS_SOCKET_OK
    int pipefds[2];
#else
    STSQ<char> commandQ;
#endif

    std::string rootDir;

    std::string infraDomain;

    std::string tagEditProgramPath;

    std::string remotefileTempFolder;

    u64 sessionHandle;
    std::string sessionSecret;

    // TS Integration: temporary until we get this working on all platforms.
    int enableTs;

    // Port range to try for service.
    // If servicePortRangeEnd == 0, then ignore this range.
    int servicePortRangeBegin;
    int servicePortRangeEnd;

    // Client listening port and socket.
    bool clientSockInit;
    VPLSocket_t clientSocket;

    // Secure CLEARFI interface listening port and socket.
    bool secureClearfiSockInit;
    VPLSocket_t secureClearfiSocket;

    // Server's address reported to VSDS.
    VPLNet_addr_t curAddr;
    VPLNet_addr_t reportedAddr;
    VPLTime_t lastReportAddrTime;

    msaGetObjectMetadataFunc_t msaGetObjectMetadataCb;
    void* msaCallbackContext;

    // Web service query mechanism
    vss_query query;

    // Time past which the system may go to sleep.
    // If active, must notify power manager to extend the awake time.
    VPLTime_t nextPostponeSleepTime;
    bool sleepDeferred; // true when sleep is being deferred, false otherwise.

    VPLMutex_t mutex;

    VPLDetachableThreadHandle_t mainThreadHandle;
    bool mainStarted;

    static const char VSS_HALT = 'H';
    static const char VSS_REFRESH = 'R';

    static const size_t VSS_SERVER_STACK_SIZE = 128 * 1024;

    // Map of connected clients (Index = socket handle)
    std::map<int, vss_client*> clients;

    // Map of connected CLEARFI clients (Index = socket handle)
    std::map<int, strm_http*> clearfi_clients;

    // Map of P2P connections in progress (Index = socket handle)
    std::map<int, vss_p2p_client*> p2p_clients;

    // Pointer to the server's self_session. This is the only type of
    // session we support.
    vss_session* self_session;

    // Map of active datasets (Index = dataset ID)
    std::map<dataset_id, dataset*> datasets;

    // Pending dataset stats updates for VSDS.
    VPLMutex_t stats_mutex;
    VPLCond_t stats_condvar;
    VPLThread_t stats_thread;
    bool stat_updates_in_progress;
    bool stat_disconnect_called;  // simple printout boolean
    std::map<dataset_id, std::pair<u64, u64> > dataset_stat_updates; // (uid, did) -> (size, version)
    VPLTime_t nextDatasetStatUpdateTime;

    static VPLTHREAD_FN_DECL main_loop_thread_fn(void* param);

    // Task queue and worker threads.
    static const int WORKER_STACK_SIZE = (32 * 1024);
    u32 num_workers;
    int workers_active;
    struct worker_state* workers;    
    bool running;
    VPLTime_t worker_active_total;
    VPLTime_t worker_active_min;
    VPLTime_t worker_active_max;
    VPLTime_t worker_active_last;
    VPLTime_t task_waiting_total;
    VPLTime_t task_waiting_min;
    VPLTime_t task_waiting_max;
    VPLTime_t task_waiting_last;
    u64 tasks_finished;
    u64 task_waiting_max_count;

    typedef struct {
        /// Function for this task
        void (*task)(void* params);

        /// Parameters for the task function
        void* params;

        VPLTime_t wait_start;
    } vss_task;

    std::deque<vss_task*> taskQ;

    // Synchronization of worker tasks and task queue
    VPLMutex_t task_mutex;
    VPLCond_t task_condvar;

#ifdef PERF_STATS
    vss_stat_t stats[256];
#endif // PERF_STATS

    Ts2::LocalInfo *localInfo;

    // Main server loop while running
    void main_loop(); 

    // Handle periodic timeout sweeps (from the main loop)
    // Returns time until next timeout occurs.
    VPLTime_t timeout_handler();

    // Send a response back to the client.
    void send_client_response(char* resp, vss_req_proc_ctx* context);

    // These functions and operations are found in vss_cmdproc.cpp.

    /// Handle a valid incoming VSS client command
    char* handle_noop(vss_req_proc_ctx* context);
    char* handle_open(vss_req_proc_ctx* context);
    char* handle_close(vss_req_proc_ctx* context);
    char* handle_start_set(vss_req_proc_ctx* context);
    char* handle_commit(vss_req_proc_ctx* context);
    char* handle_erase(vss_req_proc_ctx* context);
    char* handle_delete(vss_req_proc_ctx* context);
    char* handle_read(vss_req_proc_ctx* context);
    char* handle_read_dir(vss_req_proc_ctx* context);
    char* handle_stat(vss_req_proc_ctx* context);
    char* handle_make_dir(vss_req_proc_ctx* context);
    char* handle_remove(vss_req_proc_ctx* context);
    char* handle_rename(vss_req_proc_ctx* context);
    char* handle_rename2(vss_req_proc_ctx* context);
    char* handle_copy(vss_req_proc_ctx* context);
    char* handle_set_times(vss_req_proc_ctx* context);
    char* handle_set_size(vss_req_proc_ctx* context);
    char* handle_set_metadata(vss_req_proc_ctx* context);
    char* handle_get_space(vss_req_proc_ctx* context);
    char* handle_set_notify(vss_req_proc_ctx* context);
    char* handle_get_notify(vss_req_proc_ctx* context);
    char* handle_open_file(vss_req_proc_ctx* context);
    char* handle_read_file(vss_req_proc_ctx* context);
    char* handle_write_file(vss_req_proc_ctx* context);
    char* handle_truncate_file(vss_req_proc_ctx* context);
    char* handle_chmod_file(vss_req_proc_ctx* context);
    char* handle_set_lock(vss_req_proc_ctx* context);
    char* handle_get_lock(vss_req_proc_ctx* context);
    char* handle_set_lock_range(vss_req_proc_ctx* context);
    char* handle_release_file(vss_req_proc_ctx* context);
    char* handle_close_file(vss_req_proc_ctx* context);
    char* handle_chmod(vss_req_proc_ctx* context);
    char* handle_make_dir2(vss_req_proc_ctx* context);
    char* handle_read_dir2(vss_req_proc_ctx* context);
    char* handle_stat2(vss_req_proc_ctx* context);
    char* handle_negotiate(vss_req_proc_ctx* context);
    char* handle_authenticate(vss_req_proc_ctx* context);
#if defined(PERF_STATS)
    void dump_stats(void);
#endif // PERF_STATS

    bool syncboxArchiveStorageEnabled;
    u64 syncboxArchiveStorageDatasetId;        // Support 1 for now, could be more in the future
    u64 userId;
    std::string syncboxSyncFeaturePath;
};

// Stats update main function.
VPLThread_return_t statsFunction(VPLThread_arg_t vpserver);

// Worker task main function.
VPLThread_return_t workerFunction(VPLThread_arg_t vpserver);

// Helper functions for worker-handled tasks.
void processRequestHelper(void* vpcontext);
void requeueRequestHelper(void* vpcontext);

struct vss_dataset_info_s {
    u64 user_id;
    u64 dataset_id;
};
typedef struct vss_dataset_info_s vss_dataset_info_t;

// List info for all datasets managed by the storage node.
// This function needs to work even when there's no logged in user.
int StorageNode_ListDatasetInfos(const char *root_path, std::list<vss_dataset_info_t>& dataset_infos);

#endif // include guard
