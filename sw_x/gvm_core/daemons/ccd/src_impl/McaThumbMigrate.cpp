//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//
#include "McaThumbMigrate.hpp"

#include "vpl_plat.h"
#include "cache.h"
#include "SyncConfigDb.hpp"
#include "SyncConfigUtil.hpp"
#include "SyncFeatureMgr.hpp"
#include "gvm_rm_utils.hpp"
#include "gvm_thread_utils.h"
#include "vpl_fs.h"
#include "vplex_file.h"
#include "vplu_mutex_autolock.hpp"
#include <ccdi_rpc.pb.h>
#include <deque>
#include <log.h>

static McaMigrateThumb s_migrateThumbInstance;

McaMigrateThumb& getMcaMigrateThumbInstance()
{
    return s_migrateThumbInstance;
}

McaMigrateThumb::McaMigrateThumb()
:  m_isMigrateThreadInit(false),
   m_run(false),
   m_migrationExists(false),
   m_user_id(0),
   m_dataset_id(0),
   m_activation_id(0)
{
    int rc;
    rc = VPLMutex_Init(&m_apiMutex);
    if (rc != 0) {
        LOG_ERROR("VPLMutex_Init failed: %d", rc);
    }

    rc = VPLSem_Init(&m_threadFinishedSem, 1, 0);
    if (rc != 0) {
        LOG_ERROR("VPLSem_Init:%d", rc);
    }
}

McaMigrateThumb::~McaMigrateThumb()
{
    if(m_isMigrateThreadInit) {
        McaThumbStopMigrate();
    }
    VPLSem_Destroy(&m_threadFinishedSem);
    VPLMutex_Destroy(&m_apiMutex);
}

// TODO: Copied from SyncConfig.cpp
static int beginDbTransactions(SyncConfigDb& localDb,
                               bool& inTransaction_in_out)
{
    if(!inTransaction_in_out) {
        int dbBegin = localDb.beginTransaction();
        if(dbBegin != 0) {
            LOG_ERROR("beginTransaction:%d", dbBegin);
            return dbBegin;
        }else{
            inTransaction_in_out = true;
        }
    }
    return 0;
}

static const u32 TRANSACTION_NUM_THRESHHOLD = 500;
static const u64 TRANSACTION_BYTES_THRESHHOLD = 20*1000*1000;
static int commitDbTransactions(SyncConfigDb& localDb,
                                bool& inTransaction_in_out,
                                u32& numTransactions_in_out,
                                u64& bytesTransactionSoFar_in_out,
                                u64 numTransactionBytes,
                                bool force_commit)
{
    if(inTransaction_in_out)
    {
        ++numTransactions_in_out;
        bytesTransactionSoFar_in_out += numTransactionBytes;
        if(force_commit ||
           numTransactions_in_out >= TRANSACTION_NUM_THRESHHOLD ||
           bytesTransactionSoFar_in_out >= TRANSACTION_BYTES_THRESHHOLD)
        {
            int rc = localDb.commitTransaction();
            if(rc != 0) {
                LOG_ERROR("commitTransaction:%d - %d",
                          rc, numTransactions_in_out);
                int revertRes = localDb.rollbackTransaction();
                if(revertRes != 0) {
                    LOG_ERROR("rollbackTransaction:%d", revertRes);
                }else{  // Revert successful
                    inTransaction_in_out = false;
                    numTransactions_in_out = 0;
                    bytesTransactionSoFar_in_out = 0;
                }
                return rc;
            }else{
                inTransaction_in_out = false;
                numTransactions_in_out = 0;
                bytesTransactionSoFar_in_out = 0;
            }
        }
    }
    return 0;
}

#define BEGIN_TRANSACTIONS(db, b_inTransaction)                             \
    do {                                                                    \
        int trans_rc = beginDbTransactions(db, b_inTransaction);            \
        if(trans_rc != 0) {                                                 \
            LOG_ERROR("beginDbTransactions:%d", trans_rc);                  \
        }                                                                   \
    } while(false)

#define CHECK_END_TRANSACTION_BYTES(db, b_inTransaction, numTransactions, numTransactionBytes, numBytes, b_force) \
    do {                                                                    \
        int trans_rc = commitDbTransactions(db, b_inTransaction,            \
                                            numTransactions,                \
                                            numTransactionBytes,            \
                                            numBytes,                       \
                                            b_force);                       \
        if(trans_rc != 0) {                                                 \
            LOG_ERROR("commitDbTransactions:%d", trans_rc);                 \
        }                                                                   \
    } while(false)

static bool isRoot(const SCRow_syncHistoryTree& entry)
{
    // Root is stored in the tree.  Root is an exception case because
    // the root will be returned as a child when getting the children
    // of the root path.
    if (entry.name == "") {
        if (entry.parent_path.size() != 0) {
            FAILED_ASSERT("Unexpected parent_path \"%s\"", entry.parent_path.c_str());
        }
        ASSERT(entry.is_dir);
        return true;
    }
    return false;
}

namespace {  // anonymous namespace required if symbol definition exists anywhere else.  SCRelPath does.
/// Path relative to sync config root.
/// For storing entries within the local DB.
/// Can be empty to indicate the sync config root.
/// @invariant No leading or trailing slash.
class SCRelPath
{
public:
    SCRelPath(const std::string& path) : path(path) { checkInvariants(); }
    inline void set(const std::string& path) {
        this->path = path;
        checkInvariants();
    }
    inline void checkInvariants() {
        if (path.size() > 0) {
            ASSERT_NOT_EQUAL(path[0], '/', "%c");
            ASSERT_NOT_EQUAL(path[path.size()-1], '/', "%c");
        }
    }
    inline const std::string& str() const { return path; }
    inline const char* c_str() const { return path.c_str(); }
    SCRelPath getChild(const std::string& entryName) const {
        ASSERT(entryName.size() != 0);
        if(path.size() == 0) {
            return SCRelPath(entryName);
        }else{
            return SCRelPath(path + "/" + entryName);
        }
    }
private:
    std::string path;
};

/// Absolute path on local filesystem.
/// For use when dealing with VPL file APIs (ls a directory, stat a file, read/write a file).
/// @invariant No trailing slash.  (Leading slash is allowed.)
/// TODO: special case for the root itself ("/")?  Right now, it isn't allowed.
class AbsPath
{
public:
    AbsPath(const std::string& path) : path(path) { checkInvariants(); }
    inline void set(const std::string& path) {
        this->path = path;
        checkInvariants();
    }
    inline void checkInvariants() {
        if (path.size() > 0) {
            ASSERT(path[path.size()-1] != '/');
        }
    }
    inline const std::string& str() const { return path; }
    inline const char* c_str() const { return path.c_str(); }
private:
    std::string path;
};

}  // anonymous namespace

/// @return the relative (to sync config root) path for the entry, no leading or trailing slashes.
static SCRelPath getRelPath(const SCRow_syncHistoryTree& entry)
{
    if (entry.parent_path.size() == 0) {
        // Entry is in the sync config root (or is the sync config root itself).
        return SCRelPath(entry.name);
    } else {
        ASSERT(entry.name.size() != 0);
        return SCRelPath(entry.parent_path + "/" + entry.name);
    }
}

/// @return Absolute path by appending relative path to absolute path
static AbsPath appendRelPath(const AbsPath& absPath,
                             const SCRelPath& relativePath)
{
    if (relativePath.str().size() == 0) { // Special case if syncConfigRelPath is the sync config root.
        return absPath;
    } else {
        return AbsPath(absPath.str() + "/" + relativePath.str());
    }
}

#define COPY_FILE_BUFFER_SIZE_BYTES (4*1024)

// TODO: copied from SyncUp.cpp (SyncUpJobs::AddJob)
static int copyFile(const std::string& srcPath,
                    const std::string& dstPath)
{
    int result = 0;
    int rc;
    // open source file
    VPLFile_handle_t fin = VPLFile_Open(srcPath.c_str(),
                                        VPLFILE_OPENFLAG_READONLY,
                                        0);
    if (!VPLFile_IsValidHandle(fin)) {
        LOG_ERROR("Failed to open %s for read", srcPath.c_str());
        result = fin < 0 ? (int)fin : CCD_ERROR_FOPEN;
        return result;
    }
    ON_BLOCK_EXIT(VPLFile_Close, fin);

    // make sure destination directory exists
    rc = Util_CreatePath(dstPath.c_str(), VPL_FALSE);
    if (rc) {
        LOG_ERROR("Failed to create directory for %s", dstPath.c_str());
        return rc;
    }

    // open destination file
    VPLFile_handle_t fout = VPLFile_Open(dstPath.c_str(),
                                         VPLFILE_OPENFLAG_CREATE|
                                            VPLFILE_OPENFLAG_WRITEONLY,
                                         VPLFILE_MODE_IRUSR|
                                            VPLFILE_MODE_IWUSR);
    if (!VPLFile_IsValidHandle(fout)) {
        LOG_ERROR("Failed to open %s for write:%d", dstPath.c_str(), (int)fout);
        result = fin < 0 ? (int)fin : CCD_ERROR_FOPEN;
        return result;
    }
    ON_BLOCK_EXIT(VPLFile_Close, fout);

    // copy contents
    void *buffer = malloc(COPY_FILE_BUFFER_SIZE_BYTES);
    if (!buffer) {
        result = CCD_ERROR_NOMEM;
        return result;
    }
    ON_BLOCK_EXIT(free, buffer);
    ssize_t bytes_read;
    while ((bytes_read = VPLFile_Read(fin, buffer, COPY_FILE_BUFFER_SIZE_BYTES)) > 0) {
        ssize_t bytes_written = VPLFile_Write(fout, buffer, bytes_read);
        if (bytes_written < 0) {
            result = bytes_written;
            return result;
        }
        if (bytes_written != bytes_read) {
            LOG_ERROR("Failed to make copy of %s->%s:"FMTd_ssize_t,
                      srcPath.c_str(), dstPath.c_str(), bytes_written);
            result = CCD_ERROR_DISK_SERIALIZE;
            return result;
        }
    }
    if (bytes_read < 0) {
        result = bytes_read;
        return result;
    }
    return result;
}

void McaMigrateThumb::copyFilesPhase()
{
    AbsPath src_dir(m_migrateState.mm_thumb_src_dir());
    AbsPath dst_dir(m_migrateState.mm_thumb_dest_dir());

    const BasicSyncConfig* metadataThumbDirs = NULL;
    int metadataThumbDirsSize = 0;
    int rc;
    int rv = 0;

    Cache_GetClientSyncConfig(/*out*/ metadataThumbDirs,
                              /*out*/ metadataThumbDirsSize);

    for(int dirIndex=0; dirIndex<metadataThumbDirsSize; ++dirIndex)
    {
        if(!m_run) {
            LOG_INFO("Migrate thumbDir: aborted early");
            rv = CCD_ERROR_NOT_RUNNING;
            break;
        }
        if(metadataThumbDirs[dirIndex].syncConfigId != ccd::SYNC_FEATURE_PHOTO_THUMBNAILS &&
           metadataThumbDirs[dirIndex].syncConfigId != ccd::SYNC_FEATURE_MUSIC_THUMBNAILS &&
           metadataThumbDirs[dirIndex].syncConfigId != ccd::SYNC_FEATURE_VIDEO_THUMBNAILS )
        {
            // We're only interested in thumbnail directories.
            continue;
        }
        if(metadataThumbDirs[dirIndex].type != SYNC_TYPE_ONE_WAY_DOWNLOAD) {
            LOG_ERROR("Migration only applies for thumbnail download, skipping (syncFeature:%d, type:%d)",
                      (int)metadataThumbDirs[dirIndex].syncConfigId,
                      (int)metadataThumbDirs[dirIndex].type);
            continue;
        }
        SCRow_admin adminRow_out;
        std::string pathSuffix(metadataThumbDirs[dirIndex].pathSuffix);
        if(pathSuffix.size()>0 && pathSuffix[0]=='/') {
            pathSuffix = pathSuffix.substr(1);
        }
        SCRelPath relThumbPath(pathSuffix);
        AbsPath src_thumb_dir = appendRelPath(src_dir, relThumbPath);
        AbsPath dst_thumb_dir = appendRelPath(dst_dir, relThumbPath);

        SCRelPath relDbPath(pathSuffix+"/"+SYNC_CONFIG_DB_DIR);
        AbsPath src_thumb_dir_db = appendRelPath(src_dir, relDbPath);
        AbsPath dst_thumb_dir_db = appendRelPath(dst_dir, relDbPath);

        LOG_INFO("Migrate thumbDir: COPY %s->%s",
                 src_thumb_dir.c_str(), dst_thumb_dir.c_str());

        SyncConfigDb srcDb(src_thumb_dir_db.str(),
                           getSyncDbFamily(metadataThumbDirs[dirIndex].type),
                           m_user_id,
                           m_dataset_id,
                           SYNC_CONFIG_ID_STRING);

        SyncConfigDb dstDb(dst_thumb_dir_db.str(),
                           getSyncDbFamily(metadataThumbDirs[dirIndex].type),
                           m_user_id,
                           m_dataset_id,
                           SYNC_CONFIG_ID_STRING);
        bool dst_bInTransx = false;
        u32 dst_numTransx = 0;
        u64 dst_numTransxBytes = 0;

        rc = srcDb.openDb();
        if(rc != 0) {
            LOG_ERROR("srcDb.openDb(%s):%d", src_thumb_dir_db.c_str(), rc);
            rv = rc;
            goto err_open_db;
        }
        rc = dstDb.openDb();
        if(rc != 0) {
            LOG_ERROR("dstDb.openDb(%s):%d", dst_thumb_dir_db.c_str(), rc);
            rv = rc;
            goto err_open_db;
        }

        rc = srcDb.admin_get(adminRow_out);
        if(rc != 0) {
            LOG_ERROR("srcDb.admin_get(%s):%d", src_thumb_dir_db.c_str(), rc);
        }
        if(adminRow_out.dataset_id_exists ||
           adminRow_out.user_id_exists ||
           adminRow_out.dataset_id_exists ||
           adminRow_out.sync_config_id_exists ||
           adminRow_out.last_opened_timestamp_exists)
        {
            BEGIN_TRANSACTIONS(dstDb, dst_bInTransx);
            rc = dstDb.admin_set(adminRow_out.dataset_id,
                                 adminRow_out.user_id,
                                 adminRow_out.dataset_id,
                                 adminRow_out.sync_config_id,
                                 adminRow_out.last_opened_timestamp);
            if(rc != 0) {
                LOG_ERROR("dstDb.admin_set(%s):%d", dst_thumb_dir_db.c_str(), rc);
                rv = rc;
                goto skip_catagory;
            }
            CHECK_END_TRANSACTION_BYTES(dstDb, dst_bInTransx, dst_numTransx, dst_numTransxBytes, 0, false);
        }

        {
            //syncHistoryTree_getChildren
            SCRow_syncHistoryTree rootDir;
            std::deque<SCRelPath> dirsToTraverseQ;
            dirsToTraverseQ.push_back(SCRelPath(""));
            while(!dirsToTraverseQ.empty())
            {
                if(!m_run) {
                    LOG_ERROR("Migrate thumbDir: aborted early");
                    rv = CCD_ERROR_NOT_RUNNING;
                    goto skip_catagory;
                }
                SCRelPath toTraverse = dirsToTraverseQ.front();
                dirsToTraverseQ.pop_front();

                std::vector<SCRow_syncHistoryTree> dirEntries_out;
                rc = srcDb.syncHistoryTree_getChildren(toTraverse.str(), dirEntries_out);
                if ((rc != 0) && (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND)) {
                    LOG_ERROR("syncHistoryTree_get(%s, %s) failed: %d",
                              src_thumb_dir_db.c_str(), toTraverse.c_str(), rc);
                    // TODO: what to do?  Document why that is the right decision.
                    continue;
                }
                for(std::vector<SCRow_syncHistoryTree>::iterator it = dirEntries_out.begin();
                    it != dirEntries_out.end();
                    ++it)
                {
                    SCRow_syncHistoryTree& currDirEntry = *it;

                    // Make sure all directories/files are rescanned, and deletes are accurate
                    currDirEntry.last_seen_in_version = 0;  // used to track deletes
                    currDirEntry.version_scanned = 0;       // used to track scanned (applies to directories only)

                    if(isRoot(currDirEntry)){
                        BEGIN_TRANSACTIONS(dstDb, dst_bInTransx);
                        rc = dstDb.syncHistoryTree_add(currDirEntry);
                        if(rc != 0) {
                            LOG_ERROR("dstDb.syncHistoryTree_add(%s), root, %d",
                                      dst_thumb_dir_db.c_str(), rc);
                            continue;
                        }
                        CHECK_END_TRANSACTION_BYTES(dstDb, dst_bInTransx, dst_numTransx, dst_numTransxBytes, 0, false);
                        continue;
                    } // Skip root exception case


                    if(currDirEntry.is_dir) {
                        SCRelPath currDirPath = getRelPath(currDirEntry);
                        // Should we actually make the directory?
                        //  Answer: No need, media metadata, under our control.
                        BEGIN_TRANSACTIONS(dstDb, dst_bInTransx);
                        rc = dstDb.syncHistoryTree_add(currDirEntry);
                        if(rc != 0) {
                            LOG_ERROR("dstDb.syncHistoryTree_add(%s), %s, %d",
                                      dst_thumb_dir_db.c_str(), currDirPath.c_str(), rc);
                            continue;
                        }
                        CHECK_END_TRANSACTION_BYTES(dstDb, dst_bInTransx, dst_numTransx, dst_numTransxBytes, 0, false);

                        dirsToTraverseQ.push_back(currDirPath);
                    }else{
                        SCRelPath currEntryPath = getRelPath(currDirEntry);
                        AbsPath srcPath = appendRelPath(src_thumb_dir, currEntryPath);
                        AbsPath dstPath = appendRelPath(dst_thumb_dir, currEntryPath);
                        u64 fileSize = 0;

                        // TODO: Check if the file is already present to skip
                        //       the copy step

                        // Copy file from src to dest
                        rc = copyFile(srcPath.str(), dstPath.str());
                        if(rc != 0) {
                            // Not necessarily wrong, could be scanned and still
                            // in the process of downloading.
                            LOG_WARN("copyFile(%s->%s):%d",
                                      srcPath.c_str(), dstPath.c_str(), rc);
                            continue;
                        }

                        if(currDirEntry.local_mtime_exists) {
                            rc = VPLFile_SetTime(dstPath.c_str(), currDirEntry.local_mtime);
                            if(rc != 0) {
                                LOG_ERROR("VPLFile_SetTime("FMTu64", %s):%d",
                                          currDirEntry.local_mtime, dstPath.c_str(),
                                          rc);
                            } else {
                                VPLFS_stat_t statBuf;
                                rc = VPLFS_Stat(dstPath.c_str(), &statBuf);
                                if(rc != 0) {
                                    LOG_ERROR("VPLFS_Stat(%s):%d",
                                              dstPath.c_str(), rc);
                                    // If this fails, it isn't critical, but it will
                                    // cause a re-download
                                }else{
                                    currDirEntry.local_mtime = statBuf.vpl_mtime;
                                    fileSize = statBuf.size;
                                }
                            }
                        }

                        BEGIN_TRANSACTIONS(dstDb, dst_bInTransx);
                        rc = dstDb.syncHistoryTree_add(currDirEntry);
                        if(rc != 0) {
                            LOG_ERROR("dstDb.syncHistoryTree_add(%s):%s, %d",
                                      dst_thumb_dir_db.c_str(), currEntryPath.c_str(), rc);
                        }
                        CHECK_END_TRANSACTION_BYTES(dstDb, dst_bInTransx, dst_numTransx, dst_numTransxBytes, fileSize, false);
                    }
                }
            }  // while(!dirsToTraverseQ.empty())
        }

 skip_catagory:
        CHECK_END_TRANSACTION_BYTES(dstDb, dst_bInTransx, dst_numTransx, dst_numTransxBytes, 0, true);
 err_open_db:
        // exit label
        srcDb.closeDb();
        dstDb.closeDb();

        if(!m_run) {
            LOG_INFO("Migrate thumbDir: aborted early");
            rv = CCD_ERROR_NOT_RUNNING;
            break;
        }
    }

    if(rv != 0) {
        LOG_ERROR("Phase migration copy failed(%s->%s):%d  Skipping to next phase anyways",
                  src_dir.c_str(), dst_dir.c_str(), rv);
        // We want to continue to the next phase in order to support moving back to
        // internal once sdcard is removed.  Failure on copy would then be expected.
    }

    LOG_INFO("Migrate thumbDir: SWITCH %s->%s", src_dir.c_str(), dst_dir.c_str());

    rc = Cache_SetThumbnailMigrateDeletePhase(m_activation_id);
    if(rc != 0) {
        LOG_ERROR("Cache_SetThumbnailMigrateDeletePhase(%d):%d",
                  m_activation_id, rc);
        rv = rc;
    }
    // Will set our local copy (m_migrateState) later when we get
    // cache lock

    rc = SyncFeatureMgr_Remove(m_user_id,
                               m_dataset_id,
                               ccd::SYNC_FEATURE_PHOTO_THUMBNAILS);
    if(rc != CCD_ERROR_NOT_FOUND)
    {   // These sync_configs should be "Not found", as they were stopped earlier
        LOG_WARN("Unexpected sync_config: SyncFeatureRemove("FMTu64","FMTu64",photo_thumb):%d",
                 m_user_id,
                 m_dataset_id,
                 rc);
    }
    rc = SyncFeatureMgr_Remove(m_user_id,
                               m_dataset_id,
                               ccd::SYNC_FEATURE_MUSIC_THUMBNAILS);
    if(rc != CCD_ERROR_NOT_FOUND)
    {   // These sync_configs should be "Not found", as they were stopped earlier
        LOG_ERROR("Unexpected sync_config: SyncFeatureRemove("FMTu64","FMTu64",music_thumb):%d",
                  m_user_id,
                  m_dataset_id,
                  rc);
    }
    rc = SyncFeatureMgr_Remove(m_user_id,
                               m_dataset_id,
                               ccd::SYNC_FEATURE_VIDEO_THUMBNAILS);
    if(rc != CCD_ERROR_NOT_FOUND)
    {   // These sync_configs should be "Not found", as they were stopped earlier
        LOG_ERROR("Unexpected sync_config: SyncFeatureRemove("FMTu64","FMTu64",video_thumb):%d",
                  m_user_id,
                  m_dataset_id,
                  rc);
    }

    LOG_INFO("Migrate thumbDir: Restarting SyncConfigs.");
    CacheAutoLock lock;
    lock.LockForWrite();
    CachePlayer* user = cache_getUserByActivationId(m_activation_id);
    if(user != NULL) {
        cache_registerSyncConfigs(*user);
        m_migrateState = user->_cachedData.details().migrate_mm_thumb_download_path();
    }else{
        LOG_INFO("Migrate thumbDir: User not logged in ("FMTu64","FMTu64",%d)",
                 m_user_id,
                 m_dataset_id,
                 m_activation_id);
    }
    LOG_INFO("Migrate thumbDir: SWITCH complete, will DELETE src:%s->%s",
             src_dir.c_str(), dst_dir.c_str());
}

void McaMigrateThumb::deleteFilesPhase()
{
    AbsPath src_dir(m_migrateState.mm_thumb_src_dir());

    const BasicSyncConfig* metadataThumbDirs = NULL;
    int metadataThumbDirsSize = 0;
    int rc;
    int rv = 0;

    Cache_GetClientSyncConfig(metadataThumbDirs,
                              metadataThumbDirsSize);

    for(int dirIndex=0; dirIndex<metadataThumbDirsSize; ++dirIndex)
    {
        if(!m_run) {
            LOG_INFO("Migrate thumbDir: aborted early");
            rv = CCD_ERROR_NOT_RUNNING;
            break;
        }
        if(metadataThumbDirs[dirIndex].syncConfigId != ccd::SYNC_FEATURE_PHOTO_THUMBNAILS &&
           metadataThumbDirs[dirIndex].syncConfigId != ccd::SYNC_FEATURE_MUSIC_THUMBNAILS &&
           metadataThumbDirs[dirIndex].syncConfigId != ccd::SYNC_FEATURE_VIDEO_THUMBNAILS )
        {
            // We're only interested in thumbnail directories.
            continue;
        }
        if(metadataThumbDirs[dirIndex].type != SYNC_TYPE_ONE_WAY_DOWNLOAD) {
            LOG_ERROR("Migration only applies for thumbnail download, skipping (syncFeature:%d, type:%d)",
                      (int)metadataThumbDirs[dirIndex].syncConfigId,
                      (int)metadataThumbDirs[dirIndex].type);
            continue;
        }

        std::string pathSuffix(metadataThumbDirs[dirIndex].pathSuffix);
        if(pathSuffix.size()>0 && pathSuffix[0]=='/') {
            pathSuffix = pathSuffix.substr(1);
        }
        SCRelPath relThumbPath(pathSuffix);
        AbsPath src_thumb_dir = appendRelPath(src_dir, relThumbPath);
        AbsPath src_temp_delete = appendRelPath(src_dir, SCRelPath("to_delete"));
        LOG_INFO("Migrate thumbDir: DELETE:%s", src_thumb_dir.c_str());
        rc = Util_rmRecursive(src_thumb_dir.str(), src_temp_delete.str());
        if(rc != 0) {
            LOG_ERROR("Util_rm_dash_rf:%d, %s", rc, src_thumb_dir.c_str());
            rv = rc;
        }
    }

    if(rv == 0) {
        LOG_INFO("Migrate thumbDir: src DELETE complete");
        rc = Cache_ClearThumbnailMigrate(m_activation_id);
        if(rc != 0) {
            LOG_ERROR("Cache_ClearThumbnailMigrate(%d):%d",
                      m_activation_id, rc);
        }
        m_migrateState.Clear();  // Call not necessary, but for consistency.
    }else{
        LOG_ERROR("Phase migration delete failed:%d", rv);
    }
}

VPLTHREAD_FN_DECL McaMigrateThumb::runMcaThumbMigrate(void* arg)
{
    McaMigrateThumb* migrateInstance = (McaMigrateThumb*) arg;
    bool deletePhase = migrateInstance->m_migrateState.mm_delete_phase();
    LOG_INFO("Migrate thumbDir: thread starting");
    if(!deletePhase) {
        migrateInstance->copyFilesPhase();
    }else{
        LOG_INFO("Migrate thumbDir: Copy phase already completed.");
    }

    if(!migrateInstance->m_run) {
        LOG_INFO("Migrate thumbDir: thread stopped");
        goto exit;
    }

    deletePhase = migrateInstance->m_migrateState.mm_delete_phase();
    if(deletePhase) {
        migrateInstance->deleteFilesPhase();
    }else {
        LOG_WARN("Migrate thumbDir: Not performing delete phase");
    }

 exit:
    LOG_INFO("Migrate thread exiting");
    // No need to hold lock, thread can end whenever it wants.
    int rc = VPLSem_Post(&migrateInstance->m_threadFinishedSem);
    if (rc != 0) {
        LOG_ERROR("VPLSem_Post:%d", rc);
    }
    migrateInstance->m_run = false;
    migrateInstance->m_isMigrateThreadInit = false;
    return VPLTHREAD_RETURN_VALUE;
}

int McaMigrateThumb::McaThumbResumeMigrate(u32 activationId)
{
    int rc;
    int rv;
    MutexAutoLock lock(&m_apiMutex);
    if(m_isMigrateThreadInit) {
        LOG_ERROR("Thread already running");
        return -1;
    }
    if(m_run) {
        // Note: This is technically possible due to instruction reordering (since 
        //     m_isMigrateThreadInit is not volatile):
        // m_isMigrateThreadInit was false, yet m_run was true.
        LOG_ERROR("Already running");
        return -1;
    }

    {
        CacheAutoLock cacheLock;
        rv = cacheLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            return rv;
        }
        CachePlayer* user = cache_getUserByActivationId(activationId);
        if(user==NULL) {
            LOG_ERROR("No user for activationId:"FMTu32, activationId);
            return -1;
        }
        if(!user->_cachedData.details().has_migrate_mm_thumb_download_path()) {
            // There's no migration task, just return success.
            return 0;
        }
        m_migrateState = user->_cachedData.details().migrate_mm_thumb_download_path();
        
        m_user_id = user->user_id();
        m_activation_id = user->local_activation_id();
        const ccd::CachedUserDetails& userDetails = user->_cachedData.details();
        const vplex::vsDirectory::DatasetDetail* mediaMetadataDataset = Util_FindMediaMetadataDataset(userDetails);
        if(mediaMetadataDataset == NULL) {
            LOG_ERROR("Cannot get mediaMetadataDataset for user:"FMTu64, m_user_id);
            return CCD_ERROR_DATASET_NOT_FOUND;
        }
        m_dataset_id = mediaMetadataDataset->datasetid();
        m_isMigrateThreadInit = true;
        m_run = true;
    }

    // This looks a little fragile, especially because we VPLSem_Post above without holding the mutex,
    // but since we are protecting this function with m_isMigrateThreadInit and m_apiMutex, it shouldn't
    // be possible for any other thread to be using m_threadFinishedSem when a thread is here.
    // m_isMigrateThreadInit cannot become false until runMcaThumbMigrate is done with m_threadFinishedSem.
    
    // Reset the semaphore.  It is very likely that McaThumbStopMigrate was never
    // called so wait was never called on the signal)
    rc = VPLSem_Destroy(&m_threadFinishedSem);
    if (rc != 0) {
        LOG_ERROR("VPLSem_Destroy:%d", rc);
    }
    rc = VPLSem_Init(&m_threadFinishedSem, 1, 0);
    if (rc != 0) {
        LOG_ERROR("VPLSem_Init:%d", rc);
    }

    rv = Util_SpawnThread(runMcaThumbMigrate, this,
                              UTIL_DEFAULT_THREAD_STACK_SIZE,
                              VPL_FALSE,
                              &m_migrateThread);
    if (rv != 0) {
        LOG_ERROR("Util_SpawnThread failed: %d", rv);
    }

    return rv;
}

int McaMigrateThumb::McaThumbStopMigrate()
{
    // Cannot hold cache lock otherwise there might be a deadlock
    // possibility.
    ASSERT(!Cache_ThreadHasLock());
    int rc = 0;
    m_run = false;

    MutexAutoLock lock(&m_apiMutex);
    if (m_isMigrateThreadInit) {
        LOG_INFO("Ongoing migrate exiting.");
        rc = VPLSem_Wait(&m_threadFinishedSem);
        if (rc != 0) {
            LOG_ERROR("VPLSem_Wait:%d", rc);
        }
        LOG_INFO("Ongoing migrate exited.");
        m_isMigrateThreadInit = false;
    }
    return rc;
}
