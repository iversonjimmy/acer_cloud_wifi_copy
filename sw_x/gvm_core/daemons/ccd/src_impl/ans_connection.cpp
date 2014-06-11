//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "ans_connection.hpp"

#if CCD_USE_ANS
#include "vplu.h"
#include "vplex_ans_message.pb.h"
#include "vplex_community_notifier.pb.h"
#include "vpl_lazy_init.h"
#include "vplex_sync_agent_notifier.pb.h"
#include "vplex_mem_utils.h"
#include "vplex_safe_serialization.h"
#include "log.h"
#include "AsyncDatasetOps.hpp"
#include "cache.h"
#include "config.h"
#include "netman.hpp"
#include "query.h"
#include "DeviceStateCache.hpp"
#include "SyncFeatureMgr.hpp"
#include "ccdi.hpp"
#include "EventManagerPb.hpp"
#include "ts_client.hpp"
#include <LocalInfo.hpp>

#if CCD_ENABLE_STORAGE_NODE
#include "vss_server.hpp"
#endif

#include <deque>

static VPLLazyInitMutex_t s_mutex = VPLLAZYINITMUTEX_INIT;

/// This is the state of the API.
/// Is true between calls to #ANSConn_Start() and #ANSConn_Stop().
static bool s_isEnabled = false;

/// Must be non-null if and only if (s_isEnabled && !s_suspend)
static ans_client_t* s_client = NULL;

static VPLNet_addr_t s_currLocalAddr = VPLNET_ADDR_INVALID;

/// TRUE only after the ANS connection is established.
static VPL_BOOL s_isActive = VPL_FALSE;

/// TRUE if the OS is going to sleep (we should stop the ANS client until the OS is resumed).
static VPL_BOOL s_suspend = VPL_FALSE;

/// Non-zero whenever there is an ANS connection (even if the users are not updated from infra yet).
static u64 s_currLocalConnId = 0;

static u64 s_localConnIdCount = 0;

/// 0 indicates foreground mode.
/// Positive number indicates background mode, with the promise that someone will call
/// #ANSConn_PerformBackgroundTasks() at least once within the specified number of seconds.
static int s_backgroundModeIntervalSec = 0;

//-----------
// Only valid when (s_isEnabled)
static VPLUser_Id_t s_userId;
static u64 s_deviceId;
static s64 s_clusterId;
static std::string s_ansSessionKey;
static std::string s_ansLoginBlob;
static u64* s_deviceIdsToWatch = NULL;
static int s_numDeviceIdsToWatch = 0;
//-----------

static s32 privStart();
static void privStartIgnoreError();
static void privHandleAnsDisconnect();
static void privStop();

u64 ANSConn_GetLocalConnId()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    return s_currLocalConnId;
}

VPLNet_addr_t ANSConn_GetLocalAddr()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    return s_currLocalAddr;
}

static ans_client_t* getCurrClient()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    return s_client;
}

static bool isCurrClient(ans_client_t* client)
{
    return (client == getCurrClient());
}

static void
loginCompletedCb(ans_client_t *client)
{
    {
        // If we want to grab the cache lock, we must always do that *before* locking s_mutex,
        // since other code calls into ANSConn_* with the cache lock held.
        CacheAutoLock autoLock;
        s32 rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            return;
        }

        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        if (!isCurrClient(client)) {
            // Not reporting about the client we care about.
            LOG_INFO("Callback for old ans client; ignoring");
            return;
        }
        s_localConnIdCount++;
        s_currLocalConnId = s_localConnIdCount;

        // Connection is ready.  Mark everything to be updated now.
        LOG_INFO("ANS login complete (#"FMTu64"); requesting full update", s_currLocalConnId);

        CacheMonitor_MarkDirtyAll(CCD_USER_DATA_ALL);

        s_isActive = VPL_TRUE;
        LOG_INFO("s_isActive = TRUE");

        DeviceStateCache_SetLocalDeviceState(ccd::DEVICE_CONNECTION_ONLINE);

#if CCD_ENABLE_DOC_SAVE_N_GO
        ADO_Enable_DocSaveNGo();
#endif

        NetMan_NetworkChange();
    }

    Cache_OnAnsConnected();
}

static void
rejectSubscriptionsCb(ans_client_t *client)
{
}

static
VPL_BOOL connectionActiveCb(ans_client_t* client, VPLNet_addr_t localAddr)
{
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        if (!isCurrClient(client)) {
            // Not reporting about the client we care about.
            LOG_INFO("Callback for old ans client; ignoring");
            return VPL_TRUE;
        }
        s_currLocalAddr = localAddr;
    }

    LOG_INFO("ANS connection established; localAddr for infra connection="FMT_VPLNet_addr_t,
            VAL_VPLNet_addr_t(localAddr));

    // Update any local storage nodes.
    // Cannot call #Cache_NetworkStatusChange while holding s_mutex; since the CCD cache functions
    // rely on calling ans_connection functions while holding the cache lock, deadlock could result.
    Cache_NetworkStatusChange(localAddr);

#if CCD_ENABLE_DOC_SAVE_N_GO
    ADO_Enable_DocSaveNGo();
#endif

    return VPL_TRUE;
}

static
VPL_BOOL processSyncAgentNotification(const void* payload, u32 payloadLen, uint64_t asyncId)
{
    vplex::syncagent::notifier::SyncAgentNotification notification;
    bool success = notification.ParseFromArray(payload, payloadLen);
    if (!success) {
        LOG_ERROR("Failed to parse sync agent notification (asyncId="FMTu64")", asyncId);
        return VPL_FALSE;
    }
    Cache_ProcessSyncAgentNotification(notification, asyncId);
    return VPL_TRUE;
}

static
VPL_BOOL processProxyNotification(const void* payload, u32 payloadLen)
{
#if CCD_ENABLE_STORAGE_NODE
    s32 rv;

    {
        MutexAutoLock lock(LocalServers_GetMutex());
        vss_server* sn = LocalServers_getStorageNode();
        if (sn != NULL) {
            sn->receiveNotification(payload, payloadLen);
            // Done!
            goto out;
        }
    }

    LOG_INFO("Not currently running StorageNode");

    // StorageNode isn't currently running; make sure that our list
    // of storage nodes is up-to-date before we drop the notification.
    rv = CacheMonitor_UpdateIfNeededSyncUser(CCD_USER_DATA_STORAGE_NODES_ONLY);
    if (rv != 0) {
        if (rv == CCD_ERROR_NOT_SIGNED_IN) {
            LOG_INFO("User is no longer logged-in");
        } else {
            LOG_ERROR("%s failed: %d", "CacheMonitor_UpdateIfNeededSyncUser", rv);
        }
    }

    {
        MutexAutoLock lock(LocalServers_GetMutex());
        vss_server* sn = LocalServers_getStorageNode();
        if (sn != NULL) {
            sn->receiveNotification(payload, payloadLen);
            // Done!
            goto out;
        } else {
            LOG_WARN("We are not a StorageNode; dropping PROXY notification.");
        }
    }
out:
#endif
    return VPL_TRUE;
}

static
VPL_BOOL processClientConnNotification(const void* payload, u32 payloadLen)
{
    {
        MutexAutoLock lock(LocalServers_GetMutex());
        Ts2::LocalInfo* li = LocalServers_getLocalInfo();
        if (li != NULL) {
            li->AnsNotifyIncomingClient((const char*)payload, payloadLen);
        } else {
            LOG_WARN("LocalInfo for Ts2 is NULL.");
        }
    }
    return VPL_TRUE;
}

static
VPL_BOOL processServerWakeup(const void* payload, u32 payloadLen)
{
    //Pertains to ccd client query ccd server declaration, but unable to obtain ccd server info from pxd server
    //(Jon B will provide that function)
    return VPL_TRUE;
}

static
VPL_BOOL processSessionExpiredNotification(const void* payload, u32 payloadLen)
{
    VPLInputBuffer inBuf;
    VPLInitInputBuffer(&inBuf, payload, payloadLen);

    u64 sessionHandle;
    VPLUnpackU64(&inBuf, &sessionHandle);
    if (VPLHasOverrunIn(&inBuf)) {
        LOG_ERROR("Invalid ANS_TYPE_SESSION_EXPIRED notification!");
        return VPL_TRUE;
    }

    u32 sizeOfProto = 0;
    if (payloadLen > sizeof(u64)) {
        sizeOfProto = payloadLen - sizeof(u64);
        const void* protoStart = VPLPtr_AddUnsigned(payload, sizeof(u64));
        vplex::ans::SessionExpiredDetails sed;
        bool parsedOk = sed.ParseFromArray(protoStart, sizeOfProto);
        if (!parsedOk) {
            LOG_WARN("Invalid SessionExpiredDetails!");
        }
        if (sed.device_unlinked()) {
            LOG_INFO("Session "FMTu64" expired due to unlink.", sessionHandle);
            Cache_ProcessExpiredSession(sessionHandle, ccd::LOGOUT_REASON_DEVICE_UNLINKED);
            return VPL_TRUE;
        }
    }

    LOG_INFO("Expired session: "FMTu64, sessionHandle);
    Cache_ProcessExpiredSession(sessionHandle, ccd::LOGOUT_REASON_SESSION_INVALID);
    return VPL_TRUE;
}

static VPL_BOOL processRemoteSwUpdateReq(vplex::ans::UserDeviceMsg_t &msg)
{
#if CCD_ENABLE_STORAGE_NODE
    ccd::ListLinkedDevicesInput req;
    ccd::ListLinkedDevicesOutput resp;
    int rv = 0;

    LOG_INFO("Handling SW Update request target("FMTu64")", msg.srcdeviceid());

    req.set_user_id(s_userId);
    req.set_only_use_cache(true);
    rv = CCDIListLinkedDevices(req, resp);
    if (rv < 0) {
        LOG_ERROR("CCDIListLinkedDevices failed, rv=%d\n", rv);
        return VPL_FALSE;
    }

    // Verify the version of sender is higher than self before generating the event
    for (int i = 0; i < resp.devices_size(); i++) {
        if (msg.srcdeviceid() == resp.devices(i).device_id()) {
            if ((resp.devices(i).has_protocol_version()) &&
                (atoi(resp.devices(i).protocol_version().c_str()) > atoi(CCD_PROTOCOL_VERSION)))
            {
                ccd::CcdiEvent *event;
                // Generate SW update event
                event = new ccd::CcdiEvent();
                event->mutable_su_message()->set_source_device_id(msg.srcdeviceid()); 
                EventManagerPb_AddEvent(event);
                // event will be freed by EventManagerPb.
            }
        }
    }
    return VPL_TRUE;
#else
    LOG_ERROR("Remote SW update request can only be handled by storage node");
    return VPL_FALSE;
#endif
}

static VPL_BOOL processUserDeviceMessage(const void* payload, u32 payloadLen)
{
    vplex::ans::UserDeviceMsg_t msg;

    if (msg.ParseFromArray(payload, payloadLen) == true) {
        if (msg.msgtype() == vplex::ans::REMOTE_SW_UPDATE_REQ) {
            return processRemoteSwUpdateReq(msg);
        } else {
            LOG_ERROR("Invalid user device message type (%d)", msg.msgtype());
            return VPL_FALSE;
        }
    } else {
        LOG_ERROR("Error parsing user device message payload, size ("FMTu32")", payloadLen);
        return VPL_FALSE;
    }
}

static VPL_BOOL processPairingRequestedMessage(const void* payload, u32 payloadLen)
{
    vplex::ans::PairingRequestDetails details;
    if (details.ParseFromArray(payload, payloadLen)) {
        ccd::CcdiEvent *event = new ccd::CcdiEvent();
        ccd::EventPairingRequest *eventPairingRequest = event->mutable_pairing_request();
        for (int i = 0; i < details.pairing_attributes_size(); i++) {
            vplex::ans::PairingRequestAttribute attributes = details.pairing_attributes(i);
            ccd::PairingRequestAttribute* attribute = eventPairingRequest->add_pairing_attributes();
            attribute->set_key(attributes.key());
            attribute->set_value(attributes.value());
        }
        event->mutable_pairing_request()->set_transaction_id(details.transaction_id());
        EventManagerPb_AddEvent(event);
        // event will be freed by EventManagerPb.
        return VPL_TRUE;
    } else {
        LOG_ERROR("Error parsing pairing request message payload, size ("FMTu32")", payloadLen);
        return VPL_FALSE;
    }
}

static
VPL_BOOL receiveNotificationCb(ans_client_t* client, ans_unpacked_t* notification)
{
    if (!isCurrClient(client)) {
        // Not reporting about the client we care about.
        LOG_INFO("Callback for old ans client; ignoring");
        return VPL_TRUE;
    }
    if (notification->notificationLength < 1) {
        LOG_WARN("Ignoring empty notification");
        return VPL_TRUE;
    }
    // TODO: It should be nearly impossible to hit, but technically there is a race
    //     condition if a thread gets stopped here for a long long time.
    
    u8 type = ((u8*)notification->notification)[0];
    const void* payload = VPLPtr_AddUnsigned(notification->notification, 1);
    u32 payloadLen = notification->notificationLength - 1;
    switch (type) {
    case vplex::ans::ANS_TYPE_SYNC_AGENT:
        {
            LOG_INFO("Received "FMTu32" byte notification for SYNC_AGENT (asyncId="FMTu64")", payloadLen, notification->asyncId);
            return processSyncAgentNotification(payload, payloadLen, notification->asyncId);
        }
    case vplex::ans::ANS_TYPE_PROXY:
        {
            LOG_INFO("Received "FMTu32" byte notification for PROXY (asyncId="FMTu64")", payloadLen, notification->asyncId);
            return processProxyNotification(payload, payloadLen);
        }
    case vplex::ans::ANS_TYPE_CLIENT_CONN:
        {
            LOG_INFO("Received "FMTu32" byte Client Connect Request notification (asyncId="FMTu64")", payloadLen, notification->asyncId);
            return processClientConnNotification(payload, payloadLen);
        }
    case vplex::ans::ANS_TYPE_SERVER_WAKEUP:
        {
            LOG_INFO("Received "FMTu32" byte SERVER_WAKEUP notification (asyncId="FMTu64")", payloadLen, notification->asyncId);
            return processServerWakeup(payload, payloadLen);
        }
    case vplex::ans::ANS_TYPE_SESSION_EXPIRED:
        {
            LOG_INFO("Received "FMTu32" byte SESSION_EXPIRED notification (asyncId="FMTu64")", payloadLen, notification->asyncId);
            return processSessionExpiredNotification(payload, payloadLen);
        }
    case vplex::ans::ANS_TYPE_USER_DEVICE_MESSAGE:
        {
            LOG_INFO("Received "FMTu32" byte user device message (asyncId="FMTu64")", payloadLen, notification->asyncId);
            return processUserDeviceMessage(payload, payloadLen);
        }
    case vplex::ans::ANS_TYPE_PAIRING_REQUESTED:
        {
            LOG_INFO("Received "FMTu32" byte pairing requested message (asyncId="FMTu64")", payloadLen, notification->asyncId);
            return processPairingRequestedMessage(payload, payloadLen);
        }
    }

    LOG_WARN("Ignoring notification of type "FMTu8, type);
    return VPL_TRUE;
}

static void
receiveSleepInfoCb(ans_client_t *client, ans_unpacked_t *notification)
{
#if CCD_ENABLE_IOAC
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (!isCurrClient(client)) {
        LOG_INFO("Callback for old ans client; ignoring");
        // Not reporting about the client we care about.
        return;
    }
    LOG_INFO("Got sleep info (asyncId="FMTu64", sleepPacketLength=%d, sleepDns=%s)",
            notification->asyncId, notification->sleepPacketLength, notification->sleepDns);

    char sleepServerFqdn[HOSTNAME_MAX_LENGTH];
    CCD_GET_SLEEP_SERVER_HOSTNAME(sleepServerFqdn, notification->sleepDns);

    int temp_rv = NetMan_SetWakeOnWlanData(notification->asyncId,
            notification->wakeupKey, notification->wakeupKeyLength,
            sleepServerFqdn, notification->sleepPort, notification->sleepPacketInterval,
            notification->sleepPacket, notification->sleepPacketLength);
    if (temp_rv != 0) {
        LOG_WARN("%s failed: %d", "NetMan_SetWakeOnWlanData", temp_rv);
    }
#endif
}

/// See #ans_callbacks_t.connectionDown
static void
connectionDownCb(ans_client_t* client)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (!isCurrClient(client)) {
        LOG_INFO("Callback for old ans client; ignoring");
        // Not reporting about the client we care about.
        return;
    }
    LOG_INFO("ANS connection lost");
    privHandleAnsDisconnect();
}

/// See #ans_callbacks_t.connectionClosed
static void
connectionClosedCb(ans_client_t* client)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (!isCurrClient(client)) {
        LOG_INFO("Callback for old ans client; ignoring");
        // Not reporting about the client we care about.
        return;
    }
    LOG_INFO("ANS stopped");
    ASSERT_EQUAL(s_currLocalConnId, (u64)0, FMTu64);
    ASSERT_EQUAL(s_isActive, VPL_FALSE, FMT_VPL_BOOL);
}

static void
setPingPacketCb(ans_client_t *client, char *pingPacket, int pingLength)
{
    LOG_INFO("ANS ping response received");
}

static void
updateDeviceStatus(u64 deviceId, s8 newState, u64 newTimestamp)
{
    ccd::DeviceConnectionStatus newStatus;
    switch (newState) {
    case DEVICE_ONLINE:
        newStatus.set_state(ccd::DEVICE_CONNECTION_ONLINE);
        //newStatus.clear_standby_since();
        break;
    case DEVICE_SLEEPING:
        newStatus.set_state(ccd::DEVICE_CONNECTION_STANDBY);
        newStatus.set_standby_since(newTimestamp);
        break;
    case DEVICE_OFFLINE:
        newStatus.set_state(ccd::DEVICE_CONNECTION_OFFLINE);
        //newStatus.clear_standby_since();
        break;
    default:
        LOG_ERROR("Unexpected state %d for device "FMT_DeviceId, newState, deviceId);
        // Avoid updating.
        return;
    }
    LOG_INFO("Device "FMT_DeviceId" update: %s",
            deviceId, newStatus.ShortDebugString().c_str());
    DeviceStateCache_UpdateDeviceStatus(deviceId, newStatus);
    SyncFeatureMgr_ReportDeviceAvailability(deviceId);
}

static void
receiveDeviceStateCb(ans_client_t *client, ans_unpacked_t *notification)
{
    if (!isCurrClient(client)) {
        LOG_INFO("Callback for old ans client; ignoring");
        // Not reporting about the client we care about.
        return;
    }
    // TODO: It should be nearly impossible to hit, but technically there is a race
    //     condition if a thread gets stopped here for a long long time.
    if (notification->type == SEND_DEVICE_UPDATE) {
        LOG_INFO("Device state update (asyncId="FMTu64", deviceId="FMTu64"):",
                 notification->asyncId, notification->deviceId);
        updateDeviceStatus(notification->deviceId, notification->newDeviceState,
                notification->newDeviceTime);
    } else if (notification->type == SEND_STATE_LIST) {
        LOG_INFO("Update of device states (asyncId="FMTu64", count=%d):",
                notification->asyncId, notification->deviceCount);
        for (int i = 0; i < notification->deviceCount; i++) {
            updateDeviceStatus(notification->deviceList[i], notification->deviceStates[i],
                    notification->deviceTimes[i]);
        }
    } else {
        LOG_WARN("Unexpected receiveDeviceState notification type %d", notification->type);
    }
}

static void
rejectCredentialsCb(ans_client_t *client)
{
    s32 rv;
    u32 activationId;
    u64 sessionHandle;
    std::string iasTicket;

    LOG_INFO("ANS credentials were rejected");

    {
        // If we want to grab the cache lock, we must always do that *before* locking s_mutex,
        // since other code calls into ANSConn_* with the cache lock held.
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            return;
        }
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        if (!isCurrClient(client)) {
            LOG_INFO("Callback for old ans client; ignoring");
            // Not reporting about the client we care about.
            return;
        }
        CachePlayer* user = cache_getSyncUser(VPL_TRUE, s_userId);
        if (user == NULL) {
            LOG_ERROR("Unexpected case!");
            return;
        }
        VPLTime_t now = VPLTime_GetTimeStamp();
        // Only retry once every 30 minutes.
        if (now > user->_ansCredentialReplaceTime) {
            LOG_INFO("Will ask for new ANS credentials");
            user->_ansCredentialReplaceTime = now + VPLTime_FromMinutes(30);
            // Bug 11704: If this happened, it's possible that our IAS session is invalid.
            //   Trigger a call to privUpdateFromVsds() to log out the user from CCD
            //   if the session has been invalidated.
            CacheMonitor_MarkLinkedDevicesDirty(user->user_id());
        } else {
            // If this happens, the infra is probably misconfigured; avoid making things worse.
            LOG_ERROR("We recently tried to get new credentials ("FMT_VPLTime_t" < "FMT_VPLTime_t")",
                    now, user->_ansCredentialReplaceTime);
            return;
        }
        activationId = user->local_activation_id();
        sessionHandle = user->getSession().session_handle();
        iasTicket = user->getSession().ias_ticket();
    }
    // Request new credentials without holding any locks.
    std::string newAnsSessionKey;
    std::string newAnsLoginBlob;
    u32 instanceId;
    rv = Query_GetAnsLoginBlob(sessionHandle, iasTicket, newAnsSessionKey, newAnsLoginBlob, instanceId);
    if (rv < 0) {
        LOG_ERROR("Failed to obtain new ANS login blob: %d", rv);
        return;
    }
    // Update the user data if they are still logged-in.
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            return;
        }
        CachePlayer* user = cache_getUserByActivationId(activationId);
        if (user == NULL) {
            LOG_INFO("User logged out while we were trying to get a new ANS login blob");
            return;
        }
        user->getSession().mutable_ans_session_key()->assign(newAnsSessionKey);
        user->getSession().mutable_ans_login_blob()->assign(newAnsLoginBlob);
        user->getSession().set_instance_id(instanceId);
        user->writeCachedData(true);

        LOG_INFO("Restarting ANSConn with updated credentials");
        // Restart ANSConn with the new credentials.
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        privStop();
        s_ansSessionKey.assign(newAnsSessionKey);
        s_ansLoginBlob.assign(newAnsLoginBlob);
        privStartIgnoreError();
        LOG_INFO("Restarted ANSConn with updated credentials");
    }
}

static void
receiveResponseCb(ans_client_t *client, ans_unpacked_t *response)
{
}

static const ans_callbacks_t callbacks = {
        connectionActiveCb,
        receiveNotificationCb,
        receiveSleepInfoCb,
        receiveDeviceStateCb,
        connectionDownCb,
        connectionClosedCb,
        setPingPacketCb,
        rejectCredentialsCb,
        loginCompletedCb,
        rejectSubscriptionsCb,
        receiveResponseCb
};

static s32 privStart()
{
    if (s_suspend) {
        LOG_INFO("System suspended; delaying ans_open");
    } else {
        LOG_INFO("Calling ans_open.");
        ASSERT(VPLMutex_LockedSelf(VPLLazyInitMutex_GetMutex(&s_mutex)));
        ASSERT(s_client == NULL);

        char ansHostname[HOSTNAME_MAX_LENGTH];
        CCD_GET_ANS_HOSTNAME(ansHostname, s_clusterId);

        ans_open_t  input;
        input.clusterName = ansHostname;
        input.callbacks   = &callbacks;
        input.blob        = s_ansLoginBlob.data();
        input.blobLength  = static_cast<int>(s_ansLoginBlob.size());
        input.key         = s_ansSessionKey.data();
        input.keyLength   = static_cast<int>(s_ansSessionKey.size());
        input.deviceType  = "default";
        input.application = "ccd";
        input.verbose     = VPL_TRUE;
        input.server_tcp_port = __ccdConfig.ansPort;

        s_client = ans_open(&input);
        if (s_client == NULL) {
            LOG_ERROR("failed to malloc or failed to create thread!");
            return CCD_ERROR_INTERNAL;
        }
        // s_client defaults to foreground; change it now if needed.
        if (s_backgroundModeIntervalSec != 0) {
            ans_setForeground(s_client, VPL_FALSE, VPLTime_FromSec(s_backgroundModeIntervalSec));
        }
        if (s_deviceIdsToWatch != NULL) {
            if (!ans_setSubscriptions(s_client, 0, s_deviceIdsToWatch, s_numDeviceIdsToWatch)) {
                LOG_ERROR("Failed to update list of devices to watch!");
                return CCD_ERROR_INTERNAL;
            }
        }
    }
    return VPL_OK;
}

static void privStartIgnoreError()
{
    int temp_rv = privStart();
    if (temp_rv != 0) {
        // TODO: This is bad.  There's no good way to communicate this problem to the app; it
        //       might be best to abort the process.
        LOG_ERROR("Unable to restart ANS connection!");
    }
}

static void privHandleAnsDisconnect()
{
    ASSERT(VPLMutex_LockedSelf(VPLLazyInitMutex_GetMutex(&s_mutex)));

    s_currLocalConnId = 0;
    s_currLocalAddr = VPLNET_ADDR_INVALID;
    s_isActive = VPL_FALSE;
    LOG_INFO("s_isActive = FALSE");

    DeviceStateCache_MarkAllUpdating();
    DeviceStateCache_SetLocalDeviceState(ccd::DEVICE_CONNECTION_OFFLINE);

#if CCD_ENABLE_DOC_SAVE_N_GO
    ADO_Disable_DocSaveNGo(false);
#endif
}

static void privStop()
{
    privHandleAnsDisconnect();

    if (s_client != NULL) {
        ans_client_t* client = s_client;
        s_client = NULL;
        ans_close(client, VPL_FALSE);
    }
}

s32 ANSConn_Start(
        VPLUser_Id_t userId,
        u64 deviceId,
        s64 clusterId,
        const std::string& ansSessionKey,
        const std::string& ansLoginBlob)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));

    if (s_isEnabled) {
        return CCD_ERROR_ALREADY;
    }

    s_userId = userId;
    s_deviceId = deviceId;
    s_clusterId = clusterId;
    s_ansSessionKey.assign(ansSessionKey);
    s_ansLoginBlob.assign(ansLoginBlob);

    s32 rv = privStart();
    if (rv == VPL_OK) {
        s_isEnabled = true;
    }
    return rv;
}

s32 ANSConn_Stop()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));

    LOG_INFO("Stopping ANS");

#if CCD_ENABLE_IOAC
    {
        int temp_rv = NetMan_ClearWakeOnWlanData();
        if (temp_rv != 0) {
            LOG_WARN("%s failed: %d", "NetMan_ClearWakeOnWlanData", temp_rv);
        }
    }
#endif

    privStop();

    if (s_deviceIdsToWatch != NULL) {
        delete[] s_deviceIdsToWatch;
        s_deviceIdsToWatch = NULL;
    }
    s_isEnabled = false;
    return VPL_OK;
}

VPL_BOOL ANSConn_IsActive()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    return s_isActive;
}

VPL_BOOL ANSConn_UpdateListOfDevicesToWatch(const std::vector<u64>& deviceIds)
{
    LOG_INFO("Asking ANS to report status changes for "FMTu_size_t" device(s)", deviceIds.size());
    if (!s_isEnabled) {
        LOG_INFO("ANSConn was stopped; dropping request");
        return VPL_FALSE;
    }
    u64* tempDeviceIds = new u64[deviceIds.size()];
    for (size_t i = 0; i < deviceIds.size(); i++) {
        LOG_INFO("device["FMTu_size_t"]: "FMT_DeviceId, i, deviceIds[i]);
        tempDeviceIds[i] = deviceIds[i];
    }
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (s_deviceIdsToWatch != NULL) {
        delete[] s_deviceIdsToWatch;
    }
    s_deviceIdsToWatch = tempDeviceIds;
    s_numDeviceIdsToWatch = static_cast<int>(deviceIds.size());
    if (s_suspend) {
        LOG_INFO("System suspended; delaying ans_setSubscriptions");
        return VPL_TRUE;
    }
    if (s_client == NULL) {
        // Invariant: s_client must be non-null because (s_isEnabled && !s_suspend)
        FAILED_ASSERT("s_client was null");
        return VPL_FALSE;
    }
    return ans_setSubscriptions(s_client, 0, s_deviceIdsToWatch, s_numDeviceIdsToWatch);
}

VPL_BOOL ANSConn_RequestSleepSetup(int ioacDeviceType, u64 asyncId, const void* macAddr, int macAddrLen)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (s_suspend) {
        LOG_INFO("System suspended; dropping request");
        return VPL_FALSE;
    }
    if (s_client == NULL) {
        LOG_INFO("ANSConn was stopped; dropping request");
        return VPL_FALSE;
    }
    // Invariant: since s_client is non-null, (s_isEnabled && !s_suspend).
    ASSERT(s_isEnabled);
    VPL_BOOL result = ans_requestSleepSetup(s_client, asyncId, ioacDeviceType, macAddr, macAddrLen);
    if (!result) {
        LOG_WARN("ans_requestSleepSetup failed");
    }
    return result;
}

s32 ANSConn_WakeDevice(u64 deviceId)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (s_suspend) {
        LOG_INFO("System suspended; dropping request");
        return CCD_ERROR_LOCAL_DEVICE_OFFLINE;
    }
    if (s_client == NULL) {
        LOG_INFO("ANSConn was stopped; dropping request");
        return CCD_ERROR_LOCAL_DEVICE_OFFLINE;
    }
    // Invariant: since s_client is non-null, (s_isEnabled && !s_suspend).
    ASSERT(s_isEnabled);
    return ans_requestWakeup(s_client, deviceId) ? 0 : CCD_ERROR_ANS_FAILED;
}

void ANSConn_GoingToSleepCb()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (s_suspend) {
        LOG_ERROR("Already suspended; ignoring");
        return;
    }
    s_suspend = VPL_TRUE;

    LOG_INFO("Suspending ANS");
    privStop();
}

void ANSConn_ResumeFromSleepCb()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (!s_suspend) {
        // We will get PBT_APMRESUMEAUTOMATIC and PBT_APMRESUMESUSPEND, but not always in that
        // order.  Ignore whichever one comes second.
        LOG_INFO("Not currently suspended; ignoring");
        return;
    }
    s_suspend = VPL_FALSE;
    if (s_isEnabled) {
        LOG_INFO("Resuming ANS");
        privStartIgnoreError();
    }
}

void ANSConn_ReportNetworkConnected()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (s_suspend) {
        LOG_INFO("System suspended; ignoring network connected");
        return;
    }
    if (s_client == NULL) {
        LOG_INFO("ANSConn was stopped; ignoring network connected");
        return;
    }
    // Invariant: since s_client is non-null, (s_isEnabled && !s_suspend).
    ASSERT(s_isEnabled);
    LOG_INFO("Calling ans_onNetworkConnected");
    ans_onNetworkConnected(s_client);
}

void ANSConn_SetForegroundMode(int backgroundModeIntervalSec)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (s_backgroundModeIntervalSec == backgroundModeIntervalSec) {
        return;
    }
    s_backgroundModeIntervalSec = backgroundModeIntervalSec;
    if (s_suspend) {
        LOG_INFO("[POWER]System suspended; will switch to background interval "
                 "%d on next ans_open",
                 backgroundModeIntervalSec);
        return;
    }
    if (s_client == NULL) {
        LOG_INFO("[POWER]ANSConn was stopped; will switch to background "
                 "interval %d on next ans_open",
                 backgroundModeIntervalSec);
        return;
    }
    // Invariant: since s_client is non-null, (s_isEnabled && !s_suspend).
    ASSERT(s_isEnabled);
    LOG_INFO("[POWER]Calling ans_setForeground, backgroundModeIntervalSec=%d",
             backgroundModeIntervalSec);
    ans_setForeground(s_client, (backgroundModeIntervalSec == 0), VPLTime_FromSec(backgroundModeIntervalSec));
}

void ANSConn_PerformBackgroundTasks()
{
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
        if (s_backgroundModeIntervalSec < 1) {
            LOG_WARN("ANSConn_PerformBackgroundTasks was called when in foreground mode");
        }
        if (s_suspend) {
            LOG_INFO("System suspended; ignoring background task request");
            return;
        }
        if (s_client == NULL) {
            LOG_INFO("ANSConn was stopped; ignoring background task request");
            return;
        }
        // Invariant: since s_client is non-null, (s_isEnabled && !s_suspend).
        ASSERT(s_isEnabled);
        LOG_INFO("Calling ans_background");
        ans_background(s_client);
    } // releases lock
    LOG_INFO("Back from ans_background");

    // TODO: return as soon as ANS has finished using the CPU (for doing retransmits, etc).
    VPLThread_Sleep(VPLTime_FromSec(3));
}

int ANSConn_SendRemoteSwUpdateRequest(u64 userId, u64 deviceId)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (s_suspend) {
        LOG_INFO("System suspended; dropping request");
        return CCD_ERROR_LOCAL_DEVICE_OFFLINE;
    }
    if (s_client == NULL) {
        LOG_INFO("ANSConn was stopped; dropping request");
        return CCD_ERROR_LOCAL_DEVICE_OFFLINE;
    }
    // Invariant: since s_client is non-null, (s_isEnabled && !s_suspend).
    ASSERT(s_isEnabled);
    vplex::ans::UserDeviceMsg_t payload;
    payload.set_srcdeviceid(s_deviceId);
    payload.set_msgtype(vplex::ans::REMOTE_SW_UPDATE_REQ);
    int msgLen = 1 + payload.ByteSize();
    {
        char* msg = (char*)malloc(msgLen);
        if (msg == NULL) {
            LOG_ERROR("Fail to allocate memory");
            return VPL_ERR_NOMEM;
        }
        ON_BLOCK_EXIT(free, msg);
        msg[0] = vplex::ans::ANS_TYPE_USER_DEVICE_MESSAGE;
        payload.SerializeToArray(&msg[1], payload.ByteSize());
        if (!ans_sendUnicast(s_client, userId, deviceId, msg, msgLen, 0)) {
            LOG_ERROR("ans_sendUnicast fail");
            return REMOTE_SWU_ERR_SEND_NOTIFICATION_FAIL;
        }
    }
    return VPL_OK;
}

#endif
