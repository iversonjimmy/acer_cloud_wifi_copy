#include "gvm_errors.h"
#include "gvm_file_utils.h"
#include "vpl_fs.h"
#include "vpl_thread.h"
#include "vpl_lazy_init.h"
#include "vplex_file.h"

#include <deque>
#include <map>
#include <string>

#include <log.h>

#include "common_utils.hpp"

static volatile bool s_isRunning = false;
#define PICSTREAM_STACK_SIZE              (64 * 1024)
static VPLThread_t s_thread = VPLTHREAD_INVALID;
static VPLLazyInitMutex_t s_interfaceMutex = VPLLAZYINITMUTEX_INIT;
static VPLLazyInitMutex_t s_runThreadMutex = VPLLAZYINITMUTEX_INIT;
static VPLLazyInitCond_t s_condVar = VPLLAZYINITCOND_INIT;
static bool s_condVarWorkToDo = false;

#define COPY_FILE_BUF_SIZE  (16*1024)
static u8* s_copyFileBuf;

// Optimization to not stat PICSTREAM_ENABLE_TOUCH_FILE extremely often
static volatile bool s_bEnabled = false;
static const char* PICSTREAM_ENABLE_TOUCH_FILE = "pic_enable.touch";
static const char* PICSTREAM_TEMP_DIR = "temp"; // within the s_picstream_module_dir
static const char* PICSTREAM_DSET_DIR = "dset"; // within the s_picstream_module_dir


static bool s_bInit = false;
static VPLTime_t s_waitQueueTime;
static std::string s_picstream_module_dir;

struct PicstreamDir
{
    bool b_isDirInit;
    bool b_performFullSync;
    std::string directory;
    VPLFS_MonitorHandle monitorHandle;
    void clear();
    PicstreamDir() { clear(); }
};

void PicstreamDir::clear()
{
    b_isDirInit = false;
    b_performFullSync = false;
    directory.clear();
    monitorHandle = VPLFILE_INVALID_HANDLE;
}

static const u32 MAX_PICSTREAM_DIR = 2;
static PicstreamDir s_picstreamDir[MAX_PICSTREAM_DIR];

static const std::string touch_file_pic_enable()
{
    std::string filePath;
    filePath = s_picstream_module_dir + std::string("/") +
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

static const std::string get_temp_dir()
{
    std::string path;
    path = s_picstream_module_dir + std::string("/") +
           std::string(PICSTREAM_TEMP_DIR);
    return path;
}

static const std::string Picstream_GetDsetDir(const std::string& internalPicDir)
{
    std::string path;
    path = internalPicDir + std::string("/") + std::string(PICSTREAM_DSET_DIR);
    return path;
}

static void appendToAbsPath(const std::string& path,
                            const std::string& name,
                            std::string& absPath_out)
{
    std::string tempName;
    std::string tempPath;
    trimSlashes(name, tempName);
    rmTrailingSlashes(path, tempPath);
    if(tempName == "") {
        absPath_out = tempPath;
    }else if(tempPath == ""){
        absPath_out = tempName;
    }else {
        absPath_out = tempPath+"/"+tempName;
    }
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
    appendToAbsPath(srcDir, relDir, filePath);
    appendToAbsPath(filePath, dirent, filePath);
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

static int copyFile(const std::string& localPath,
                    const std::string& dirent,
                    const std::string& tmpDir,
                    u64 size)
{
    VPLFile_handle_t fHDst = VPLFILE_INVALID_HANDLE;
    VPLFile_handle_t fHSrc = VPLFILE_INVALID_HANDLE;
    const int flagDst = VPLFILE_OPENFLAG_CREATE |
                        VPLFILE_OPENFLAG_WRITEONLY |
                        VPLFILE_OPENFLAG_TRUNCATE;

    int rv = 0;
    std::string srcFilePath = localPath + "/" + dirent;
    std::string dstFilePath = tmpDir + "/" + dirent;

    // Make sure temp directory exists.
    rv = Util_CreatePath(tmpDir.c_str(), VPL_TRUE);
    if(rv != 0) {
        LOG_ERROR("Unable to create temp path:%s", tmpDir.c_str());
        return rv;
    }

    fHDst = VPLFile_Open(dstFilePath.c_str(), flagDst, 0666);
    if (!VPLFile_IsValidHandle(fHDst)) {
        LOG_ERROR("Fail to create or open tmp file %s",
                  dstFilePath.c_str());
        rv = -1;
        goto exit;
    }

    fHSrc = VPLFile_Open(srcFilePath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(fHSrc)) {
        LOG_ERROR("Fail to open source %s", srcFilePath.c_str());
        rv = -1;
        goto exit;
    }

    {  // Perform the copy in chunks
        for (ssize_t bytesTransfered = 0; bytesTransfered < size;) {
            ssize_t bytesRead = VPLFile_Read(fHSrc,
                                             s_copyFileBuf,
                                             COPY_FILE_BUF_SIZE);
            if (bytesRead > 0) {
                ssize_t wrCnt = VPLFile_Write(fHDst, s_copyFileBuf, bytesRead);
                if (wrCnt != bytesRead) {
                    LOG_ERROR("Fail to copy temp file %s, %d/%d",
                              dstFilePath.c_str(),
                              (int)bytesRead, (int)wrCnt);
                    rv = -1;
                    goto exit;
                }
                bytesTransfered += bytesRead;
            } else {
                break;
            }
        }
    }
 exit:
    if (VPLFile_IsValidHandle(fHDst)) {
        VPLFile_Close(fHDst);
    }
    if (VPLFile_IsValidHandle(fHSrc)) {
        VPLFile_Close(fHSrc);
    }
    return rv;
}

static int moveToPicstreamDset(const std::string& localPath,
                               const std::string& dirent,
                               const std::string& tmpDir,
                               const std::string& dstDir,
                               u64 size)
{
    int rc;
    rc = copyFile(localPath, dirent, tmpDir, size);
    if(rc != 0) {
        LOG_ERROR("copyFile:%d, %s, %s, %s, size:"FMTu64,
                  rc, localPath.c_str(), dirent.c_str(), tmpDir.c_str(), size);
        return rc;
    }

    // Make sure dstDir exists
    rc = Util_CreatePath(dstDir.c_str(), VPL_TRUE);
    if(rc != 0) {
        LOG_ERROR("Unable to create dset path:%s", dstDir.c_str());
        return rc;
    }
    std::string srcFile;
    appendToAbsPath(tmpDir, dirent, srcFile);
    std::string dstFile;
    appendToAbsPath(dstDir, dirent, dstFile);
    rc = VPLFile_Rename(srcFile.c_str(), dstFile.c_str());
    if(rc != 0) {
        LOG_ERROR("Moving file:%d, %s->%s", rc, srcFile.c_str(), dstFile.c_str());
        return rc;
    }

    LOG_INFO("Successfully moved file %s,%s->%s",
             localPath.c_str(), dirent.c_str(), dstFile.c_str());
    // success
    return 0;
}

static int performFullSync(VPLTime_t enableTime,
                           VPLTime_t srcTime,
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
        appendToAbsPath(srcDir, currDir, localPath);
        // direntsInDir is an optimization so that scannedDirectories need not contain all files
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
            appendToAbsPath(localPath, dirent, localFile);
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

            if(srcTime > modified) {
                // srcTime indicates this picture already processed
                continue;
            }

            VPLTime_t currTime = VPLTime_GetTime();
            if(VPLTime_DiffClamp(currTime, modified) > s_waitQueueTime) {
                rc = moveToPicstreamDset(localPath,
                                         dirent,
                                         tmpDir,
                                         dstDir,
                                         statBuf.size);
                if(rc != 0) {
                    LOG_ERROR("moveToPicstreamDset:%d, %s, %s, %s, %s,"FMTu64,
                              rc, localPath.c_str(), dirent.c_str(),
                              tmpDir.c_str(), dstDir.c_str(), (u64)statBuf.size);
                    rv = rc;
                }
            } else {
                insertWaitQueue(index, srcDir, currDir, dirent);
            }
        }

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
        std::string enableTouchFile = touch_file_pic_enable();
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
            std::string srcTouchFile;
            PicstreamDir picstreamDir;
            VPLFS_stat_t srcStat;
            std::string tempDir;
            std::string dsetDir;

            VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));

            picstreamDir = s_picstreamDir[index];
            srcTouchFile = touch_file_pic_src(index);
            rc = VPLFS_Stat(srcTouchFile.c_str(), &srcStat);
            if(rc == VPL_OK) {
                srcTime = VPLTIME_FROM_SEC(srcStat.mtime);
            }
            tempDir = get_temp_dir();
            dsetDir = Picstream_GetDsetDir(s_picstream_module_dir);

            VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));

            if(picstreamDir.b_isDirInit && picstreamDir.b_performFullSync) {
                // could take a long time copying over files
                rc = performFullSync(enableTime,
                                     srcTime,
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
                    destructiveTouch(srcTouchFile.c_str());
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
                appendToAbsPath(s_picstreamDir[index].directory,
                                (*waitIter).second.relDir,
                                localPath);
                std::string absPath;
                appendToAbsPath(localPath, dirent, absPath);
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
                rc = moveToPicstreamDset(localPath,
                                         dirent,
                                         tmpDir,
                                         dstDir,
                                         statBuf.size);
                if(rc != 0) {
                    LOG_ERROR("FAILED moveToPicstreamDset. Skipping:%d, %s, %s, %s, %s,"FMTu64,
                              rc, localPath.c_str(), dirent.c_str(),
                              tmpDir.c_str(), dstDir.c_str(), (u64)statBuf.size);
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

        // Touch the src pointer once all work is done.
        if(s_waitQ.size() == 0 && waitQWorkDone) {
            for(int stateIndex = 0; stateIndex < MAX_PICSTREAM_DIR; ++stateIndex)
            {
                if(s_picstreamDir[stateIndex].b_isDirInit) {
                    std::string srcTouchFile = touch_file_pic_src(stateIndex);
                    int rc = destructiveTouch(srcTouchFile.c_str());
                    if(rc != 0) {
                        LOG_ERROR("touch failed:%d, %s", rc, srcTouchFile.c_str());
                    }
                }
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
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    }
    LOG_ALWAYS("RunThread Stopping");
    return NULL;
}

static int Picstream_Init(const char* internalPicDir, VPLTime_t waitQTime)
{
    int rv = 0;
    int rc;
    VPLThread_attr_t thread_attr;
    u32 dirIndex;
    LOG_INFO("Internal PicstreamDir:%s, waitQTime:"FMTu64,
             internalPicDir, waitQTime);

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));

    if(s_bInit) {
        LOG_ERROR("Already initialized");
        rv = CCD_ERROR_ALREADY_INIT;
        goto fail_already_init;
    }

    for(dirIndex = 0; dirIndex < MAX_PICSTREAM_DIR; ++dirIndex)
    {
        s_picstreamDir[dirIndex].clear();
    }

    rc = Util_CreatePath(internalPicDir, true);
    if(rc != VPL_OK) {
        LOG_ERROR("Util_CreatePath:%d, %s", rc, internalPicDir);
        rv = rc;
        goto fail_create_path;
    }

    s_copyFileBuf = (u8*) malloc(COPY_FILE_BUF_SIZE);
    if(!s_copyFileBuf) {
        LOG_ERROR("Out of memory: %d", COPY_FILE_BUF_SIZE);
        rv = CCDI_ERROR_OUT_OF_MEM;
        goto fail_out_mem;
    }

    rc = VPLThread_AttrInit(&thread_attr);
    if(rv != VPL_OK) {
        LOG_ERROR("Failed to initialize thread attributes:%d", rv);
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
    s_picstream_module_dir.assign(internalPicDir);
    s_bInit = true;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;

 fail_thread_create:
    s_isRunning = false;
    VPLThread_AttrDestroy(&thread_attr);
 fail_attr_init:
    free(s_copyFileBuf);
    s_copyFileBuf = NULL;
 fail_out_mem:
 fail_create_path:
 fail_already_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

static int Picstream_Destroy()
{
    int rv = 0;
    LOG_INFO("Called");
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    if(!s_bInit) {
        LOG_ERROR("Not initialized");
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

    free(s_copyFileBuf);
    s_copyFileBuf = NULL;
 fail_not_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

static int Picstream_Enable(bool enable)
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

    filePath = touch_file_pic_enable();

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
        LOG_ERROR("Monitor handle not found:%d, %d", handle, error);
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
            switch(monitorEvent->action) {
            case VPLFS_MonitorEvent_FILE_ADDED:
                relativePath = std::string(monitorEvent->filename);
                appendToAbsPath(srcDir, relativePath, filePath);
                if(isFile(filePath)) {
                    relDir = parent(relativePath, 1);
                    dirent = child(relativePath, 1);
                    LOG_INFO("PicMonitor:Added:%d, %s, %s, %s",
                             index, srcDir.c_str(), relDir.c_str(), dirent.c_str());
                    insertWaitQueue(index, srcDir, relDir, dirent);
                    numEvents++;
                }else{
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
                }
                break;
            case VPLFS_MonitorEvent_FILE_REMOVED:
                // Not interesting to picstream
                break;
            case VPLFS_MonitorEvent_FILE_MODIFIED:
                relativePath = std::string(monitorEvent->filename);
                appendToAbsPath(srcDir, relativePath, filePath);
                if(isFile(filePath)) {
                    relDir = parent(relativePath, 1);
                    dirent = child(relativePath, 1);
                    LOG_INFO("PicMonitor:Modified:%d, %s, %s, %s",
                               index, srcDir.c_str(), relDir.c_str(), dirent.c_str());
                    insertWaitQueue(index, srcDir, relDir, dirent);
                    numEvents++;
                }
                break;
            case VPLFS_MonitorEvent_FILE_RENAMED:
                // Not interesting to picstream
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

static int Picstream_AddMonitorDir(const char* srcDir,
                                   int index,
                                   bool rmPrevSrcTime)
{
    int rv = 0;
    int rc;
    const static int NUM_EVENTS_BUFFER = 10;
    std::string srcTouchFile;

    LOG_INFO("Added PicstreamDir %s, Index:%d, rmPrevSrcTime:%d",
             srcDir, index, rmPrevSrcTime);
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    if(!s_bInit) {
        LOG_ERROR("Picstream not init.");
        rv = CCD_ERROR_NOT_INIT;
        goto fail_not_init;
    }

    // check max index/size
    if(index >= MAX_PICSTREAM_DIR) {
        LOG_ERROR("Index of %d greater than max supported %d",
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
    // monitor dir
    rc = VPLFS_MonitorDir(srcDir,
                          NUM_EVENTS_BUFFER,
                          picstream_monitorCb,
                          &s_picstreamDir[index].monitorHandle);
    if(rc != VPL_OK) {
        LOG_ERROR("VPLFS_MonitorDir:%d, %s. If not initialized, is VSCore initialized?",
                  rc, srcDir);
        rv = rc;
        goto fail_monitor_dir;
    }
    s_condVarWorkToDo = true;
    VPLCond_Signal(VPLLazyInitCond_GetCond(&s_condVar));
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;  // Success!

 fail_monitor_dir:
    s_picstreamDir[index].clear();
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_runThreadMutex));
 fail_already_init:
 fail_bad_index:
 fail_not_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    return rv;
}

static int Picstream_RemoveMonitorDir(u32 index)
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

    rc = VPLFS_MonitorDirStop(s_picstreamDir[index].monitorHandle);
    if(rc != 0) {
        LOG_ERROR("Failed VPLFS_MonitorDirStop:%d, index:%d, dir:%s.  Continuing.",
                  rc, index, s_picstreamDir[index].directory.c_str());
    }

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

static int Picstream_GetEnable(bool& enabled)
{
    int rv = 0;
    int rc;
    VPLFS_stat_t statBuf;
    std::string touchfile;
    enabled = false;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&s_interfaceMutex));
    if(!s_bInit) {
        LOG_ERROR("Picstream not init.");
        rv = CCD_ERROR_NOT_INIT;
        goto fail_not_init;
    }

    touchfile = touch_file_pic_enable();
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

static const char* PICSTREAM_DIR = "./picstreamDir";
int main(int argc, char ** argv)
{
    LOGInit(NULL, NULL);
    LOG_ALWAYS("hello world");
    int rc = 0;
    bool enabled;
    rc = VPLFS_MonitorInit();
    if(rc != 0) {
        LOG_ERROR("VPLFS_MonitorInit:%d", rc);
    }

    Picstream_Init(PICSTREAM_DIR, VPLTime_FromSec(10));
    rc = Picstream_GetEnable(enabled);
    LOG_INFO("Enabled:%d", enabled);
    rc = Picstream_Enable(true);
    rc = Picstream_GetEnable(enabled);
    LOG_INFO("Enabled:%d", enabled);
    rc = Picstream_AddMonitorDir("./mypics1", 0, true);
    rc = Picstream_AddMonitorDir("./mypics2", 1, true);
    VPLThread_Sleep(VPLTime_FromSec(10));
    rc = Picstream_Enable(true);
    rc = Picstream_GetEnable(enabled);
    LOG_INFO("Enabled:%d", enabled);
    VPLThread_Sleep(VPLTime_FromSec(100));
    rc = Picstream_Enable(false);
    rc = Picstream_GetEnable(enabled);
    LOG_INFO("Enabled:%d", enabled);
    VPLThread_Sleep(VPLTime_FromSec(7));
    rc = Picstream_RemoveMonitorDir(0);
    rc = Picstream_RemoveMonitorDir(1);
    Picstream_Destroy();

    rc = VPLFS_MonitorDestroy();
    if(rc != 0) {
        LOG_ERROR("VPLFS_MonitorDestroy:%d", rc);
    }
}
