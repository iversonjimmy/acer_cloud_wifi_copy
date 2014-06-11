#include "picstream.hpp"
#include "ccdi_orig_types.h"
#include <ccdi_rpc.pb.h>
#include "gvm_errors.h"
#include "gvm_file_utils.h"
#include "scopeguard.hpp"
#include "vpl_fs.h"
#include "vpl_thread.h"
#include "vpl_lazy_init.h"
#include "vplu_mutex_autolock.hpp"
#include "vplu_sstr.hpp"
#include "vplex_file.h"
#include "vpl_time.h"
#include "vplex_time.h"
#include "config.h"
#include "cache.h"
#include "ccd_features.h"
#include "exif_parser.h"
#if CCD_ENABLE_SYNCUP
#include "SyncUp.hpp"
#endif
#include "virtual_device.hpp"
#include "vcs_utils.hpp"

#include <algorithm>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include <log.h>

static volatile bool s_isRunning = false;
#if VPLFS_MONITOR_SUPPORTED
static bool s_isInitFileMonitor = false;
#endif
#define PICSTREAM_STACK_SIZE              (64 * 1024)
static VPLThread_t s_thread = VPLTHREAD_INVALID;
static VPLLazyInitMutex_t s_interfaceMutex = VPLLAZYINITMUTEX_INIT;
static VPLLazyInitMutex_t s_runThreadMutex = VPLLAZYINITMUTEX_INIT;
static VPLLazyInitCond_t s_condVar = VPLLAZYINITCOND_INIT;
static bool s_condVarWorkToDo = false;

// Optimization to not stat PICSTREAM_ENABLE_TOUCH_FILE extremely often
static volatile bool s_bEnabled = false;
static const char* PICSTREAM_ENABLE_TOUCH_FILE = "pic_enable.touch";
static const char* PICSTREAM_TEMP_DIR = "temp"; // within the s_picstream_module_dir
static const char* PICSTREAM_DSET_DIR = "dset"; // within the s_picstream_module_dir

static bool s_bInit = false;
static VPLTime_t s_waitQueueTime;
static std::string s_picstream_module_dir;
static u64 s_userId;

struct PicstreamDir
{
    bool b_isDirInit;
    bool b_performFullSync;
    std::string directory;
    VPLFS_MonitorHandle monitorHandle;
    u64 userId;
    void clear();
    PicstreamDir() { clear(); }
};

void PicstreamDir::clear()
{
    b_isDirInit = false;
    b_performFullSync = false;
    userId = 0;
    directory.clear();
}

static PicstreamDir s_picstreamDir[MAX_PICSTREAM_DIR];

static const std::string touch_file_pic_enable(const std::string& module_dir)
{
    std::string filePath;
    filePath = module_dir + std::string("/") +
               std::string(PICSTREAM_ENABLE_TOUCH_FILE);
    return filePath;
}

static const std::string touch_file_pic_src(u32 index)
{
    // examples "pic_src_0.touch", "pic_src_1.touch"
    std::string filePath;
    char indexStr[15];
    sprintf(indexStr, "%d", index);
    filePath = s_picstream_module_dir + std::string("/") +
               std::string("pic_src_")+std::string(indexStr)+std::string(".touch");
    return filePath;
}

static const std::string touch_file_pic_src_cb(u32 index)
{
    // examples "pic_src_cb_0.touch", "pic_src_cb_1.touch"
    return SSTR(s_picstream_module_dir << "/pic_src_cb_" << index << ".touch");
}

static const std::string get_temp_dir()
{
    std::string path;
    path = s_picstream_module_dir + std::string("/") +
           std::string(PICSTREAM_TEMP_DIR);
    return path;
}

const std::string Picstream_GetDsetDir(const std::string& internalPicDir)
{
    std::string path;
    path = internalPicDir + std::string("/") + std::string(PICSTREAM_DSET_DIR);
    return path;
}

static int destructiveTouch(const char* file)
{
    int rv = 0;
    VPLFile_handle_t touchFile;
    touchFile = VPLFile_Open(file,
                             VPLFILE_OPENFLAG_WRITEONLY | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_CREATE,
                             0600);
    if(!VPLFile_IsValidHandle(touchFile)) {
        LOG_ERROR("Create file error. file:%s, %d)", file, touchFile);
        rv = -1;
    }else{
        int rc = VPLFile_Close(touchFile);
        if(rc != VPL_OK) {
            LOG_ERROR("Could not close %d, %d, %s", rc, touchFile, file);
        }
    }
    return rv;
}

static char toLower(char in){
  if(in<='Z' && in>='A')
    return in-('Z'-'z');
  return in;
}

// PRODUCT SPEC OF SUPPORTED FILES: 12-17-2012
// (Case insentive)
// JPEG (.jpg, .jpeg)
// TIFF (.tif, .tiff)
// PNG (.png)
// BMP (.bmp)
static bool isPhotoFile(const std::string& dirent) {
    std::string ext;
    std::string toCompare;
    static const char* supported_exts[] = {
        ".jpg",
        ".jpeg",
        ".png",
        ".tif",
        ".tiff",
        ".bmp",
        ".mpo"
    };

    for (unsigned int i = 0; i < ARRAY_ELEMENT_COUNT(supported_exts); i++) {
        ext.assign(supported_exts[i]);
        if (dirent.size() >= ext.size()) {
            toCompare = dirent.substr(dirent.size() - ext.size(), ext.size());
            for (unsigned int i = 0; i < toCompare.size(); i++) {
                toCompare[i] = toLower(toCompare[i]);
            }
            if (toCompare == ext) {
                return true;
            }
        }
    }

    return false;
}


static bool isPhotoTempFile(const std::string& dirent) {
    std::string ext;
    std::string toCompare;
    static const char* tmp_ext = ".tmp";
    
    ext.assign(tmp_ext);
    if (dirent.size() >= ext.size()) {
        toCompare = dirent.substr(dirent.size() - ext.size(), ext.size());
        for (unsigned int i = 0; i < toCompare.size(); i++) {
            toCompare[i] = toLower(toCompare[i]);
        }
        if (toCompare == ext) {
            return true;
        }
    }

    return false;
}


struct PicstreamWaitStruct
{
    VPLTime_t insertTime;
    u32 picDirIndex;
    std::string relDir;
    std::string dirent;
};

// map (filePath, waitStruct)
typedef std::map<std::string, PicstreamWaitStruct> PicstreamWaitQ;
static PicstreamWaitQ s_waitQ;

static void insertWaitQueue(u32 index,
                            const std::string& srcDir,
                            const std::string& relDir,
                            const std::string& dirent)
{
    PicstreamWaitStruct waitStruct;
    waitStruct.insertTime = VPLTime_GetTimeStamp();
    waitStruct.picDirIndex = index;
    waitStruct.relDir = relDir;
    waitStruct.dirent = dirent;

    std::string filePath;
    Util_appendToAbsPath(srcDir, relDir, filePath);
    Util_appendToAbsPath(filePath, dirent, filePath);
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    s_waitQ[filePath] = waitStruct;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
}

static bool isBeginWithPeriod(const std::string& filename)
{
    if(filename.size()>0 && filename[0]=='.') {
        return true;
    }
    return false;
}

static int addAsNewSyncUpJob(u64 userId, const std::string &localPath, bool make_copy)
{
#if CCD_ENABLE_SYNCUP
    u64 timestamp; 
    VPLTime_CalendarTime dateTime;
    int rv;

    u64 datasetId;
    int err = VCS_getDatasetID(userId, "PicStream", datasetId);
    if (err != 0 && err != CCD_ERROR_DATASET_SUSPENDED) {
        LOG_ERROR("PicStream dataset not found: %d", err);
        return err;
    } // else
      // we want to add to SyncUp db if no error, or dataset is suspended
      // (not an infra operation)

    rv = EXIFGetImageTimestamp(localPath, dateTime, timestamp);
    if (rv != 0) {
        VPLFS_stat_t stat;
        timestamp = VPLTime_ToSec(VPLTime_GetTime()) - timezone;  // Convert to localtime

        rv = VPLFS_Stat(localPath.c_str(), &stat);
        if (rv == VPL_OK) {
            timestamp = stat.ctime - timezone; // Convert to localtime
            LOG_INFO("Use ctime");
        } else {
            LOG_INFO("Use send time");
        }

        if (daylight != 0) { // Daylight savings exist, check for and add daylight savings difference
            time_t rawTime;
            time(&rawTime);
            if (-1 == rawTime) {
                LOG_ERROR("time Error");
            } else {
                struct tm *curTime;
                curTime = localtime(&rawTime);
                if (NULL == curTime) {
                    LOG_ERROR("localtime Error");
                } else if (curTime->tm_isdst > 0){
                    timestamp += 3600;
                }
            }
        }

        rv = 0;
    }

    std::string cpath;
    {
        std::string basename = Util_getChild(localPath);
        std::ostringstream oss;
        oss << VirtualDevice_GetDeviceId() << "/" << basename;
        cpath.assign(oss.str());
    }

    LOG_INFO("Upload Pic to VCS: %s,willCopy:%d,exifTime:"FMTu64",datasetId:"FMTu64
             ",cpath:%s",
             localPath.c_str(), make_copy, timestamp, datasetId, cpath.c_str());
    SyncUp_AddJob(localPath, make_copy, /*ctime*/timestamp, /*mtime*/timestamp,
                  datasetId, cpath, ccd::SYNC_FEATURE_PICSTREAM_UPLOAD);
#endif // CCD_ENABLE_SYNCUP

    return 0;
}

static int processFile(const std::string& localPath,
                       const std::string& dirent,
                       const std::string& tmpDir,
                       const std::string& dstDir)
{
    std::string srcFile;
    Util_appendToAbsPath(localPath, dirent, srcFile);
    return addAsNewSyncUpJob(s_userId, srcFile, false);
}


static int performFullSync(u64 userId,
                           VPLTime_t enableTime,
                           VPLTime_t srcTime,
                           VPLTime_t currStartScanTime,
                           u32 index,
                           const std::string& srcDir,
                           const std::string& tmpDir,
                           const std::string& dstDir)
{
    int rv = VPL_OK;
    int rc;
    std::deque<std::string> dirRelLocalPathQ;
    dirRelLocalPathQ.push_back(std::string(""));

    while(!dirRelLocalPathQ.empty() && rv==VPL_OK && s_isRunning) {
        VPLFS_dir_t dp;
        VPLFS_dirent_t dirp;
        std::string currDir(dirRelLocalPathQ.front());
        std::string localPath;
        Util_appendToAbsPath(srcDir, currDir, localPath);
        dirRelLocalPathQ.pop_front();

        rc = VPLFS_Opendir(localPath.c_str(), &dp);
        if (rc != VPL_OK) {
            LOG_ERROR("Error opening (%s) err=%d",
                      localPath.c_str(), rc);
            rv = rc;
            rc = 0;
            break;
        }

        while (((rc = VPLFS_Readdir(&dp, &dirp)) == VPL_OK) &&
                (rv==VPL_OK) && s_isRunning) {
            std::string dirent(dirp.filename);
            if(dirent=="." || dirent=="..") {
                continue;
            }
            if(isBeginWithPeriod(dirent)) {
                LOG_INFO("Skipping. hidden file posix:%s",
                         dirent.c_str());
                continue;
            }

            VPLFS_stat_t statBuf;
            std::string localFile;
            Util_appendToAbsPath(localPath, dirent, localFile);
            std::string relativeFile = currDir + std::string("/") + dirent;
            rc = VPLFS_Stat(localFile.c_str(), &statBuf);
            if (rc != VPL_OK) {
                LOG_ERROR("Error(%d) using stat on (%s,%s)",
                          rc,
                          localPath.c_str(), dirent.c_str());
                rv = rc;
                rc = 0;
                break;
            }
            if(statBuf.isHidden) {
                LOG_INFO("Skipping hidden:%s", dirent.c_str());
                continue;
            }
            if(statBuf.isSymLink) {
                LOG_INFO("Skipping symLink:%s", dirent.c_str());
                continue;
            }

            bool isDir = (statBuf.type==VPLFS_TYPE_DIR);
            VPLTime_t created = VPLTIME_FROM_SEC(statBuf.ctime);
            VPLTime_t modified = VPLTIME_FROM_SEC(statBuf.mtime);

            if(isDir) {
                dirRelLocalPathQ.push_back(relativeFile);
                continue;
            }

            if(!isPhotoFile(dirent)) {
                continue;
            }

            if(enableTime > modified) {
                // Camera was not enabled for this pic.
                continue;
            }

            if(srcTime >= modified) {
                // srcTime indicates this picture already processed
                continue;
            }

            if(currStartScanTime < modified) {
                // currStartScanTime indicates start full sync time
                continue;
            }

            VPLTime_t currTime = VPLTime_GetTime();
            LOG_INFO("Pic Discovered:%s,%s, created:"FMT_VPLTime_t",modified:"FMT_VPLTime_t", "
                     "srcTime:"FMT_VPLTime_t",enableTime:"FMT_VPLTime_t", currTime:"FMT_VPLTime_t,
                     localPath.c_str(), dirent.c_str(), created, modified,
                     srcTime, enableTime, currTime);
            if(VPLTime_DiffClamp(currTime, modified) > s_waitQueueTime) {
                rc = processFile(localPath,
                                 dirent,
                                 tmpDir,
                                 dstDir);
                if(rc != 0) {
                    LOG_ERROR("processFile(%s, %s, %s, %s):%d, size:"FMTu64,
                              localPath.c_str(), dirent.c_str(),
                              tmpDir.c_str(), dstDir.c_str(),
                              rc, (u64)statBuf.size);
                    rv = rc;
                }
            } else {
                insertWaitQueue(index, srcDir, currDir, dirent);
            }
        }
        // VPL_ERR_MAX is a valid return code for VPLFS_Readdir when there are
        // no more files.
        if(rc != VPL_ERR_MAX && rc != VPL_OK)
        { // Something wrong reading more entries in the dir stream.
            LOG_ERROR("Reading dir entries:%d", rc);
            rv = rc;
        }

        rc = VPLFS_Closedir(&dp);
        if(rc != VPL_OK) {
            LOG_ERROR("Closedir failed:%d", rc);
            rv = rc;
        }

        if(rv != VPL_OK) {
            LOG_ERROR("Local traverse error, ending:%d", rv);
            continue;
        }
    }

    if(rv != VPL_OK) {
        LOG_ERROR("Local traverse additions:%d", rv);
    }
    return rv;
}

static void* picstreamRunThread(void* unused)
{
    LOG_INFO("Run thread started");
    s_condVarWorkToDo = false;
    while(s_isRunning) {
        int rc;
        VPLTime_t enableTime = 0;
        VPLFS_stat_t enableStat;
        std::string enableTouchFile = touch_file_pic_enable(s_picstream_module_dir);
        rc = VPLFS_Stat(enableTouchFile.c_str(), &enableStat);
        if(rc == VPL_OK) {
            enableTime = VPLTIME_FROM_SEC(enableStat.mtime);
            s_bEnabled = true;
        }else{
            s_bEnabled = false;
        }

        // Check for full scan required first
        for(int index = 0;
            index < MAX_PICSTREAM_DIR && s_bEnabled;
            index++)
        {
            VPLTime_t srcTime = 0;
            VPLTime_t preFullScanTime = 0, preCBAddTime = 0, currScanStartTime = 0;
            std::string srcTouchFile;
            PicstreamDir picstreamDir;
            VPLFS_stat_t srcStat;
            std::string tempDir;
            std::string dsetDir;

            VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));

            picstreamDir = s_picstreamDir[index];
            tempDir = get_temp_dir();
            dsetDir = Picstream_GetDsetDir(s_picstream_module_dir);

            VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));

            if(picstreamDir.b_isDirInit && picstreamDir.b_performFullSync) {
                /*
                  previous full scan time              fileMonitorCB add files                Current Scan Start
                  (preFullScanTime)                                (preCBAddTime)             (currScanStartTime)
                  ------|-----------------------------------|---|------|-------------------------|-------------------> time 

                   Only photos added between preFullScanTime and currScanStartTime  or preCBAddTime and currScanStartTime 
                   will be processed by performFullSync().
                */
                srcTouchFile = touch_file_pic_src_cb(index);
                rc = VPLFS_Stat(srcTouchFile.c_str(), &srcStat);
                if(rc == VPL_OK) {
                    preCBAddTime = VPLTIME_FROM_SEC(srcStat.mtime);
                }
                srcTouchFile = touch_file_pic_src(index);
                rc = VPLFS_Stat(srcTouchFile.c_str(), &srcStat);
                if(rc == VPL_OK) {
                    preFullScanTime = VPLTIME_FROM_SEC(srcStat.mtime);
                }

                srcTime = (preCBAddTime > preFullScanTime) ? preCBAddTime : preFullScanTime;

                destructiveTouch(srcTouchFile.c_str());
                rc = VPLFS_Stat(srcTouchFile.c_str(), &srcStat);
                if(rc == VPL_OK) {
                    currScanStartTime = VPLTIME_FROM_SEC(srcStat.mtime);
                } else {
                    currScanStartTime = VPLTime_GetTime(); //it's better not this case!
                }
                // could take a long time copying over files
                rc = performFullSync(picstreamDir.userId,
                                     enableTime,
                                     srcTime,
                                     currScanStartTime,
                                     index,
                                     picstreamDir.directory,
                                     tempDir, dsetDir);
                if(rc != 0) {
                    LOG_ERROR("performFullSync:%d, index:%d, Skipping to the next",
                              rc, index);
                    continue;
                }

                VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
                if(s_picstreamDir[index].b_isDirInit == true &&
                   s_picstreamDir[index].directory == picstreamDir.directory)
                {
                    //destructiveTouch(srcTouchFile.c_str());
                    s_picstreamDir[index].b_performFullSync = false;
                } else {
                    LOG_INFO("User changed picstream srcDir:%d, or %s -> %s",
                             s_picstreamDir[index].b_isDirInit,
                             picstreamDir.directory.c_str(),
                             s_picstreamDir[index].directory.c_str());
                }

                VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
            }
        }

        // Handle wait queue, also find out length of time to time out for.
        bool waitQWorkDone = false;
        VPLTime_t waitTimeout = VPL_TIMEOUT_NONE;

        // Process wait Q
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
        if(!s_bEnabled) {  // No need to wait for anything, camera roll disabled!
            s_waitQ.clear();
        }
        PicstreamWaitQ::iterator waitIter = s_waitQ.begin();
        while(waitIter != s_waitQ.end() && s_isRunning) {
            VPLTime_t currTime = VPLTime_GetTimeStamp();
            VPLTime_t difference = VPLTime_DiffClamp(currTime,
                                                     (*waitIter).second.insertTime);
            VPLTime_t remainingTime = s_waitQueueTime - difference;
            int index = (*waitIter).second.picDirIndex;
            if(!s_picstreamDir[index].b_isDirInit) {
                LOG_ERROR("Should never happen:%d.  Skipping %s.", index, s_picstreamDir[index].directory.c_str());
                ++waitIter;
                continue;
            }
            if(difference >= s_waitQueueTime) {
                std::string dirent = (*waitIter).second.dirent;
                std::string tmpDir = get_temp_dir();
                std::string dstDir = Picstream_GetDsetDir(s_picstream_module_dir);
                std::string localPath;
                Util_appendToAbsPath(s_picstreamDir[index].directory,
                                     (*waitIter).second.relDir,
                                     localPath);
                std::string absPath;
                Util_appendToAbsPath(localPath, dirent, absPath);
                VPLFS_stat_t statBuf;
                int rc;

                rc = VPLFS_Stat(absPath.c_str(), &statBuf);
                PicstreamWaitQ::iterator toDelete = waitIter;
                ++waitIter;
                s_waitQ.erase(toDelete);  // current iterator is invalid after erase
                if(rc != VPL_OK) {
                    LOG_ERROR("VPLFS_Stat:%d, %s", rc, absPath.c_str());
                    continue;
                }

                // Want to do file IO without lock.
                VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));

                LOG_INFO("WaitQ Pic Discovered:%s,%s, created:"FMT_VPLTime_t",modified:"FMT_VPLTime_t", "
                         ",enableTime:"FMT_VPLTime_t", currTime:"FMT_VPLTime_t,
                         localPath.c_str(), dirent.c_str(), (u64)statBuf.ctime, (u64)statBuf.mtime,
                         enableTime, currTime);
                rc = processFile(localPath,
                                 dirent,
                                 tmpDir,
                                 dstDir);
                if(rc != 0) {
                    LOG_ERROR("FAILED processFile(%s,%s,%s,%s):%d size:"FMTu64". Skipping.",
                              localPath.c_str(), dirent.c_str(),
                              tmpDir.c_str(), dstDir.c_str(),
                              rc, (u64)statBuf.size);
                }
                VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));

                waitQWorkDone = true;
                waitIter = s_waitQ.begin();
            }else if(remainingTime < waitTimeout) {
                // Find the minimum time we need to wait before wait Q will be ready
                waitTimeout = remainingTime;
                ++waitIter;
            }else{
                ++waitIter;
            }
        }

        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
        LOG_INFO("workToDo:%d, waitTimeout:"FMTu64,
                 s_condVarWorkToDo, waitTimeout);
        if(!s_condVarWorkToDo) {
            VPLCond_TimedWait(VPLLazyInitCond_GetCond(&s_condVar),
                              VPLLazyInitMutex_GetMutex(&s_runThreadMutex),
                              waitTimeout);
        }
        s_condVarWorkToDo = false;
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    }
    LOG_ALWAYS("RunThread Stopping");
    return NULL;
}

int Picstream_Init(u64 userId, const std::string& internalPicDir, VPLTime_t waitQTime)
{
    int rv = 0;
    int rc;
    VPLThread_attr_t thread_attr;
    u32 dirIndex;
    LOG_INFO("Internal PicstreamDir:%s, waitQTime:"FMTu64,
             internalPicDir.c_str(), waitQTime);

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));

    if(s_bInit) {
        rv = CCD_ERROR_ALREADY_INIT;
        goto fail_already_init;
    }

    s_userId = userId; 

    for(dirIndex = 0; dirIndex < MAX_PICSTREAM_DIR; ++dirIndex)
    {
        s_picstreamDir[dirIndex].clear();
    }

    rc = Util_CreatePath(internalPicDir.c_str(), true);
    if(rc != VPL_OK) {
        LOG_ERROR("Util_CreatePath:%d, %s", rc, internalPicDir.c_str());
        rv = rc;
        goto fail_create_path;
    }

    s_picstream_module_dir.assign(internalPicDir);

#if VPLFS_MONITOR_SUPPORTED
    rc = VPLFS_MonitorInit();
    if(rc != 0) {
        //No effect if user only sync down photos
        LOG_WARN("VPLFS_MonitorInit:%d", rc);
    } else {
        s_isInitFileMonitor = true;
    }
#endif

    rc = VPLThread_AttrInit(&thread_attr);
    if(rc != VPL_OK) {
        LOG_ERROR("Failed to initialize thread attributes:%d", rc);
        rv = rc;
        goto fail_attr_init;
    }

    VPLThread_AttrSetStackSize(&thread_attr, PICSTREAM_STACK_SIZE);
    LOG_INFO("Starting picstream thread with stack size %d", PICSTREAM_STACK_SIZE);
    s_isRunning = true;

    rc = VPLThread_Create(&s_thread,
                          picstreamRunThread, NULL,
                          &thread_attr, "picstreamThread");
    if(rc != VPL_OK) {
        LOG_ERROR("VPLThread_Create:%d", rc);
        rv = rc;
        goto fail_thread_create;
    }
    VPLThread_AttrDestroy(&thread_attr);

    s_waitQueueTime = waitQTime;
    s_bInit = true;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;

 fail_thread_create:
    s_isRunning = false;
    VPLThread_AttrDestroy(&thread_attr);
 fail_attr_init:
 fail_create_path:
 fail_already_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

int Picstream_Destroy()
{
    int rv = 0;
    LOG_INFO("Called");
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    if(!s_bInit) {
        LOG_WARN("Not initialized");
        rv = CCD_ERROR_NOT_INIT;
        goto fail_not_init;
    }
    LOG_INFO("Signal PicStream Destroy");
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    s_isRunning = false;
    s_condVarWorkToDo = true; // The work is to destroy itself;
    VPLCond_Signal(VPLLazyInitCond_GetCond(&s_condVar));
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    VPLThread_Join(&s_thread, NULL);
    s_bInit = false;
    s_thread = VPLTHREAD_INVALID;
#if VPLFS_MONITOR_SUPPORTED
    if(s_isInitFileMonitor) {
        VPLFS_MonitorDestroy();
        s_isInitFileMonitor = false;
    }
#endif

 fail_not_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

int Picstream_Enable(bool enable)
{
    int rv = 0;
    int rc;
    std::string filePath;
    LOG_INFO("Enable:%d", enable);
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));

    if(!s_bInit) {
        LOG_ERROR("Picstream not init.");
        rv = CCD_ERROR_NOT_INIT;
        goto fail_not_init;
    }

    filePath = touch_file_pic_enable(s_picstream_module_dir);

    if(enable) {
        rc = destructiveTouch(filePath.c_str());
        if(rc != VPL_OK) {
            LOG_ERROR("destructiveTouch:%d, %s", rc, filePath.c_str());
            rv = rc;
        }
        s_bEnabled = true;
    }else{
        rc = VPLFile_Delete(filePath.c_str());
        if(!(rc == VPL_OK || rc == VPL_ERR_NOENT)) {
            LOG_ERROR("VPLFile_Delete:%d, %s", rc, filePath.c_str());
            rv = rc;
        }
        s_bEnabled = false;
    }

 fail_not_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

int Picstream_UnInitClearEnable(const std::string& internalPicDir)
{
    int rv = 0;
    int rc;
    std::string filePath;
    LOG_INFO("UnInitClearEnable:%s", internalPicDir.c_str());
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));

    filePath = touch_file_pic_enable(internalPicDir);

    rc = VPLFile_Delete(filePath.c_str());
    if(!(rc == VPL_OK || rc == VPL_ERR_NOENT || rc == VPL_ERR_NOTDIR)) {
        LOG_ERROR("VPLFile_Delete:%d, %s", rc, filePath.c_str());
        rv = rc;
    }
    s_bEnabled = false;

    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

static bool isFile(const std::string& filePath)
{
    int rc;
    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(filePath.c_str(), &statBuf);
    if(rc != VPL_OK) {
        LOG_ERROR("VPLStat:%d, %s", rc, filePath.c_str());
        return false;
    }
    if(statBuf.type == VPLFS_TYPE_FILE) {
        return true;
    }
    return false;
}

static std::string parent(const std::string& path, int levelsUp)
{
    std::string parent(path);

    for(;levelsUp > 0; levelsUp--) {
        ssize_t index = parent.rfind('/');
        if(index == -1) {
            parent = "";
            break;
        }
        parent = parent.substr(0, index);
    }

    return parent;
}

static std::string child(const std::string& path, int levelsUp)
{
    std::string child;
    ssize_t index = path.size();
    for(;levelsUp > 0; levelsUp--) {
        index = path.rfind('/', index-1);
        if(index == -1) {
            break;
        }
    }
    if(index >= 0) {
        child = path.substr(index, path.size()-index);
    }else{
        child = path;
    }

    return child;
}

static void picstream_monitorCb(VPLFS_MonitorHandle handle,
                                void* eventBuffer,
                                int eventBufferSize,
                                int error)
{
    u32 numEvents = 0;
    u32 index = MAX_PICSTREAM_DIR;
    std::string srcDir;
    std::string relDir;
    std::string dirent;
    std::string direntSrc;
    std::string srcTouchFile;

    if(!s_bEnabled) {  // Optimization to not stat touchfile
        LOG_INFO("Ignoring event, CameraRoll not enabled");
        return;
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    for(int findIndex = 0; findIndex < MAX_PICSTREAM_DIR; ++findIndex) {
        if(s_picstreamDir[findIndex].b_isDirInit &&
           s_picstreamDir[findIndex].monitorHandle == handle)
        {
            index = findIndex;
            break;
        }
    }

    if(index < MAX_PICSTREAM_DIR) {
        srcDir = s_picstreamDir[index].directory;
    }else{
        LOG_ERROR("Monitor handle not found: %d", error);
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
        return;
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));

    if(error == VPLFS_MonitorCB_OK) {
        int bufferSize = 0;
        void* currEventBuf = eventBuffer;

        while(bufferSize < eventBufferSize) {
            VPLFS_MonitorEvent* monitorEvent = (VPLFS_MonitorEvent*) currEventBuf;
            std::string filePath;
            std::string relativePath;
            std::string relativePathSrc;
            VPLFS_stat_t statBuf;
            int rc;
            switch(monitorEvent->action) {
            case VPLFS_MonitorEvent_FILE_ADDED:
                relativePath = std::string(monitorEvent->filename);
                Util_appendToAbsPath(srcDir, relativePath, filePath);
                rc = VPLFS_Stat(filePath.c_str(), &statBuf);
                if(rc != VPL_OK) {
                    LOG_ERROR("VPLStat failed:%d, skipping add-event of missing file %s.",
                              rc, filePath.c_str());
                    break;
                }
                if(statBuf.type == VPLFS_TYPE_FILE) {
                    relDir = parent(relativePath, 1);
                    dirent = child(relativePath, 1);
                    if(isPhotoFile(dirent)) {
                        LOG_INFO("PicMonitor:Added:%d, %s, %s, %s",
                                 index, srcDir.c_str(), relDir.c_str(), dirent.c_str());
                        insertWaitQueue(index, srcDir, relDir, dirent);
                        srcTouchFile = touch_file_pic_src_cb(index);
                        destructiveTouch(srcTouchFile.c_str());
                        numEvents++;
                    } else {
                        LOG_WARN("PicMonitor:Added: Unsupported picstream file:%s, %s",
                                 srcDir.c_str(), dirent.c_str());
                    }
                }else if(statBuf.type == VPLFS_TYPE_DIR){
                    // If a directory is moved here.
                    LOG_ERROR("PicMonitor:Directory moved/created in Picstream (%s in %s)"
                              " Please don't do this (picture repeats may happen). "
                              "Performing full rescan.",
                              monitorEvent->filename, srcDir.c_str());
                    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
                    if(s_picstreamDir[index].b_isDirInit &&
                       s_picstreamDir[index].monitorHandle == handle)
                    {
                        s_picstreamDir[index].b_performFullSync = true;
                    }
                    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
                    numEvents++;
                }else{
                    LOG_ERROR("Unrecognized file type for %s:%d, Skipping",
                              filePath.c_str(),
                              statBuf.type);
                    break;
                }
                break;
            case VPLFS_MonitorEvent_FILE_REMOVED:
                // Not interesting to picstream
                break;
            case VPLFS_MonitorEvent_FILE_MODIFIED:
                relativePath = std::string(monitorEvent->filename);
                Util_appendToAbsPath(srcDir, relativePath, filePath);
                if(isFile(filePath)) {
                    relDir = parent(relativePath, 1);
                    dirent = child(relativePath, 1);
                    if(isPhotoFile(dirent)) {
                        LOG_INFO("PicMonitor:Modified:%d, %s, %s, %s",
                                 index, srcDir.c_str(), relDir.c_str(), dirent.c_str());
                        insertWaitQueue(index, srcDir, relDir, dirent);
                        srcTouchFile = touch_file_pic_src_cb(index);
                        destructiveTouch(srcTouchFile.c_str());
                        numEvents++;
                    } else {
                        LOG_WARN("PicMonitor:Modified: Unsupported picstream file:%s, %s",
                                 srcDir.c_str(), dirent.c_str());
                    }
                }
                break;
            case VPLFS_MonitorEvent_FILE_RENAMED:
                // Now interesting to picstream for Acer C11 Camera app special case
                relativePath = std::string(monitorEvent->moveTo);
                Util_appendToAbsPath(srcDir, relativePath, filePath);
                if(isFile(filePath)) {
                    relDir = parent(relativePath, 1);
                    dirent = child(relativePath, 1);
                    relativePathSrc = std::string(monitorEvent->filename);
                    direntSrc = child(relativePathSrc, 1);
                    if(isPhotoTempFile(direntSrc) && isPhotoFile(dirent)) {
                        LOG_INFO("PicMonitor:Renamed:%d, %s, %s, %s",
                                index, srcDir.c_str(), relDir.c_str(), dirent.c_str());
                        insertWaitQueue(index, srcDir, relDir, dirent);
                        srcTouchFile = touch_file_pic_src_cb(index);
                        destructiveTouch(srcTouchFile.c_str());
                        numEvents++;
                    } else {
                        LOG_WARN("PicMonitor:Renamed: Unsupported picstream file:%s, source:%s, destination:%s",
                                srcDir.c_str(), direntSrc.c_str(), dirent.c_str());
                    }
                }
                break;
            case VPLFS_MonitorEvent_NONE:
            default:
                LOG_ERROR("FileMonitor action not supported:%d, %s",
                          monitorEvent->action, monitorEvent->filename);
                break;
            }

            if(monitorEvent->nextEntryOffsetBytes == 0) {
                // Done traversing events
                break;
            }else{
                // Go to next event
                currEventBuf = (char*)currEventBuf + monitorEvent->nextEntryOffsetBytes;
            }

            bufferSize += monitorEvent->nextEntryOffsetBytes;
        }

    }else if(error == VPLFS_MonitorCB_OVERFLOW) {
        LOG_INFO("VPL File Monitor overflowed for %s. Picstream repeats may happen",
                 srcDir.c_str());
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
        if(s_picstreamDir[index].b_isDirInit &&
           s_picstreamDir[index].monitorHandle == handle)
        {
            s_picstreamDir[index].b_performFullSync = true;
        }
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
        numEvents++;
    }

    if(numEvents > 0) {
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
        s_condVarWorkToDo = true;
        VPLCond_Signal(VPLLazyInitCond_GetCond(&s_condVar));
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    }
}

int Picstream_AddMonitorDir(const char* srcDir,
                            u32 index,
                            bool rmPrevSrcTime)
{
    int rv = 0;
    int rc;
#if !defined(IOS) && !defined(VPL_PLAT_IS_WINRT)
    const static int NUM_EVENTS_BUFFER = 10;
#endif
    std::string srcTouchFile;

    LOG_INFO("Added PicstreamDir %s, Index:"FMTu32", rmPrevSrcTime:%d",
             srcDir, index, rmPrevSrcTime);
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    if(!s_bInit) {
        LOG_ERROR("Picstream not init.");
        rv = CCD_ERROR_NOT_INIT;
        goto fail_not_init;
    }

    // check max index/size
    if(index >= MAX_PICSTREAM_DIR) {
        LOG_ERROR("Index of "FMTu32" greater than max supported %d",
                  index, MAX_PICSTREAM_DIR);
        rv = CCD_ERROR_PARAMETER;
        goto fail_bad_index;
    }

    if(s_picstreamDir[index].b_isDirInit) {
        LOG_ERROR("Index %d already initialized to %s, remove before reinitializing",
                  index, s_picstreamDir[index].directory.c_str());
        rv = CCD_ERROR_ALREADY_INIT;
        goto fail_already_init;
    }

    if(rmPrevSrcTime) {
        std::string touchFile = touch_file_pic_src(index);
        rc = VPLFile_Delete(touchFile.c_str());
        if(rc != VPL_OK && rc != VPL_ERR_NOENT) {
            LOG_ERROR("VPLFile_Delete:%d, %s, Not critical, continuing.",
                      rc, touchFile.c_str());
        }
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    s_picstreamDir[index].b_isDirInit = true;
    s_picstreamDir[index].b_performFullSync = true;
    s_picstreamDir[index].directory = srcDir;
#if !defined(IOS) && !defined(VPL_PLAT_IS_WINRT)
    // monitor dir
    rc = VPLFS_MonitorDir(srcDir,
                          NUM_EVENTS_BUFFER,
                          picstream_monitorCb,
                          &s_picstreamDir[index].monitorHandle);
    if(rc != VPL_OK) {
        LOG_ERROR("VPLFS_MonitorDir:%d, %s.",
                  rc, srcDir);
        rv = rc;
        goto fail_monitor_dir;
    }
#endif
    s_condVarWorkToDo = true;
    VPLCond_Signal(VPLLazyInitCond_GetCond(&s_condVar));
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;  // Success!
#if !defined(IOS) && !defined(VPL_PLAT_IS_WINRT)
fail_monitor_dir:
    s_picstreamDir[index].clear();
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
#endif
 fail_already_init:
 fail_bad_index:
 fail_not_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

int Picstream_PerformFullSyncDir(u64 userId, u32 index)
{
    int rv = 0;
    LOG_INFO("Trigger camera roll upload with index:%d", index);
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    if(!s_bInit) {
        LOG_ERROR("Picstream not init.");
        rv = CCD_ERROR_NOT_INIT;
        goto fail_not_init;
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    // check max index/size
    if(index >= MAX_PICSTREAM_DIR) {
        LOG_ERROR("Index of %d greater than max supported %d.",
                  index, MAX_PICSTREAM_DIR);
        rv = CCD_ERROR_PARAMETER;
        goto bad_index;
    }

    if(!s_picstreamDir[index].b_isDirInit) {
        LOG_ERROR("Index %d not initialized.", index);
        rv = CCD_ERROR_PARAMETER;
        goto already_not_init;
    }

    s_picstreamDir[index].userId = userId;
    s_picstreamDir[index].b_performFullSync = true;
    s_condVarWorkToDo = true;
    VPLCond_Signal(VPLLazyInitCond_GetCond(&s_condVar));

already_not_init:
bad_index:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
fail_not_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

int Picstream_RemoveMonitorDir(u32 index)
{
    int rv = 0;
    int rc;
    std::string touchFile;
    LOG_INFO("Removed %d", index);
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    if(!s_bInit) {
        LOG_ERROR("Picstream not init.");
        rv = CCD_ERROR_NOT_INIT;
        goto fail_not_init;
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    // check max index/size
    if(index >= MAX_PICSTREAM_DIR) {
        LOG_ERROR("Index of %d greater than max supported %d, nothing to remove.",
                  index, MAX_PICSTREAM_DIR);
        goto bad_index;
    }

    if(!s_picstreamDir[index].b_isDirInit) {
        LOG_ERROR("Index %d not initialized, nothing to remove", index);
        goto already_not_init;
    }
#if !defined(IOS) && !defined(VPL_PLAT_IS_WINRT)
    rc = VPLFS_MonitorDirStop(s_picstreamDir[index].monitorHandle);
    if(rc != 0) {
        LOG_ERROR("Failed VPLFS_MonitorDirStop:%d, index:%d, dir:%s.  Continuing.",
                  rc, index, s_picstreamDir[index].directory.c_str());
    }
#endif
    touchFile = touch_file_pic_src(index);
    rc = VPLFile_Delete(touchFile.c_str());
    if(rc != VPL_OK && rc != VPL_ERR_NOENT) {
        LOG_ERROR("VPLFile_Delete:%d, %s, Not critical, continuing.",
                  rc, touchFile.c_str());
    }

    s_picstreamDir[index].clear();

 already_not_init:
 bad_index:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
 fail_not_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

int Picstream_GetEnable(bool& enabled)
{
    int rv = 0;
    int rc;
    VPLFS_stat_t statBuf;
    std::string touchfile;
    enabled = false;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));

    if(!s_bInit) {
        LOG_INFO("Picstream not init, so pic stream must not be enabled.");
        rv = CCD_ERROR_NOT_INIT;
        goto fail_not_init;
    }
    touchfile = touch_file_pic_enable(s_picstream_module_dir);

    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));

    rc = VPLFS_Stat(touchfile.c_str(), &statBuf);
    if(rc != VPL_OK) {
        enabled = false;
        LOG_INFO("Picstream disabled");
    }else{
        enabled = true;
        LOG_INFO("Picstream enable time:"FMTu64, VPLTime_FromSec(statBuf.mtime));
    }
    return rv;

 fail_not_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

static int Picstream_SendToVcsCameraRoll(u64 userId, const std::string& file)
{
    return addAsNewSyncUpJob(userId, file, true);
}

int Picstream_SendToCameraRoll(u64 userId, const std::string& file)
{
    if(!isPhotoFile(file)) {
        LOG_ERROR("Unsupported PicStream File:%s", file.c_str());
        return CCD_ERROR_PARAMETER;
    }

    return Picstream_SendToVcsCameraRoll(userId, file);
}

