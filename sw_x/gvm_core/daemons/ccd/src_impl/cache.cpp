//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "cache.h"

#include "ans_connection.hpp"
#include "AsyncDatasetOps.hpp"
#include "ccd_core.h"
#include "ccd_core_priv.hpp"
#include "ccd_features.h"
#include "ccd_storage.hpp"
#include "ccd_util.hpp"
#if CCD_ENABLE_DOC_SAVE_N_GO
#include "CloudDocMgr.hpp"
#endif
#include "DeviceStateCache.hpp"
#include "EventManagerPb.hpp"
#include "LocalInfo_Ccd.hpp"
#include "McaThumbMigrate.hpp"
#include "MediaMetadata.hpp"
#if CCD_ENABLE_STORAGE_NODE
#include "MediaMetadataServer.hpp"
#endif
#include "NotificationQ.hpp"
#include "picstream.hpp"
#include "query.h"
#include "scopeguard.hpp"
#include "HttpService.hpp"
#if CCD_ENABLE_SYNCDOWN
#include "SyncDown.hpp"
#endif
#include "SyncFeatureMgr.hpp"
#if CCD_ENABLE_SYNCUP
#include "SyncUp.hpp"
#endif
#include "StatManager.hpp"
#include "virtual_device.hpp"
#include "vsds_query.hpp"
#include "vpl_fs.h"
#include "vpl_lazy_init.h"
#include "vplex_strings.h"
#include "vplex_file.h"
#include "vpl_time.h"
#include "vplu_sstr.hpp"
#include <memory>
#include <errno.h>
#include "netman.hpp"

#include "config.h"
#include "gvm_utils.h"
#include "gvm_rm_utils.hpp"
#include "protobuf_file_reader.hpp"
#include "protobuf_file_writer.hpp"
#include "vplex_shared_object.h"
#include "ccd_build_info.h"
#include "pin_manager.hpp"

#if CCD_ENABLE_STORAGE_NODE
#    include "vss_server.hpp"
#    include "executable_manager.hpp"
#endif

using namespace std;
using namespace ccd;
using namespace vplex::syncagent;

static s32 Cache_ActivateUserNewLogin(VPLUser_Id_t userId);
static s32 Cache_ActivateUserCcdRestart(VPLUser_Id_t userId);
static void Cache_DeactivateUserForLogout(u64 sessionHandle, LogoutReason_t reason);
static void Cache_DeactivateUserForCcdStop(VPLUser_Id_t userId);

static s32 startModulesForUser(u64 userId);
static s32 stopModulesForUser(u64 userId, bool userLogout);

enum ActivateOrDeactivateAction {
    ACTIVATE_FOR_NEW_LOGIN = 1,
    ACTIVATE_FOR_CCD_RESTART,
    DEACTIVATE_FOR_LOGOUT,
    DEACTIVATE_FOR_CCD_STOP,
};
static bool isDeactivate(ActivateOrDeactivateAction action)
{
    return (action == DEACTIVATE_FOR_LOGOUT) || (action == DEACTIVATE_FOR_CCD_STOP);
}
static s32 Cache_ActivateOrDeactivateUser(VPLUser_Id_t userId, ActivateOrDeactivateAction action);

#ifndef VPL_PLAT_IS_WIN_DESKTOP_MODE
static s32 privSetForegroundMode(int intervalToUse);
#endif  //VPL_PLAT_IS_WIN_DESKTOP_MODE

static u32 s_ccdLocalActivationCount = 0;

enum PowerMode {
    S_POWER_NOT_INIT = 0,
    S_POWER_NO_SYNC,
    S_POWER_FOREGROUND,
    S_POWER_BACKGROUND
};

class CCDCache
{
public:
    /// Lock for this entire structure.
    /// Writes should be infrequent enough that a single lock is sufficient.
    RWLock rwLock;

    /// CCD-wide state that will be persisted to disk.
    ccd::CCDMainState mainState;

#if CCD_ENABLE_USER_SUMMARIES
    /// Cached list of users that have logged in at some point; not
    /// necessarily currently active (signed-in).
    /// Can be refreshed by reading from the disk.
    ccd::CachedUserSummaryList userList;
#endif

    /// Array of active (signed-in) players.
    /// Number of elements is equal to #CCDState_t.maxPlayers.
    // TODO: switch to _players and provide an accessor that checks isValid.
    CachePlayer* players;

    bool deprecatedNetworkDisable;

    /// Identifier of the app that last requested foreground mode.
    /// Empty indicates that CCD is allowed to be in background mode.
    std::string foregroundAppId;
    ccd::CcdApp_t foregroundAppType;
    int requestedBgModeIntervalSec;

    /// Indicates whether there is only mobile network available.  No need to
    /// persist this because the Android java-side service would set this
    /// every time on startup, and would crash along-side ccd if ccd ever
    /// crashes.
    bool only_mobile_network_available;

    /// If an application is intending to do streaming, this should be set to
    /// true in order to prevent the system from going into NO_SYNC mode.
    /// In NO_SYNC mode, streaming is not possible.
    bool stream_power_mode;

    PowerMode powerMode;
};

static CCDCache s_ccdCache;
static bool s_ccdCacheIsInit = false;

static CCDCache& __cache()
{
    return s_ccdCache;
}

//---------------------------------------------------

#ifndef _MSC_VER
#include <pthread.h>

// Incremented on lock.
// Decremented on unlock.
// These *must* be thread local.
// When __holdsWriteLock > 0, some read locks may be counted within
// __holdsWriteLock instead of __holdsReadLock.  This doesn't change
// the behavior though.
struct ThreadLocalLocks {
    int holdsCacheReadLock;
    int holdsCacheWriteLock;

    ThreadLocalLocks() :
        holdsCacheReadLock(0),
        holdsCacheWriteLock(0)
    {}
};

static pthread_key_t threadLocalLocksKey;
static pthread_once_t threadLocalLocksKey_once = PTHREAD_ONCE_INIT;

static void destructLocks(void* locks)
{
    delete((ThreadLocalLocks*)locks);
}

static void makeGlobalKey()
{
    int rc = pthread_key_create(&threadLocalLocksKey, destructLocks);
    if (rc != 0) {
        LOG_CRITICAL("pthread_key_create failed: %d", rc);
    }
}

static ThreadLocalLocks* getThreadLocks()
{
    int rc = pthread_once(&threadLocalLocksKey_once, makeGlobalKey);
    if (rc != 0) {
        LOG_CRITICAL("pthread_once failed: %d", rc);
    }
    ThreadLocalLocks* ptr = (ThreadLocalLocks*)pthread_getspecific(threadLocalLocksKey);
    if (ptr == NULL) {
        ptr = new ThreadLocalLocks();
        rc = pthread_setspecific(threadLocalLocksKey, ptr);
        if (rc != 0) {
            LOG_CRITICAL("pthread_setspecific failed: %d", rc);
        }
    }
    return ptr;
}

#else

// Incremented on lock.
// Decremented on unlock.
// These *must* be thread local.
// When __holdsWriteLock > 0, some read locks may be counted within
// __holdsWriteLock instead of __holdsReadLock.  This doesn't change
// the behavior though.
struct ThreadLocalLocks {
    int holdsCacheReadLock;
    int holdsCacheWriteLock;

    // NOTE: MSVC does not allow a constructor for thread local objects.
};

// "__thread" is not implemented for Android, so we need the above work-around with pthread_key_t
// (and annoyingly, we get no warning about this; the variables simply misbehave at run-time).
// This also appears to not work for mingw.  I'm not sure about Linux yet.
static VPL_THREAD_LOCAL ThreadLocalLocks tl_locks = {0,0};

static ThreadLocalLocks* getThreadLocks()
{
    return &tl_locks;
}

#endif

s32 CacheAutoLock::LockForRead()
{
    s32 rv = Cache_Lock();
    if (rv == 0) {
        needsToUnlock = true;
    }
    return rv;
}


s32 CacheAutoLock::LockForWrite()
{
    s32 rv = Cache_LockWrite();
    if (rv == 0) {
        needsToUnlock = true;
    }
    return rv;
}

void CacheAutoLock::UnlockNow()
{
    if (needsToUnlock) {
        Cache_Unlock();
        needsToUnlock = false;
    }
}

VPL_BOOL
Cache_ThreadHasLock()
{
    ThreadLocalLocks* locks = getThreadLocks();
    return RWLock::threadHasLock(locks->holdsCacheReadLock, locks->holdsCacheWriteLock);
}

VPL_BOOL
Cache_ThreadHasWriteLock()
{
    ThreadLocalLocks* locks = getThreadLocks();
    return RWLock::threadHasWriteLock(locks->holdsCacheReadLock, locks->holdsCacheWriteLock);
}

/// Should only be called by #Cache_Init() and #Cache_LockWrite().
static s32
cache_uncheckedLockWrite(void)
{
    ThreadLocalLocks* locks = getThreadLocks();
    return __cache().rwLock.lockWrite(locks->holdsCacheReadLock, locks->holdsCacheWriteLock);
}

/// Should only be called by #Cache_Quit() and #Cache_Unlock().
static s32
cache_uncheckedUnlock(void)
{
    ThreadLocalLocks* locks = getThreadLocks();
    return __cache().rwLock.unlock(locks->holdsCacheReadLock, locks->holdsCacheWriteLock);
}

s32
Cache_Lock(void)
{
    ThreadLocalLocks* locks = getThreadLocks();
    s32 rv = __cache().rwLock.lock(locks->holdsCacheReadLock, locks->holdsCacheWriteLock);
    if ((rv == 0) && !s_ccdCacheIsInit) {
        rv = CCD_ERROR_SHUTTING_DOWN;
        cache_uncheckedUnlock();
    }
    return rv;
}

s32
Cache_LockWrite(void)
{
    s32 rv = cache_uncheckedLockWrite();
    if ((rv == 0) && !s_ccdCacheIsInit) {
        rv = CCD_ERROR_SHUTTING_DOWN;
        cache_uncheckedUnlock();
    }
    return rv;
}

s32
Cache_Unlock(void)
{
    ASSERT(s_ccdCacheIsInit);
    return cache_uncheckedUnlock();
}

//---------------------------------------------------

static VPL_BOOL
isPlayerIndexValid(int playerIndex)
{
    return (playerIndex >= 0) && (playerIndex < CCD_MAX_USERS);
}

CachePlayer*
cache_getTheUser()
{
    ASSERT(Cache_ThreadHasLock());
    if (!__cache().players[0].isSignedIn()) {
        return NULL;
    }
    return &(__cache().players[0]);
}

CachePlayer*
cache_getUserByPlayerIndex(int playerIndex)
{
    ASSERT(Cache_ThreadHasLock());

    if (!isPlayerIndexValid(playerIndex)) {
        return NULL;
    }
    if (!__cache().players[playerIndex].isSignedIn()) {
        return NULL;
    }
    return &(__cache().players[playerIndex]);
}

CachePlayer*
cache_getUserByUserId(VPLUser_Id_t userId)
{
    ASSERT(Cache_ThreadHasLock());

    for (int i = 0; i < CCD_MAX_USERS; i++) {
        CachePlayer* player = &__cache().players[i];
        if (player->isSignedIn() && (player->user_id() == userId)) {
            return player;
        }
    }
    return NULL;
}

CachePlayer*
cache_getUserByActivationId(u32 activationId)
{
    ASSERT(Cache_ThreadHasLock());

    for (int i = 0; i < CCD_MAX_USERS; i++) {
        CachePlayer* player = &__cache().players[i];
        if (player->isSignedIn() && (player->local_activation_id() == activationId)) {
            return player;
        }
    }
    return NULL;
}

s32
cache_getUser(int playerIndex, VPLUser_Id_t userId, VPL_BOOL warnIfNotSignedIn, CachePlayer** user_out)
{
    CachePlayer* user;
    if (playerIndex != CACHE_PLAYER_INDEX_NOT_SIGNED_IN) {
        user = cache_getUserByPlayerIndex(playerIndex);
        if ((userId != 0) && (user != NULL) && (user->user_id() != userId)) {
            return CCD_ERROR_WRONG_USER_ID;
        }
    } else if (userId != VPLUSER_ID_NONE) {
        user = cache_getUserByUserId(userId);
    } else {
        LOG_ERROR("Must specify either playerIndex or userId");
        return CCD_ERROR_PARAMETER;
    }
    if (user == NULL) {
        if (warnIfNotSignedIn) {
            if (playerIndex != CACHE_PLAYER_INDEX_NOT_SIGNED_IN) {
                LOG_WARN("playerIndex %d not signed in", playerIndex);
            } else {
                LOG_WARN("userId "FMT_VPLUser_Id_t" not signed in", userId);
            }
        }
        return CCD_ERROR_NOT_SIGNED_IN;
    }
    *user_out = user;
    return CCD_OK;
}

CachePlayer* cache_getSyncUser(bool useUserId, VPLUser_Id_t userId)
{
    CachePlayer* user;
    if (useUserId) {
        user = cache_getUserByUserId(userId);
        if (user == NULL) {
            LOG_WARN("userId "FMT_VPLUser_Id_t" not signed in", userId);
        }
    } else {
        user = cache_getUserByPlayerIndex(0);
        if (user == NULL) {
            LOG_WARN("no user logged in");
        }
    }
    return user;
}

static CachePlayer*
getSignedInUserByUsername(const char* username)
{
    ASSERT(Cache_ThreadHasLock());

    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    for (int i = 0; i < CCD_MAX_USERS; i++) {
        CachePlayer* player = &__cache().players[i];
        if (player->isSignedIn() && (player->username().compare(username) == 0)) {
            return player;
        }
    }
    return NULL;
}

//---------------------------------------------------

#if CCD_ENABLE_USER_SUMMARIES
/// Read each cached user's userId, username and profile picture.
/// This operation blocks, waiting for the disk, and the cache is
/// completely locked during this time.
static s32
Cache_UpdateUserSummaries(void)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);

    s32 rv = 0;
    VPLFS_dir_t directory;
    VPLFS_dirent_t entry;

    char dirPath[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForAllCachedUsers(sizeof(dirPath), dirPath);

    rv = VPLFS_Opendir(dirPath, &directory);
    if (rv != VPL_OK) {
        LOG_ERROR("VPLFS_Opendir %s failed: %d", dirPath, rv);
        rv = CCD_ERROR_OPENDIR;
        goto failed_opendir;
    }

    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }

        __cache().userList.Clear();

        while ((rv = VPLFS_Readdir(&directory, &entry)) == VPL_OK) {
            if (strcmp(entry.filename, "..") == 0
                || strcmp(entry.filename, ".") == 0) {
                continue;
            }

            VPLUser_Id_t userId = Util_ParseStrictUserId(entry.filename);
            if (userId == VPLUSER_ID_NONE) {
                LOG_INFO("Not a valid userId: \"%s\"; skipping.", entry.filename);
            } else {
                CachedUserSummary* currUser = __cache().userList.add_users();
                rv = CachePlayer::readCachedUserData(userId, *currUser, NULL);
                if (rv < 0) {
                    LOG_WARN("Failed to read user %s from cache; skipping.", entry.filename);
                    // Must clean up failed element.
                    __cache().userList.mutable_users()->RemoveLast();
                    // Just ignore the invalid summary file.
                    rv = 0;
                }
            }
        }
        if (rv == VPL_ERR_MAX) {
            // This is the expected case.
            rv = 0;
        }
    }

out:
    VPLFS_Closedir(&directory);
failed_opendir:
    return rv;
}

static const CachedUserSummary*
Cache_getCachedUserSummary(const char* username)
{
    CacheAutoLock lock;
    s32 rv = lock.LockForRead();
    if (rv < 0) {
        LOG_ERROR("Failed to obtain lock: %d", rv);
        return NULL;
    }

    for (int i = 0; i < __cache().userList.users_size(); i++) {
        if (__cache().userList.users(i).username().compare(username) == 0) {
            return &__cache().userList.users(i);
        }
    }
    return NULL;
}
#endif

//---------------------------------------------------

#define CCD_CACHED_USER_DATA_VERSION 1

#define CCD_CACHED_MAIN_DATA_VERSION 1

// static method
s32 CachePlayer::readCachedUserData(VPLUser_Id_t userId,
        ccd::CachedUserSummary& summary_out,
        ccd::CachedUserDetails* details_out)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);

    s32 rv;
    {
        char filename[CCD_PATH_MAX_LENGTH];

        summary_out.Clear();

        DiskCache::getPathForUserCacheFile(userId, sizeof(filename), filename);
        ProtobufFileReader reader;
        rv = reader.open(filename);
        if (rv < 0) {
            LOG_ERROR("reader.open(%s) returned %d", filename, rv);
            goto out;
        }
        LOG_DEBUG("Parsing from %s", filename);
        google::protobuf::io::CodedInputStream tempStream(reader.getInputStream());
        {
            u32 version;
            if (!tempStream.ReadVarint32(&version)) {
                LOG_ERROR("Failed to read version from %s", filename);
                rv = CCD_ERROR_PARSE_CONTENT;
                goto out;
            }
            if (version != CCD_CACHED_USER_DATA_VERSION) {
                LOG_ERROR("Unsupported version ("FMTu32") read from %s", version, filename);
                rv = CCD_ERROR_USER_DATA_VERSION;
                goto out;
            }
        }
        {
            u32 summarySize;
            if (!tempStream.ReadVarint32(&summarySize)) {
                LOG_ERROR("Failed to read summary size from %s", filename);
                rv = CCD_ERROR_PARSE_CONTENT;
                goto out;
            }
            google::protobuf::io::CodedInputStream::Limit limit =
                    tempStream.PushLimit(static_cast<int>(summarySize));
            if (!summary_out.ParseFromCodedStream(&tempStream)) {
                LOG_ERROR("Failed to parse summary from %s", filename);
                rv = CCD_ERROR_PARSE_CONTENT;
                goto out;
            }
            tempStream.PopLimit(limit);

            if (summary_out.user_id() != userId) {
                LOG_ERROR("Serialized userId "FMT_VPLUser_Id_t" doesn't match filename \"%s\"", summary_out.user_id(), filename);
                rv = CCD_ERROR_PARSE_CONTENT;
                goto out;
            }
        }
        LOG_DEBUG("Success parsing summary from %s", filename);

        if (details_out != NULL) {
            details_out->Clear();
            if (!details_out->ParsePartialFromCodedStream(&tempStream)) {
                LOG_ERROR("Failed to parse details from %s", filename);
                rv = CCD_ERROR_PARSE_CONTENT;
                goto out;
            }
            if (!details_out->IsInitialized()) {
                LOG_WARN("Possible problem parsing details from %s: %s",
                        filename, details_out->InitializationErrorString().c_str());
                // TODO: can enable/disable stricter checking:
                //rv = CCD_ERROR_PARSE_CONTENT;
                //goto out;
            }

            // Cleanup any obsolete fields.
            details_out->DiscardUnknownFields();

            LOG_DEBUG("Success parsing details from %s", filename);
        }
    }
out:
    return rv;
}

s32 CachePlayer::writeCachedData(bool isUserCredChange)
{
    s32 rv;

    LOG_INFO("Begin writing cache file for user "FMT_VPLUser_Id_t, user_id());

    // We need to be tolerant of crashing or powering-off at any time; we
    // need to create a temporary file when writing the data and atomically
    // rename it to the real location.
    char tempFilename[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForUserCacheFileTemp(user_id(), sizeof(tempFilename), tempFilename);

    if (!_cachedData.summary().has_username()) {
        LOG_ERROR("Attempting to write invalid user data");
        rv = CCD_ERROR_WRONG_STATE;
        goto out;
    }

    {
        ProtobufFileWriter writer;
        rv = writer.open(tempFilename, CCD_NEW_FILE_MODE);
        if (rv < 0) {
            LOG_ERROR("writer.open(%s) returned %d", tempFilename, rv);
            goto out;
        }
        LOG_DEBUG("Serializing to %s", tempFilename);
        google::protobuf::io::CodedOutputStream tempStream(writer.getOutputStream());
        {
            u32 version = CCD_CACHED_USER_DATA_VERSION;
            tempStream.WriteVarint32(version);
            if (tempStream.HadError()) {
                LOG_ERROR("Failed to write version to %s", tempFilename);
                rv = CCD_ERROR_DISK_SERIALIZE;
                goto out;
            }
        }
        {
            u32 summarySize = static_cast<u32>(_cachedData.summary().ByteSize());
            tempStream.WriteVarint32(summarySize);
            if (tempStream.HadError()) {
                LOG_ERROR("Failed to write summary size to %s", tempFilename);
                rv = CCD_ERROR_DISK_SERIALIZE;
                goto out;
            }
        }
        if (!_cachedData.summary().SerializeToCodedStream(&tempStream)) {
            LOG_ERROR("Failed to write summary to %s", tempFilename);
            rv = CCD_ERROR_DISK_SERIALIZE;
            goto out;
        }
        if (!_cachedData.details().SerializeToCodedStream(&tempStream)) {
            LOG_ERROR("Serializing to %s failed", tempFilename);
            rv = CCD_ERROR_DISK_SERIALIZE;
            goto out;
        }
        LOG_DEBUG("Success serializing to %s", tempFilename);
    } // Destroying the ProtobufFileWriter closes the file.

    // Make sure to close the file before renaming it.
    {
        char finalFilename[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForUserCacheFile(user_id(), sizeof(finalFilename), finalFilename);
        rv = VPLFile_Rename(tempFilename, finalFilename);
        if (rv < 0) {
            LOG_ERROR("VPLFile_Rename(\"%s\", \"%s\") failed: %d", tempFilename, finalFilename, rv);
            goto out;
        }
        
        // Post an event if it is an user credentials update.
        if (isUserCredChange) {
            ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
            ccdiEvent->mutable_user_cred_change()->set_local_file_path(finalFilename);
            EventManagerPb_AddEvent(ccdiEvent);
        }
    }
    LOG_INFO("Finished writing cache file for user "FMT_VPLUser_Id_t, user_id());

#if CCD_ENABLE_USER_SUMMARIES
    // In case the summary changed, reload it.
    // TODO: be more efficient here
    rv = Cache_UpdateUserSummaries();
    if (rv < 0) {
        LOG_ERROR("Cache_UpdateUserSummaries() returned %d", rv);
        goto out;
    }
#endif

out:
    return rv;
}

//-----------------------------------------

/// Can be used to:
/// - Sign-in a user, loading their data from the disk cache.
/// - Update a currently signed-in user with his/her data from the disk cache.
static s32
Cache_ReadUserFromDisk(int playerIndex, VPLUser_Id_t userId)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;

    if (!isPlayerIndexValid(playerIndex)) {
        LOG_ERROR("playerIndex=%d", playerIndex);
        rv = CCD_ERROR_PARAMETER;
        goto out;
    }

    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }

        CachePlayer* user = &__cache().players[playerIndex];

        // If this player slot is already signed-in, make sure we are simply
        // updating it with the same user.
        if (user->isSignedIn() && (user->user_id() != userId)) {
            LOG_ERROR("playerIndex %d is occupied by \"%s\"", playerIndex, user->username().c_str());
            rv = CCD_ERROR_SIGNED_IN;
            goto out;
        }

        rv = user->readCachedData(userId);
        if (rv < 0) {
            LOG_ERROR("Failed to read from disk cache, %d", rv);
            user->set_player_index(CACHE_PLAYER_INDEX_NOT_SIGNED_IN);
            goto out;
        }
        user->set_player_index(playerIndex);
    }

out:
    return rv;
}

//---------------------------------------------------

s32
Cache_GetAnyActiveSession(ccd::UserSession& anySession_out)
{
    CacheAutoLock lock;
    s32 rv = lock.LockForRead();
    if (rv < 0) {
        LOG_ERROR("Failed to obtain lock");
        return rv;
    }
    for (int i = 0; i < CCD_MAX_USERS; i++) {
        CachePlayer* player = &__cache().players[i];
        if (player->isSignedIn()) {
            anySession_out = player->getSession();
            return CCD_OK;
        }
    }
    return CCD_ERROR_NOT_SIGNED_IN;
}

s32
Cache_GetMaxPlayers(void)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    return CCD_MAX_USERS;
}

s32
Cache_IsSignedIn(
        int playerIndex,
        bool* out)
{
    CacheAutoLock lock;
    s32 rv = lock.LockForRead();
    if (rv < 0) {
        LOG_ERROR("Failed to obtain lock");
        goto out;
    }
    *out = (isPlayerIndexValid(playerIndex) &&
            __cache().players[playerIndex].isSignedIn());
out:
    return rv;
}

#if CCD_ENABLE_STORAGE_NODE
void Cache_GoingToSuspend()
{
    MutexAutoLock lock(LocalServers_GetMutex());
    vss_server* sn = LocalServers_getStorageNode();
    if (sn != NULL) {
        sn->disconnectAllClients();
    }
}
#endif

s32 Cache_GetSessionByActivationId(
        u32 activationId,
        ccd::UserSession& session_out)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;
    CachePlayer* user;

    CacheAutoLock autoLock;
    rv = autoLock.LockForRead();
    if (rv < 0) {
        LOG_ERROR("Failed to obtain lock");
        goto out;
    }

    user = cache_getUserByActivationId(activationId);
    if (user == NULL) {
        LOG_ERROR("activationId "FMTu32" no longer valid", activationId);
        rv = CCD_ERROR_NOT_SIGNED_IN;
        goto out;
    }
    session_out = user->getSession();
out:
    return rv;
}

// Prefer Cache_GetSessionByActivationId above
s32 Cache_GetSessionByUser(
        VPLUser_Id_t userId,
        ccd::UserSession& session_out)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;
    CachePlayer* user;

    CacheAutoLock autoLock;
    rv = autoLock.LockForRead();
    if (rv < 0) {
        LOG_ERROR("Failed to obtain lock");
        goto out;
    }

    user = cache_getUserByUserId(userId);
    if (user == NULL) {
        LOG_ERROR("userId "FMT_VPLUser_Id_t" not signed in", userId);
        rv = CCD_ERROR_NOT_SIGNED_IN;
        goto out;
    }
    session_out = user->getSession();
out:
    return rv;
}

s32
Cache_GetSessionForVsdsCommon(
        const VPLUser_Id_t* userId,
        int playerIndex,
        ServiceSessionInfo_t& sessionInfo)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;
    CachePlayer* user;

    CacheAutoLock autoLock;
    rv = autoLock.LockForRead();
    if (rv < 0) {
        LOG_ERROR("Failed to obtain lock");
        goto out;
    }

    if (userId != NULL) {
        user = cache_getUserByUserId(*userId);
        if (user == NULL) {
            LOG_ERROR("user "FMT_VPLUser_Id_t" not signed-in", *userId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }
    } else {
        user = cache_getUserByPlayerIndex(playerIndex);
        if (user == NULL) {
            LOG_ERROR("playerIndex %d not signed-in", playerIndex);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }
    }
    sessionInfo.sessionHandle = user->getSession().session_handle();
    sessionInfo.serviceTicket = user->getSession().vs_ticket();
out:
    return rv;
}

s32
Cache_GetAnySignedInPlayer(void)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;

    CacheAutoLock autoLock;
    rv = autoLock.LockForRead();
    if (rv < 0) {
        LOG_ERROR("Failed to obtain lock");
        goto out;
    }
    for (int i = 0; i < CCD_MAX_USERS; i++) {
        if (__cache().players[i].isSignedIn()) {
            rv = i;
            goto out;
        }
    }
    rv = CCD_ERROR_NOT_SIGNED_IN;
out:
    return rv;
}

#if CCD_ENABLE_USER_SUMMARIES
s32
Cache_GetCachedUserSummaries(CachedUserSummaryList& summaries_out)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    int rv;

    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        summaries_out = __cache().userList;

        rv = CCD_OK;
    }

out:
    return rv;
}
#endif

static s32
cache_writeMainState(bool isUserStateChange)
{
    s32 rv;

    LOG_INFO("Begin writing CCD main state");

    // We need to be tolerant of crashing or powering-off at any time; we
    // need to create a temporary file when writing the data and atomically
    // rename it to the real location.
    char tempFilename[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForCcdMainStateTemp(sizeof(tempFilename), tempFilename);

    {
        ProtobufFileWriter writer;
        rv = writer.open(tempFilename, CCD_NEW_FILE_MODE);
        if (rv < 0) {
            LOG_ERROR("writer.open(%s) returned %d", tempFilename, rv);
            goto out;
        }
        LOG_DEBUG("Serializing to %s", tempFilename);
        google::protobuf::io::CodedOutputStream tempStream(writer.getOutputStream());
        {
            u32 version = CCD_CACHED_MAIN_DATA_VERSION;
            tempStream.WriteVarint32(version);
            if (tempStream.HadError()) {
                LOG_ERROR("Failed to write version to %s", tempFilename);
                rv = CCD_ERROR_DISK_SERIALIZE;
                goto out;
            }
        }
        if (!__cache().mainState.SerializeToCodedStream(&tempStream)) {
            LOG_ERROR("Serializing to %s failed", tempFilename);
            rv = CCD_ERROR_DISK_SERIALIZE;
            goto out;
        }
        LOG_DEBUG("Success serializing to %s: %s", tempFilename,
                __cache().mainState.ShortDebugString().c_str());
    } // Destroying the ProtobufFileWriter closes the file.

    // Make sure to close the file before renaming it.
    {
        char finalFilename[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForCcdMainState(sizeof(finalFilename), finalFilename);
        rv = VPLFile_Rename(tempFilename, finalFilename);
        if (rv < 0) {
            LOG_ERROR("VPLFile_Rename(\"%s\", \"%s\") failed: %d", tempFilename, finalFilename, rv);
            goto out;
        }
        
        // Post an event if it is an user credentials update.
        // Since CCD main state also stores the current logged in user ID,
        // we take it as part of the user credentials.
        if (isUserStateChange) {
            ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
            ccdiEvent->mutable_user_cred_change()->set_local_file_path(finalFilename);
            EventManagerPb_AddEvent(ccdiEvent);
        }
    }
    LOG_INFO("Finished writing CCD main state");
out:
    return rv;
}

int Cache_GetActivationIdForSyncUser(u32* activationId_out, bool useUserId, VPLUser_Id_t userId)
{
    int rv;
    *activationId_out = ACTIVATION_ID_NONE;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv != 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        CachePlayer* user = cache_getSyncUser(useUserId, userId);
        if (user == NULL) {
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }
        *activationId_out = user->local_activation_id();
    }
out:
    return rv;
}

static void
cache_setupMmThumbSync(CachePlayer& user)
{
    ASSERT(Cache_ThreadHasLock());

    // Create list of thumbnail types for which syncing is disabled by default
    user._cachedData.mutable_details()->clear_mm_thumb_sync_disabled_types();
#if CCD_MOBILE_DEVICE_CONSERVE_STORAGE
    // For mobile platforms, save space by not syncing photo thumbnails by default
    LOG_INFO("Disabling photo thumbnail syncing for mobile device");
    user._cachedData.mutable_details()->add_mm_thumb_sync_disabled_types(SYNC_FEATURE_PHOTO_THUMBNAILS);
    // Delete existing cache of photo thumbnails
    cache_clearMMCacheByType(user, SYNC_FEATURE_PHOTO_THUMBNAILS);
#endif // CCD_MOBILE_DEVICE_CONSERVE_STORAGE
    user._cachedData.mutable_details()->set_mm_thumb_sync_converted(true);
}

/// For the following cases:
/// 1. (YES) Login via CCDILogin.
/// 2. (YES) Login via shared credentials.
/// 3. (NO) Reactivate user at CCD start.
static s32
privWriteUserCacheFileAfterLogin(UserSession& session, const char* username,
        const vplex::ias::LoginResponseType& iasOutput)
{
    s32 rv = CCD_OK;
    {
        VPLUser_Id_t userId = iasOutput.userid();
        string accountId = iasOutput.accountid();
        s64 clusterId = iasOutput.storageclusterid();

        // We don't want to upload pictures that were taken while player
        // was not logged in.
        {
            int rc;
            char* internalPicDir = (char*)malloc(CCD_PATH_MAX_LENGTH);
            if(internalPicDir == NULL) {
                LOG_ERROR("Out of mem");
                goto out;
            }
            ON_BLOCK_EXIT(free, internalPicDir);
            internalPicDir[0] = '\0';
            DiskCache::getPathForUserUploadPicstream(userId,
                                                     CCD_PATH_MAX_LENGTH,
                                                     internalPicDir);
            rc = Picstream_UnInitClearEnable(std::string(internalPicDir));
            if(rc != 0) {
                LOG_ERROR("Picstream_UnInitClearEnable:%d", rc);
            }
        }
        
        {
            CacheAutoLock autoLock;
            rv = autoLock.LockForWrite();
            if (rv < 0) {
                LOG_ERROR("Failed to obtain lock");
                goto out;
            }
            
            CachePlayer player;
#if CCD_ENABLE_USER_SUMMARIES
            bool cacheFileAlreadyExists = (Cache_getCachedUserSummary(username) != NULL);
#endif
            if (cacheFileAlreadyExists) {
                // read existing data from disk,
                rv = player.readCachedData(userId);
                if (rv < 0) {
                    // If unable to read the existing data, just start fresh.
                    LOG_ERROR("Failed to read from disk cache, %d", rv);
                    player.initialize(userId, username, accountId.c_str());
                } else {
                    // but remove any cached data that might be stale now.
                    player._cachedData.mutable_details()->clear_datasets();
                    player._cachedData.mutable_details()->clear_cached_devices();
                    player._cachedData.mutable_details()->clear_cached_user_storage();
                }
            }
            else {
                player.initialize(userId, username, accountId.c_str());
            }
            player._cachedData.mutable_summary()->set_cluster_id(clusterId);
            player.getSession() = session;

            // Do this now, otherwise we will write the user cache file again when we call
            // cache_convertCacheState() within Cache_ActivateOrDeactivateUser().
            // See https://bugs.ctbg.acer.com/show_bug.cgi?id=16135.
            cache_setupMmThumbSync(player);

            // Write the data to the filesystem.
            rv = player.writeCachedData(true);
            if (rv != 0) {
                LOG_ERROR("writeCachedData() returned %d", rv);
                goto out;
            }
        }
    }
out:
    return rv;
}

static void
cache_restoreMainState()
{
    s32 rv;
    ASSERT(Cache_ThreadHasWriteLock());
    {
        char staticStateFile[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForCcdMainState(sizeof(staticStateFile), staticStateFile);
        ProtobufFileReader reader;
        rv = reader.open(staticStateFile, false);
        if (rv < 0) {
            LOG_INFO("Unable to open static CCD state file at \"%s\"; it may not exist yet", staticStateFile);
            goto skip_parse;
        }
        LOG_DEBUG("Parsing from %s", staticStateFile);
        google::protobuf::io::CodedInputStream tempStream(reader.getInputStream());
        {
            u32 version;
            if (!tempStream.ReadVarint32(&version)) {
                LOG_ERROR("Failed to read version from %s", staticStateFile);
                rv = CCD_ERROR_PARSE_CONTENT;
                goto skip_parse;
            }
            if (version != CCD_CACHED_MAIN_DATA_VERSION) {
                LOG_ERROR("Unsupported version ("FMTu32") read from %s", version, staticStateFile);
                rv = CCD_ERROR_MAIN_DATA_VERSION;
                goto skip_parse;
            }
        }
        if (!__cache().mainState.ParseFromCodedStream(&tempStream)) {
            LOG_ERROR("Failed to parse from %s", staticStateFile);
            rv = CCD_ERROR_PARSE_CONTENT;
            goto skip_parse;
        }
        LOG_DEBUG("Success parsing from %s", staticStateFile);
    }
skip_parse:
    // Needs to be before user activated, needs to be after mainState rebuilt.
    __cache().requestedBgModeIntervalSec = cache_getBackgroundModeIntervalSecs();
}

static int
cache_createAllUsersRoot()
{
    char allUsersRoot[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForAllCachedUsers(sizeof(allUsersRoot), allUsersRoot);
    return Util_CreatePath(allUsersRoot, VPL_TRUE);
}

struct InfraLogoutContext
{
    InfraLogoutContext()
    :  doLogout(false)
    {}

    // These other fields are only valid when doLogout is true.
    u64 sessionHandle;
    VPLUser_Id_t userId;
    string iasTicket;

    bool doLogout;
};

static void Cache_DeactivateUserForLogout(u64 sessionHandle, LogoutReason_t reason)
{
    s32 rv;

    VPLUser_Id_t loggedOutUserId = VPLUSER_ID_NONE;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            return;
        }
        CachePlayer* user = cache_getTheUser();
        if (user == NULL) {
            LOG_WARN("No logged-in user.");
            return;
        }
        if (sessionHandle != user->session_handle()) {
            LOG_WARN("Session handle has changed; expect="FMTu64", actual="FMTu64,
                    sessionHandle,  user->session_handle());
            return;
        }

        loggedOutUserId = user->user_id();
        LOG_INFO("Deactivating user "FMT_VPLUser_Id_t", activation="FMTu32, loggedOutUserId,
                user->local_activation_id());

        // Record the logout information so it can be retrieved via CCDI.
        {
            __cache().mainState.mutable_logged_out_users()->Clear();
            ccd::LoggedOutUser* loggedOut = __cache().mainState.add_logged_out_users();
            loggedOut->set_user_id(loggedOutUserId);
            loggedOut->set_username(user->username());
            loggedOut->set_reason(reason);
        }

        // Remove the session credentials for added safety, in case the IAS logout call doesn't get through.
        user->getSession().Clear();
        user->writeCachedData(true);

    } // releases cache lock

#if CCD_USE_SHARED_CREDENTIALS
    {
        // Bug 10632: If the local device is being unlinked, set the reason as "CCDI_LOGOUT".
        if (reason == ccd::LOGOUT_REASON_DEVICE_UNLINKED) {
            char* isDeviceUnlinking = NULL;
            // Get the cross-app flag
            const char* credentialsLocation = VPLSharedObject_GetCredentialsLocation();
            VPLSharedObject_GetString(credentialsLocation, VPL_SHARED_IS_DEVICE_UNLINKING_ID, &isDeviceUnlinking);
            ON_BLOCK_EXIT(VPLSharedObject_FreeString, isDeviceUnlinking);
            // When the VPL_SHARED_IS_DEVICE_UNLINKING_ID flag is set, the foreground application is intending to logout the user.
            // If we are a background application, avoid overwriting the reason with "LOGOUT_REASON_DEVICE_UNLINKED".
            if (isDeviceUnlinking != NULL) {
                LOG_INFO("Logging out due to self-unlink; treat as LOGOUT_REASON_CCDI_LOGOUT");
                reason = ccd::LOGOUT_REASON_CCDI_LOGOUT;
            }
        }
    }
#endif
    
    rv = Cache_ActivateOrDeactivateUser(loggedOutUserId, DEACTIVATE_FOR_LOGOUT);
    if (rv < 0) {
        LOG_WARN("Ignoring error during deactivation: %d", rv);
    }

#if CCD_USE_SHARED_CREDENTIALS
    {
        // Don't clear the shared data when it's just a logout request that's come from Cache_Login.
        // This can happen if:
        // # This CCD instance has user A logged in.
        // # Suspend this instance and switch to another app.
        // # User A logs out and user B logs in.
        // # Switch back to this CCD instance.
        // User A will be logged out of this instance (and we'll end up here), but we must not clear out user B from the shared area.
        if (reason != LOGOUT_REASON_CCDI_LOGIN){
            vplex::sharedCredential::UserCredential userCredential;
            GetUserCredential(userCredential);
            // Do not delete the user credentials if it contains different session.
            // This might be a local user logout only.
            if (userCredential.has_ias_output()) {
                vplex::ias::LoginResponseType sharedIASOutput;
                sharedIASOutput.ParsePartialFromString(userCredential.ias_output());
                
                if (sessionHandle == sharedIASOutput.sessionhandle()) {
                    // Delete shared credentials
                    DeleteCredential(VPL_USER_CREDENTIAL);
                    // Mark device as unlinked
                    const char* credentialsLocation = VPLSharedObject_GetCredentialsLocation();
                    VPLSharedObject_AddString(credentialsLocation, VPL_SHARED_IS_DEVICE_LINKED_ID, VPL_SHARED_DEVICE_NOT_LINKED);
                }
            }
        }
    }
#endif

    // In case any other events are generated while deactivating the user,
    // we want to post the event after the user is deactivated.
    {
        CcdiEvent* event = new CcdiEvent();
        event->mutable_user_logout()->set_user_id(loggedOutUserId);
        event->mutable_user_logout()->set_reason(reason);
        EventManagerPb_AddEvent(event);
        // event will be freed by EventManagerPb.
    }
}

static void Cache_DeactivateUserForCcdStop(VPLUser_Id_t userId)
{
    LOG_INFO("Deactivating user "FMT_VPLUser_Id_t, userId);
    int rv = Cache_ActivateOrDeactivateUser(userId, DEACTIVATE_FOR_CCD_STOP);
    if (rv < 0) {
        LOG_WARN("Ignoring error during deactivation: %d", rv);
    }
}

/// Do additional cleanup that shouldn't be done with the cache lock held.
static void privCleanupAfterLogoutIfNeeded(const InfraLogoutContext& context)
{
    ASSERT(!Cache_ThreadHasLock());

    if (context.doLogout) {

        // TODO: This seems like a strange place for Util_CloseVsCore, since we only end up here
        //   when calling IAS Logout.  Cases such as "CCD Stop" and "Shared Credentials changed
        //   by different app" don't result in IAS Logout!
        Util_CloseVsCore();

        // Invalidate the session in IAS.
        {
            s32 rv = Query_Logout(context.sessionHandle, context.iasTicket);
            if (rv != 0) {
                LOG_WARN("Query_Logout returned %d for user="FMT_VPLUser_Id_t,
                        rv, context.userId);
            }
        }
    }
}

s32
Cache_Init()
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;

    rv = cache_createAllUsersRoot();
    if (rv < 0) {
        LOG_ERROR("Failed to create cache users root: %d", rv);
        goto out;
    }

    cache_uncheckedLockWrite();

    __cache().players = new CachePlayer[CCD_MAX_USERS];
    __cache().only_mobile_network_available = false;
    __cache().deprecatedNetworkDisable = false;
    __cache().foregroundAppType = ccd::CCD_APP_DEFAULT;
    __cache().stream_power_mode = false;
    // Whenever ccd starts fresh, default powermode is foreground.
    __cache().powerMode = S_POWER_NOT_INIT;

    // __cache().requestedBgModeIntervalSec set inside cache_restoreMainState call below.
    // (needs to be after the mainState file has been read, and before activating user).

    //
    // Remember to keep these in sync with Cache_Quit()!
    //---------------------------------------------------

    s_ccdCacheIsInit = true;

    cache_restoreMainState();

    Cache_Unlock();

    // Check if there is already a logged-in user.  If so, activate that user now.
    // Note: it is vital to do this *after* setting s_ccdCacheIsInit
    {
        VPLUser_Id_t userId = __cache().mainState.logged_in_user();
        if (userId != VPLUSER_ID_NONE) {
            LOG_INFO("Reactivating user "FMT_VPLUser_Id_t, userId);
            rv = Cache_ActivateUserCcdRestart(userId);
            if (rv < 0) {
                // Not really expected, but not fatal.
                LOG_ERROR("Failed to activate user "FMT_VPLUser_Id_t, userId);
            }
        }
    }

#if CCD_ENABLE_USER_SUMMARIES
    rv = Cache_UpdateUserSummaries();
#endif

out:
    return rv;
}

static s32 privUpdateFromVsds(UpdateContext& context)
{
    s32 rv;
    {
        // FIXME: override api version string for now, as vplex::...version() currently returns "1.0"
        rv = Query_GetCloudInfo(context.userId, context.session, context.deviceId,
                                "5.0",  // VCS API VERSION
                                context.cloudInfoOutput);
        if (rv != 0) {
            goto out;
        }
    }
    
    // Check if local device is linked.  Update protocol version, build info, OS version, and/or
    // model number if there are any changes.
    {
        const vplex::vsDirectory::GetCloudInfoOutput& vsds_response = context.cloudInfoOutput;
        for (int i = 0; i < vsds_response.devices_size(); i++) {
            if (vsds_response.devices(i).deviceid() == context.deviceId) {
                bool needUpdate = false;
                
                LOG_INFO("Local device is linked for user "FMT_VPLUser_Id_t"; deviceName=\"%s\"",
                         context.userId, vsds_response.devices(i).devicename().c_str());
                
                //check protocol version
                std::string updateLocalProtocolVersion;
                {
                    const std::string& version = vsds_response.devices(i).protocolversion();
                    if (version.compare(CCD_PROTOCOL_VERSION) != 0) {
                        LOG_INFO("Protocol version (original %s) updated to %s", version.c_str(), CCD_PROTOCOL_VERSION);
                        updateLocalProtocolVersion = CCD_PROTOCOL_VERSION;
                        needUpdate = true;
                    }
                }

                //check build info
                std::string updateLocalBuildInfo;
                if(vsds_response.devices(i).has_buildinfo()){
                    const std::string& buildInfo = vsds_response.devices(i).buildinfo();
                    if (buildInfo.compare(SW_CCD_BUILD_INFO) != 0) {
                        LOG_INFO("BuildInfo (original %s) updated to %s", buildInfo.c_str(), SW_CCD_BUILD_INFO);
                        updateLocalBuildInfo = SW_CCD_BUILD_INFO;
                        needUpdate = true;
                    }
                }else{
                    LOG_INFO("BuildInfo not found, set to %s", SW_CCD_BUILD_INFO);
                    updateLocalBuildInfo = SW_CCD_BUILD_INFO;
                    needUpdate = true;
                }

                //check os version
                std::string updateLocalOSVersion;
                {
                    char* localOSVersion = NULL;
                    rv = VPL_GetOSVersion(&localOSVersion);
                    if(rv == VPL_OK && localOSVersion != NULL){
                        if(vsds_response.devices(i).has_osversion()){
                            const std::string& osversion = vsds_response.devices(i).osversion();
                            if (osversion.compare(localOSVersion) != 0) {
                                updateLocalOSVersion = localOSVersion;
                                needUpdate = true;
                                LOG_INFO("OSVersion (original %s) updated to %s", osversion.c_str(), updateLocalOSVersion.c_str());
                            }
                        }else{
                            updateLocalOSVersion = localOSVersion;
                            needUpdate = true;
                            LOG_INFO("OSVersion not found, set to %s", updateLocalOSVersion.c_str());
                        }
                    } else {
                        LOG_ERROR("VPL_GetOSVersion failed: %d", rv);
                    }
                    VPL_ReleaseOSVersion(localOSVersion);
                }

                //check model number
                std::string updateLocalModelNumber;
                {
                    char* manufacturer = NULL;
                    char* model = NULL;
                    rv = VPL_GetDeviceInfo(&manufacturer, &model);
                    if(rv == VPL_OK && model != NULL && manufacturer != NULL){
                        std::string localModelNumber = manufacturer;
                        localModelNumber += ":";
                        localModelNumber += model;

                        if(vsds_response.devices(i).has_modelnumber()){
                            const std::string& modelNumber = vsds_response.devices(i).modelnumber();
                            if (modelNumber.compare(localModelNumber) != 0) {
                                updateLocalModelNumber = localModelNumber;
                                needUpdate = true;
                                LOG_INFO("ModelNumber (original %s) updated to %s", modelNumber.c_str(), updateLocalModelNumber.c_str());
                            }
                        }else{
                            updateLocalModelNumber = localModelNumber;
                            needUpdate = true;
                            LOG_INFO("ModelNumber not found, set to %s", updateLocalModelNumber.c_str());
                        }
                    } else {
                        LOG_ERROR("VPL_GetDeviceInfo failed: %d", rv);
                    }
                    VPL_ReleaseDeviceInfo(manufacturer, model);
                }

                if(needUpdate){
                    rv = Query_UpdateDeviceInfo(context.userId, context.session,
                            "", updateLocalOSVersion, updateLocalProtocolVersion,
                            updateLocalModelNumber, updateLocalBuildInfo);
                    if (rv != 0) {
                        LOG_ERROR("Fail to update DeviceInfo: %d", rv);
                        goto out;
                    }
                }

                break;
            }
        }
    }
out:
    return rv;
}

static int cacheMonitor_privWaitForUpdate(u32 activationId, u64 ansConnId)
{
    CachePlayer::UpdateRecordWaiter waiter;
    int rv;
    rv = VPLSem_Init(&waiter.semaphore, 1, 0);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "VPLSem_Init", rv);
        return rv;
    }
    ON_BLOCK_EXIT(VPLSem_Destroy, &waiter.semaphore);

    // Register our semaphore.
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        } else {
            CachePlayer* user;
            user = cache_getUserByActivationId(activationId);
            if (user == NULL) {
                LOG_ERROR("user for activationId "FMTu32" is no longer signed-in", activationId);
                rv = CCD_ERROR_NOT_SIGNED_IN;
                goto out;
            }
            user->_vsdsUpdateThreadState.registerWaiter(&waiter);
        }
    }
    // Wait for the other thread to post our semaphore.
    {
        // We better not be holding the cache lock while we wait.
        ASSERT(!Cache_ThreadHasLock());
        LOG_INFO("Waiting for updater thread");
        rv = VPLSem_Wait(&waiter.semaphore);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "VPLSem_Wait", rv);
        } else {
            rv = waiter.errCode;
            LOG_INFO("Other thread finished updating, rv=%d", rv);
        }
    }
out:
    return rv;
}

static void privProcessChangesAfterCacheWrite(const UpdateContext& context)
{
    int rv;
    
#if CCD_ENABLE_STORAGE_NODE
    bool isNowStorageNode;
#endif
    VPLUser_Id_t updatedUserID = VPLUSER_ID_NONE;
    google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo> cachedDevices;
    google::protobuf::RepeatedPtrField<vplex::vsDirectory::UserStorage> cachedUserStorage;
    
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_WARN("Failed to obtain lock: %d", rv); // presumably, the cache has been shut down
            goto out;
        }
        CachePlayer* user = cache_getUserByActivationId(context.localActivationId);
        if (user == NULL) {
            // This is an expected case.
            rv = CCD_ERROR_NOT_SIGNED_IN;
            LOG_INFO("User for activationId "FMTu32" is no longer signed-in", context.localActivationId);
            goto out;
        }
#if CCD_ENABLE_STORAGE_NODE
        isNowStorageNode = user->isLocalDeviceStorageNode();
#endif
        updatedUserID = user->user_id();
        cachedDevices = user->_cachedData.details().cached_devices();
        cachedUserStorage = user->_cachedData.details().cached_user_storage();
    } // release read lock
    
    // Process the changes (after writing user cache to disk).
    {
#if CCD_ENABLE_STORAGE_NODE
        // Start or stop StorageNode if needed.
        if (context.wasLocalDeviceStorageNode != isNowStorageNode) {
            MutexAutoLock lock(LocalServers_GetMutex());
            vss_server* storageNode = LocalServers_getStorageNode();
            bool storageNodeRunning = (storageNode != NULL);
            if (storageNodeRunning && !isNowStorageNode) {
                LOG_INFO("This device is no longer a storage node");
                LocalServers_StopStorageNode();
            } else if (!storageNodeRunning && isNowStorageNode) {
                LOG_INFO("This device is now a storage node");
                StartStorageNodeContext snContext;
                {
                    CacheAutoLock autoLock;
                    rv = autoLock.LockForRead();
                    if (rv < 0) {
                        LOG_WARN("Failed to obtain lock: %d", rv); // presumably, the cache has been shut down
                        goto out;
                    }
                    CachePlayer* user = cache_getUserByActivationId(context.localActivationId);
                    if (user == NULL) {
                        // This is an expected case.
                        rv = CCD_ERROR_NOT_SIGNED_IN;
                        LOG_INFO("User for activationId "FMTu32" is no longer signed-in", context.localActivationId);
                        goto out;
                    }
                    LocalServers_PopulateStartStorageNodeContext(*user, snContext);
                } // release read lock
                LocalServers_StartStorageNode(snContext);
            }
        }
#endif
        // Update the CCD global cache (which notifies ANS).
        DeviceStateCache_UpdateList(updatedUserID, cachedDevices, cachedUserStorage);
    }
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_WARN("Failed to obtain lock: %d", rv); // presumably, the cache has been shut down
            goto out;
        }
        CachePlayer* user = cache_getUserByActivationId(context.localActivationId);
        if (user == NULL) {
            // This is an expected case.
            rv = CCD_ERROR_NOT_SIGNED_IN;
            LOG_INFO("User for activationId "FMTu32" is no longer signed-in", context.localActivationId);
            goto out;
        }
        cache_processUpdatedDatasetList(*user);
    } // release write lock
out:
    return;
}

static void CacheMonitor_CleanupSemaphore(u32 activationId, VPLSem_t* sem)
{
    {
        CacheAutoLock autoLock;
        int rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock"); // presumably, the cache has been shut down
            goto out;
        }
        CachePlayer* user = cache_getUserByActivationId(activationId);
        if (user == NULL) {
            // This is an expected case.
            rv = CCD_ERROR_NOT_SIGNED_IN;
            LOG_INFO("activationId "FMTu32" no longer valid", activationId);
            goto out;
        }
        // The semaphore will be invalid, set doUpdateSem to NULL.
        user->_vsdsUpdateThreadState.doUpdateSem = NULL;
    }
out:
    VPLSem_Destroy(sem);
}

static VPLTHREAD_FN_DECL CacheMonitor_VsdsThreadFn(void* arg)
{
    u32 activationId = (u32)arg;
    int rv;
    // This thread owns the semaphore.
    VPLSem_t mySem;
    int retryCount = 0;

    LOG_INFO("This is the CacheMonitor thread for activation "FMTu32, activationId);

    {
        // Initialize.
        rv = VPLSem_Init(&mySem, 1, 0);
        if (rv != 0) {
            // This is really bad.  If this happens, I think we might as well restart the process.
            LOG_CRITICAL("%s failed: %d", "", rv);
            CCDShutdown();
            goto out;
        }
        ON_BLOCK_EXIT(CacheMonitor_CleanupSemaphore, activationId, &mySem);
        {
            CacheAutoLock autoLock;
            rv = autoLock.LockForWrite();
            if (rv < 0) {
                LOG_ERROR("Failed to obtain lock"); // presumably, the cache has been shut down
                goto out;
            }
            CachePlayer* user = cache_getUserByActivationId(activationId);
            if (user == NULL) {
                // This is an expected case.
                rv = CCD_ERROR_NOT_SIGNED_IN;
                LOG_INFO("activationId "FMTu32" no longer valid", activationId);
                goto out;
            }
            user->_vsdsUpdateThreadState.doUpdateSem = &mySem;
        }

        // Loop.
        while(1) {
            bool needToWait;
            VPLTime_t sleepDuration = VPLTIME_INVALID; // amount of time to sleep (valid when needToWait == true)

            { // Check if we should update.
                CacheAutoLock autoLock;
                rv = autoLock.LockForWrite();
                if (rv < 0) {
                    LOG_ERROR("Failed to obtain lock"); // presumably, the cache has been shut down
                    goto out;
                }
                CachePlayer* user = cache_getUserByActivationId(activationId);
                if (user == NULL) {
                    // This is an expected case.
                    rv = CCD_ERROR_NOT_SIGNED_IN;
                    LOG_INFO("User for activationId "FMTu32" is no longer signed-in", activationId);
                    goto out;
                }
                CachePlayer::UpdateRecord& record = user->_vsdsUpdateThreadState;
                LOG_INFO("next="FMT_VPLTime_t", max="FMT_VPLTime_t", waitersInProgress="FMTu_size_t", waitersNew="FMTu_size_t,
                        record.nextUpdateTimestamp, record.maxNextUpdateTimestamp,
                        record.waitersInProgress.size(), record.waitersNew.size());
                if (record.nextUpdateTimestamp == VPLTIME_INVALID) {
                    // No work to do; wait on the semaphore.
                    needToWait = true;
                } else {
                    VPLTime_t now = VPLTime_GetTimeStamp();
                    if ((now >= record.nextUpdateTimestamp) || (now >= record.maxNextUpdateTimestamp)) {
                        LOG_INFO("Time to update!");
                        needToWait = false;
                        record.willBeUpToDate = CCD_USER_DATA_ALL;
                        record.nextUpdateTimestamp = VPLTIME_INVALID;
                        record.maxNextUpdateTimestamp = VPLTIME_INVALID;
                        record.moveWaiters();
                    } else {
                        needToWait = true;
                        sleepDuration = MIN((record.nextUpdateTimestamp - now), (record.maxNextUpdateTimestamp - now));
                    }
                }
            }
            if (needToWait) {
                int wakeReason;
                if (sleepDuration == VPLTIME_INVALID) {
                    LOG_INFO("Nothing to do; waiting");
                    wakeReason = VPLSem_Wait(&mySem);
                } else {
                    LOG_INFO("Work to do in "FMT_VPLTime_t"us; waiting", sleepDuration);
                    wakeReason = VPLSem_TimedWait(&mySem, sleepDuration);
                }
                LOG_INFO("Woke up (%d)", wakeReason);
            } else {
                UpdateContext updateContext;

                // Gather the context and mark that we are in the process of updating.
                {
                    CacheAutoLock autoLock;
                    rv = autoLock.LockForWrite();
                    if (rv < 0) {
                        LOG_ERROR("Failed to obtain lock"); // presumably, the cache has been shut down
                        goto out;
                    } else {
                        CachePlayer* user = cache_getUserByActivationId(activationId);
                        if (user == NULL) {
                            // This is an expected case.
                            rv = CCD_ERROR_NOT_SIGNED_IN;
                            LOG_INFO("User for activationId "FMTu32" is no longer signed-in", activationId);
                            goto out;
                        }
                        u64 thisDeviceId;
                        rv = ESCore_GetDeviceGuid(&thisDeviceId);
                        if (rv != 0) {
                            GVM_ASSERT_FAILED("%s failed: %d", "ESCore_GetDeviceGuid", rv);
                            goto out;
                        }
                        updateContext.deviceId = thisDeviceId;
                        updateContext.wasLocalDeviceStorageNode = user->isLocalDeviceStorageNode();
                        updateContext.session = user->getSession();
                        updateContext.userId = user->user_id();
                        updateContext.localActivationId = user->local_activation_id();
#if CCD_ENABLE_STORAGE_NODE
                        updateContext.updateSyncboxArchiveStorageSeqNum =
                                Cache_GetUpdateSyncboxArchiveStorageSeqNum();
#endif
                    }
                } // release lock

                // Retrieve info from infra without holding any locks.
                int rv_for_waiters = privUpdateFromVsds(updateContext);
                if (rv_for_waiters == 0) {
                    // Success.  No need to retry.
                    retryCount = 0;
                } else {
                    LOG_WARN("privUpdateFromVsds (retry %d) for user "FMT_VPLUser_Id_t" failed: %d",
                            retryCount, updateContext.userId, rv_for_waiters);
                    if ((rv_for_waiters == VPL_VS_DIRECTORY_ERR_INVALID_SESSION) ||
                        (rv_for_waiters == VPL_VS_DIRECTORY_ERR_UNLINKED_SESSION))
                    {
                        LOG_INFO("Session handle "FMTu64" was invalidated.",
                                updateContext.session.session_handle());
                        int rc = CCDPrivBeginLogout(true);
                        if (rc < 0) {
                            if (rc == CCD_ERROR_SHUTTING_DOWN) {
                                LOG_INFO("CCD is shutting down!");
                            } else {
                                LOG_WARN("CCDPrivBeginLogout failed: %d", rc);
                            }
                            goto out;
                        }
                        ON_BLOCK_EXIT(CCDPrivEndLogout);

                        if (rv_for_waiters == VPL_VS_DIRECTORY_ERR_INVALID_SESSION) {
                            LOG_INFO("User session has been invalidated; logging out user");
                            Cache_DeactivateUserForLogout(updateContext.session.session_handle(),
                                    LOGOUT_REASON_SESSION_INVALID);
                        } else {
                            ASSERT(rv_for_waiters == VPL_VS_DIRECTORY_ERR_UNLINKED_SESSION);
                            LOG_INFO("Session invalid due to device unlink; logging out user");
                            Cache_DeactivateUserForLogout(updateContext.session.session_handle(),
                                    LOGOUT_REASON_DEVICE_UNLINKED);
                        }
                        goto out; // CCDPrivEndLogout() gets called now.
                    } else {
                        // Retry (after a short delay) for other error codes.
                        if (retryCount >= 3){
                            // Retry still failed; give up for now.
                            retryCount = 0;
                        } else {
                            retryCount++;
                            CacheAutoLock autoLock;
                            rv = autoLock.LockForWrite();
                            if (rv < 0) {
                                LOG_ERROR("Failed to obtain lock"); // presumably, the cache has been shut down
                                goto out;
                            }
                            CachePlayer* user = cache_getUserByActivationId(activationId);
                            if (user == NULL) {
                                // This is an expected case.
                                rv = CCD_ERROR_NOT_SIGNED_IN;
                                LOG_INFO("User for activationId "FMTu32" is no longer signed-in", activationId);
                                goto out;
                            }
                            CachePlayer::UpdateRecord& record = user->_vsdsUpdateThreadState;
                            // Copied from cacheMonitor_privMarkDirty:
                            VPLTime_t now = VPLTime_GetTimeStamp();
                            if (record.nextUpdateTimestamp == VPLTIME_INVALID) {
                                record.nextUpdateTimestamp = now + VPLTIME_FROM_SEC(3);
                                record.maxNextUpdateTimestamp = VPLTime_GetTimeStamp() + VPLTIME_FROM_SEC(10);
                            } else if (now + VPLTIME_FROM_SEC(1) > record.nextUpdateTimestamp) {
                                record.nextUpdateTimestamp = now + VPLTIME_FROM_SEC(1);
                            }
                        }
                    }
                }

                list<u64> removedStorageNodeDeviceIds;
                list<u64> changedStorageNodeDeviceIds;
                // Write to the local cache and perform any actions due to changes.
                {
                    CacheAutoLock autoLock;
                    rv = autoLock.LockForWrite();
                    if (rv < 0) {
                        LOG_ERROR("Failed to obtain lock"); // presumably, the cache has been shut down
                        goto out;
                    }

                    // Make sure the user hasn't logged out.
                    CachePlayer* user = cache_getUserByActivationId(updateContext.localActivationId);
                    if (user == NULL) {
                        // This is an expected case.
                        rv = CCD_ERROR_NOT_SIGNED_IN;
                        LOG_INFO("User for activationId "FMTu32" is no longer signed-in", activationId);
                        goto out;
                    }
                    ASSERT_EQUAL(user->user_id(), updateContext.userId, FMT_VPLUser_Id_t);
                    if (rv_for_waiters == 0) {
                        // 
                        // Actual updating of the local state happens here:
                        // 
                        rv_for_waiters = user->processUpdates(true,
                                updateContext,
                                /*out*/ removedStorageNodeDeviceIds,
                                /*out*/ changedStorageNodeDeviceIds);
                        if (rv_for_waiters != 0) {
                            LOG_ERROR("processUpdates for user "FMT_VPLUser_Id_t" failed: %d",
                                    updateContext.userId, rv_for_waiters);
                        }
                    }
                } // release write lock
                
                // Perform rest of the changes without cache lock
                if (rv_for_waiters == 0) {
                    rv = CCDPrivBeginOther();
                    if (rv < 0) {
                        if (rv == CCD_ERROR_SHUTTING_DOWN) {
                            LOG_INFO("CCD is shutting down!");
                        } else {
                            LOG_WARN("CCDPrivBeginOther failed: %d", rv);
                        }
                        goto out;
                    }
                    ON_BLOCK_EXIT(CCDPrivEndOther);
                    
                    list<u64>::const_iterator it;
                    for(it=removedStorageNodeDeviceIds.begin(); it != removedStorageNodeDeviceIds.end(); ++it){
                        u64 deviceId = *it;
                        // Tell stream_service.
                        LocalServers_GetHttpService().remove_stream_server(deviceId);
                        // Tell the SyncConfigs.
                        SyncFeatureMgr_ReportDeviceAvailability(deviceId);
                        // Tell the PinnedMediaManager.
                        int temp_rc = PinnedMediaManager_RemoveAllPinnedMediaItemsByDeviceId(deviceId);
                        if (temp_rc) {
                            LOG_WARN("Failed to remove pinned media items for device_id "FMTu64": %d",
                                     deviceId, temp_rc);
                        }
                    }
                    
                    for(it=changedStorageNodeDeviceIds.begin(); it != changedStorageNodeDeviceIds.end(); ++it){
                        u64 deviceId = *it;
                        // Tell stream_service.
                        LocalServers_GetHttpService().add_or_change_stream_server(deviceId, /*force_drop_conns*/false);
                        
                        // Tell the SyncConfigs.
                        SyncFeatureMgr_ReportDeviceAvailability(deviceId);
                    }
                    
                    privProcessChangesAfterCacheWrite(updateContext);

#if CCD_ENABLE_STORAGE_NODE
                    // Re-create syncbox dataset if necessary, it happens when database is deleted by users
                    u64 syncboxDatasetToRemove = 0;
                    {
                        CacheAutoLock autoLock;
                        rv = autoLock.LockForRead();
                        if (rv < 0) {
                            LOG_WARN("Failed to obtain lock: %d", rv); // presumably, the cache has been shut down
                            goto out;
                        }
                        // Make sure the user hasn't logged out.
                        CachePlayer* user = cache_getUserByActivationId(updateContext.localActivationId);
                        if (user == NULL) {
                            // This is an expected case.
                            rv = CCD_ERROR_NOT_SIGNED_IN;
                            LOG_INFO("User for activationId "FMTu32" is no longer signed-in", activationId);
                            goto out;
                        }
                        bool isArchiveStorage = user->_cachedData.details().syncbox_sync_settings().is_archive_storage();
                        if (isArchiveStorage && user->_cachedData.details().has_need_to_recreate_syncbox_dataset()) {
                            syncboxDatasetToRemove = user->_cachedData.details().need_to_recreate_syncbox_dataset();
                        }
                    } // release cache lock

                    if (syncboxDatasetToRemove != 0) { // call VSDS
                        LOG_INFO("Need to re-create syncbox dataset. Original dataset("FMTu64") will be removed.", syncboxDatasetToRemove);
                        rv = Cache_DeleteSyncboxDataset(updateContext.localActivationId, syncboxDatasetToRemove);
                        if (rv == 0) {
                            u64 newDatasetId = 0;
                            rv = Cache_CreateAndAssociateSyncboxDatasetIfNeeded(updateContext.localActivationId, /*out*/ newDatasetId, true/*ignore cache*/);
                            if (rv == 0) {
                                // Syncbox dataset has been re-created, now setup new staging area
                                std::string stagingAreaPath;
                                {
                                    MutexAutoLock lock(LocalServers_GetMutex());
                                    vss_server* storageNode = LocalServers_getStorageNode();
                                    if (storageNode != NULL) {
                                        // Create syncbox staging area if it does not exist yet
                                        stagingAreaPath = storageNode->getSyncboxArchiveStorageStagingAreaPath(newDatasetId);
                                        rv = Util_CreateDir(stagingAreaPath.c_str());
                                        if (rv != 0) {
                                            LOG_ERROR("Syncbox staging area %s creation fail: %d", stagingAreaPath.c_str(), rv);
                                            // TODO: update cache setting to disable Syncbox to avoid infinite retry.
                                        }
                                        storageNode->updateSyncboxArchiveStorageDataset(newDatasetId);
                                    } else {
                                        // Cannot proceed without stagingAreaPath.
                                        LOG_ERROR("Storage node is not running.");
                                        // Not expected with current apps!!!
                                        // TODO: update cache setting to disable Syncbox to avoid infinite retry.
                                    }
                                }

                                // If everything done good, lock cache and set config then VsdsThread will not retry re-create dataset again.
                                if (rv == 0) {
                                    CacheAutoLock autoLock;
                                    rv = autoLock.LockForWrite();
                                    if (rv < 0) {
                                        LOG_WARN("Failed to obtain lock: %d", rv); // presumably, the cache has been shut down
                                        goto out;
                                    }
                                    // Make sure the user hasn't logged out.
                                    CachePlayer* user = cache_getUserByActivationId(updateContext.localActivationId);
                                    if (user == NULL) {
                                        // This is an expected case.
                                        rv = CCD_ERROR_NOT_SIGNED_IN;
                                        LOG_INFO("User for activationId "FMTu32" is no longer signed-in", activationId);
                                        goto out;
                                    }
                                    user->_cachedData.mutable_details()->set_need_to_recreate_syncbox_dataset(0);
                                    user->_cachedData.mutable_details()->mutable_syncbox_sync_settings()->set_syncbox_staging_area_abs_path(stagingAreaPath);
                                    user->_cachedData.mutable_details()->mutable_syncbox_sync_settings()->set_syncbox_dataset_id(newDatasetId);
                                    user->writeCachedData(false);
                                    LOG_INFO("Done re-create SyncBox dataset:"FMTu64, newDatasetId);
                                }
                            } else {
                                LOG_ERROR("Cache_CreateAndAssociateSyncboxDatasetIfNeeded failed:%d", rv);
                                // skip for now, can retry later
                            }
                        } else {
                            LOG_ERROR("Cache_DeleteSyncboxDataset failed:%d", rv);
                            // skip for now, can retry later
                        }
                    }
#endif // CCD_ENABLE_STORAGE_NODE
                }

                {
                    CacheAutoLock autoLock;
                    rv = autoLock.LockForRead();
                    if (rv < 0) {
                        LOG_WARN("Failed to obtain lock: %d", rv); // presumably, the cache has been shut down
                        goto out;
                    }
                    // Make sure the user hasn't logged out.
                    CachePlayer* user = cache_getUserByActivationId(updateContext.localActivationId);
                    if (user == NULL) {
                        // This is an expected case.
                        rv = CCD_ERROR_NOT_SIGNED_IN;
                        LOG_INFO("User for activationId "FMTu32" is no longer signed-in", activationId);
                        goto out;
                    }
                    
                    CachePlayer::UpdateRecord& record = user->_vsdsUpdateThreadState;
                    if (rv_for_waiters == 0) {
                        user->_upToDate.set(record.willBeUpToDate);
                    }
                    // processProxyNotification requires this to happen after we process the
                    // updates and start storage node.
                    record.wakeupAndClearWaiters(rv_for_waiters);
                } // release read lock

            }

        } // while(1)
    }
out:
    LOG_INFO("CacheMonitor thread exiting");
    return VPLTHREAD_RETURN_VALUE;
}

static s32
privSetupSession(UserSession& session, const vplex::ias::LoginResponseType& iasOutput)
{
    s32 rv = CCD_OK;
    {
        session.set_session_handle(iasOutput.sessionhandle());
        session.set_session_secret(iasOutput.sessionsecret());
        
        LOG_INFO("Session handle = "FMTu64, iasOutput.sessionhandle());
        
        rv = Query_GenerateServiceTickets(session);
    }
    return rv;
}

// TODO: bug 15682: finish cleanup
#if 0
static s32
privLoginAndActivateUser(UserSession& session, int playerIndex, const char* username, const CachedUserSummary* summary, const vplex::ias::LoginResponseType& iasOutput)
{
    s32 rv = CCD_OK;
    {
        VPLUser_Id_t userId = iasOutput.userid();
        string accountId = iasOutput.accountid();
        s64 clusterId = iasOutput.storageclusterid();
        
        CachePlayer player;
#if CCD_ENABLE_USER_SUMMARIES
        if (summary != NULL) { // If the user cache file already exists,
            // read existing data from disk,
            rv = player.readCachedData(userId);
            if (rv < 0) {
                // If unable to read the existing data, just start fresh.
                LOG_ERROR("Failed to read from disk cache, %d", rv);
                player.initialize(userId, username, accountId.c_str());
            } else {
                // but remove any cached data that might be stale now.
                player._cachedData.mutable_details()->clear_datasets();
                player._cachedData.mutable_details()->clear_cached_devices();
                player._cachedData.mutable_details()->clear_cached_storage_nodes();
                player._cachedData.mutable_details()->clear_cached_user_storage();
            }
        }
        else
#endif
        {
            player.initialize(userId, username, accountId.c_str());
        }
        player._cachedData.mutable_summary()->set_cluster_id(clusterId);
        player.getSession() = session;
        
        // We don't want to upload pictures that were taken while player
        // was not logged in.
        {
            int rc;
            char* internalPicDir = (char*)malloc(CCD_PATH_MAX_LENGTH);
            if(internalPicDir == NULL) {
                LOG_ERROR("Out of mem");
                goto out;
            }
            ON_BLOCK_EXIT(free, internalPicDir);
            internalPicDir[0] = '\0';
            DiskCache::getPathForUserUploadPicstream(userId,
                                                     CCD_PATH_MAX_LENGTH,
                                                     internalPicDir);
            rc = Picstream_UnInitClearEnable(std::string(internalPicDir));
            if(rc != 0) {
                LOG_ERROR("Picstream_UnInitClearEnable:%d", rc);
            }
        }
        rv = player.writeCachedData(true);
        if (rv != 0) {
            LOG_ERROR("writeCachedData() returned %d", rv);
            goto out;
        }
        
#if CCD_ENABLE_USER_SUMMARIES
        // TODO: We really only need to refresh the user that is logging in.
        rv = Cache_UpdateUserSummaries();
        if (rv < 0) {
            LOG_ERROR("Cache_UpdateUserSummaries() returned %d", rv);
            goto out;
        }
#endif
        rv = cache_activateUser(playerIndex, userId);
        if (rv < 0) {
            LOG_ERROR("Failed to activate user "FMT_VPLUser_Id_t": %d", userId, rv);
            goto out;
        }
    }
 out:
    return rv;
}

// All the process that must run after user activation should put in this function.
// Any runtime error in this function will trigger deactivating user.
static s32
privProcessAfterActivateUser(int playerIndex, VPLUser_Id_t userId)
{
    s32 rv = CCD_OK;
    
    {
        // Write to disk that this user is the one that is logged-in.
        __cache().mainState.set_logged_in_user(userId);
        __cache().mainState.mutable_logged_out_users()->Clear();
        rv = cache_writeMainState(true);
        if (rv < 0) {
            LOG_ERROR("Failed to persist main CCD state: %d", rv);
            goto failed_after_activate;
        }
        
        // At this point, we are committed to succeed.
        
        //----------------------
        // Post a LOGIN event.
        {
            ccd::CcdiEvent* event = new ccd::CcdiEvent();
            event->mutable_user_login()->set_user_id(userId);
            EventManagerPb_AddEvent(event);
            // event will be freed by EventManagerPb.
        }
        //----------------------
    }
    goto out;
    
failed_after_activate:
    // Undo __cache().mainState.set_logged_in_user(userId);
    __cache().mainState.set_logged_in_user(VPLUSER_ID_NONE);
    // Undo cache_activateUser().
    cache_deactivateUser(playerIndex, false);
out:
    return rv;
}
#endif

s32
Cache_Login(
        const ccd::LoginInput& request,
        ccd::LoginOutput& response)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;
    const char* username = request.user_name().c_str();
    const char* password = request.password().empty() ? NULL : request.password().c_str();
    const char* pairingToken = request.pairing_token().empty() ? NULL : request.pairing_token().c_str();
    int playerIndex = request.player_index();

    response.set_user_id(0);
    
    vplex::ias::LoginResponseType iasOutput;
    UserSession session;

    ASSERT(!Cache_ThreadHasLock());
    rv = CCDPrivBeginLogin();
    if (rv < 0) {
        LOG_INFO("CCDPrivBeginLogin returned %d", rv);
        return rv;
    }
    ON_BLOCK_EXIT(CCDPrivEndLogin);

    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }

        if (!isPlayerIndexValid(playerIndex) || (username == NULL)) {
            LOG_ERROR("playerIndex=%d, username="FMT0xPTR, playerIndex, username);
            rv = CCD_ERROR_PARAMETER;
            goto out;
        }

        if (__cache().players[playerIndex].isSignedIn()) {
            if (request.skip_if_already_correct() &&
                    (__cache().players[playerIndex].username().compare(username) == 0)) {
                LOG_INFO("%s is already logged-in at index %d; skipping login", username, playerIndex);
                response.set_user_id(__cache().players[playerIndex].user_id());
                rv = CCD_OK;
                goto out;
            }
            LOG_ERROR("__cache().players[%d].isSignedIn() = true", playerIndex);
            rv = CCD_ERROR_SIGNED_IN;
            goto out;
        }

        // Check that the user isn't already signed in under another slot.
        {
            CachePlayer* sameUser = getSignedInUserByUsername(username);
            if (sameUser != NULL) {
                LOG_ERROR("Requested user already signed in under index %d", sameUser->player_index());
                rv = CCD_ERROR_SIGNED_IN;
                goto out;
            }
        }

        if ((password == NULL) && (pairingToken == NULL)) {
            LOG_ERROR("Cannot use an empty password and empty pairingToken");
            rv = CCDI_ERROR_NO_PASSWORD;
            goto out;
        }
    } // releases cache lock.

    {
        // Verified that the request is good; start the CCDLogin log now.
        LOGStartSpecialLog("CCDLogin.log", 256 * 1024);

        if (!VirtualDevice_HasCredentials()) {
            rv = VirtualDevice_Register(username, password, pairingToken);
            if (rv < 0) {
                LOG_ERROR("Failed to register device");
                goto out;
            }
            LOG_INFO("Device registered successfully");
            rv = VirtualDevice_LoadCredentials();
            if (rv < 0) {
                LOG_ERROR("Failed to load credentials");
                goto out;
            }
            LOG_INFO("Device credentials loaded successfully");
        }

        {
            u64 thisDeviceId;
            rv = ESCore_GetDeviceGuid(&thisDeviceId);
            if (rv != 0) {
                LOG_ERROR("ESCore_GetDeviceGuid:%d", rv);
                goto out;
            }

            rv = Util_InitVsCore(thisDeviceId);
            if (rv != 0) {
                LOG_ERROR("Util_InitVsCore:%d", rv);
                goto out;
            }
        }

        VPL_BOOL* acEulaAgreed = NULL;
        VPL_BOOL tempAcEulaAgreed;
        if (request.has_ac_eula_agreed()) {
            tempAcEulaAgreed = request.ac_eula_agreed();
            acEulaAgreed = &tempAcEulaAgreed;
        }

        rv = Query_Login(/*out*/ iasOutput, username, password, pairingToken, acEulaAgreed);
        if (rv < 0) {
            LOG_ERROR("Query_Login() returned %d", rv);
            goto out;
        } else if (iasOutput.userid() == 0) {
            LOG_ERROR("Query_Login() returned userId 0");
            rv = CCD_ERROR_BAD_SERVER_RESPONSE;
            goto out;
        }
        
        rv = privSetupSession(session, iasOutput);
        if (rv < 0) {
            LOG_ERROR("privSetupSession() returned %d", rv);
            goto failed_after_login;
        }
#if CCD_USE_ANS
        u32 instance_id = 0;
        rv = Query_GetAnsLoginBlob(session.session_handle(), session.ias_ticket(),
                                   /*out*/ *session.mutable_ans_session_key(),
                                   /*out*/ *session.mutable_ans_login_blob(),
                                   /*out*/ instance_id);
        if (rv != VPL_OK) {
            goto failed_after_login;
        }
        session.set_instance_id(instance_id);
#endif
        
        rv = privWriteUserCacheFileAfterLogin(session, username, iasOutput);
        if (rv < 0) {
            LOG_ERROR("privWriteUserCacheFileAfterLogin failed: %d", rv);
            goto failed_after_login;
        }
    }

    rv = Cache_ActivateUserNewLogin(iasOutput.userid());
    if (rv < 0) {
        LOG_ERROR("activateUser failed: %d", rv);
        goto failed_after_login;
    }
    
    response.set_user_id(iasOutput.userid());
    
#if CCD_USE_SHARED_CREDENTIALS
    {
        // Store user credential to shared location
        vplex::sharedCredential::UserCredential userCredential;
        userCredential.set_user_name(username);
        string iasOutputString;
        iasOutput.SerializeToString(&iasOutputString);
        userCredential.set_ias_output(iasOutputString);
        
        WriteUserCredential(userCredential);
        // Make sure there is no unlinking device flag which might be set by other apps.
        const char* credentialsLocation = VPLSharedObject_GetCredentialsLocation();
        VPLSharedObject_DeleteObject(credentialsLocation, VPL_SHARED_IS_DEVICE_UNLINKING_ID);
    }
#endif
    goto out;
    
failed_after_login:
    { // Undo Query_Login().
        // Note that we need to use the session returned from Query_Login,
        // since __cache().players[playerIndex] is not valid at this point!
        s32 temp_rv = Query_Logout(session);
        if (temp_rv != 0) {
            LOG_WARN("Query_Logout returned %d during abort login", temp_rv);
        }
    }
 out:
    return rv; // CCDPrivEndLogin() will be called now (due to ON_BLOCK_EXIT).
}

#if CCD_USE_SHARED_CREDENTIALS
// Using username and iasOutput which retrieved from shared location to login.
// This is for supporting SSO on sandboxed devices.
s32
Cache_LoginWithSession(const vplex::sharedCredential::UserCredential& userCredential)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;
    UserSession session;
    int playerIndex = 0;
    
    // TODO: Why not compare userId instead of username?
    const char* username = userCredential.user_name().c_str();
    
    vplex::ias::LoginResponseType sharedIASOutput;
    // TODO: handle case when this fails?
    sharedIASOutput.ParsePartialFromString(userCredential.ias_output());
    
    // TODO: Bug 12778: This causes CCDLogin to be overwritten every time the app comes to the foreground!
    LOGStartSpecialLog("CCDLogin.log", 256 * 1024);
    
    LOG_INFO("Login with shared session");
    LOG_DEBUG("Shared accountId %s", sharedIASOutput.accountid().c_str());
    LOG_DEBUG("Shared clusterId "FMTu64, sharedIASOutput.storageclusterid());
    LOG_DEBUG("Shared userId "FMTu64, sharedIASOutput.userid());

    ASSERT(!Cache_ThreadHasLock());
    rv = CCDPrivBeginLogin();
    if (rv < 0) {
        LOG_INFO("CCDPrivBeginLogin returned %d", rv);
        return rv;
    }
    ON_BLOCK_EXIT(CCDPrivEndLogin);

    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        
        if (__cache().players[playerIndex].isSignedIn()) {
            if ((__cache().players[playerIndex].username().compare(username) == 0) &&
                (__cache().players[playerIndex].getSession().session_handle() == sharedIASOutput.sessionhandle())) {
                LOG_INFO("%s is already logged-in; skipping login", username);
                rv = CCD_OK;
                goto out;
            }
            autoLock.UnlockNow();
            // We need to release the cache lock here, but CCDPrivBeginLogin() should prevent
            // any other threads from interfering.
            rv = Cache_Logout(playerIndex, VPLUSER_ID_NONE, VPL_TRUE, LOGOUT_REASON_CCDI_LOGIN);
            if (rv < 0) {
                LOG_ERROR("Failed to logout index %d", playerIndex);
                goto out;
            }
        }
    }

    {
        {
            u64 thisDeviceId;
            rv = ESCore_GetDeviceGuid(&thisDeviceId);
            if (rv != 0) {
                LOG_ERROR("ESCore_GetDeviceGuid:%d", rv);
                goto out;
            }

            rv = Util_InitVsCore(thisDeviceId);
            if (rv != 0) {
                LOG_ERROR("Util_InitVsCore:%d", rv);
                goto out;
            }
        }
        
        rv = privSetupSession(session, sharedIASOutput);
        if (rv < 0) {
            LOG_ERROR("privSetupSession() returned %d", rv);
            goto out;
        }
#if CCD_USE_ANS
        u32 instance_id = 0;
        rv = Query_GetAnsLoginBlob(session.session_handle(), session.ias_ticket(),
                                   /*out*/ *session.mutable_ans_session_key(),
                                   /*out*/ *session.mutable_ans_login_blob(),
                                   /*out*/ instance_id);
        if (rv != VPL_OK) {
            goto out;
        }
        session.set_instance_id(instance_id);
#endif
        
        rv = privWriteUserCacheFileAfterLogin(session, username, sharedIASOutput);
        if (rv < 0) {
            LOG_ERROR("privWriteUserCacheFileAfterLogin failed: %d", rv);
            goto out;
        }
    }
    
    rv = Cache_ActivateUserNewLogin(sharedIASOutput.userid());
    if (rv < 0) {
        LOG_ERROR("activateUser failed: %d", rv);
        goto out;
    }
    
out:
    return rv; // CCDPrivEndLogin() will be called now (due to ON_BLOCK_EXIT).
}
#endif

s32 Cache_Logout(int optPlayerIndex,
                 VPLUser_Id_t optUserId,
                 VPL_BOOL warnIfNotSignedIn,
                 LogoutReason_t reason)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;

    ASSERT(!Cache_ThreadHasLock());
    rv = CCDPrivBeginLogout(false);
    if (rv < 0) {
        goto out;
    } else {
        ON_BLOCK_EXIT(CCDPrivEndLogout);

        u64 sessionHandle;
        {
            CacheAutoLock autoLock;
            rv = autoLock.LockForRead();
            if (rv < 0) {
                LOG_ERROR("Failed to obtain lock");
                goto out;
            }

            CachePlayer* user;
            rv = cache_getUser(optPlayerIndex, optUserId, warnIfNotSignedIn, &user);
            if (rv < 0) {
                if ((rv == CCD_ERROR_NOT_SIGNED_IN) && !warnIfNotSignedIn) {
                    rv = CCD_OK;
                }
                goto out;
            }
            
            sessionHandle = user->session_handle();
        } // releases cache lock
        
        Cache_DeactivateUserForLogout(sessionHandle, reason);

    } // ON_BLOCK_EXIT will call CCDPrivEndLogout() now.
out:
    return rv;
}

s32 Cache_Logout_Unlink(VPLUser_Id_t userId,
                        const ccd::UserSession& session,
                        u64 deviceId)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    s32 rv;

    ASSERT(!Cache_ThreadHasLock());
    rv = CCDPrivBeginLogout(false);
    if (rv < 0) {
        goto out;
    } else {
        ON_BLOCK_EXIT(CCDPrivEndLogout);

        // TODO: If this fails for any reason (most likely intermittent network conditions), we
        // currently won't retry, so the device will still be linked for the user (and infra and
        // other devices will display it as such).
        vplex::vsDirectory::UnlinkDeviceOutput vsds_response;
        rv = Query_UnlinkDevice(userId,
                                session,
                                deviceId,
                                /*out*/ vsds_response);
        if (rv < 0) {
            LOG_WARN("Query_UnlinkDevice failed: %d; ignoring.", rv);
            rv = 0;
        }

        Cache_DeactivateUserForLogout(session.session_handle(), LOGOUT_REASON_CCDI_LOGOUT);

    } // ON_BLOCK_EXIT will call CCDPrivEndLogout() now.
out:
    return rv;
}

void Cache_LogAllCaches(void)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    Cache_LogSummary();
    Cache_LogUserCache();
}

void Cache_LogSummary(void)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);

    {
        CacheAutoLock autoLock;
        int rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_WARN("LockForRead returned %d", rv);
        }

        LOG_INFO("-------------------");
        LOG_INFO("CACHE (summary)");
#if CCD_ENABLE_USER_SUMMARIES
        for (int i = 0; i < __cache().userList.users_size(); i++) {
            LOG_INFO("cached_user[%d]: "FMT_VPLUser_Id_t" \"%s\"", i,
                    __cache().userList.users(i).user_id(),
                    __cache().userList.users(i).username().c_str());
        }
#endif
        for (int i = 0; i < CCD_MAX_USERS; i++) {
            const char* currName =  __cache().players[i].isSignedIn() ?
                    __cache().players[i].username().c_str() :
                    "<not signed in>";
            LOG_INFO("player[%d]: "FMT_VPLUser_Id_t" \"%s\"", i, __cache().players[i].user_id(), currName);
        }
        LOG_INFO("-------------------");
    }
}

void
Cache_LogUserCache(void)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);

    LOG_INFO("-------------------");
    LOG_INFO("CACHE (users)");
    {
        CacheAutoLock autoLock;
        int rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_WARN("LockForRead returned %d", rv);
        }

#if CCD_ENABLE_USER_SUMMARIES
        for (int i = 0; i < __cache().userList.users_size(); i++) {
            LOG_INFO("cached_user[%d]: userId="FMT_VPLUser_Id_t",\n%s",
                    i, __cache().userList.users(i).user_id(), __cache().userList.users(i).DebugString().c_str());
        }
#endif

        for (int i = 0; i < CCD_MAX_USERS; i++) {
            const CachePlayer& currPlayer = __cache().players[i];
            if (!currPlayer.isSignedIn()) {
                LOG_INFO("player[%d]: <not signed in>", i);
            } else {
                LOG_INFO("player[%d]: \"%s\", userId="FMT_VPLUser_Id_t",\n"
                         "cachedData=%s\n"
                         "session=%s",
                        i, currPlayer.username().c_str(), currPlayer.user_id(),
                        currPlayer._cachedData.DebugString().c_str(),
                        currPlayer.getSession().DebugString().c_str());
            }
        }
    }
    LOG_INFO("-------------------");
}

s32
Cache_Quit(void)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);

    VPLUser_Id_t userId = VPLUSER_ID_NONE;
    {
        CacheAutoLock autoLock;
        int rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_WARN("LockForRead returned %d", rv);
        }
        if (__cache().players[0].isSignedIn()) {
            userId = __cache().players[0].user_id();
        }
    }

    if (userId != VPLUSER_ID_NONE) {
        Cache_DeactivateUserForCcdStop(userId);
    }

    Cache_LockWrite();

    s_ccdCacheIsInit = false;

#if CCD_ENABLE_USER_SUMMARIES
    __cache().userList.Clear();
#endif

    delete[] __cache().players;

    cache_uncheckedUnlock();

    return CCD_OK;
}

s32
Cache_Clear(void)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);

    s32 rv = CCD_OK;

    {
        CacheAutoLock autoLock;
        int rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_WARN("LockForWrite returned %d", rv);
        }

#if CCD_ENABLE_USER_SUMMARIES
        Cache_UpdateUserSummaries();
        for (int i = 0; i < __cache().userList.users_size(); i++) {
            Cache_RemoveUser(__cache().userList.users(i).user_id());
        }
#endif

        delete[] __cache().players;
        __cache().players = new CachePlayer[CCD_MAX_USERS];

#if CCD_ENABLE_USER_SUMMARIES
        __cache().userList.Clear();
#endif
    }

    return rv;
}

static u64
parseCreatedForDeviceStr(const char* createdFor)
{
    u64 result = 0;
    size_t len = strlen(createdFor);
    if (len >= 3) {
        if (createdFor[0] == 'd' && createdFor[1] == '_') {
            errno = 0;
            result = strtoull(&(createdFor[2]), NULL, 16);
            if (errno != 0) {
                LOG_WARN("Failed to parse \"createdFor\"=\"%s\"", createdFor);
                result = 0;
            }
        }
    }
    return result;
}

s32 Cache_ListOwnedDatasets(VPLUser_Id_t userId,
                            ccd::ListOwnedDatasetsOutput& response,
                            bool onlyUseCache)
{
    int rv;

    if (!onlyUseCache) {
        rv = CacheMonitor_UpdateDatasetsIfNeeded(userId);
        if(rv != 0) {
            LOG_ERROR("%s failed: %d", "CacheMonitor_UpdateDatasetsIfNeeded", rv);
            goto out;
        }
    }

    // Return the results from the cache.
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        CachePlayer* user = cache_getUserByUserId(userId);
        if (user == NULL) {
            LOG_ERROR("user "FMT_VPLUser_Id_t" not signed-in", userId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }
        u64 deviceId;
        rv = ESCore_GetDeviceGuid(&deviceId);
        if (rv != 0) {
            LOG_ERROR("ESCore_GetDeviceGuid failed: %d", rv);
            goto out;
        }
        response.clear_dataset_details();
        response.clear_created_by_this_device();
        const CachedUserDetails& userDetails = user->_cachedData.details();
        for (int i = 0; i < userDetails.datasets_size(); i++) {
            *(response.add_dataset_details()) = userDetails.datasets(i).details();
            // Check if the dataset was created by this device or not.
            bool createdByThisDevice = false;
            u64 parsedAs = parseCreatedForDeviceStr(userDetails.datasets(i).details().createdfor().c_str());
            LOG_DEBUG("Comparing "FMTu64" and "FMTu64, deviceId, parsedAs);
            createdByThisDevice = (deviceId == parsedAs);
            response.add_created_by_this_device(createdByThisDevice);
        }
    }
 out:
    return rv;
}

#if CCD_ENABLE_SYNCDOWN
void Cache_NotifySyncDownModule(u64 userId,
                                u64 datasetId,
                                bool overrideAnsCheck)
{
    // No cache lock being requested throughout the NotifyDatasetContentChange processes.
    // The DocSyncDown and SyncDown seems never holding both cache lock and their own lock at the same time.
    // It should be safe to call NotifyDatasetContentChange with cache lock held
    {
        // Send notification to SyncDown.
        CacheAutoLock autoLock;
        if (autoLock.LockForRead() < 0) {
            LOG_ERROR("Failed to obtain lock");
        }
        else {
            CachePlayer *player = cache_getUserByUserId(userId);
            if (player) {
                for (int i = 0; i < player->_cachedData.details().datasets_size(); i++) {
                    const vplex::vsDirectory::DatasetDetail &dd = player->_cachedData.details().datasets(i).details();
                    if (dd.suspendedflag()) {
                        continue;
                    }
                    if (dd.datasetid() == datasetId || datasetId == 0) {
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
                        if (dd.datasetname() == "Cloud Doc") {
                            DocSyncDown_NotifyDatasetContentChange(dd.datasetid(),
                                                                overrideAnsCheck);
                        }
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC
#if CCD_ENABLE_SYNCDOWN_PICSTREAM
                        if (dd.datasetname() == "PicStream"  ||
                            dd.datasetname() == "Shared by Me" ||
                            dd.datasetname() == "Shared with Me")
                        {
                            SyncDown_NotifyDatasetContentChange(dd.datasetid(),
                                                                overrideAnsCheck,
                                                                false,
                                                                FileType_Ignored /*syncDownFileType, will be ignored if syncDownAdded is false*/);

                        }
#endif // CCD_ENABLE_SYNCDOWN_PICSTREAM
                        if(dd.datasetid() == datasetId)
                        {   // Found the particular datasetId we want.
                            break;
                        }
                    }
                }
            }
        }
    }
    return;
}
#endif // CCD_ENABLE_SYNCDOWN

s32
Cache_RemoveUser(VPLUser_Id_t userId)
{
    LOG_DEBUG("%s "FMT_VPLUser_Id_t, __func__, userId);

    s32 rv = CCD_OK;
    char dirName[CCD_PATH_MAX_LENGTH];

    DiskCache::getPathForCachedUser(userId, sizeof(dirName), dirName);

    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }

        // Logout the user if they are currently signed in.
        for (int i = 0; i < CCD_MAX_USERS; i++) {
            CachePlayer* user = cache_getUserByPlayerIndex(i);
            if (user != NULL) {
                if (user->user_id() == userId) {
                    Cache_Logout(i, VPLUSER_ID_NONE, VPL_TRUE, LOGOUT_REASON_CLEAR_CACHE);
                }
            }
        }

        rv = Util_RemoveAllFilesInDir(dirName);
        if (rv < 0) {
            LOG_WARN("Util_RemoveAllFilesInDir(%s) returned %d", dirName, rv);
        }

        rv = remove(dirName);
        if (rv < 0) {
            LOG_WARN("remove(%s) returned %d", dirName, rv);
        }
    } // unlock

#if CCD_ENABLE_USER_SUMMARIES
    rv = Cache_UpdateUserSummaries();
#endif

out:
    return rv;
}

//-------------------------

static void populateLogoutContext(const CachePlayer& user, InfraLogoutContext& context)
{
    context.sessionHandle = user.getSession().session_handle();
    context.userId = user.user_id();
    context.iasTicket = user.getSession().ias_ticket();
    context.doLogout = true;
}

static s32 Cache_ActivateOrDeactivateUser(VPLUser_Id_t userId, ActivateOrDeactivateAction action)
{
    s32 rv = 0;
    // Currently CCD only supports single logged in user, so the playerIndex will always be 0.
    const int playerIndex = 0;
    bool allowInfraLogout = true;
    bool allowRemoveFromCache = true;
    InfraLogoutContext context;

    ASSERT(!Cache_ThreadHasLock());

    if (action == DEACTIVATE_FOR_CCD_STOP) {
        // We really just want to stop the modules and the cache's VSDS update thread and player table.
        allowInfraLogout = false;
        allowRemoveFromCache = false;
    }

    if (isDeactivate(action)) {
        goto deactivate;
    }

    {
        CachePlayer& user = __cache().players[playerIndex];

        // Use manual locking (instead of a CacheAutoLock) here so that we can jump
        // to failed_to_read_user, failed_after_read, etc. without unlocking in between.
        rv = Cache_LockWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock: %d", rv);
            goto out;
        }

        // Load from disk (needs lock).
        rv = Cache_ReadUserFromDisk(playerIndex, userId);
        if (rv < 0) {
            LOG_ERROR("Failed to reload user "FMT_VPLUser_Id_t, userId);
            goto failed_to_read_user;
        }


        // Check validity (needs lock).
        {
            const UserSession& session = user._cachedData.details().session();

            // We write the session info during the login flow.
            // We clear the session info at the beginning of the logout flow.
            // If we got here and the session info is missing, CCD must have been killed before
            // the logout flow completed.
            {
                if (session.session_handle() == 0) {
                    // Incomplete logout detected; deactivate the user.
                    LOG_WARN("session_handle was 0; deactivating user");
                    rv = CCD_ERROR_NOT_SIGNED_IN;
                    allowInfraLogout = false; // It's pointless to call IAS with an invalid session.
                    goto failed_after_read;
                }
                if (session.ias_ticket().size() != 20) {
                    LOG_ERROR("Unexpected ias_ticket length "FMTu_size_t, session.ias_ticket().size());
                }
                if (session.vs_ticket().size() != 20) {
                    LOG_ERROR("Unexpected vs_ticket length "FMTu_size_t, session.vs_ticket().size());
                }
            }
#if CCD_ENABLE_STORAGE_NODE
            // Bug 7772 workaround:
            // If the user is logged in but the session secret is missing, automatically log out the user.
            if (session.session_secret().size() < 1) {
                LOG_WARN("No cached session secret; user must log in again.");
                rv = CCD_ERROR_NOT_SIGNED_IN;
                goto failed_after_read;
            }
#endif
            if (user.user_id() == 0) {
                LOG_ERROR("userId was 0!");
                rv = CCD_ERROR_INTERNAL;
                goto failed_after_read;
            }

            if (!VirtualDevice_HasCredentials()) {
                // This shouldn't happen.  We already have a logged-in user, but the device credentials
                // are missing.
                LOG_ERROR("Device credentials have been removed; logging-out "FMT_VPLUser_Id_t, userId);
                goto failed_after_read;
            }
        }

        // Track this activation.
        s_ccdLocalActivationCount++;
        // To be safe, handle wrap around case.
        if (s_ccdLocalActivationCount == ACTIVATION_ID_NONE) {
            s_ccdLocalActivationCount = 1;
        }
        user.set_local_activation_id(s_ccdLocalActivationCount);

        user._elevatedPrivilegeEndTime = 0;
        user._upToDate = CCD_USER_DATA_NONE;
        user._vsdsUpdateThreadState.init();
        user._ansCredentialReplaceTime = 0;
        user._tempDisableMetadataUpload = false;

        cache_convertCacheState(user);

        rv = Util_SpawnThread(CacheMonitor_VsdsThreadFn, (void*)user.local_activation_id(),
                UTIL_DEFAULT_THREAD_STACK_SIZE, VPL_FALSE, NULL);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "Util_SpawnThread", rv);
            goto failed_spawn_updater_thread;
        }

        // Update cache mainState so that the user will be reactivated if CCD restarts.
        // (This is redundant if we are reactivating the user during CCD restart.)
        if (action == ACTIVATE_FOR_NEW_LOGIN) {
            __cache().mainState.set_logged_in_user(userId);
            __cache().mainState.mutable_logged_out_users()->Clear();
            rv = cache_writeMainState(true);
            if (rv < 0) {
                LOG_ERROR("Failed to persist main CCD state: %d", rv);
                goto failed_to_write_main_state;
            }
        }

        Cache_Unlock();
    }

    // Start modules (must not hold cache lock).
    rv = startModulesForUser(userId);
    if (rv < 0) {
        LOG_ERROR("startModulesForUser failed: %d", rv);
        goto failed_to_start_modules;
    }

    // At this point, we are committed to succeed.

    // Technically, we shouldn't generate this event in the ACTIVATE_FOR_CCD_RESTART case:
    //if (action == ACTIVATE_FOR_NEW_LOGIN)
    { // Post a LOGIN event.
        ccd::CcdiEvent* event = new ccd::CcdiEvent();
        event->mutable_user_login()->set_user_id(userId);
        EventManagerPb_AddEvent(event);
        // event will be freed by EventManagerPb.
    }

    // Success!
    LOG_INFO("User activated.");
    goto out;

deactivate:
    // Stop modules (must not hold cache lock).
    ASSERT(!Cache_ThreadHasLock());
    {
        int temp_rc = stopModulesForUser(userId, allowRemoveFromCache);
        if (temp_rc < 0) {
            LOG_WARN("stopModulesForUser failed: %d", temp_rc);
        }
    }

 failed_to_start_modules:
    // Need cache lock for the next step.
    rv = Cache_LockWrite();
    if (rv < 0) {
        LOG_ERROR("Failed to obtain lock: %d", rv);
        goto out;
    }
 failed_to_write_main_state:
    // We will undo __cache().mainState.set_logged_in_user(userId) last (see below).

 failed_spawn_updater_thread:
    LOG_INFO("Starting VSDS thread cleanup");
    ASSERT(Cache_ThreadHasWriteLock());
    // Undo user._vsdsUpdateThreadState.init()
    __cache().players[playerIndex]._vsdsUpdateThreadState.cleanup();
    LOG_INFO("VSDS thread cleanup finished");
 failed_after_read:
    ASSERT(Cache_ThreadHasWriteLock());
    if (allowInfraLogout) {
        LOG_INFO("Starting populate logout context");
        populateLogoutContext(__cache().players[playerIndex], context);
        LOG_INFO("Populate logout context finished");
    }

    // Undo Cache_ReadUserFromDisk().
    LOG_INFO("Unloading user from ActivePlayerList");
    __cache().players[playerIndex].unloadFromActivePlayerList();
    LOG_INFO("Unload user from ActivePlayerList finished");
 failed_to_read_user:
    ASSERT(Cache_ThreadHasWriteLock());

    // Doing this last, so that it also happens in the case when "reactivating the
    // user failed during CCD restart".
    if (action != DEACTIVATE_FOR_CCD_STOP) {
        LOG_INFO("Satrting to reset logged_in_user");
        // Undo __cache().mainState.set_logged_in_user(userId);
        __cache().mainState.set_logged_in_user(VPLUSER_ID_NONE);
        {
            int temp_rc = cache_writeMainState(true);
            if (temp_rc < 0) {
                LOG_ERROR("Failed to persist main CCD state: %d", temp_rc);
            }
        }
        LOG_INFO("Reset logged_in_user finished");
    }

    // Safe to unlock now.
    Cache_Unlock();

    LOG_INFO("User deactivated, starting cleanup.");

    // Perform some final cleanup steps that should happen after any user deactivation.
    {
        // Allow other Windows users to use the local IOAC device.
        NetMan_CheckAndClearCache();
    }

    // Only has an effect if we need to logout from infra.
    privCleanupAfterLogoutIfNeeded(context);

    LOG_INFO("User deactivated, cleanup complete.");

out:
    ASSERT(!Cache_ThreadHasLock());
    return rv;
}

static s32 Cache_ActivateUserNewLogin(VPLUser_Id_t userId)
{
    return Cache_ActivateOrDeactivateUser(userId, ACTIVATE_FOR_NEW_LOGIN);
}

static s32 Cache_ActivateUserCcdRestart(VPLUser_Id_t userId)
{
    return Cache_ActivateOrDeactivateUser(userId, ACTIVATE_FOR_CCD_RESTART);
}

static s32
privStartOrStopModulesForUser(u64 userId, bool stop, bool userLogout)
{
    s32 rv;

    ASSERT(!Cache_ThreadHasLock());

    if (stop) {
        rv = 0;
        goto stop_modules;
    }

    { // Begin "start modules" logic:

        // Get the things we will need from the cache.
        s64 clusterId;
        ccd::UserSession session;
        google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo> cachedDevices;
        google::protobuf::RepeatedPtrField<vplex::vsDirectory::UserStorage> cachedUserStorage;
        bool localDeviceIsStorageNode;
        u32 localActivationId;
#if CCD_ENABLE_STORAGE_NODE
        StartStorageNodeContext snContext;
#endif
        {
            CacheAutoLock autoLock;
            rv = autoLock.LockForRead();
            if (rv < 0) {
                LOG_ERROR("Failed to obtain lock");
                goto out;
            }

            CachePlayer* user = cache_getUserByUserId(userId);
            if (user == NULL) {
                LOG_CRITICAL("User should not be null");
                goto out;
            }
            clusterId = user->cluster_id();
            session = user->getSession();
            cachedDevices = user->_cachedData.details().cached_devices();
            cachedUserStorage = user->_cachedData.details().cached_user_storage();
            localDeviceIsStorageNode = user->isLocalDeviceStorageNode();
            localActivationId = user->local_activation_id();
#if CCD_ENABLE_STORAGE_NODE
            if (localDeviceIsStorageNode) {
                LocalServers_PopulateStartStorageNodeContext(*user, snContext);
            }
#endif
        } // releases Cache Lock.

        // Update the hostnames to use the appropriate regional cluster.
        Query_UpdateClusterId(clusterId);

#if CCD_USE_ANS
        {
            // Start the ANS device client.
            rv = ANSConn_Start(
                    userId,
                    VirtualDevice_GetDeviceId(),
                    clusterId,
                    session.ans_session_key(),
                    session.ans_login_blob());
            if (rv != 0) {
                LOG_ERROR("%s failed: %d", "ANSConn_Start", rv);
                goto failed_ansconn_start;
            }
        }
#endif
        // Restore previous list of devices.
        DeviceStateCache_UpdateList(userId, cachedDevices, cachedUserStorage);
        if (session.vs_ticket().size() != VPL_USER_SERVICE_TICKET_LENGTH) {
            LOG_WARN("Unexpected ticket length "FMTu_size_t, session.vs_ticket().size());
        }

#if CCD_ENABLE_MEDIA_CLIENT
        rv = MCAInit(userId);
        if (rv != 0) {
            LOG_ERROR("MCAInit:%d", rv);
            goto failed_mca_init;
        }
#endif

        LocalServers_CreateLocalInfoObj(userId, session.instance_id());
        // TODO: do we care about failure here?

#if CCD_ENABLE_STORAGE_NODE
        if (localDeviceIsStorageNode) {
            LocalServers_StartStorageNode(snContext);
            // TODO: do we care about failure here?
        }
#endif

#if CCD_USE_MEDIA_STREAM_SERVICE
        {
            LOG_INFO("Starting streaming service for user "FMT_VPLUser_Id_t, userId);
            rv = LocalServers_StartHttpService(userId);
            if (rv < 0) {
                LOG_ERROR("Stream service failure: start error %d", rv);
                rv = CCD_ERROR_STREAM_SERVICE;
                goto failed_start_stream_service;
            }
        }
#endif

#if CCD_ENABLE_SYNCDOWN
        {
            rv = SyncDown_Start(userId);
            if (rv) {
                LOG_ERROR("Failed to start SyncDown: %d", rv);
                goto failed_start_syncdown;
            }

            CacheAutoLock autoLock;
            rv = autoLock.LockForWrite();
            if (rv < 0) {
                LOG_ERROR("Failed to obtain lock");
                goto failed_set_catchup;
            }
            CachePlayer* user = cache_getUserByUserId(userId);
            if (user == NULL) {
                LOG_CRITICAL("User should not be null");
                goto failed_set_catchup;
            }
            user->syncdown_needs_catchup_notification = true;
            LOG_DEBUG("req catchup notify to be sent");
        }
#endif
#if CCD_ENABLE_SYNCUP
        {
            rv = SyncUp_Start(userId);
            if (rv) {
                LOG_ERROR("Failed to start SyncUp: %d", rv);
                goto failed_start_syncup;
            }
        }
#endif

        // NOTE: cache_processUpdatedDatasetList() must come *after* SyncDown_Start() and MCAInit(),
        // since it assumes that both SyncDown and MCA are already running.
        {
            CacheAutoLock autoLock;
            rv = autoLock.LockForWrite();
            if (rv < 0) {
                LOG_ERROR("Failed to obtain lock");
                goto failed_process_dataset_list;
            }
            CachePlayer* user = cache_getUserByUserId(userId);
            if (user == NULL) {
                LOG_CRITICAL("User should not be null");
                goto failed_process_dataset_list;
            }
            cache_processUpdatedDatasetList(*user);
        }

#if CCD_ENABLE_DOC_SAVE_N_GO
        {
            LOG_INFO("Enabling DocSaveNGo");
            ADO_Enable_DocSaveNGo();
            LOG_INFO("Starting DocSyncDown for user "FMT_VPLUser_Id_t, userId);
            rv = DocSyncDown_Start(userId);
            if (rv) {
                LOG_ERROR("Failed to start DocSyncDown: %d", rv);
                goto failed_start_doc_sync_down;
            }
        }
#endif
        {
            LOG_INFO("Starting StatMgr for user "FMT_VPLUser_Id_t, userId);
            rv = StatMgr_Start(userId);
            if (rv) {
                LOG_ERROR("Failed to start StatMgr: %d", rv);
                goto failed_start_statmgr;
            }
            LOG_INFO("Started StatMgr for user "FMT_VPLUser_Id_t, userId);
        }
        {
            int temp_rc;
            McaMigrateThumb &migrateThumbInstance = getMcaMigrateThumbInstance();
            temp_rc = migrateThumbInstance.McaThumbResumeMigrate(localActivationId);
            if(temp_rc != 0)
            {   // Return 0 if success or has no work to do
                LOG_ERROR("McaThumbResumeMigrate("FMTu64",%d):%d",
                          userId, localActivationId, temp_rc);
            }
        }
        {
            int temp_rc = Cache_UpdatePowerMode(localActivationId);
            if(temp_rc != 0) {
                LOG_ERROR("Cache_UpdatePowerMode:%d, activationId:%d",
                          temp_rc, localActivationId);
            }
        }
        {
            LOG_INFO("Starting PinnedMediaManager");
            rv = PinnedMediaManager_Init();
            if (rv != 0) {
                LOG_ERROR("PinnedMediaManager_Init(): %d", rv);
                goto failed_start_pin_manager;
            }
            LOG_INFO("Started PinnedMediaManager");
        }
    }

    // Success!
    goto out;

stop_modules: // Begin "stop modules" logic.
    {
        int temp_rc;
        if (userLogout) {
            // Clean up the pinned media items in database.
            LOG_INFO("Try to clean up all pinned media items in database");
            temp_rc = PinnedMediaManager_RemoveAllPinnedMediaItems();
            if (temp_rc != 0) {
                LOG_ERROR("PinnedMediaManager_RemoveAllPinnedMediaItems(): %d", temp_rc);
            }
        }

        temp_rc = PinnedMediaManager_Destroy();
        if (temp_rc != 0) {
            LOG_ERROR("PinnedMediaManager_Destroy(): %d", temp_rc);
        }
    }
#if CCD_ENABLE_STORAGE_NODE
    {
        int temp_rc;
        MutexAutoLock lock(LocalServers_GetMutex());
        vss_server* storageNode = LocalServers_getStorageNode();
        if (userLogout && storageNode != NULL) {
            LOG_INFO("Try to clean up all executables in database");
            // Clean up the executables in database.
            temp_rc = RemoteExecutableManager_RemoveAllExecutables();
            if (temp_rc != 0) {
                LOG_ERROR("Error in RemoteExecutableManager_RemoveAllExecutables(): %d", temp_rc);
            }
        }
    }
#endif
 failed_start_pin_manager:
    LOG_INFO("Calling stop migrate");
    getMcaMigrateThumbInstance().McaThumbStopMigrate();
    LOG_INFO("Back from stop migrate");

    LOG_INFO("Stopping StatMgr.");
    {
        int temp_rc = StatMgr_Stop();
        if (temp_rc) {
            LOG_ERROR("Failed to stop StatMgr: %d", temp_rc);
        }
    }
    LOG_INFO("Back from stopping StatMgr.");

 failed_start_statmgr:
#if CCD_ENABLE_DOC_SAVE_N_GO
    {
        LOG_INFO("Disabling DocSaveNGo.");
        ADO_Disable_DocSaveNGo(userLogout);
        LOG_INFO("Stopping DocSyncDown.");
        int temp_rc = DocSyncDown_Stop(userLogout);
        if (temp_rc) {
            LOG_ERROR("Failed to stop DocSyncDown: %d", temp_rc);
        }
    }
 failed_start_doc_sync_down:
#endif

    {
        LOG_INFO("Removing all SyncFeatures");
        int temp_rc = SyncFeatureMgr_RemoveAll();
        if (temp_rc != 0) {
            LOG_ERROR("SyncFeatureMgr_RemoveAll:%d", temp_rc);
        }
        LOG_INFO("Back from removing all SyncFeatures");
    }
 failed_process_dataset_list:
#if CCD_ENABLE_SYNCUP
    {
        LOG_INFO("Stopping SyncUp.");
        int temp_rc = SyncUp_Stop(userLogout);
        if (temp_rc) {
            LOG_ERROR("Failed to stop SyncUp: %d", temp_rc);
        }
    }
 failed_start_syncup:
#endif
#if CCD_ENABLE_SYNCDOWN
 failed_set_catchup:
    {
        LOG_INFO("Stopping SyncDown.");
        int temp_rc = SyncDown_Stop(userLogout);
        if (temp_rc) {
            LOG_ERROR("Failed to stop SyncDown: %d", temp_rc);
        }
    }
 failed_start_syncdown:
#endif

 failed_start_stream_service:
    // Undo LocalServers_StartHttpService() and LocalServers_StartStorageNode()
    LocalServers_StopServers(userLogout);

    LOG_INFO("Calling Picstream_Destroy");
    Picstream_Destroy();
    LOG_INFO("Done calling Picstream_Destroy");

#if CCD_ENABLE_MEDIA_CLIENT
    // Undo #MCAInit().
    LOG_INFO("Calling MCALogout");
    MCALogout();
    LOG_INFO("Back from MCALogout");
 failed_mca_init:
#endif
#if CCD_USE_ANS
    // Undo #ANSConn_Start().
    {
        LOG_INFO("Stopping ANS connection.");
        (IGNORE_RESULT)ANSConn_Stop();
    }
 failed_ansconn_start:
#endif

    LOG_INFO("Restoring default hostnames.");

    // Undo #Query_UpdateClusterId(clusterId);
    Query_UpdateClusterId();

#if CCD_ENABLE_STORAGE_NODE && defined(VPL_PLAT_IS_WIN_DESKTOP_MODE)
    // Delete RemoteFile AccessControlList
    if(userLogout){
        char tempFilename[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForUserRFAccessControlList(userId, sizeof(tempFilename), tempFilename);
        int temp_rc = VPLFile_Delete(tempFilename);
        if (temp_rc) {
            LOG_ERROR("Delete %s failed, %d", tempFilename, temp_rc);
        }
    }
#endif

    // Undo #DeviceStateCache_UpdateList().
    // TODO: Probably a race condition.  I think the ANS connection thread could still add an
    //     entry after this call.
    //     But for now, do this last to minimize the chance of hitting the race.
    DeviceStateCache_UserLoggedOut(userId);

    LOG_INFO("Modules stopped.");

out:
    return rv;
}

static s32
startModulesForUser(u64 userId)
{
    return privStartOrStopModulesForUser(userId, false, false);
}

static s32
stopModulesForUser(u64 userId, bool userLogout)
{
    return privStartOrStopModulesForUser(userId, true, userLogout);
}

//-------------------------
static void cacheMonitor_privMarkDirty(CachePlayer& user, const CachedUserDataFlags& whichData)
{
    ASSERT(Cache_ThreadHasWriteLock());
    user._upToDate.unset(whichData);
    user._vsdsUpdateThreadState.willBeUpToDate.unset(whichData);

    VPLTime_t now = VPLTime_GetTimeStamp();
    if (user._vsdsUpdateThreadState.nextUpdateTimestamp == VPLTIME_INVALID) {
        user._vsdsUpdateThreadState.nextUpdateTimestamp = now + VPLTIME_FROM_SEC(3);
        user._vsdsUpdateThreadState.maxNextUpdateTimestamp = VPLTime_GetTimeStamp() + VPLTIME_FROM_SEC(10);
        // Updater thread won't wake on its own; tell it that it has work to do.
        user._vsdsUpdateThreadState.wakeUpdaterThread();
    } else if (now + VPLTIME_FROM_SEC(1) > user._vsdsUpdateThreadState.nextUpdateTimestamp) {
        user._vsdsUpdateThreadState.nextUpdateTimestamp = now + VPLTIME_FROM_SEC(1);
        // No need to wake the updater thread; it was already going to wake.
    }
}

static void cacheMonitor_requestRecreateSyncboxDataset(CachePlayer& user)
{
    // re-create dataset if need_to_recreate_syncbox_dataset is not zero.
    ASSERT(Cache_ThreadHasWriteLock());

    // Notify CacheMonitor_VsdsThreadFn to re-create syncbox dataset.
    // Simply wake up the CacheMonitor_VsdsThreadFn then it will do so.
    VPLTime_t now = VPLTime_GetTimeStamp();
    user._vsdsUpdateThreadState.nextUpdateTimestamp = now;
    user._vsdsUpdateThreadState.maxNextUpdateTimestamp = now;
    user._vsdsUpdateThreadState.wakeUpdaterThread();

}

void CacheMonitor_MarkDirty(VPLUser_Id_t userId, const CachedUserDataFlags& whichData)
{
    int rv;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        CachePlayer* user = cache_getUserByUserId(userId);
        if (user == NULL) {
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }
        cacheMonitor_privMarkDirty(*user, whichData);
    }
out:
    if (rv != 0) {
        LOG_WARN("Failed: %d", rv);
    }
}

void CacheMonitor_MarkDirtyAll(const CachedUserDataFlags& whichData)
{
    int rv;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        for (int i = 0; i < CCD_MAX_USERS; i++) {
            CachePlayer* curr = cache_getUserByPlayerIndex(i);
            if (curr != NULL) {
                cacheMonitor_privMarkDirty(*curr, whichData);
            }
        }
    }
out:
    if (rv != 0) {
        LOG_WARN("Failed: %d", rv);
    }
}

int CacheMonitor_UpdateIfNeeded(u32 activationId, const CachedUserDataFlags& whichData)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    int rv;

    // When this function is called, there is a chance we will need to block and wait for the cache
    // updater thread.  But if this thread is holding the cache lock, the cache updater thread
    // will also be blocked, leading to deadlock.
    if (Cache_ThreadHasLock()) {
        GVM_ASSERT_FAILED("Cannot update from infra with the cache lock held.");
        return CCD_ERROR_COULD_DEADLOCK;
    }
    
    u64 ansConnId = 0;
    bool needToQueryInfra = false;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        CachePlayer* user = cache_getUserByActivationId(activationId);
        if (user == NULL) {
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }
        if (!ANSConn_IsActive()) {
            // If ANS connection is not active, we definitely need to query.
            needToQueryInfra = true;
        } else {
            // Even if ANS is active, we may still need to query.
            ansConnId = ANSConn_GetLocalConnId();
            if ((whichData.datasets && !user->_upToDate.datasets) ||
                (whichData.subscriptions && !user->_upToDate.subscriptions) ||
                (whichData.linkedDevices && !user->_upToDate.linkedDevices) ||
                (whichData.storageNodes && !user->_upToDate.storageNodes) ||
                (whichData.communityData && !user->_upToDate.communityData)) {
                needToQueryInfra = true;
            }
        }
    }
    if (needToQueryInfra) {
        rv = cacheMonitor_privWaitForUpdate(activationId, ansConnId);
    } else {
        LOG_DEBUG("We are connected to ANS and requested items appear up-to-date: %s", whichData.debugString().c_str());
    }
out:
    return rv;
}

int CacheMonitor_UpdateIfNeeded(bool useUserId, VPLUser_Id_t userId, const CachedUserDataFlags& whichData)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    int rv;
    u32 activationId;

    // When this function is called, there is a chance we will need to block and wait for the cache
    // updater thread.  But if this thread is holding the cache lock, the cache updater thread
    // will also be blocked, leading to deadlock.
    if (Cache_ThreadHasLock()) {
        GVM_ASSERT_FAILED("Cannot update from infra with the cache lock held.");
        return CCD_ERROR_COULD_DEADLOCK;
    }

    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        CachePlayer* user = cache_getSyncUser(useUserId, userId);
        if (user == NULL) {
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }
        activationId = user->local_activation_id();
    }
    rv = CacheMonitor_UpdateIfNeeded(activationId, whichData);
out:
    return rv;
}

//-------------------------
void Cache_OnDifferentNetwork()
{
    LocalServers_GetHttpService().update_stream_servers(/*force_drop_conns*/true);
}

void Cache_OnAnsConnected()
{
    int rv;
    bool found = false;
    u64 userId = 0;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if(rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        CachePlayer* curr = cache_getUserByPlayerIndex(0);
        if(curr != NULL) {
            found = true;
            userId = curr->user_id();
        }
    }

    ASSERT(!Cache_ThreadHasLock());
    if(found) {
        LocalServers_GetHttpService().update_stream_servers(/*force_drop_conns*/false);

        LocalServers_ReportDeviceOnline();

        SyncFeatureMgr_RequestRemoteScansForUser(userId);
        Cache_NotifySyncDownModule(userId, 0, false);
#if CCD_ENABLE_SYNCUP
        int rc = SyncUp_NotifyConnectionChange(false);
        if(rc != 0) {
            LOG_WARN("SyncUp_NotifyConnectionChange:%d", rc);
        }
#endif // CCD_ENABLE_SYNCUP
        int err = StatMgr_NotifyConnectionChange();
        if(err != 0) {
            LOG_WARN("StatMgr_NotifyConnectionChange:%d", err);
        }
    } else {
        LOG_WARN("CacheUser not found when ANS connected");
    }
 out:
    if(rv != 0) {
        LOG_WARN("Failed: %d", rv);
    }
}

void Cache_NetworkStatusChange(VPLNet_addr_t localAddr)
{
#if CCD_ENABLE_STORAGE_NODE
    MutexAutoLock lock(LocalServers_GetMutex());
    vss_server* storageNode = LocalServers_getStorageNode();
    if (storageNode != NULL) {
        storageNode->notifyNetworkChange(localAddr);
    }
#endif
}

void Cache_ProcessSyncAgentNotification(const notifier::SyncAgentNotification& notification, uint64_t asyncId)
{
    LOG_INFO("Handling notification (asyncId="FMTu64"): %s", asyncId, notification.DebugString().c_str());

    for (int i = 0; i < notification.user_storage_update_size(); i++) {
        // A Storage Node was added, deleted, or had its feature list updated.
        const notifier::UserStorageUpdate& curr = notification.user_storage_update(i);
        // We will need to update CCD's cache from the infrastructure.
        CacheMonitor_MarkStorageNodesDirty(curr.recipient_uid());
        // Also mark the datasets dirty, since changes to the feature list can cause
        // VIRT_DRIVE and FS datasets to become visible/invisible.
        CacheMonitor_MarkDatasetsDirty(curr.recipient_uid());
    }
    for (int i = 0; i < notification.psn_connection_update_size(); i++) {
        // Sent by the infra when it receives a VSDS UpdateStorageNodeConnection request to
        // update the IP address/port (reported and detected) of a user's Storage Node.
        const notifier::PSNConnectionUpdate& curr = notification.psn_connection_update(i);
        // For datasets that reside on this storage node, the "dataset location" will be updated.
        CacheMonitor_MarkDatasetsDirty(curr.recipient_uid());
        // The cached StorageNode data will need to be updated.
        CacheMonitor_MarkStorageNodesDirty(curr.recipient_uid());
    }
    for (int i = 0; i < notification.dataset_update_size(); i++) {
        const notifier::DatasetUpdate& curr = notification.dataset_update(i);
        // We will need to update CCD's cache from the infrastructure.
        CacheMonitor_MarkDatasetsDirty(curr.recipient_uid());
    }
    for (int i = 0; i < notification.dataset_content_update_size(); i++) {
        const notifier::DatasetContentUpdate& curr = notification.dataset_content_update(i);

        {
            ccd::CcdiEvent *ep = new ccd::CcdiEvent();
            ccd::EventDatasetContentChange *cp = ep->mutable_dataset_content_change();
            cp->set_dataset_id(curr.dataset_id());
            EventManagerPb_AddEvent(ep);
            // event will be freed by EventManagerPb.
        }

        SyncFeatureMgr_ReportRemoteChange(curr);

#if CCD_ENABLE_SYNCDOWN
        Cache_NotifySyncDownModule(curr.recipient_uid(),
                                   curr.dataset_id(),
                                   false);
#endif // CCD_ENABLE_SYNCDOWN
    }
    for (int i = 0; i < notification.subscription_update_size(); i++) {
        const notifier::SubscriptionUpdate& curr = notification.subscription_update(i);
        if (VirtualDevice_GetDeviceId() == curr.device_id()) {
            // We will need to update CCD's cache from the infrastructure.
            CacheMonitor_MarkSubscriptionsDirty(curr.recipient_uid());
        }
    }
    for (int i = 0; i < notification.device_update_size(); i++) {
        const notifier::DeviceUpdate& curr = notification.device_update(i);
        CacheMonitor_MarkLinkedDevicesDirty(curr.recipient_uid());
    }
    // TODO: replaced by device_update; remove when ready
    for (int i = 0; i < notification.device_unlinked_size(); i++) {
        const notifier::DeviceUnlinked& curr = notification.device_unlinked(i);
        CacheMonitor_MarkLinkedDevicesDirty(curr.recipient_uid());
    }
    // TODO: replaced by device_update; remove when ready
    for (int i = 0; i < notification.device_linked_size(); i++) {
        const notifier::DeviceLinked& curr = notification.device_linked(i);
        CacheMonitor_MarkLinkedDevicesDirty(curr.recipient_uid());
    }
}

void Cache_ProcessExpiredSession(u64 sessionHandle, ccd::LogoutReason_t reason)
{
    s32 rv;

    u64 actualSessionHandle = 0;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        CachePlayer* user = cache_getTheUser();
        if (user != NULL) {
            if ((sessionHandle == 0) || (sessionHandle == user->session_handle())) {
                LOG_INFO("Local session has been invalidated; logging out user.");
                actualSessionHandle = user->session_handle();
            } else {
                LOG_INFO("Expired session is not the local session; ignoring.");
            }
        }
    } // releases cache lock

    ASSERT(!Cache_ThreadHasLock());

    if (actualSessionHandle != 0) {
        rv = CCDPrivBeginLogout(true);
        if (rv < 0) {
            goto out;
        } else {
            ON_BLOCK_EXIT(CCDPrivEndLogout);
            Cache_DeactivateUserForLogout(actualSessionHandle, reason);
        } // ON_BLOCK_EXIT will call CCDPrivEndLogout() now.
    }
out:
    return;
}

s32 cache_getBackgroundModeIntervalSecs()
{
    ASSERT(Cache_ThreadHasLock());
    int result = __cache().mainState.background_mode_interval_sec();
    if (result < 0) {
        // Use the platform-specific default.
#ifdef ANDROID
        result = 30 * VPLTIME_SECONDS_PER_MINUTE;
#else
        result = 0;
#endif
    }
    return result;
}

#ifndef VPL_PLAT_IS_WIN_DESKTOP_MODE
static s32 privSetForegroundMode(int intervalToUse)
{
    ANSConn_SetForegroundMode(intervalToUse);
    return CCD_OK;
}
#endif  //VPL_PLAT_IS_WIN_DESKTOP_MODE

static int requestPicstreamInit(CachePlayer& user)
{
    // Picstream does not use cache lock by itself, so it's safe to call into Picstream with Cache lock held.
    ASSERT(Cache_ThreadHasWriteLock());
    int rc;
    {
        char* internalUploadPicDir = (char*)malloc(CCD_PATH_MAX_LENGTH);
        if(internalUploadPicDir == NULL) {
            LOG_ERROR("Out of mem");
            return CCD_ERROR_NOMEM;
        }
        ON_BLOCK_EXIT(free, internalUploadPicDir);
        internalUploadPicDir[0] = '\0';
        DiskCache::getPathForUserUploadPicstream(user.user_id(),
                                                 CCD_PATH_MAX_LENGTH,
                                                 internalUploadPicDir);
        rc = Util_CreatePath(internalUploadPicDir, VPL_TRUE);
        if(rc != 0) {
            LOG_ERROR("Unable to create internal picstream upload path:%s, %d",
                      internalUploadPicDir, rc);
            return rc;
        }
        const VPLTime_t waitQTime = VPLTime_FromSec(2);
        rc = Picstream_Init(user.user_id(), std::string(internalUploadPicDir), waitQTime);
        if(rc == CCD_ERROR_ALREADY_INIT) {
            return 0;
        }else if(rc != 0) {
            LOG_ERROR("Picstream_Init:%d, %s", rc, internalUploadPicDir);
            return rc;
        }
    }

    int rv = 0;
    {
        int rc;

        google::protobuf::RepeatedPtrField<ccd::PicstreamDir> mutableUploadDirs;
        user.get_camera_upload_dirs(&mutableUploadDirs);
        for(int iter = 0; iter<mutableUploadDirs.size(); ++iter) {
            rc = Picstream_AddMonitorDir(mutableUploadDirs.Get(iter).directory().c_str(),
                                         (u32)mutableUploadDirs.Get(iter).index(),
                                         mutableUploadDirs.Get(iter).never_init());
            if(rc != 0) {
                LOG_ERROR("Picstream_AddMonitorDir, index:%d, rc:%d, %s, %d",
                          (int)mutableUploadDirs.Get(iter).index(),
                          rc,
                          mutableUploadDirs.Get(iter).directory().c_str(),
                          mutableUploadDirs.Get(iter).never_init());
            }else if(mutableUploadDirs.Get(iter).never_init()) {
                u32 index_out;
                rc = user.clear_camera_upload_never_init(
                        mutableUploadDirs.Get(iter).directory(), index_out);
            }
        }

        if(user.get_enable_camera_roll_trigger()) {
            rc = Picstream_Enable(true);
            if(rc != 0) {
                LOG_ERROR("Picstream Enable from previously set trigger:%d", rc);
                rv = rc;
            }else{
                user.set_enable_camera_roll_trigger(false);
            }
        }
        rc = user.writeCachedData(false);
        if(rc != 0) {
            LOG_ERROR("Write cache file:%d", rc);
            rv = rc;
        }

        if (rv != 0) {
            LOG_ERROR("Picstream Init failed:%d", rv);
            return rv;
        }
    }
    return rv;
}

int GetMediaMetadataDatasetByUserId(VPLUser_Id_t userId,
                                    u64& datasetId_out)
{
    datasetId_out = 0;
    CacheAutoLock autoLock;
    int rc = autoLock.LockForRead();
    if (rc != 0) {
        LOG_ERROR("LockForRead:%d", rc);
    }
    CachePlayer* user = cache_getUserByUserId(userId);
    if(user==NULL) {
        LOG_ERROR("cache_getUserByUserId("FMTu64")", userId);
        return CCD_ERROR_NOT_SIGNED_IN;
    }

    const vplex::vsDirectory::DatasetDetail* dset = Util_FindMediaMetadataDataset(user->_cachedData.details());
    if(dset == NULL) {
        LOG_WARN("User("FMTu64") does not have media metadata dataset.", userId);
        return CCD_ERROR_DATASET_NOT_FOUND;
    }
    datasetId_out = dset->datasetid();
    return CCD_OK;
}

static const SyncPolicy SYNC_POLICY_DEFAULT;

static SyncPolicy getMediaMetadataSyncPolicy()
{
    SyncPolicy result;
    result.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_USE_SIZE_FOR_MIGRATION;
    return result;
}
static const SyncPolicy SYNC_POLICY_MM = getMediaMetadataSyncPolicy();

static const char* METADATA_DIRS[] = {
        "/ph/i",
        "/ph/t",
        "/mu/i",
        "/mu/t",
        "/vi/i",
        "/vi/t"
};

static BasicSyncConfig MCA_SYNC_CONFIGS[] = {
#   if CCD_ENABLE_MEDIA_METADATA_DOWNLOAD
        { SYNC_FEATURE_PHOTO_METADATA,   SYNC_TYPE_ONE_WAY_DOWNLOAD, SYNC_POLICY_MM,      METADATA_DIRS[0] },
        { SYNC_FEATURE_PHOTO_THUMBNAILS, SYNC_TYPE_ONE_WAY_DOWNLOAD, SYNC_POLICY_MM,      METADATA_DIRS[1] },
        { SYNC_FEATURE_MUSIC_METADATA,   SYNC_TYPE_ONE_WAY_DOWNLOAD, SYNC_POLICY_MM,      METADATA_DIRS[2] },
        { SYNC_FEATURE_MUSIC_THUMBNAILS, SYNC_TYPE_ONE_WAY_DOWNLOAD, SYNC_POLICY_MM,      METADATA_DIRS[3] },
        { SYNC_FEATURE_VIDEO_METADATA,   SYNC_TYPE_ONE_WAY_DOWNLOAD, SYNC_POLICY_MM,      METADATA_DIRS[4] },
        { SYNC_FEATURE_VIDEO_THUMBNAILS, SYNC_TYPE_ONE_WAY_DOWNLOAD, SYNC_POLICY_MM,      METADATA_DIRS[5] },
#   endif //CCD_ENABLE_MEDIA_METADATA_DOWNLOAD
        { SYNC_FEATURE_PLAYLISTS,        SYNC_TYPE_TWO_WAY,          SYNC_POLICY_DEFAULT, MEDIA_METADATA_SUBDIR_PLAYLISTS },
};

/// For Syncbox, we want to keep retrying errors for roughly 6 months.
// 6 months * 30 days per month * 24 hours per day * 4 times per hour (assuming the usual 15 minute retry).
#define SYNCBOX_ERROR_RETRY_LIMIT                       (6 * 30 * 24 * 4)

static SyncPolicy getSyncboxSyncPolicy()
{
    SyncPolicy result;
    result.what_to_do_with_loser = SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER;
    result.max_error_retry_count = SYNCBOX_ERROR_RETRY_LIMIT;
    result.how_to_compare = SyncPolicy::FILE_COMPARE_POLICY_USE_HASH;
    result.case_insensitive_compare = true;
    return result;
}
static const SyncPolicy SYNC_POLICY_SYNCBOX = getSyncboxSyncPolicy();

void Cache_GetClientSyncConfig(const BasicSyncConfig*& clientSyncConfigs_out,
                               int& clientSyncConfigsSize_out)
{
    clientSyncConfigs_out = MCA_SYNC_CONFIGS;
    clientSyncConfigsSize_out = ARRAY_ELEMENT_COUNT(MCA_SYNC_CONFIGS);
}

/// Catalog ID (same as the deviceId of the device that is serving the media).
#define CATALOG_DIR_PREFIX  "ca"

#if CCD_ENABLE_MEDIA_SERVER_AGENT

struct MsaSyncConfigPath_t {
    const char* path;
    SyncFeature_t feature;
    MsaSyncConfigPath_t(const char* myPath, SyncFeature_t myFeature)
    :  path(myPath),
       feature(myFeature)
    {}
};

static const MsaSyncConfigPath_t MSA_SYNC_CONFIG_PATHS[] = {
        MsaSyncConfigPath_t(METADATA_DIRS[0], SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD),
        MsaSyncConfigPath_t(METADATA_DIRS[1], SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD),
        MsaSyncConfigPath_t(METADATA_DIRS[2], SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD),
        MsaSyncConfigPath_t(METADATA_DIRS[3], SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD),
        MsaSyncConfigPath_t(METADATA_DIRS[4], SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD),
        MsaSyncConfigPath_t(METADATA_DIRS[5], SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD)
};
#endif  //CCD_ENABLE_MEDIA_SERVER_AGENT

#if !CCD_SYNC_CONFIGS_ALWAYS_ACTIVE
static inline bool isAppActive(CcdApp_t appType)
{
    return (__cache().foregroundAppType == appType);
}
# if CCD_ALLOW_BACKGROUND_SYNC
// Both foreground and background are able to sync.
static bool isSyncEnabled()
{
    return (__cache().powerMode == S_POWER_FOREGROUND) ||
           (__cache().powerMode == S_POWER_BACKGROUND);
}
# else
// Sync only enabled when app is in foreground.
static bool isSyncEnabled()
{
    return (__cache().powerMode == S_POWER_FOREGROUND);
}
# endif
#endif

static inline std::string getArchiveStorageSyncboxDatasetName(u64 deviceId)
{
    return SSTR(SYNCBOX_DATASET_NAME_BASE << "-" << deviceId);
}

std::string GetLocalArchiveStorageSyncboxDatasetName()
{
    u64 localDeviceId = VirtualDevice_GetDeviceId();
    return getArchiveStorageSyncboxDatasetName(localDeviceId);
}

static s32 privUpdateSyncConfigPriorities(const CachePlayer& user,
                                          bool blockingDisable)
{
    ASSERT(Cache_ThreadHasLock());
    int rv = 0;

    const VPLUser_Id_t userId = user.user_id();
    const ccd::CachedUserDetails& userDetails = user._cachedData.details();

    // Media metadata
    {
        const vplex::vsDirectory::DatasetDetail* mediaMetadataDataset = Util_FindMediaMetadataDataset(userDetails);
        if (mediaMetadataDataset == NULL) {
            LOG_INFO("Media metadata dataset not found.");
            goto skip_media_metadata;
        }

        // Note: disabling this code path is an optimization to avoid redundant calls
        // to SyncFeatureMgr_SetActive.  It should actually be correct to run this code
        // even when CCD_SYNC_CONFIGS_ALWAYS_ACTIVE is true.
#if !CCD_SYNC_CONFIGS_ALWAYS_ACTIVE

        // Bug 9608: Disable metadata download for Orbe.
        // Bug 10887: But we still need playlists.
# if CCD_ENABLE_MEDIA_METADATA_DOWNLOAD
        // Photo
        {
            bool active = isSyncEnabled() &&
                    (isAppActive(CCD_APP_PHOTO) ||
                     isAppActive(CCD_APP_ALL_MEDIA));
            SyncFeatureMgr_SetActive(userId, mediaMetadataDataset->datasetid(),
                    SYNC_FEATURE_PHOTO_METADATA, active);
            SyncFeatureMgr_SetActive(userId, mediaMetadataDataset->datasetid(),
                    SYNC_FEATURE_PHOTO_THUMBNAILS, active);
        }
        // Music
        {
            bool active = isSyncEnabled() &&
                    (isAppActive(CCD_APP_MUSIC) ||
                    isAppActive(CCD_APP_MUSIC_AND_VIDEO) ||
                    isAppActive(CCD_APP_ALL_MEDIA));
            SyncFeatureMgr_SetActive(userId, mediaMetadataDataset->datasetid(),
                    SYNC_FEATURE_MUSIC_METADATA, active);
            SyncFeatureMgr_SetActive(userId, mediaMetadataDataset->datasetid(),
                    SYNC_FEATURE_MUSIC_THUMBNAILS, active);
        }
        // Video
        {
            bool active = isSyncEnabled() &&
                    (isAppActive(CCD_APP_VIDEO) ||
                    isAppActive(CCD_APP_MUSIC_AND_VIDEO) ||
                    isAppActive(CCD_APP_ALL_MEDIA));
            SyncFeatureMgr_SetActive(userId, mediaMetadataDataset->datasetid(),
                    SYNC_FEATURE_VIDEO_METADATA, active);
            SyncFeatureMgr_SetActive(userId, mediaMetadataDataset->datasetid(),
                    SYNC_FEATURE_VIDEO_THUMBNAILS, active);
        }
# endif

        // Playlists
        {
            bool active = isSyncEnabled() &&
                    (isAppActive(CCD_APP_MUSIC) ||
                    isAppActive(CCD_APP_MUSIC_AND_VIDEO) ||
                    isAppActive(CCD_APP_ALL_MEDIA));
            SyncFeatureMgr_SetActive(userId, mediaMetadataDataset->datasetid(),
                    SYNC_FEATURE_PLAYLISTS, active);
        }
#endif

#if CCD_ENABLE_MEDIA_SERVER_AGENT
        {
            bool active = !user._tempDisableMetadataUpload;
            // Tell all sync configs to change state
            for (int i = 0; i < ARRAY_ELEMENT_COUNT(MSA_SYNC_CONFIG_PATHS); i++) {
                SyncFeatureMgr_SetActive(userId, mediaMetadataDataset->datasetid(),
                        MSA_SYNC_CONFIG_PATHS[i].feature, active);
            }

            if(blockingDisable && !active)
            {   // If blockingDisable is requested and we are disabling, wait on each of
                // the syncFeatures to end.
                for (int i = 0; i < ARRAY_ELEMENT_COUNT(MSA_SYNC_CONFIG_PATHS); i++) {
                    SyncFeatureMgr_WaitForDisable(userId,
                                                  mediaMetadataDataset->datasetid(),
                                                  MSA_SYNC_CONFIG_PATHS[i].feature);
                }
            }
        }
#endif //CCD_ENABLE_MEDIA_SERVER_AGENT
    }
skip_media_metadata:

    // Notes
#if 1
    // For now, SYNC_FEATURE_NOTES is always active while enabled via the CCDI API.
    // SyncFeatureMgr_Add(SYNC_FEATURE_NOTES, active=true) is called in cache_registerSyncConfigs() and
    // we never call SyncFeatureMgr_SetActive() for it.
#else
    if (userDetails.enable_notes_sync()) {
        const vplex::vsDirectory::DatasetDetail* notesDataset = Util_FindNotesDataset(userDetails);
        if (notesDataset == NULL) {
            LOG_INFO("Notes dataset not found.");
            goto skip_notes;
        }

        bool active = TODO_PUT_CONDITION_HERE;
        SyncFeatureMgr_SetActive(userId, notesDataset->datasetid(),
                SYNC_FEATURE_NOTES, active);
    }
skip_notes:
#endif

    // Syncbox
#if 1
    // For now, SYNC_FEATURE_SYNCBOX is always active while enabled via the CCDI API.
    // SyncFeatureMgr_Add(SYNC_FEATURE_SYNCBOX, active=true) is called in cache_registerSyncConfigs() and
    // we never call SyncFeatureMgr_SetActive() for it.
#else
    if (userDetails.syncbox_sync_settings().enable_sync()) {
        const vplex::vsDirectory::DatasetDetail* syncboxDataset = Util_FindSyncboxArchiveStorageDataset(userDetails);
        if (syncboxDataset == NULL) {
            LOG_INFO("Syncbox dataset not found.");
            goto skip_syncbox;
        }

        bool active = TODO_PUT_CONDITION_HERE;
        SyncFeatureMgr_SetActive(userId, syncboxDataset->datasetid(),
                SYNC_FEATURE_SYNCBOX, active);
    }
skip_syncbox:
#endif

    // For now, SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES and SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES
    // are always active while enabled via the CCDI API.

    return rv;
}

int SyncFeatureMgr_DisableEnableMetadataUpload(u64 userId, bool b_enable)
{
    int rv;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }

        CachePlayer* user = cache_getUserByUserId(userId);
        if (user == NULL) {
            LOG_ERROR("Could not get session for user:"FMT_VPLUser_Id_t, userId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }
        bool newTempDisable = !b_enable;
        if (user->_tempDisableMetadataUpload != newTempDisable) {
            user->_tempDisableMetadataUpload = newTempDisable;
            privUpdateSyncConfigPriorities(*user, true);
        }
    }
out:
    return rv;
}

// Mapping of appType to syncFeatures.  Does not require lock, all static.
void GetSyncFeaturesFromAppType(ccd::CcdApp_t appType,
                                std::vector<SyncFeature>& features_out)
{
    features_out.clear();
    if(appType == CCD_APP_PHOTO ||
       appType == CCD_APP_ALL_MEDIA)
    {
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_PHOTO_METADATA, false));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_PHOTO_THUMBNAILS, false));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD, true));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_METADATA_PHOTO_THUMB_UPLOAD, true));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_PICSTREAM_DOWNLOAD_FULL_RES, false));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_PICSTREAM_DOWNLOAD_LOW_RES, false));
    }
    if(appType == CCD_APP_MUSIC ||
       appType == CCD_APP_MUSIC_AND_VIDEO ||
       appType == CCD_APP_ALL_MEDIA)
    {
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_MUSIC_METADATA, false));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_MUSIC_THUMBNAILS, false));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD, true));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_METADATA_MUSIC_THUMB_UPLOAD, true));
    }
    if(appType == CCD_APP_VIDEO ||
       appType == CCD_APP_MUSIC_AND_VIDEO ||
       appType == CCD_APP_ALL_MEDIA)
    {
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_VIDEO_METADATA, false));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_VIDEO_THUMBNAILS, false));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD, true));
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_METADATA_VIDEO_THUMB_UPLOAD, true));
    }
    if(appType == CCD_APP_MUSIC ||
       appType == CCD_APP_MUSIC_AND_VIDEO ||
       appType == CCD_APP_ALL_MEDIA)
    {
        features_out.push_back(SyncFeature((int)SYNC_FEATURE_PLAYLISTS, true));
    }
}

static s32 privUpdateSyncConfigPriorities(u32 activationId)
{
    ASSERT(Cache_ThreadHasLock());
    if (activationId == ACTIVATION_ID_NONE) {
        return 0;
    }
    CachePlayer* user = cache_getUserByActivationId(activationId);
    if (user == NULL) {
        LOG_INFO("User is no longer logged in");
        return CCD_ERROR_NOT_SIGNED_IN;
    }
    return privUpdateSyncConfigPriorities(*user, false);
}

int Cache_SetThumbnailMigrateDeletePhase(u32 activationId)
{
    int rv = 0;
    CacheAutoLock autoLock;
    rv = autoLock.LockForWrite();
    if(rv != 0) {
        LOG_ERROR("autoLock.LockForWrite:%d", rv);
        return rv;
    }
    CachePlayer* user = cache_getUserByActivationId(activationId);
    if (user == NULL) {
        rv = CCD_ERROR_NOT_SIGNED_IN;
        return rv;
    }

    if(user->_cachedData.details().has_migrate_mm_thumb_download_path()) {
        user->_cachedData.mutable_details()->set_mm_thumb_download_path(
                user->_cachedData.details().
                    migrate_mm_thumb_download_path().mm_thumb_dest_dir());
        LOG_INFO("Migrate thumbDir: Set mm_thumb_download_path to %s",
                 user->_cachedData.details().mm_thumb_download_path().c_str());

        user->_cachedData.mutable_details()->
                mutable_migrate_mm_thumb_download_path()->
                set_mm_delete_phase(true);
    }
    user->writeCachedData(false);
    return rv;
}

int Cache_ClearThumbnailMigrate(u32 activationId)
{
    int rv = 0;
    CacheAutoLock autoLock;
    rv = autoLock.LockForWrite();
    if(rv != 0) {
        LOG_ERROR("autoLock.LockForWrite:%d", rv);
        return rv;
    }
    CachePlayer* user = cache_getUserByActivationId(activationId);
    if (user == NULL) {
        rv = CCD_ERROR_NOT_SIGNED_IN;
        return rv;
    }

    user->_cachedData.mutable_details()->clear_migrate_mm_thumb_download_path();
    user->writeCachedData(false);
    return rv;
}

void cache_registerSyncConfigs(CachePlayer& user)
{
    ASSERT(Cache_ThreadHasWriteLock());
    const VPLUser_Id_t userId = user.user_id();
    LocalStagingArea syncboxStagingAreaInfo;


    const ccd::CachedUserDetails& userDetails = user._cachedData.details();

    // First, remove any SyncConfigs that have had their dataset deleted.
    {
        vector<u64> currentDatasetIdList;
        currentDatasetIdList.reserve(userDetails.datasets_size());
        for (int i = 0; i < userDetails.datasets_size(); i++) {
            currentDatasetIdList.push_back(userDetails.datasets(i).details().datasetid());
        }
        int temp_rc = SyncFeatureMgr_RemoveDeletedDatasets(user.user_id(), currentDatasetIdList);
        if (temp_rc < 0) {
            LOG_ERROR("SyncFeatureMgr_RemoveDeletedDatasets failed: %d", temp_rc);
            // I'm not sure why this would happen; continue anyway.
        }
    }

    // Media Metadata
    {
        const vplex::vsDirectory::DatasetDetail* mediaMetadataDataset = Util_FindMediaMetadataDataset(userDetails);
        if (mediaMetadataDataset == NULL) {
            LOG_INFO("Media Metadata dataset not found");
            goto skip_media_metadata;
        }

        if (mediaMetadataDataset->suspendedflag()) {
            LOG_INFO("User's Media Metadata dataset:"FMTu64" SUSPENDED",
                     mediaMetadataDataset->datasetid());
            // Check to remove suspended syncConfigs
            int rc = SyncFeatureMgr_RemoveByDatasetId(user.user_id(),
                                                      mediaMetadataDataset->datasetid());
            if (rc != 0) {
                LOG_ERROR("SyncFeatureMgr_RemoveByDatasetId("FMTu64","FMTu64"):%d",
                          user.user_id(), mediaMetadataDataset->datasetid(), rc);
            }
            goto skip_media_metadata;
        }

        LOG_INFO("User's Media Metadata dataset:"FMTu64, mediaMetadataDataset->datasetid());
        char defaultMediaMetaDownDir[CCD_PATH_MAX_LENGTH];
        DiskCache::getDirectoryForMediaMetadataDownload(userId, sizeof(defaultMediaMetaDownDir), defaultMediaMetaDownDir);
        for (int i = 0; i < ARRAY_ELEMENT_COUNT(MCA_SYNC_CONFIGS); i++) {
            SyncType syncType = MCA_SYNC_CONFIGS[i].type;
            std::string mediaMetadataDownDir(defaultMediaMetaDownDir);
            SyncFeature_t syncFeature = MCA_SYNC_CONFIGS[i].syncConfigId;
            if(syncFeature == SYNC_FEATURE_PHOTO_THUMBNAILS ||
               syncFeature == SYNC_FEATURE_MUSIC_THUMBNAILS ||
               syncFeature == SYNC_FEATURE_VIDEO_THUMBNAILS)
            {   // Check if thumbnail download path has been migrated.
                if(user._cachedData.details().has_migrate_mm_thumb_download_path() &&
                   !user._cachedData.details().migrate_mm_thumb_download_path().mm_delete_phase())
                {   // Must disable thumbnail download SyncConfigs while migrating.
                    LOG_INFO("Currently in migration.  Will not start SyncConfig %d", syncFeature);
                    continue;
                }
                // Need to check the list for disabled thumbnail syncing
                int numDisabled = user._cachedData.details().mm_thumb_sync_disabled_types_size();
                if(numDisabled != 0) {
                    int i;
                    for(i = 0; i < numDisabled; i++) {
                        if(syncFeature == user._cachedData.details().mm_thumb_sync_disabled_types(i)) {
                            break;
                        }
                    }
                    if(i < numDisabled) {
                        LOG_INFO("thumb_sync_disabled for %d", syncFeature);
                        if (__ccdConfig.mediaMetadataThumbVirtDownload) {
                            syncType = SYNC_TYPE_ONE_WAY_DOWNLOAD_PURE_VIRTUAL_SYNC;
                        } else {
                            // Completely skip this SyncConfig.
                            continue;
                        }
                    }
                }
                if(user._cachedData.details().has_mm_thumb_download_path()) {
                    mediaMetadataDownDir =
                            user._cachedData.details().mm_thumb_download_path();
                }
            }
            bool createDedicatedThread = false;
            if(syncFeature == SYNC_FEATURE_MUSIC_METADATA ||
               syncFeature == SYNC_FEATURE_PHOTO_METADATA ||
               syncFeature == SYNC_FEATURE_VIDEO_METADATA)
            {
                createDedicatedThread = true;
            }
            int rv = SyncFeatureMgr_Add(
                    userId,
                    mediaMetadataDataset->datasetid(),
                    VCS_CATEGORY_METADATA,
                    MCA_SYNC_CONFIGS[i].syncConfigId,
                    CCD_SYNC_CONFIGS_ALWAYS_ACTIVE, // On mobile platforms, this SyncFeature may be paused while enabled.
                    syncType,
                    MCA_SYNC_CONFIGS[i].sync_policy,
                    mediaMetadataDownDir + MCA_SYNC_CONFIGS[i].pathSuffix,
                    MCA_SYNC_CONFIGS[i].pathSuffix,
                    createDedicatedThread,
                    0,
                    0,
                    NULL,
                    true);
            if (rv != 0) {
                LOG_ERROR("SyncFeatureMgr_Add for MCA_SYNC_CONFIGS[%d] failed: %d", i, rv);
            }
        }

#if CCD_ENABLE_MEDIA_SERVER_AGENT
        {
            char mediaMetaUpDir[CCD_PATH_MAX_LENGTH];
            DiskCache::getDirectoryForMediaMetadataUpload(userId, sizeof(mediaMetaUpDir), mediaMetaUpDir);

            char thisDevice[17];
            {
                u64 thisDeviceId;
                int rv = ESCore_GetDeviceGuid(&thisDeviceId);
                if (rv != 0) {
                    LOG_ERROR("%s failed: %d", "ESCore_GetDeviceGuid", rv);
                    goto skip_media_metadata;
                }
                snprintf(thisDevice, ARRAY_SIZE_IN_BYTES(thisDevice), "%016"PRIx64, thisDeviceId);
            }
            SyncType syncType;
            if (__ccdConfig.mediaMetadataUploadVirtualSync == 0) {
                syncType = SYNC_TYPE_ONE_WAY_UPLOAD;
            } else if (__ccdConfig.mediaMetadataUploadVirtualSync == 2) {
                syncType = SYNC_TYPE_ONE_WAY_UPLOAD_PURE_VIRTUAL_SYNC;
            } else {
                syncType = SYNC_TYPE_ONE_WAY_UPLOAD_HYBRID_VIRTUAL_SYNC;
            }
            for (int i = 0; i < ARRAY_ELEMENT_COUNT(MSA_SYNC_CONFIG_PATHS); i++)
            {
                SyncFeature_t syncFeature = MSA_SYNC_CONFIG_PATHS[i].feature;
                bool createDedicatedThread = false;
                if(syncFeature == SYNC_FEATURE_METADATA_MUSIC_INDEX_UPLOAD ||
                   syncFeature == SYNC_FEATURE_METADATA_PHOTO_INDEX_UPLOAD ||
                   syncFeature == SYNC_FEATURE_METADATA_VIDEO_INDEX_UPLOAD)
                {
                    createDedicatedThread = true;
                }

                int rv = SyncFeatureMgr_Add(userId,
                        mediaMetadataDataset->datasetid(),
                        VCS_CATEGORY_METADATA,
                        MSA_SYNC_CONFIG_PATHS[i].feature,
                        true, // This SyncFeature will never be paused while enabled.
                        syncType,
                        SYNC_POLICY_MM,
                        std::string(mediaMetaUpDir) + MSA_SYNC_CONFIG_PATHS[i].path + "/"CATALOG_DIR_PREFIX + thisDevice,
                        std::string(MSA_SYNC_CONFIG_PATHS[i].path) + "/"CATALOG_DIR_PREFIX + thisDevice,
                        createDedicatedThread,
                        0,
                        0,
                        NULL,
                        true);
                if (rv != 0) {
                    LOG_ERROR("SyncFeatureMgr_Add for MSA_SYNC_CONFIG_PATHS[%d] failed: %d", i, rv);
                }
            }
        }
#endif //CCD_ENABLE_MEDIA_SERVER_AGENT
    }
 skip_media_metadata:

    // Notes
    if (user._cachedData.details().enable_notes_sync()) {
        if (!user._cachedData.details().has_notes_sync_path()) {
            LOG_WARN("No notes sync path; cannot sync notes!");
            goto skip_notes;
        }

        const vplex::vsDirectory::DatasetDetail* notesDataset = Util_FindNotesDataset(userDetails);
        if (notesDataset == NULL) {
            LOG_INFO("Notes dataset not found");
            goto skip_notes;
        }

        if (notesDataset->suspendedflag()) {
            LOG_INFO("User's Notes dataset:"FMTu64" SUSPENDED",
                    notesDataset->datasetid());
            // Check to remove suspended syncConfigs
            int rc = SyncFeatureMgr_RemoveByDatasetId(user.user_id(),
                                                      notesDataset->datasetid());
            if (rc != 0) {
                LOG_ERROR("SyncFeatureMgr_RemoveByDatasetId("FMTu64","FMTu64"):%d",
                          user.user_id(), notesDataset->datasetid(), rc);
            }
            goto skip_notes;
        }

        LOG_INFO("User's Notes dataset="FMTu64", local path=\"%s\"",
                notesDataset->datasetid(),
                user._cachedData.details().notes_sync_path().c_str());

        int rv = SyncFeatureMgr_Add(
                userId,
                notesDataset->datasetid(),
                VCS_CATEGORY_NOTES,
                SYNC_FEATURE_NOTES,
                true, // This SyncFeature will never be paused while enabled.
                SYNC_TYPE_TWO_WAY,
                SYNC_POLICY_DEFAULT,
                user._cachedData.details().notes_sync_path(),
                "/sync",
                // TODO: no dedicated thread(s) for notes?
                false,
                0,
                0,
                NULL,
                true);
        if (rv != 0) {
            LOG_ERROR("SyncFeatureMgr_Add for Notes failed: %d", rv);
        }
    }
 skip_notes:

    // Syncbox
    if (userDetails.syncbox_sync_settings().enable_sync()) {
            bool syncConfigHostArchiveEnabled = false;

            // Find the first "active" Syncbox dataset.
            const vplex::vsDirectory::DatasetDetail* syncboxDataset = Util_FindSyncboxArchiveStorageDataset(userDetails);
            // Remove SyncConfigs for all other Syncbox datasets (even if they have an archive storage device).
            // (If there is no "active" Syncbox dataset, this will remove all Syncbox SyncConfigs.)
            for (int i = 0; i < userDetails.datasets_size(); i++) {
                if (userDetails.datasets(i).details().datasettype() == vplex::vsDirectory::SYNCBOX) {
                    if ((syncboxDataset == NULL) || (userDetails.datasets(i).details().datasetid() != syncboxDataset->datasetid())) {
                        LOG_INFO("SYNCBOX dataset("FMTu64") is not the active dataset, removing by datasetId",
                                userDetails.datasets(i).details().datasetid());
                        SyncFeatureMgr_RemoveByDatasetId(user.user_id(), userDetails.datasets(i).details().datasetid());
                    }
                }
            }

            // If there is no active Syncbox dataset, there is nothing else to do.
            if (syncboxDataset == NULL) {
                LOG_WARN("No active Syncbox dataset found");
                goto skip_syncbox;
            }

            if (syncboxDataset->suspendedflag()) {
                LOG_INFO("User's Syncbox dataset:"FMTu64" SUSPENDED", syncboxDataset->datasetid());
                // Check to remove suspended syncConfigs
                int rc = SyncFeatureMgr_RemoveByDatasetId(user.user_id(), syncboxDataset->datasetid());
                if (rc != 0) {
                    LOG_ERROR("SyncFeatureMgr_RemoveByDatasetId("FMTu64","FMTu64"):%d",
                              user.user_id(), syncboxDataset->datasetid(), rc);
                }
                goto skip_syncbox;
            }

            LOG_INFO("User's Syncbox dataset="FMTu64", local path=\"%s\"",
                     syncboxDataset->datasetid(),
                     userDetails.syncbox_sync_settings().sync_feature_path().c_str());

#if CCD_ENABLE_STORAGE_NODE
            // If local device is the Syncbox server, SyncFeatureMgr needs to use the
            // SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE SyncType. Extra Syncbox staging area
            // info needs to be passed into SyncConfig too.
            if (Util_IsLocalDeviceArchiveStorage(*syncboxDataset)) {
                if (!userDetails.syncbox_sync_settings().has_syncbox_staging_area_abs_path()) { 
                    LOG_ERROR("Fail to create Syncbox staging area. Cannot activate Syncbox sync.");
                    goto skip_syncbox;
                }

                u64 deviceStorageDatasetId;
                syncboxStagingAreaInfo.absPath = userDetails.syncbox_sync_settings().syncbox_staging_area_abs_path();
                int temp_rc = user.getLocalDeviceStorageDatasetId(/*out*/ deviceStorageDatasetId);
                if (temp_rc != 0) {
                    LOG_ERROR("Failed to get Device Storage dataset: %d.  Cannot activate Syncbox sync.", temp_rc);
                    goto skip_syncbox;
                }
                std::ostringstream urlPrefix;
                urlPrefix << "acer-ts://" << userId 
                    << "/" << VirtualDevice_GetDeviceId() 
                    << "/0/rf/file/" << deviceStorageDatasetId
                    << "/[stagingArea:" << syncboxDataset->datasetid() << "]/";
                syncboxStagingAreaInfo.urlPrefix = urlPrefix.str();
                syncConfigHostArchiveEnabled = true;
            }
#endif
            SyncPolicy syncboxPolicy = SYNC_POLICY_SYNCBOX;
            // The .conf files have not been read yet when C++ performs static-initialization of
            // SYNC_POLICY_SYNCBOX, so this needs to happen here:
            if (__ccdConfig.syncboxSyncConfigErrPollInterval != 0) {
                syncboxPolicy.error_retry_interval = VPLTime_FromSec(__ccdConfig.syncboxSyncConfigErrPollInterval);
                LOG_INFO("Modify syncbox error poll to "FMTu64, syncboxPolicy.error_retry_interval);
            }
            bool allowCreateDB;
            if (syncConfigHostArchiveEnabled &&
                user._cachedData.details().has_allow_syncbox_archive_storage_create_db() &&
                user._cachedData.details().allow_syncbox_archive_storage_create_db() == false) {
                LOG_INFO("Not allow to create SyncBox DB");
                allowCreateDB = false;
            } else {
                LOG_INFO("Allow to create SyncBox DB");
                allowCreateDB = true;
            }

            int rv = SyncFeatureMgr_Add(
                        userId,
                        syncboxDataset->datasetid(),
                        VCS_CATEGORY_ASD,
                        SYNC_FEATURE_SYNCBOX,
                        true, // This SyncFeature will never be paused while enabled.
                        syncConfigHostArchiveEnabled? SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE: SYNC_TYPE_TWO_WAY,
                        syncboxPolicy,
                        Util_CleanupPath(userDetails.syncbox_sync_settings().sync_feature_path().c_str()),
                        "",
                        // TODO: no dedicated thread(s)?
                        false,
                        0,
                        0,
                        syncConfigHostArchiveEnabled? &syncboxStagingAreaInfo:NULL,
                        allowCreateDB);
            if (rv == 0) {
                if (syncConfigHostArchiveEnabled) {
                    // If the SyncConfig was successfully created and we are the archive storage device,
                    // we must never create the SyncConfig localDB again.
                    // See bug 17601 for details.
                    user._cachedData.mutable_details()->set_allow_syncbox_archive_storage_create_db(false);
                }
            } else {
                LOG_ERROR("SyncFeatureMgr_Add for Syncbox failed: %d", rv);
                if (rv == SYNC_AGENT_DB_NOT_EXIST_TO_OPEN) {
                    // (This error code implies syncConfigHostArchiveEnabled == true.)
                    // The SyncConfig localDB has been lost.
                    // To avoid getting the dataset into a bad state, the only safe action is
                    // to delete and recreate the dataset.
                    // See bug 17601 for details.
                    LOG_WARN("we don't allow to create DB (because it was already created),"
                            " but got error SYNC_AGENT_DB_NOT_EXIST_TO_OPEN."
                            " Now trigger SyncBox dataset recreate to recover it.");
                    user._cachedData.mutable_details()->set_need_to_recreate_syncbox_dataset(syncboxDataset->datasetid());
                    cacheMonitor_requestRecreateSyncboxDataset(user);
                }
            }
    }
 skip_syncbox:

    int rv = privUpdateSyncConfigPriorities(user, false);
    if (rv != 0) {
        LOG_ERROR("privUpdateSyncConfigPriorities failed: %d", rv);
    }
}

void cache_getLoggedOutUsers(google::protobuf::RepeatedPtrField<ccd::LoggedOutUser>& users_out)
{
    users_out.CopyFrom(__cache().mainState.logged_out_users());
}

void cache_processUpdatedDatasetList(CachePlayer& user)
{
    int rv;
    // ASSERT(Cache_ThreadHasWriteLock()); will be done by cache_registerSyncConfigs.
    cache_registerSyncConfigs(user);
    rv = requestPicstreamInit(user);
    if(rv != 0) {
        LOG_ERROR("requestPicstreamInit:%d, user:"FMTu64, rv, user.user_id());
    }

#if CCD_ENABLE_SYNCDOWN
    {

        if (user.syncdown_needs_catchup_notification && user._cachedData.details().datasets_size() > 0) {
            LOG_INFO("Notifying SyncDown");
            Cache_NotifySyncDownModule(user.user_id(), 0, false);
            user.syncdown_needs_catchup_notification = false;
            LOG_DEBUG("sent catchup notify");
        }
    }
#endif // CCD_ENABLE_SYNCDOWN
    {   // Resume any thumbnail migrate, if needed.
        LOG_INFO("Init thumbnail migration");
        McaMigrateThumb &migrateThumbInstance = getMcaMigrateThumbInstance();
        int rc = migrateThumbInstance.McaThumbResumeMigrate(user.local_activation_id());
        if(rc != 0) {
            LOG_WARN("McaThumbResumeMigrate("FMTu64", %d):%d",
                     user.user_id(), user.local_activation_id(), rc);
        }
    }
}

// TODO: this class was copy & pasted
/// Absolute path on local filesystem.
/// For use when dealing with VPL file APIs (ls a directory, stat a file, read/write a file).
/// @invariant No trailing slash.  (Leading slash is allowed.)
/// TODO: special case for the root itself ("/")?  Right now, it isn't allowed.
namespace {  // anonymous namespace, required if there are multiple copies
class AbsPath
{
public:
    AbsPath(const string& path) : path(path) { checkInvariants(); }
    inline void set(const string& path) {
        this->path = path;
        checkInvariants();
    }
    inline void checkInvariants() {
        if (path.size() > 0) {
            ASSERT(path[path.size()-1] != '/');
        }
    }
    inline const string& str() const { return path; }
    inline const char* c_str() const { return path.c_str(); }
    AbsPath appendRelPath(const std::string& relativePath) const {
        if (relativePath.size() == 0) { // Special case if syncConfigRelPath is the sync config root.
            return AbsPath(path);
        } else {
            return AbsPath(path + "/" + relativePath);
        }
    }
private:
    string path;
};
} // anonymous namespace

void
cache_clearMMCacheByType(const CachePlayer& user, int syncType)
{
    const BasicSyncConfig* metadataThumbDirs = NULL;
    int metadataThumbDirsSize = 0;
    int rc;
    char defaultSyncDir[CCD_PATH_MAX_LENGTH];

    ASSERT(Cache_ThreadHasLock());

    DiskCache::getDirectoryForMediaMetadataDownload(user.user_id(),
                                                    sizeof(defaultSyncDir),
                                                    defaultSyncDir);
    std::string syncDirStr(defaultSyncDir);
    if(user._cachedData.details().has_mm_thumb_download_path()) {
        syncDirStr = user._cachedData.details().mm_thumb_download_path();
    }
    AbsPath src_mm_dir(syncDirStr);

    Cache_GetClientSyncConfig(metadataThumbDirs,
                              metadataThumbDirsSize);

    for(int dirIndex=0; dirIndex<metadataThumbDirsSize; ++dirIndex)
    {
        if(metadataThumbDirs[dirIndex].syncConfigId != syncType) {
            continue;
        }
        if(metadataThumbDirs[dirIndex].type != SYNC_TYPE_ONE_WAY_DOWNLOAD) {
            LOG_ERROR("Only allow deletion of cache for one way download (syncFeature:%d, type:%d)",
                      (int)metadataThumbDirs[dirIndex].syncConfigId,
                      (int)metadataThumbDirs[dirIndex].type);
            continue;
        }

        std::string pathSuffix(metadataThumbDirs[dirIndex].pathSuffix);
        if(pathSuffix.size()>0 && pathSuffix[0]=='/') {
            pathSuffix = pathSuffix.substr(1);
        }
        AbsPath src_thumb_dir = src_mm_dir.appendRelPath(pathSuffix);
        AbsPath src_temp_delete = src_mm_dir.appendRelPath("to_delete");
        LOG_INFO("Clear cached thumbnail Dir: DELETE:%s", src_thumb_dir.c_str());
        rc = Util_rmRecursive(src_thumb_dir.str(), src_temp_delete.str());
        if(rc != 0) {
            LOG_ERROR("Util_rm_dash_rf:%d, %s", rc, src_thumb_dir.c_str());
        }
    }
}

void
cache_convertCacheState(CachePlayer& user)
{
    ASSERT(Cache_ThreadHasLock());

    // Check for conversion to new 2.6 fields to control Thumbnail Syncing
    if(!user._cachedData.details().has_mm_thumb_sync_converted() ||
        !user._cachedData.details().mm_thumb_sync_converted()) {

        cache_setupMmThumbSync(user);
        user.writeCachedData(false);
    }
}

s32 Cache_GetForegroundApp(std::string& appId_out,
                           ccd::CcdApp_t& appType_out)
{
    s32 rv = 0;
    appId_out.clear();
    appType_out = CCD_APP_DEFAULT;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();

        appId_out = __cache().foregroundAppId;
        appType_out = __cache().foregroundAppType;
    }
    return rv;
}

s32 Cache_SetForegroundMode(VPL_BOOL foregroundMode,
                            const std::string& appId,
                            ccd::CcdApp_t appType)
{
    s32 rv;
    int intervalToUse = 0;
    u32 activationId;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }

        if (Cache_GetActivationIdForSyncUser(&activationId, false, 0) != 0) {
            LOG_DEBUG("Could not get activationId.  Acceptable, continuing.");
        }

        if (foregroundMode) {
            if (appId == __cache().foregroundAppId) {
                LOG_INFO("Ignoring redundant foreground request, appId=\"%s\"", appId.c_str());
                goto out;
            } else {
                // Force foreground.
                LOG_INFO("Request foreground mode, new appId=\"%s\", prev foregroundAppId=\"%s\"",
                        appId.c_str(), __cache().foregroundAppId.c_str());
                __cache().foregroundAppId = appId;
                __cache().foregroundAppType = appType;
                intervalToUse = 0;
            }
        } else {
            // Allow background if same as last foreground app.
            if (appId == __cache().foregroundAppId) {
                LOG_INFO("Request background mode, appId=\"%s\"", appId.c_str());
                __cache().foregroundAppId.clear();
# if !CCD_ALLOW_BACKGROUND_SYNC
                // Don't reset the app type if it's allowed to do background sync.
                __cache().foregroundAppType = ccd::CCD_APP_DEFAULT;
#endif
                intervalToUse = cache_getBackgroundModeIntervalSecs();
            } else {
                // Remain in previous state.
                LOG_INFO("Ignoring background mode for appId=\"%s\", foregroundAppId=\"%s\"",
                        appId.c_str(), __cache().foregroundAppId.c_str());
                goto out;
            }
        }
        __cache().requestedBgModeIntervalSec = intervalToUse;
    }
    rv = Cache_UpdatePowerMode(activationId);
    if(rv != 0) {
        LOG_ERROR("Cache_UpdatePowerMode:%d, %d", rv, activationId);
    }
out:
    return rv;
}

static bool cache_isAppForeground()
{
    ASSERT(Cache_ThreadHasLock());
#ifndef VPL_PLAT_IS_WIN_DESKTOP_MODE
    bool toReturn = (__cache().requestedBgModeIntervalSec <= 0) ||
                     !(__cache().foregroundAppId.empty());
#else
    // TODO: It might be cleaner to have the win32 app call SyncOnce rather
    // then use the BG->FG transition to sync.
    bool toReturn = !(__cache().foregroundAppId.empty());
#endif
    return toReturn;
}

ccd::PowerMode_t cache_getPowerMode()
{
    ASSERT(Cache_ThreadHasLock());
    ccd::PowerMode_t toReturn = ccd::POWER_NO_SYNC;
    switch(__cache().powerMode)
    {
    case S_POWER_NOT_INIT:
        toReturn = ccd::POWER_NO_SYNC;
        break;
    case S_POWER_NO_SYNC:
        toReturn = ccd::POWER_NO_SYNC;
        break;
    case S_POWER_FOREGROUND:
        toReturn = ccd::POWER_FOREGROUND;
        break;
    case S_POWER_BACKGROUND:
        toReturn = ccd::POWER_BACKGROUND;
        break;
    default:
        LOG_ERROR("Case not handled");
        break;
    }
    return toReturn;
}

// Reads the cache, determines the power state (foreground, background, no_sync)
// and makes the power state so (if the current one is different).
s32 Cache_UpdatePowerMode(u32 activationId)
{
    int rv = 0;
    PowerMode newPowerMode;
    PowerMode powerMode;
    bool powerModeChanged = false;
    int bgInterval = 0;
    ccd::PowerMode_t displayPowerMode = POWER_NO_SYNC;
    bool userIdValid = false;
    u64 userId = 0;

    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        CachePlayer* user = NULL;
        if(activationId != ACTIVATION_ID_NONE) {
            user = cache_getUserByActivationId(activationId);
            if(user == NULL) {
                LOG_ERROR("activationId "FMTu32" no longer valid", activationId);
            }
        }
        if(user == NULL) {
            newPowerMode = S_POWER_NO_SYNC;
        } else {
            userIdValid = true;
            userId = user->user_id();
            bool allowed_auto_sync = user->get_auto_sync();
            bool allowed_background_data = user->get_background_data();
            bool allowed_mobile_network_sync = user->get_mobile_network_data();

            bool onlyMobileNetworkAvailable = __cache().only_mobile_network_available;
            bool stream_power_mode = __cache().stream_power_mode;
            bgInterval = __cache().requestedBgModeIntervalSec;
            bool app_is_foreground = cache_isAppForeground();

            // See http://www.ctbg.acer.com/wiki/index.php/CCD_Power_State_And_Sync_Management#New_SYNC_state_logic
            if((   !stream_power_mode &&
                   !app_is_foreground &&
                   (!allowed_background_data || !allowed_auto_sync)) ||
               (!allowed_mobile_network_sync && onlyMobileNetworkAvailable) ||
               __cache().deprecatedNetworkDisable)
            {
                newPowerMode = S_POWER_NO_SYNC;
            }else if(app_is_foreground) {
                newPowerMode = S_POWER_FOREGROUND;
            }else{
                newPowerMode = S_POWER_BACKGROUND;
            }
        }

        powerMode = __cache().powerMode;
        if(newPowerMode != __cache().powerMode) {
            powerModeChanged = true;
            __cache().powerMode = newPowerMode;
            displayPowerMode = cache_getPowerMode();
        }
    }

    if(powerModeChanged) {

#ifndef VPL_PLAT_IS_WIN_DESKTOP_MODE
        switch(newPowerMode)
        {
            case S_POWER_FOREGROUND:
                LOG_INFO("[POWER]Switching to foreground mode");
                privSetForegroundMode(0);
                break;
            case S_POWER_BACKGROUND:
                LOG_INFO("[POWER]Switching to background mode");
                privSetForegroundMode(bgInterval);
                break;
            case S_POWER_NO_SYNC:
                LOG_INFO("[POWER]Switching to no-sync mode, disabling services");
                ANSConn_GoingToSleepCb();
                break;
            case S_POWER_NOT_INIT:
                LOG_ERROR("[POWER]Cannot switch state to POWER_NOT_INIT");
                break;
            default:
                LOG_ERROR("All cases not covered:%d", newPowerMode);
                break;
        }

        if(powerMode == S_POWER_NO_SYNC) {  // Turning on the power mode.
            LOG_INFO("[POWER]Switching out of no-sync mode, starting services");
            ANSConn_ResumeFromSleepCb();
        }
#endif  // VPL_PLAT_IS_WIN_DESKTOP_MODE

        if(powerMode == S_POWER_BACKGROUND &&
           newPowerMode == S_POWER_FOREGROUND)
        {   // In this case, ANS state does not change, so we cannot depend on ANS
            // connect callback to trigger a sync.  We need this sync to cover the
            // case when there's an internal infra failure, and the ANS
            // notification is missed.  See Bug 2692.
            LOG_INFO("[POWER]Performing background to foreground sync");
            if (userIdValid) {
                SyncFeatureMgr_RequestRemoteScansForUser(userId);
                Cache_NotifySyncDownModule(userId, 0, false);
#if CCD_ENABLE_SYNCUP
                int rc;
                rc = SyncUp_NotifyConnectionChange(false);
                if(rc != 0) {
                    LOG_WARN("SyncUp_NotifyConnectionChange:%d", rc);
                }
#endif
            }
        }

        ccd::CcdiEvent* ccdiEvent = new ccd::CcdiEvent();
        ccdiEvent->mutable_power_mode_change()->set_power_mode(displayPowerMode);
        EventManagerPb_AddEvent(ccdiEvent);
        // ccdiEvent will be freed by EventManagerPb.
    } else {

#ifndef VPL_PLAT_IS_WIN_DESKTOP_MODE
        // perhaps the power mode did not change, but the interval did.
        rv = privSetForegroundMode(bgInterval);
        if(rv != 0) {
            LOG_ERROR("privSetForegroundMode:%d, %d", bgInterval, rv);
        }
#endif  //VPL_PLAT_IS_WIN_DESKTOP_MODE

    }
    {
        int rc;
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        rc = privUpdateSyncConfigPriorities(activationId);
        if(rc != 0) {
            // This can fail under normal use.
            LOG_INFO("privUpdateSyncConfigPriorities:%d", rc);
        }
    }
 out:
    return rv;
}

s32 Cache_SetOnlyMobileNetworkAvailable(bool onlyMobileNetworkAvailable)
{
    int rv;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }

        __cache().only_mobile_network_available = onlyMobileNetworkAvailable;
    }
 out:
    return rv;
}

s32 Cache_GetOnlyMobileNetworkAvailable(bool &onlyMobileNetworkAvailable)
{
    int rv;
    onlyMobileNetworkAvailable = false;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
        }
        onlyMobileNetworkAvailable = __cache().only_mobile_network_available;
    }
    return rv;
}

s32 Cache_SetStreamPowerMode(bool streamPowerMode)
{
    int rv;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }

        __cache().stream_power_mode = streamPowerMode;
    }
 out:
    return rv;
}

s32 Cache_GetStreamPowerMode(bool& streamPowerMode)
{
    int rv;
    streamPowerMode = false;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
        }
        streamPowerMode = __cache().stream_power_mode;
    }
    return rv;
}

s32 Cache_SetDeprecatedNetworkEnable(bool networkEnable)
{
    int rv;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        __cache().deprecatedNetworkDisable = !networkEnable;
    }
 out:
    return rv;
}

s32 Cache_GetDeprecatedNetworkEnable(bool &networkEnable)
{
    int rv;
    networkEnable = false;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
        }
        networkEnable = !__cache().deprecatedNetworkDisable;
    }
    return rv;
}

s32 Cache_SetBackgroundModeInterval(int secs)
{
    CacheAutoLock autoLock;
    s32 rv = autoLock.LockForWrite();
    if (rv < 0) {
        LOG_ERROR("Failed to obtain lock");
        goto out;
    }
    if (__cache().mainState.background_mode_interval_sec() != secs)
    {
        __cache().mainState.set_background_mode_interval_sec(secs);
        __cache().requestedBgModeIntervalSec = secs;
        cache_writeMainState(false);
    }
out:
    return rv;
}

s32 Cache_PerformBackgroundTasks()
{
    ANSConn_PerformBackgroundTasks();
    return CCD_OK;
}

s32 Cache_CreateNotesDatasetIfNeeded(u32 activationId)
{
    s32 rv;
    u64 userId = 0;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        CachePlayer* user = cache_getUserByActivationId(activationId);
        if(user == NULL) {
            LOG_ERROR("activationId "FMTu32" no longer valid", activationId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }

        if (Util_FindNotesDataset(user->_cachedData.details()) != NULL) {
            // Notes dataset already exists.
            return 0;
        }

        LOG_INFO("Need to create Notes dataset for user "FMT_VPLUser_Id_t", clusterId="FMTs64,
                user->user_id(), user->cluster_id());

        vplex::vsDirectory::AddDataSetInput vsds_request;
        Query_FillInVsdsSession(vsds_request.mutable_session(), user->getSession());
        vsds_request.set_userid(user->user_id());
        vsds_request.set_datasetname(NOTES_DATASET_NAME);
        vsds_request.set_datasettypeid(NOTES_DATASET_TYPE);
        vsds_request.set_storageclusterid(user->cluster_id());
        vsds_request.set_version("3.0");
        vplex::vsDirectory::AddDataSetOutput vsds_response;
        rv = QUERY_VSDS(VPLVsDirectory_AddDataSet, vsds_request, vsds_response);
        // Treat VPL_VS_DIRECTORY_ERR_DATASET_NAME_ALREADY_EXISTS as success, since we may race
        // with other clients to create it.  As long as the dataset exists now, we are good.
        if ((rv != 0) && (rv != VPL_VS_DIRECTORY_ERR_DATASET_NAME_ALREADY_EXISTS)) {
            LOG_ERROR("VPLVsDirectory_AddDataSet failed: %d", rv);
            goto out;
        }
        LOG_INFO("Notes datasetId="FMTu64, vsds_response.datasetid());
        userId = user->user_id();  // To later mark cache dirty
    }
    if (userId != 0) {  // This means Notes dataset was created.
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }
        CacheMonitor_MarkDatasetsDirty(userId);
    }
out:
    return rv;
}

s32 Cache_CreateSbmDatasetIfNeeded(u32 activationId)
{
    s32 rv;
    u64 userId = 0;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        CachePlayer* user = cache_getUserByActivationId(activationId);
        if(user == NULL) {
            LOG_ERROR("activationId "FMTu32" no longer valid", activationId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }

        if (Util_FindSbmDataset(user->_cachedData.details()) == NULL) {
            LOG_INFO("Need to create SBM dataset for user "FMT_VPLUser_Id_t", clusterId="FMTs64,
                    user->user_id(), user->cluster_id());

            vplex::vsDirectory::AddDataSetInput vsds_request;
            Query_FillInVsdsSession(vsds_request.mutable_session(), user->getSession());
            vsds_request.set_userid(user->user_id());
            vsds_request.set_datasetname(SBM_DATASET_NAME);
            vsds_request.set_datasettypeid(SBM_DATASET_TYPE);
            vsds_request.set_storageclusterid(user->cluster_id());
            vsds_request.set_version("3.0");
            vplex::vsDirectory::AddDataSetOutput vsds_response;
            rv = QUERY_VSDS(VPLVsDirectory_AddDataSet, vsds_request, vsds_response);
            // Treat VPL_VS_DIRECTORY_ERR_DATASET_NAME_ALREADY_EXISTS as success, since we may race
            // with other clients to create it.  As long as the dataset exists now, we are good.
            if ((rv != 0) && (rv != VPL_VS_DIRECTORY_ERR_DATASET_NAME_ALREADY_EXISTS)) {
                LOG_ERROR("VPLVsDirectory_AddDataSet(%s): %d", SBM_DATASET_NAME, rv);
            } else {
                LOG_INFO("SBM datasetId="FMTu64, vsds_response.datasetid());
                userId = user->user_id();  // To later mark cache dirty
            }
        }
    }
    if (userId != 0) {  // This means SBM dataset was created.
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }
        CacheMonitor_MarkDatasetsDirty(userId);
    }
out:
    return rv;
}

s32 Cache_CreateSwmDatasetIfNeeded(u32 activationId)
{
    s32 rv;
    u64 userId = 0;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        CachePlayer* user = cache_getUserByActivationId(activationId);
        if(user == NULL) {
            LOG_ERROR("activationId "FMTu32" no longer valid", activationId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }

        if (Util_FindSwmDataset(user->_cachedData.details()) == NULL) {
            LOG_INFO("Need to create SWM dataset for user "FMT_VPLUser_Id_t", clusterId="FMTs64,
                    user->user_id(), user->cluster_id());

            vplex::vsDirectory::AddDataSetInput vsds_request;
            Query_FillInVsdsSession(vsds_request.mutable_session(), user->getSession());
            vsds_request.set_userid(user->user_id());
            vsds_request.set_datasetname(SWM_DATASET_NAME);
            vsds_request.set_datasettypeid(SWM_DATASET_TYPE);
            vsds_request.set_storageclusterid(user->cluster_id());
            vsds_request.set_version("3.0");
            vplex::vsDirectory::AddDataSetOutput vsds_response;
            rv = QUERY_VSDS(VPLVsDirectory_AddDataSet, vsds_request, vsds_response);
            // Treat VPL_VS_DIRECTORY_ERR_DATASET_NAME_ALREADY_EXISTS as success, since we may race
            // with other clients to create it.  As long as the dataset exists now, we are good.
            if ((rv != 0) && (rv != VPL_VS_DIRECTORY_ERR_DATASET_NAME_ALREADY_EXISTS)) {
                LOG_ERROR("VPLVsDirectory_AddDataSet(%s): %d", SWM_DATASET_NAME, rv);
            } else {
                LOG_INFO("SWM datasetId="FMTu64, vsds_response.datasetid());
                userId = user->user_id();  // To later mark cache dirty
            }
        }
    }
    if (userId != 0) {  // This means SWM dataset was created.
        CacheAutoLock autoLock;
        rv = autoLock.LockForWrite();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }
        CacheMonitor_MarkDatasetsDirty(userId);
    }
out:
    return rv;
}

#if CCD_ENABLE_STORAGE_NODE

static VPLLazyInitMutex_t s_updateSyncboxArchiveStorageMutex = VPLLAZYINITMUTEX_INIT;
/// When greater than 0, the local CCD is calling VSDS to update the Syncbox Archive Storage Device
/// to be the local device.  As a result, any VSDS GetCloudInfo call made during this time
/// may or may not reflect our request.  It is not safe to remove the association based
/// on the result.
static int s_updateSyncboxArchiveStorageInProgress = 0;
/// Keep a sequence number to make sure we don't miss any updates between when we
/// start GetCloudInfo and when we process the results.
static u32 s_updateSyncboxArchiveStorageSeqNum = 0;

static void Cache_SetUpdateSyncboxArchiveStorageInProgress(bool inProgress)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_updateSyncboxArchiveStorageMutex));
    if (inProgress) {
        s_updateSyncboxArchiveStorageInProgress++;
    } else {
        ASSERT(s_updateSyncboxArchiveStorageInProgress > 0);
        s_updateSyncboxArchiveStorageInProgress--;
        s_updateSyncboxArchiveStorageSeqNum++;
    }
}

// Doc comment in header.
u32 Cache_GetUpdateSyncboxArchiveStorageSeqNum()
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_updateSyncboxArchiveStorageMutex));
    return s_updateSyncboxArchiveStorageSeqNum;
}

// Doc comment in header.
bool Cache_DidWeTryToUpdateSyncboxArchiveStorage(u32 seqNum)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_updateSyncboxArchiveStorageMutex));
    return (s_updateSyncboxArchiveStorageInProgress == 0) &&
            (seqNum == s_updateSyncboxArchiveStorageSeqNum);
}

s32 Cache_DeleteSyncboxDataset(u32 activationId, u64 datasetId)
{
    s32 rv;
    VPLUser_Id_t userId;
    ccd::UserSession userSession;

    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        CachePlayer* user = cache_getUserByActivationId(activationId);
        if(user == NULL) {
            LOG_ERROR("activationId "FMTu32" no longer valid", activationId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }

        userId = user->user_id();
        userSession = user->getSession();
    } // releases lock
    {
        LOG_INFO("Delete dataset id "FMTu64" for user"FMT_VPLUser_Id_t,
                datasetId, userId);

        vplex::vsDirectory::DeleteDataSetInput vsds_request;
        vplex::vsDirectory::DeleteDataSetOutput vsds_response;
        Query_FillInVsdsSession(vsds_request.mutable_session(), userSession);
        vsds_request.set_userid(userId);
        vsds_request.set_datasetid(datasetId);
        rv = QUERY_VSDS(VPLVsDirectory_DeleteDataSet, vsds_request, vsds_response);
        if (rv != 0) {
            LOG_ERROR("VPLVsDirectory_DeleteDataSet failed: %d, uid="FMT_VPLUser_Id_t" did=%"PRIx64,
                      rv, userId, datasetId);
            goto out;
        }
        CacheMonitor_MarkDatasetsDirty(userId);
    }

out:
    return rv;
}

s32 Cache_CreateAndAssociateSyncboxDatasetIfNeeded(u32 activationId, u64& datasetId_out, bool ignore_cache)
{
    s32 rv;
    
    // Design: 
    // http://www.ctbg.acer.com/wiki/index.php/Home_Storage_Virtual_Sync_Design#Infra_Implementation_Changes
    
    u64 localDeviceId = VirtualDevice_GetDeviceId();
    std::string syncbox_dataset_name = getArchiveStorageSyncboxDatasetName(localDeviceId);
    
    bool needToCreateDataset = false;
    VPLUser_Id_t userId;
    u64 infraClusterId;
    ccd::UserSession userSession;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        CachePlayer* user = cache_getUserByActivationId(activationId);
        if(user == NULL) {
            LOG_ERROR("activationId "FMTu32" no longer valid", activationId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }
        if (ignore_cache == true) {
            needToCreateDataset = true;
        } else {
            const vplex::vsDirectory::DatasetDetail* syncboxDataset =
                    Util_FindDataset(user->_cachedData.details(), syncbox_dataset_name.c_str(), SYNCBOX_DATASET_TYPE);
            if (syncboxDataset != NULL) {
                datasetId_out = syncboxDataset->datasetid();
            } else {
                needToCreateDataset = true;
            }
        }
        userId = user->user_id();
        infraClusterId = user->cluster_id();
        userSession = user->getSession();
    } // releases lock

    {
        // Creates a Syncbox dataset to be associated with local device if it does not exist yet.
        if (needToCreateDataset) {
            LOG_INFO("Create %s dataset for user "FMT_VPLUser_Id_t", clusterId="FMTs64,
                    syncbox_dataset_name.c_str(), userId, infraClusterId);
            
            vplex::vsDirectory::AddDataSetInput vsds_request;
            Query_FillInVsdsSession(vsds_request.mutable_session(), userSession);
            vsds_request.set_userid(userId);
            vsds_request.set_datasetname(syncbox_dataset_name);
            vsds_request.set_datasettypeid(SYNCBOX_DATASET_TYPE);
            // VCS is the "master" of this dataset, so specify the user's infra cluster_id here.
            vsds_request.set_storageclusterid(infraClusterId);
            vsds_request.set_version("3.0");
            vplex::vsDirectory::AddDataSetOutput vsds_response;
            rv = QUERY_VSDS(VPLVsDirectory_AddDataSet, vsds_request, vsds_response);
            // NOTE: not technically needed for this case, but leaving here in case someone copy & pastes this later:
            //   Treat VPL_VS_DIRECTORY_ERR_DATASET_NAME_ALREADY_EXISTS as success, since we may race
            //   with other clients to create it.  As long as the dataset exists now, we are good.
            if ((rv != 0) && (rv != VPL_VS_DIRECTORY_ERR_DATASET_NAME_ALREADY_EXISTS)) {
                LOG_ERROR("VPLVsDirectory_AddDataSet(%s): %d", syncbox_dataset_name.c_str(), rv);
                goto out;
            }
            LOG_INFO("%s datasetId="FMTu64, syncbox_dataset_name.c_str(), vsds_response.datasetid());
            // TODO: Bug 18258: If VPLVsDirectory_AddDataSet returned
            //   VPL_VS_DIRECTORY_ERR_DATASET_NAME_ALREADY_EXISTS, the datasetid will currently be
            //   0, so VPLVsDirectory_AddDatasetArchiveStorageDevice is doomed to fail.
            datasetId_out = vsds_response.datasetid();
        }

        // Associate the dataset with the archive storage device.
        {
            LOG_INFO("Associate archive storage device("FMTu64") to dataset %s for user("FMTu64")",
                    localDeviceId, syncbox_dataset_name.c_str(), userId);

            vplex::vsDirectory::AddDatasetArchiveStorageDeviceInput vsds_request;
            Query_FillInVsdsSession(vsds_request.mutable_session(), userSession);
            vsds_request.set_datasetid(datasetId_out);
            vsds_request.add_archivestoragedeviceid(localDeviceId);
            vsds_request.set_userid(userId);
            vsds_request.set_version("3.0");
            vplex::vsDirectory::AddDatasetArchiveStorageDeviceOutput vsds_response;
            Cache_SetUpdateSyncboxArchiveStorageInProgress(true);
            rv = QUERY_VSDS(VPLVsDirectory_AddDatasetArchiveStorageDevice, vsds_request, vsds_response);
            Cache_SetUpdateSyncboxArchiveStorageInProgress(false);
            // Note: VSDS should return success if the ASD is already associated.
            if (rv != 0) {
                LOG_ERROR("VPLVsDirectory_AddDatasetArchiveStorageDevice failed: %d", rv);
                goto out;
            }
            CacheMonitor_MarkDatasetsDirty(userId);
        }
    }

out:
    return rv;
}

s32 Cache_DissociateSyncboxDatasetIfNeeded(u32 activationId)
{
    s32 rv;
    {
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        CachePlayer* user = cache_getUserByActivationId(activationId);
        if(user == NULL) {
            LOG_ERROR("activationId "FMTu32" no longer valid", activationId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }

        u64 localDeviceId = VirtualDevice_GetDeviceId();
        
        // Check if the dataset exists.
        std::string syncbox_dataset_name = getArchiveStorageSyncboxDatasetName(localDeviceId);
        const vplex::vsDirectory::DatasetDetail* syncboxDataset =
                Util_FindDataset(user->_cachedData.details(), syncbox_dataset_name.c_str(), SYNCBOX_DATASET_TYPE);
        if (syncboxDataset != NULL) {
            // Dissociate the dataset and the archive storage device.
            LOG_INFO("Dissociate archive storage device("FMTu64") from dataset %s for user("FMTu64")",
                    localDeviceId, syncbox_dataset_name.c_str(), user->user_id());

            vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceInput vsds_request;
            Query_FillInVsdsSession(vsds_request.mutable_session(), user->getSession());
            vsds_request.set_datasetid(syncboxDataset->datasetid());
            vsds_request.add_archivestoragedeviceid(localDeviceId);
            vsds_request.set_userid(user->user_id());
            vsds_request.set_version("3.0");
            vplex::vsDirectory::RemoveDatasetArchiveStorageDeviceOutput vsds_response;
            rv = QUERY_VSDS(VPLVsDirectory_RemoveDatasetArchiveStorageDevice, vsds_request, vsds_response);
            if (rv != 0) { 
                if (rv == VPL_VS_DIRECTORY_ERR_NO_ASD_TO_REMOVE) {
                    rv = 0;
                    LOG_WARN("ASD (device "FMTu64") has already been removed", localDeviceId);
                } else {
                    LOG_ERROR("VPLVsDirectory_RemoveDatasetArchiveStorageDevice failed: %d", rv);
                    goto out;
                }
            } 
        } else {
            LOG_WARN("Dataset %s not found", syncbox_dataset_name.c_str());
            goto out;
        }
    }
out:
    return rv;
}
#endif

void Cache_TSCredQuery(TSCredQuery_t* query)
{
    int rc = 0, rv = 0;
    u64 session_handle = 0;
    std::string ias_ticket;
    std::string ans_login_blob;

    if(query == NULL)
        return;

    query->resp_key.clear();
    query->resp_blob.clear();

    switch (query->type) {
    case TS_CRED_QUERY_SVR_KEY:
        LOG_INFO("Query CCD server key");
        break;
    case TS_CRED_QUERY_PXD_CRED:
        LOG_INFO("Query PXD credential");
        break;
    case TS_CRED_QUERY_CCD_CRED:
        LOG_INFO("Query CCD-to-CCD credential");
        break;
    default:
        rv = CCD_ERROR_NOT_IMPLEMENTED;
        LOG_ERROR("Query type %d is not supported.", query->type);
        goto out;
    }

    // Step 0:
    // Check if it is a reset request
    if(query->resetCred) {
        LOG_INFO("Reset credentials request");
        CacheAutoLock autoLock;
        CachePlayer* user = NULL;

        rv = autoLock.LockForWrite();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        user = cache_getUserByUserId(query->user_id);
        if(user == NULL) {
            LOG_ERROR("user for userId "FMTu64" is no longer signed-in", query->user_id);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }

        switch(query->type) {
        case TS_CRED_QUERY_SVR_KEY:
            if(user->getSession().has_ccd_server_key()) {
                user->getSession().clear_ccd_server_key();
            }
            break;
        case TS_CRED_QUERY_PXD_CRED:
            if(user->getSession().has_pxd_login_blob()) {
                user->getSession().clear_pxd_login_blob();
            }
            if(user->getSession().has_pxd_session_key()) {
                user->getSession().clear_pxd_session_key();
            }
            break;
        case TS_CRED_QUERY_CCD_CRED:
            for(int i = 0; i < user->getSession().ccd_creds_size(); i++) {
                const ccd::CCDToCCDCredential& curr = user->getSession().ccd_creds(i);
                if(curr.ccd_svr_user_id() == query->target_svr_user_id &&
                   curr.ccd_svr_device_id() == query->target_svr_device_id &&
                   curr.ccd_svr_inst_id() == query->target_svr_instance_id) {
                    int size = user->getSession().ccd_creds_size();
                    user->getSession().mutable_ccd_creds()->SwapElements(i, size -1);
                    user->getSession().mutable_ccd_creds()->RemoveLast();
                }
            }
            break;
        } // switch

        if((rc = user->writeCachedData(false)) != CCD_OK) {
            LOG_WARN("writeCachedData failed %d", rc);
        }
        // Reset request complete.
        goto out;
    }

    // Step 1:
    // Check Cache and see if the request can be satisfied from there.
    // If not, go to step 2.
    {
        CacheAutoLock autoLock;
        CachePlayer* user = NULL;

        rv = autoLock.LockForRead();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        user = cache_getUserByUserId(query->user_id);
        if(user == NULL) {
            LOG_ERROR("user for userId "FMTu64" is no longer signed-in", query->user_id);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }

        switch(query->type) {
        case TS_CRED_QUERY_SVR_KEY:
            if(user->getSession().has_ccd_server_key()) {
                query->resp_key.assign(user->getSession().ccd_server_key());
                goto out;
            }
            break;
        case TS_CRED_QUERY_PXD_CRED:
            if(user->getSession().has_pxd_login_blob() &&
               user->getSession().has_pxd_session_key()) {
                query->resp_key.assign(user->getSession().pxd_session_key());
                query->resp_blob.assign(user->getSession().pxd_login_blob());
                query->resp_instance_id = user->getSession().instance_id();
                goto out;
            }
            break;
        case TS_CRED_QUERY_CCD_CRED:
            for(int i = 0; i < user->getSession().ccd_creds_size(); i++) {
                const ccd::CCDToCCDCredential& curr = user->getSession().ccd_creds(i);
                if(curr.ccd_svr_user_id() == query->target_svr_user_id &&
                   curr.ccd_svr_device_id() == query->target_svr_device_id &&
                   curr.ccd_svr_inst_id() == query->target_svr_instance_id) {
                    query->resp_key.assign(curr.ccd_session_key());
                    query->resp_blob.assign(curr.ccd_login_blob());
                    query->resp_instance_id = user->getSession().instance_id();
                    goto out;
                }
            }
            break;
        } // switch

        // If we get here, it means we could not satisfy the request from Cache.
        // Before we let go of the read lock, make a copy of what we need from Cache to call Infra.
        session_handle = user->getSession().session_handle();
        ias_ticket = user->getSession().ias_ticket();
        ans_login_blob = user->getSession().ans_login_blob();
    } // block enclosing read lock

    // Step 2:
    // Ask Infra to satisfy the request.
    // (Defer saving the Infra response to Cache to Step 3.)
    switch(query->type) {
    case TS_CRED_QUERY_SVR_KEY:
        rv = Query_GetCCDServerKey(session_handle, ias_ticket,
                                   query->user_id, query->resp_key);
        if(rv < 0) {
            LOG_ERROR("Query_GetCCDServerKey failed: %d", rv);
            goto out;
        }
        break;
    case TS_CRED_QUERY_PXD_CRED:
        rv = Query_GetPxdLoginBlob(session_handle, ias_ticket,
                                   ans_login_blob,
                                   query->resp_key, query->resp_blob, query->resp_instance_id);
        if(rv < 0) {
            LOG_ERROR("Query_GetPxdLoginBlob failed: %d", rv);
            goto out;
        }
        break;
    case TS_CRED_QUERY_CCD_CRED:
        rv = Query_GetCCDLoginBlob(session_handle, ias_ticket,
                                   query->target_svr_user_id, query->target_svr_device_id, query->target_svr_instance_id,
                                   ans_login_blob,
                                   query->resp_key, query->resp_blob, query->resp_instance_id);
        if(rv < 0) {
            LOG_ERROR("Query_GetCCDLoginBlob failed: %d", rv);
            goto out;
        }
        break;
    } // switch

    // Step 3: Save the Infra response to Cache.
    {
        CacheAutoLock autoLock;
        CachePlayer* user = NULL;

        rv = autoLock.LockForWrite();
        if(rv != CCD_OK) {
            LOG_ERROR("Cannot obtain lock:%d", rv);
            goto out;
        }

        user = cache_getUserByUserId(query->user_id);
        if(user == NULL) {
            LOG_ERROR("user for userId "FMTu64" is no longer signed-in", query->user_id);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }

        switch(query->type) {
        case TS_CRED_QUERY_SVR_KEY:
            user->getSession().set_ccd_server_key(query->resp_key);
            break;
        case TS_CRED_QUERY_PXD_CRED:
            user->getSession().set_pxd_session_key(query->resp_key);
            user->getSession().set_pxd_login_blob(query->resp_blob);
            user->getSession().set_instance_id(query->resp_instance_id);
            break;
        case TS_CRED_QUERY_CCD_CRED:
            user->getSession().set_instance_id(query->resp_instance_id);
            ccd::CCDToCCDCredential* newCreds = user->getSession().add_ccd_creds();
            newCreds->set_ccd_svr_user_id(query->target_svr_user_id);
            newCreds->set_ccd_svr_device_id(query->target_svr_device_id);
            newCreds->set_ccd_svr_inst_id(query->target_svr_instance_id);
            newCreds->set_ccd_session_key(query->resp_key);
            newCreds->set_ccd_login_blob(query->resp_blob);
            break;
        } // switch

        if((rc = user->writeCachedData(false)) != CCD_OK) {
            LOG_WARN("writeCachedData failed %d", rc);
        }
    } // block enclosing write lock

out:
    query->result = rv;
}

int Cache_GetDeviceCcdProtocolVersion(u64 userId, bool useOnlyCache, u64 deviceId)
{
    int protocolVersion = -1;  // unset
    int err;
    u32 activationId;

    err = Cache_GetActivationIdForSyncUser(&activationId, true, userId);
    if (err) {
        // Error message logged by Cache_GetActivationIdForSyncUser().
        goto out;
    }

    if (!useOnlyCache) {
        err = CacheMonitor_UpdateIfNeeded(activationId, CCD_USER_DATA_UPDATE_DEVICES);
        if (err) {
            // Error message logged by CacheMonitor_UpdateIfNeeded().
            goto out;
        }
    }

    if (deviceId == VirtualDevice_GetDeviceId()) {  // device is the local device
        protocolVersion = atoi(CCD_PROTOCOL_VERSION);
        goto out;
    }
    // assert: device is NOT for the local device

    {
        CacheAutoLock autoLock;
        err = autoLock.LockForRead();
        if (err < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        CachePlayer* user = cache_getUserByActivationId(activationId);
        if (user == NULL) {
            LOG_INFO("User is no longer logged in");
            err = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }
        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo> &devices =
                user->_cachedData.details().cached_devices();
        google::protobuf::RepeatedPtrField<vplex::vsDirectory::DeviceInfo>::const_iterator it;
        for (it = devices.begin(); it != devices.end(); it++) {
            if (it->deviceid() == deviceId) {
                protocolVersion = it->has_protocolversion() ? atoi(it->protocolversion().c_str()) : 0;
                break;
            }
        }
    } // scope for cache auto-lock

 out:
    return err ? err : protocolVersion;
}

int Cache_GetDeviceInfo(u64 userId, std::string& deviceName_out)
{
    u32 activationId;
    int err = 0;

    err = Cache_GetActivationIdForSyncUser(&activationId, true, userId);
    if (err) {
        // Error message logged by Cache_GetActivationIdForSyncUser().
        goto out;
    }

    {
        CacheAutoLock autoLock;
        err = autoLock.LockForRead();
        if (err < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }

        CachePlayer* user = cache_getUserByActivationId(activationId);
        if (user == NULL) {
            LOG_INFO("User is no longer logged in");
            err = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }

        deviceName_out = user->localDeviceName();
    }

 out:
    return err;

}

int Cache_GetSyncboxSettings(u64 userId, ccd::SyncBoxSettings& settings_out)
{
    int err;
    {
        CacheAutoLock autoLock;
        err = autoLock.LockForRead();
        if (err < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }

        CachePlayer* user = cache_getUserByUserId(userId);
        if (user == NULL) {
            LOG_INFO("User is no longer logged in");
            err = CCD_ERROR_NOT_SIGNED_IN;
            goto out;
        }

        settings_out = user->_cachedData.details().syncbox_sync_settings();
    }

 out:
    return err;
}

//-----------------------------------------

// TODO: consider a reader/writer lock so that HttpSvc::Ccd::Handler_Helper::ForwardToServerCcd
//   can have multiple simultaneous callers.
static VPLLazyInitMutex_t s_localServersMutex = VPLLAZYINITMUTEX_INIT;
static Ts2::LocalInfo* localInfo;
#if CCD_ENABLE_STORAGE_NODE
static vss_server* storageNode;
#endif
static HttpService httpService;

VPLMutex_t* LocalServers_GetMutex()
{
    // TODO: bug 17701: ASSERT(!Cache_ThreadHasLock());
    return VPLLazyInitMutex_GetMutex(&s_localServersMutex);
}

Ts2::LocalInfo* LocalServers_getLocalInfo()
{
    ASSERT(VPLMutex_LockedSelf(VPLLazyInitMutex_GetMutex(&s_localServersMutex)));
    return localInfo;
}

void LocalServers_ReportNetworkConnected()
{
    // TODO: bug 17701: ASSERT(!Cache_ThreadHasLock());
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_localServersMutex));
    if(localInfo) {
        localInfo->ReportNetworkConnected();
    } else {
        LOG_WARN("LocalInfo obj is null");
    }
}

void LocalServers_ReportDeviceOnline()
{
    // TODO: bug 17701: ASSERT(!Cache_ThreadHasLock());
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_localServersMutex));
    if(localInfo) {
        localInfo->AnsReportDeviceOnline(localInfo->GetDeviceId());
    } else {
        LOG_WARN("LocalInfo obj is null");
    }
}

HttpService& LocalServers_GetHttpService()
{
    return httpService; // HttpService has its own mutex, so no need for any additional locking.
}

int LocalServers_CreateLocalInfoObj(VPLUser_Id_t userId, u32 instanceId)
{
    // TODO: bug 17701: ASSERT(!Cache_ThreadHasLock());
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_localServersMutex));
    if (localInfo) {
        LOG_WARN("LocalInfo obj already present");
        return CCD_OK;
    }

    u64 deviceId = VirtualDevice_GetDeviceId();
    if (deviceId == 0) {
        LOG_ERROR("No deviceId assigned locally");
        return CCD_ERROR_NOT_INIT;
    }

    localInfo = new (std::nothrow) Ts2::LocalInfo_Ccd(userId, deviceId, instanceId);
    if (!localInfo) {
        LOG_ERROR("No memory for LocalInfo_Ccd obj");
        return CCD_ERROR_NOMEM;
    }
    return CCD_OK;
}

int LocalServers_StartHttpService(VPLUser_Id_t userId)
{
    // HttpService has its own mutex, so no need for any additional locking.
    int rv;
    u64 thisDeviceId;

    if(httpService.is_running()) {
        LOG_ERROR("HTTP Service Service already started");
        return VPL_ERR_ALREADY;
    }

    rv = ESCore_GetDeviceGuid(&thisDeviceId);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "ESCore_GetDeviceGuid", rv);
        goto out;
    }

    httpService.configure_service(userId,
                                  thisDeviceId,
                                  VPLNet_GetAddr(__ccdConfig.clearfiUserServerName),
                                  __ccdConfig.clearfiUserServerPort,
                                  localInfo);
    rv = httpService.start();
    if (rv != 0) {
        LOG_ERROR("Stream service failed to start for user "FMT_VPLUser_Id_t": %d", userId, rv);
        httpService.stop(/*userLogout*/false);
    }

 out:
    return rv;
}

#if CCD_ENABLE_STORAGE_NODE

vss_server* LocalServers_getStorageNode()
{
    ASSERT(VPLMutex_LockedSelf(VPLLazyInitMutex_GetMutex(&s_localServersMutex)));
    return storageNode;
}

void LocalServers_PopulateStartStorageNodeContext(CachePlayer& user, StartStorageNodeContext& context_out)
{
    context_out.userId = user.user_id();
    context_out.activationId = user.local_activation_id();
    context_out.clusterId = user.cluster_id();
    //Find dataset
    context_out.deviceStorageDatasetId = 0;
    {
        int rv = user.getLocalDeviceStorageDatasetId(context_out.deviceStorageDatasetId);
        if (rv != 0) {
            LOG_ERROR("Failed to find device storage datasetId for storage node");
        }
    }

    { // Check and init syncbox archive storage parameters
        context_out.isSyncboxArchiveStorage = false;
        if (user._cachedData.details().has_syncbox_sync_settings())  {
            if (user._cachedData.details().syncbox_sync_settings().enable_sync() &&
                user._cachedData.details().syncbox_sync_settings().is_archive_storage()) {
                if (user._cachedData.details().syncbox_sync_settings().coherent()) {
                    // Use the saved local settings.
                    context_out.syncboxArchiveStorageDatasetId = user._cachedData.details().syncbox_sync_settings().syncbox_dataset_id();
                    context_out.isSyncboxArchiveStorage = true;
                } else {
                    // Syncbox feature is enabled for the local device. Try to find the dataset.
                    const vplex::vsDirectory::DatasetDetail* syncboxDataset = Util_FindSyncboxArchiveStorageDataset(user._cachedData.details());
                    if ((syncboxDataset != NULL) && Util_IsLocalDeviceArchiveStorage(*syncboxDataset)) {
                        context_out.syncboxArchiveStorageDatasetId = syncboxDataset->datasetid();
                        context_out.isSyncboxArchiveStorage = true;
                    } else {
                        // leave context_out.isSyncboxArchiveStorage = false;
                        // context_out.syncboxArchiveStorageDatasetId will be ignored.
                    }
                }
                context_out.syncboxSyncFeaturePath = Util_CleanupPath(user._cachedData.details().syncbox_sync_settings().sync_feature_path());
            }
        }
    }

    context_out.session = user.getSession();
}

int LocalServers_StartStorageNode(const StartStorageNodeContext& context)
{
    int rv;
    ASSERT(!Cache_ThreadHasLock());
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_localServersMutex));
    if (storageNode != NULL) {
        LOG_INFO("Storage node already running for user "FMT_VPLUser_Id_t, context.userId);
        rv = VPL_OK;
        goto out;
    }
    LOG_INFO("Starting storage node for user "FMT_VPLUser_Id_t, context.userId);
    {
        u64 deviceId;
        rv = ESCore_GetDeviceGuid(&deviceId);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "ESCore_GetDeviceGuid", rv);
            goto out;
        }

        vplex::vsDirectory::SessionInfo vsdsSession;
        Query_FillInVsdsSession(&vsdsSession, context.session);

        char snRoot[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForStorageNode(sizeof(snRoot), snRoot);
        Util_CreatePath(snRoot, VPL_TRUE);

        char vsdsHostname[HOSTNAME_MAX_LENGTH];
        CCD_GET_INFRA_CLUSTER_HOSTNAME(vsdsHostname, context.clusterId);

        char remotefileTempFolder[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForRemoteFile(sizeof(remotefileTempFolder), remotefileTempFolder);

        char remoteExecutableDBFolder[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForRemoteExecutable(sizeof(remoteExecutableDBFolder), remoteExecutableDBFolder);

        rv = RemoteExecutableManager_Init(remoteExecutableDBFolder);
        if (rv != 0) {
            LOG_ERROR("Error in RemoteExecutableManager_Init(): %d", rv);
            goto out;
        }

        storageNode = new vss_server();
        storageNode->set_server_config(vsdsSession, 
                                       context.session.session_secret(),
                                       context.userId, deviceId, snRoot,
                                       __ccdConfig.serverServicePortRange,
                                       __ccdConfig.infraDomain,
                                       vsdsHostname,
                                       __ccdConfig.tagEditPath,
                                       remotefileTempFolder,
                                       __ccdConfig.vsdsContentPort,
                                       CachePlayer::MSAGetObjectMetadata,
                                       NULL,
                                       __ccdConfig.enableTs,
                                       localInfo);
        rv = storageNode->start();
        if (rv != 0) {
            LOG_ERROR("Failed to start storage node for user "FMT_VPLUser_Id_t": %d", context.userId, rv);
            LocalServers_StopStorageNode();
        }
        else {
            // Bug 9230: try to initialize MSA component when storageNode started
            MMError rc = MSAInitForDiscovery(context.userId, deviceId);
            if (rc < 0) {
                // Failed to initialize MSA component
                LOG_ERROR("Failed to initialize MSA: %d ", rc);
            }
        }

        // Need to report the local address (in case we are already connected to ANS).
        storageNode->notifyNetworkChange(ANSConn_GetLocalAddr());

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        // Init RemoteFile access control list
        {
            ccd::RemoteFileAccessControlDirs rfaclDirs;
            char filename[CCD_PATH_MAX_LENGTH];
            DiskCache::getPathForUserRFAccessControlList(context.userId, sizeof(filename), filename);
            ProtobufFileReader reader;
            int err = reader.open(filename, false);
            if (err < 0) {
                LOG_WARN("reader.open(%s) returned %d; expected if "
                        "CCDIUpdateStorageNode(add_remotefile_access_control_dir) has not been called.",
                        filename, err);
                //skip restoring rf acl
                goto skip_rf_acl;
            }
            LOG_DEBUG("Parsing from %s", filename);
            google::protobuf::io::CodedInputStream tempStream(reader.getInputStream());
            if(!rfaclDirs.ParseFromCodedStream(&tempStream)) {
                LOG_ERROR("Failed to parse from %s", filename);
                //rv = CCD_ERROR_PARSE_CONTENT;
                //goto skip_rf_acl;
            } else {
                LOG_DEBUG("Success parsing from %s", filename);
            }
            if(context.deviceStorageDatasetId){
                for(int i=0; i<rfaclDirs.dirs_size(); i++){
                    const ccd::RemoteFileAccessControlDirSpec& dir = rfaclDirs.dirs(i);
                    storageNode->updateRemoteFileAccessControlDir(context.userId,
                            context.deviceStorageDatasetId, dir, true);
                }
            }else{
                LOG_ERROR("Cannot find datasetId");
            }
        }
skip_rf_acl:
#endif  //#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

        // Initialize storage node to handle syncbox
        storageNode->setSyncboxArchiveStorageParam(context.syncboxArchiveStorageDatasetId, context.isSyncboxArchiveStorage, context.syncboxSyncFeaturePath);

        // Create syncbox staging area if not exist yet
        if (context.isSyncboxArchiveStorage) {
            std::string stagingAreaPath = storageNode->getSyncboxArchiveStorageStagingAreaPath(context.syncboxArchiveStorageDatasetId);
            rv = Util_CreateDir(stagingAreaPath.data());
            if (rv != 0) {
                LOG_ERROR("Syncbox staging area %s creation fail", stagingAreaPath.c_str());
            }
        }

    }
out:
    return rv;
}

void LocalServers_StopStorageNode()
{
    ASSERT(!Cache_ThreadHasLock());
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_localServersMutex));
    if (storageNode != NULL) {
        LOG_INFO("Stopping Storage Node.");
        lock.UnlockNow();
        storageNode->stop();
        lock.Relock(VPLLazyInitMutex_GetMutex(&s_localServersMutex));
        delete storageNode;
        storageNode = NULL;
        RemoteExecutableManager_Destroy();
        MSADestroy();
    }
}

#endif // #if CCD_ENABLE_STORAGE_NODE

void LocalServers_StopServers(bool userLogout)
{
    // TODO: bug 17701: ASSERT(!Cache_ThreadHasLock());
    LOG_INFO("Stopping local HTTP service.");
    httpService.stop(userLogout);
#if CCD_ENABLE_STORAGE_NODE
    LOG_INFO("Stopping storage node.");
    LocalServers_StopStorageNode();
#endif
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_localServersMutex));
    delete localInfo;
    localInfo = NULL;
}

//-----------------------------------------
