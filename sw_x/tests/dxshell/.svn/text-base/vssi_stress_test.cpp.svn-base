#include "vssi_stress_test.hpp"
#include "common_utils.hpp"
#include "dx_common.h"
//#include "vssi.h"
//#include "vssi_error.h"

#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vpl_conv.h"
#include "vplex_named_socket.h"
#include "vpl_thread.h"
#include "vpl_th.h"
//#include "vssi_common.hpp"
#include "vplex_math.h"

#include "vssts.hpp"
#include "vssts_error.hpp"

#include <ccdi.hpp>
#include "ccdi_client.hpp"
#include "ccd_utils.hpp"
#include "csl.h"
#include "cslsha.h"

#include <log.h>
#include <cerrno>

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <set>
#include <queue>
#include <stack>

/// These are Linux specific non-abstracted headers.
#if defined(WIN32)
#include <getopt_win.h>
#else
#include <getopt.h>
#endif

#define TEST_CASE_FILE_IO "FileIo"
#define TEST_CASE_DIR_CREATE_DEST "DirCreateDestruct"
#define TEST_FILE_MODE_ATTR "FileModeAttr"
#define TEST_FILE_LOCK "FileLock"
#define TEST_MANY_FILES "ManyFiles"

#define CHECK_AND_PRINT_EXPECTED_TO_FAIL(testsuite, subtestname, result, bug) { \
        if (result < 0) {                                       \
            LOG_ERROR("%s fail rv (%d)", subtestname, result);     \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=VSSIStress_%s_%s(BUG %s)", \
                       (rv == 0)? "PASS":"EXPECTED_TO_FAIL", testsuite, subtestname, bug); \
        } else {                                                \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=VSSIStress_%s_%s(BUG %s)", \
                       (rv == 0)? "PASS":"EXPECTED_TO_FAIL", testsuite, subtestname, bug); \
        }                                                       \
}
#define CHECK_AND_PRINT_RESULT(testsuite, subtestname, result) {              \
        if (result < 0) {                                       \
            LOG_ERROR("%s fail rv (%d)", subtestname, result);     \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=VSSIStress_%s_%s", (result == 0)? "PASS":"FAIL", testsuite, subtestname); \
            goto exit;                                          \
        } else {                                                \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=VSSIStress_%s_%s", (result == 0)? "PASS":"FAIL", testsuite, subtestname); \
        }                                                       \
}

static char* worker_str = (char *)"Worker";

enum VSSI_STRESS_TEST_CASE
{
    VSSI_STRESS_TEST_NONE,
    VSSI_STRESS_TEST_CASE_FILE_IO,
    VSSI_STRESS_TEST_CASE_DIR_CREATE_DEST,
    VSSI_STRESS_TEST_FILE_MODE_ATTR,
    VSSI_STRESS_TEST_FILE_LOCK,
    VSSI_STRESS_TEST_MANY_FILES,
};

struct vssi_stress_worker_args
{
    char dir_based[256];
    VSSI_STRESS_TEST_CASE case_type;
    bool keep_running_worker;
    int iterations;

    int file_io_block_size;
    int file_io_max_blocks;

    int dir_create_des_levels;
    int dir_create_des_num_per_level;
    
    int file_mod_attr_number_of_files;

    char file_lock_file_name[256];
    int file_lock_sleep_time;
    int num_of_test;
};

typedef int (*vssi_stress_test_subcmd_fn)(int argc, const char *argv[]);
static std::vector<int> worker_list;

// routines
static std::string user_login_id;
static std::string password;
static std::string domain = "pc.igware.net";

static std::vector<std::pair<VPLThread_t *, int> > test_thrs_running;
static std::vector<std::string> worker_args;
static VSSI_Object dataset_handle;

static VPLMutex_t thread_state_mutex;
static VPLCond_t thread_state_cond;
static int threads_running = 0;

static void thread_start()
{
    VPLMutex_Lock(&thread_state_mutex);
    threads_running++;

    // Wait for "Go" signal from main thread
    VPLCond_TimedWait(&thread_state_cond, &thread_state_mutex, VPLTime_FromSec(600));

    VPLMutex_Unlock(&thread_state_mutex);
}

static void thread_end()
{
    VPLMutex_Lock(&thread_state_mutex);
    threads_running--;

    // Tell main thread we're done
    VPLCond_Signal(&thread_state_cond);

    VPLMutex_Unlock(&thread_state_mutex);
}

static u32 __vsf_get_tid()
{
    return VAL_VPLThread_t(VPLThread_Self());
}

// test case section
typedef struct  {
    VPLSem_t sem;
    int rv;
} vssi_stress_test_context_t;

static void vssi_stress_test_callback(void* ctx, VSSI_Result rv)
{
    vssi_stress_test_context_t* context = (vssi_stress_test_context_t*)ctx;

    context->rv = rv;
    VPLSem_Post(&(context->sem));
}

static void
dumpData(const char *msg, u8 *array, int size)
{
    int i;

	printf( "%s", msg );
	for( i = 0; i < size; i++ )
	{
		if ( i % 16 == 0 )
			printf( "\n" );
		printf( "%02X ", array[i] );
	}
	printf( "\n" );
}

static int __vsf_mkdir(std::string& dirPath)
{
    int rv = VSSI_SUCCESS;
    u32 attrs = 0;
    vssi_stress_test_context_t test_context;

    do {
        VPL_SET_UNINITIALIZED(&(test_context.sem));
        if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
            LOG_ERROR("Failed to create semaphore.");
            rv = VSSI_INVALID;
            break;
        } 
        VSSI_MkDir2(dataset_handle, dirPath.c_str(), attrs,
                    &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;

        VPLSem_Destroy(&(test_context.sem));

    } while(0);
    LOG_ALWAYS("Mkdir %s rv %d", dirPath.c_str(), rv);

    return rv;
}

static int __vsf_mkdirp(std::string& dirPath)
{
    int rv = VSSI_SUCCESS;
    size_t slash;

    LOG_ALWAYS("MkdirP %s", dirPath.c_str());

    slash = dirPath.find_first_of('/');
    while(slash != std::string::npos) {
        std::string beginPath = dirPath.substr(0, slash);
        rv = __vsf_mkdir(beginPath);
        if(rv != VSSI_SUCCESS && rv != VSSI_ISDIR) break;
        slash = dirPath.find_first_of('/', slash + 1);
    }
    rv = __vsf_mkdir(dirPath);
    if(rv == VSSI_ISDIR) rv = VSSI_SUCCESS;

    return rv;
}

static int __vsf_remove_file(std::string& filePath)
{
    int rv = VSSI_SUCCESS;
    vssi_stress_test_context_t test_context;

    LOG_ALWAYS("Remove %s", filePath.c_str());

    do {
        VPL_SET_UNINITIALIZED(&(test_context.sem));
        if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
            LOG_ERROR("Failed to create semaphore.");
            rv = VSSI_INVALID;
            break;
        } 
        VSSI_Remove(dataset_handle, filePath.c_str(),
                    &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;

        VPLSem_Destroy(&(test_context.sem));

    } while(0);


    return rv;
}

#define MIN_VSF_BLKSIZE 512

//
// Create a file and fill it with unique data that can be verified later
//
static int __vsf_write_file(std::string& filePath, int numBlocks, int blkSize)
{
    int rv = 0;
    u8 *wbuf = NULL;
    CSL_ShaContext hashCtx;
    u8 hashVal[32];   // Really SHA1, but be on the safe side
    char *name = NULL;
    int fileNameLen = filePath.size() + 1;
    int useNameLen;
    u32 flags;
    u32 attrs = 0;
    u32 wrLen;
    CSLOSAesKey key;
    CSLOSAesIv iv;
    CSL_AesContext aesCtx;
    VSSI_File fileHandle = NULL;
    vssi_stress_test_context_t test_context;
    int blkNo;

    LOG_ALWAYS("filePath %s, fileNameLen %d, numBlks %d, blkSz %d",
        filePath.c_str(), fileNameLen, numBlocks, blkSize);

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to create semaphore.");
        rv = VSSI_INVALID;
        goto exit;
    } 

    do {
        if((name = (char*)malloc(fileNameLen)) == NULL) {
            rv = VSSI_NOMEM;
            break;
        }
        memcpy(name, filePath.c_str(), fileNameLen);
        CSL_ResetSha(&hashCtx);
        CSL_InputSha(&hashCtx, name, fileNameLen);
        CSL_ResultSha(&hashCtx, hashVal);
        free(name);
        name = NULL;

        if(blkSize < MIN_VSF_BLKSIZE) {
            rv = VSSI_INVALID;
            break;
        }
        if((wbuf = (u8*)malloc(blkSize)) == NULL) {
            rv = VSSI_NOMEM;
            break;
        }

        // Make sure write buffer does not overflow
        useNameLen = MIN(fileNameLen, blkSize - (int)sizeof(blkNo));

        // Open and create the file
        flags = VSSI_FILE_OPEN_CREATE | VSSI_FILE_OPEN_READ | VSSI_FILE_OPEN_WRITE;
        VSSI_OpenFile(dataset_handle, filePath.c_str(), 
                      flags, attrs, &fileHandle,
                      &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
            LOG_ERROR("OpenFile %s failed %d", filePath.c_str(), rv);
            break;
        }

        // Use first 16 bytes of hash as AES key
        // For each block, the IV is the block number
        memcpy(key, hashVal, sizeof(key));

        for(blkNo = 0; blkNo < numBlocks; blkNo++) {
            memset(wbuf, 0, blkSize);
            memcpy(wbuf, &blkNo, sizeof(blkNo));
            memcpy(&wbuf[sizeof(blkNo)], filePath.c_str(), useNameLen);
            memset(iv, 0, sizeof(iv));
            memcpy(iv, &blkNo, sizeof(blkNo));

            if((rv = CSL_ResetEncryptAes(&aesCtx, key, iv)) != CSL_OK) {
                break;
            }
            if((rv = CSL_EncryptAes(&aesCtx, wbuf, blkSize, wbuf)) != CSL_OK) {
                break;
            }
            wrLen = blkSize;
            VSSI_WriteFile(fileHandle, blkNo * blkSize, &wrLen, (const char *)wbuf,
                           &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("WriteFile %s failed %d", filePath.c_str(), rv);
                break;
            }
            if(wrLen != blkSize) {
                LOG_ERROR("WriteFile %s failed: wrote %d, exp %d", 
                    filePath.c_str(), wrLen, blkSize);
                rv = VSSI_INVALID;
                break;
            }
        }

    } while(0);

    if(fileHandle != NULL) {
        VSSI_CloseFile(fileHandle,
                       &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        if(test_context.rv != VSSI_SUCCESS) {
            LOG_ERROR("CloseFile %s failed %d", filePath.c_str(), test_context.rv);
        }
    }
    VPLSem_Destroy(&(test_context.sem));

exit:
    if(name != NULL) free(name);
    if(wbuf != NULL) free(wbuf);

    return rv;
}

//
// Verify an entire file created by __vsf_write_file sequentially
//
static int __vsf_verify_file(std::string& filePath, int numBlocks, int blkSize)
{
    int rv = 0;
    u8 *rbuf = NULL;
    CSL_ShaContext hashCtx;
    u8 hashVal[32];   // Really SHA1, but be on the safe side
    char *name = NULL;
    int fileNameLen = filePath.size() + 1;
    int useNameLen;
    int offset;
    u32 flags;
    u32 attrs = 0;
    u32 rdLen;
    CSLOSAesKey key;
    CSLOSAesIv iv;
    CSL_AesContext aesCtx;
    VSSI_File fileHandle = NULL;
    vssi_stress_test_context_t test_context;
    int blkNo;

    LOG_ALWAYS("filePath %s, fileNameLen %d, numBlks %d, blkSz %d",
        filePath.c_str(), fileNameLen, numBlocks, blkSize);

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        LOG_ERROR("Failed to create semaphore.");
        rv = VSSI_INVALID;
        goto exit;
    } 

    do {
        if((name = (char*)malloc(fileNameLen)) == NULL) {
            rv = VSSI_NOMEM;
            break;
        }
        memcpy(name, filePath.c_str(), fileNameLen);
        CSL_ResetSha(&hashCtx);
        CSL_InputSha(&hashCtx, name, fileNameLen);
        CSL_ResultSha(&hashCtx, hashVal);
        free(name);
        name = NULL;

        if(blkSize < MIN_VSF_BLKSIZE) {
            rv = VSSI_INVALID;
            break;
        }
        if((rbuf = (u8*)malloc(blkSize)) == NULL) {
            rv = VSSI_NOMEM;
            break;
        }

        // Truncate file name len if necessary
        useNameLen = MIN(fileNameLen, blkSize - (int)sizeof(blkNo));

        // Open the file
        flags = VSSI_FILE_OPEN_READ;
        VSSI_OpenFile(dataset_handle, filePath.c_str(), 
                      flags, attrs, &fileHandle,
                      &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
            LOG_ERROR("OpenFile %s failed %d", filePath.c_str(), rv);
            break;
        }

        // Use first 16 bytes of hash as AES key
        // For each block, the IV is the block number
        memcpy(key, hashVal, sizeof(key));

        for(blkNo = 0; blkNo < numBlocks; blkNo++) {
            memset(rbuf, 0, blkSize);
            memset(iv, 0, sizeof(iv));
            memcpy(iv, &blkNo, sizeof(blkNo));

            rdLen = blkSize;
            VSSI_ReadFile(fileHandle, blkNo * blkSize, &rdLen, (char*)rbuf,
                          &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("ReadFile %s failed %d", filePath.c_str(), rv);
                break;
            }
            if(rdLen != blkSize) {
                LOG_ERROR("ReadFile %s failed: read %d, exp %d", 
                    filePath.c_str(), rdLen, blkSize);
                rv = VSSI_INVALID;
                break;
            }

            if((rv = CSL_ResetDecryptAes(&aesCtx, key, iv)) != CSL_OK) {
                break;
            }
            if((rv = CSL_DecryptAes(&aesCtx, rbuf, blkSize, rbuf)) != CSL_OK) {
                break;
            }

            //
            // Check contents of the block
            //
            if(memcmp(rbuf, &blkNo, sizeof(blkNo)) != 0) {
                LOG_ERROR("Data miscompare on file %s at blkNo %d", filePath.c_str(), blkNo);
                dumpData("Read Data", rbuf, sizeof(blkNo));
                dumpData("Expected Data", (u8*)&blkNo, sizeof(blkNo));
                rv = VSSI_INVALID;
                break;
            }
            if(memcmp(&rbuf[sizeof(blkNo)], filePath.c_str(), useNameLen) != 0) {
                LOG_ERROR("Data miscompare on file %s at blkNo %d", filePath.c_str(), blkNo);
                dumpData("Read Data", &rbuf[sizeof(blkNo)], useNameLen);
                dumpData("Expected Data", (u8*)filePath.c_str(), useNameLen);
                rv = VSSI_INVALID;
                break;
            }
            offset = useNameLen + sizeof(blkNo);
            while(offset & (sizeof(blkNo) - 1)) {
                if(rbuf[offset] != 0) {
                    LOG_ERROR("Data miscompare on file %s at blkNo %d offset %d: %02x",
                        filePath.c_str(), blkNo, offset, rbuf[offset]);
                    rv = VSSI_INVALID;
                    break;
                }
                offset++;
            }
            while(offset < blkSize) {
                if(*((u32 *)&rbuf[offset]) != 0) {
                    LOG_ERROR("Data miscompare on file %s at blkNo %d offset %d: %08x",
                        filePath.c_str(), blkNo, offset, *((u32 *)&rbuf[offset]));
                    rv = VSSI_INVALID;
                    break;
                }
                offset += sizeof(u32);
            }
            //LOG_ALWAYS("Data correct for %s at blkNo %d", filePath.c_str(), blkNo);
        }

    } while(0);

    if(fileHandle != NULL) {
        VSSI_CloseFile(fileHandle,
                       &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        if(test_context.rv != VSSI_SUCCESS) {
            LOG_ERROR("CloseFile %s failed %d", filePath.c_str(), test_context.rv);
        }
    }
    VPLSem_Destroy(&(test_context.sem));

exit:
    if(name != NULL) free(name);
    if(rbuf != NULL) free(rbuf);

    return rv;
}

//
// Verify some randomly selected blocks from a file created by __vsf_write_file
//
static int __vsf_verify_file_blocks(std::string& filePath, int numBlocks, int blkSize)
{
    int rv = 0;
    u8 *rbuf = NULL;
    CSL_ShaContext hashCtx;
    u8 hashVal[32];   // Really SHA1, but be on the safe side
    char *name = NULL;
    int fileNameLen = filePath.size() + 1;
    int useNameLen;
    int offset;
    u32 flags;
    u32 attrs = 0;
    u32 rdLen;
    CSLOSAesKey key;
    CSLOSAesIv iv;
    CSL_AesContext aesCtx;
    VSSI_File fileHandle = NULL;
    vssi_stress_test_context_t test_context;
    int blkNo;
    int randBlocks;

    LOG_ALWAYS("filePath %s, fileNameLen %d, numBlks %d, blkSz %d",
        filePath.c_str(), fileNameLen, numBlocks, blkSize);

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        LOG_ERROR("Failed to create semaphore.");
        rv = VSSI_INVALID;
        goto exit;
    } 

    do {
        if((name = (char*)malloc(fileNameLen)) == NULL) {
            rv = VSSI_NOMEM;
            break;
        }
        memcpy(name, filePath.c_str(), fileNameLen);
        CSL_ResetSha(&hashCtx);
        CSL_InputSha(&hashCtx, name, fileNameLen);
        CSL_ResultSha(&hashCtx, hashVal);
        free(name);
        name = NULL;

        if(blkSize < MIN_VSF_BLKSIZE) {
            rv = VSSI_INVALID;
            break;
        }
        if((rbuf = (u8*)malloc(blkSize)) == NULL) {
            rv = VSSI_NOMEM;
            break;
        }

        // Truncate file name len if necessary
        useNameLen = MIN(fileNameLen, blkSize - (int)sizeof(blkNo));

        // Open the file
        flags = VSSI_FILE_OPEN_READ;
        VSSI_OpenFile(dataset_handle, filePath.c_str(), 
                      flags, attrs, &fileHandle,
                      &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
            LOG_ERROR("OpenFile %s failed %d", filePath.c_str(), rv);
            break;
        }

        // Use first 16 bytes of hash as AES key
        // For each block, the IV is the block number
        memcpy(key, hashVal, sizeof(key));

        for(randBlocks = 16; randBlocks > 0; randBlocks--) {
            memset(rbuf, 0, blkSize);
            memset(iv, 0, sizeof(iv));

            // Pick a random block number
            blkNo = (int) VPLMath_Rand();
            blkNo = blkNo % numBlocks;

            memcpy(iv, &blkNo, sizeof(blkNo));

            rdLen = blkSize;
            VSSI_ReadFile(fileHandle, blkNo * blkSize, &rdLen, (char*)rbuf,
                          &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("ReadFile %s failed %d", filePath.c_str(), rv);
                break;
            }
            if(rdLen != blkSize) {
                LOG_ERROR("ReadFile %s failed: read %d, exp %d", 
                    filePath.c_str(), rdLen, blkSize);
                rv = VSSI_INVALID;
                break;
            }

            if((rv = CSL_ResetDecryptAes(&aesCtx, key, iv)) != CSL_OK) {
                break;
            }
            if((rv = CSL_DecryptAes(&aesCtx, rbuf, blkSize, rbuf)) != CSL_OK) {
                break;
            }

            //
            // Check contents of the block
            //
            if(memcmp(rbuf, &blkNo, sizeof(blkNo)) != 0) {
                LOG_ERROR("Data miscompare on file %s at blkNo %d", filePath.c_str(), blkNo);
                dumpData("Read Data", rbuf, sizeof(blkNo));
                dumpData("Expected Data", (u8*)&blkNo, sizeof(blkNo));
                rv = VSSI_INVALID;
                break;
            }
            if(memcmp(&rbuf[sizeof(blkNo)], filePath.c_str(), useNameLen) != 0) {
                LOG_ERROR("Data miscompare on file %s at blkNo %d", filePath.c_str(), blkNo);
                dumpData("Read Data", &rbuf[sizeof(blkNo)], useNameLen);
                dumpData("Expected Data", (u8*)filePath.c_str(), useNameLen);
                rv = VSSI_INVALID;
                break;
            }
            offset = useNameLen + sizeof(blkNo);
            while(offset & (sizeof(blkNo) - 1)) {
                if(rbuf[offset] != 0) {
                    LOG_ERROR("Data miscompare on file %s at blkNo %d offset %d: %02x",
                        filePath.c_str(), blkNo, offset, rbuf[offset]);
                    rv = VSSI_INVALID;
                    break;
                }
                offset++;
            }
            while(offset < blkSize) {
                if(*((u32 *)&rbuf[offset]) != 0) {
                    LOG_ERROR("Data miscompare on file %s at blkNo %d offset %d: %08x",
                        filePath.c_str(), blkNo, offset, *((u32 *)&rbuf[offset]));
                    rv = VSSI_INVALID;
                    break;
                }
                offset += sizeof(u32);
            }
            //LOG_ALWAYS("Data correct for %s at blkNo %d", filePath.c_str(), blkNo);
        }

    } while(0);

    if(fileHandle != NULL) {
        VSSI_CloseFile(fileHandle,
                       &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        if(test_context.rv != VSSI_SUCCESS) {
            LOG_ERROR("CloseFile %s failed %d", filePath.c_str(), test_context.rv);
        }
    }
    VPLSem_Destroy(&(test_context.sem));

exit:
    if(name != NULL) free(name);
    if(rbuf != NULL) free(rbuf);

    return rv;
}

//
// File IO Stress Test
// 
// Each one of these workers loops creating a file of random size, writing
// a unique pattern in each block then reopening the file and verifying the
// contents.  With multiple instances running, this tests that IO requests
// are being correctly handled.
//
static VPLThread_return_t vssi_stress_case_file_io(VPLThread_arg_t arg)
{
    std::string curTestName = TEST_CASE_FILE_IO;
    struct vssi_stress_worker_args *ap = (struct vssi_stress_worker_args *)arg;
    int rv = 0;
    u32 myTid;
    static int fileNo = 0;
    std::string baseDir;
    std::string filePath;
    int blkSize;
    int maxBlocks;
    int numBlocks;
    int blkRand;
    int iter;
    int loops = 0;
    bool keep_going = false;

    // Sync with other threads
    thread_start();

    myTid = __vsf_get_tid();
    baseDir = ap->dir_based;
    if((rv = __vsf_mkdirp(baseDir)) != 0) goto exit;

    blkSize = ap->file_io_block_size;
    maxBlocks = ap->file_io_max_blocks;
    keep_going = ap->keep_running_worker;
    iter = ap->iterations;

    while(1) {
        blkRand = (int) VPLMath_Rand();
        numBlocks = blkRand % maxBlocks;
        //
        // In case we get unlucky, check for zero.  That doesn't work since we
        // do mod by the number of blocks.
        //
        if(numBlocks == 0) numBlocks = 1;
        {
            std::stringstream nameStr;
            nameStr << "/file_io_test." << myTid << "." << numBlocks << "." << fileNo++;
            filePath = baseDir + nameStr.str();
        }

        LOG_ALWAYS("filePath %s, bSize %d, nBlks %d, iter %d/%d",
                    filePath.c_str(), blkSize, numBlocks, loops, iter);

        if((rv = __vsf_write_file(filePath, numBlocks, blkSize)) != 0) break;
        if((rv = __vsf_verify_file(filePath, numBlocks, blkSize)) != 0) break;
        if((rv = __vsf_verify_file_blocks(filePath, numBlocks, blkSize)) != 0) break;
        if((rv = __vsf_remove_file(filePath)) != 0) break;
        // Go around again?
        if(iter >= 0 && ++loops < iter) continue;
        if(!keep_going) break;
    }

exit:
    thread_end();

    //
    // If -k mode, abort the tests as soon as one worker fails.  Otherwise the remaining
    // threads will keep running and the failure message may be lost.
    //
    if(keep_going && rv != 0) {
        LOG_ERROR("Worker thread failed, calling abort() to stop the run");
        VPLThread_Sleep(VPLTime_FromMillisec(1000));
        abort();
    }

    delete ap;

    return (VPLThread_return_t)rv;
}

struct test_dir_tree_node
{
    std::string path;
    int level;
};

static void str_replace(std::string& str, const std::string& oldStr, const std::string& newStr)
{
    size_t pos = 0;
    while((pos = str.find(oldStr, pos)) != std::string::npos) {
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}

//
// Directory Creation and Destruction Test
//
static VPLThread_return_t vssi_stress_case_dir_create_dest(VPLThread_arg_t arg)
{
    std::string curTestName = TEST_CASE_DIR_CREATE_DEST;
    struct vssi_stress_worker_args *ap = (struct vssi_stress_worker_args *)arg;
    int rv = 0;
    std::string base_dir;
    std::string base_dir_renamed;
    int max_blocks;
    int num_blocks;
    int blk_size;
    int num_levels;
    int num_per_levels;
    bool keep_going;
    int iter;
    int blk_rand;
    u32 myTid = __vsf_get_tid();
    std::stringstream name_str;
    std::queue<test_dir_tree_node> node_queue;
    std::stack<std::string> node_stack;
    std::vector<std::string> nodes;
    test_dir_tree_node root;
    vssi_stress_test_context_t test_context;

    thread_start();

    max_blocks = ap->file_io_max_blocks;
    blk_size = ap->file_io_block_size;
    keep_going = ap->keep_running_worker;
    iter = ap->iterations;
    base_dir = ap->dir_based;
    blk_rand = (int) VPLMath_Rand();
    num_blocks = blk_rand % max_blocks;
    num_per_levels = ap->dir_create_des_num_per_level;
    num_levels = ap->dir_create_des_levels;
    
    VPL_SET_UNINITIALIZED(&(test_context.sem));
    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        LOG_ERROR("Failed to create semaphore.");
        rv = VSSI_INVALID;
        goto exit;
    }
   
    name_str.str(std::string());
    name_str << base_dir << "/root" << myTid;
    base_dir = name_str.str();
    base_dir_renamed = name_str.str();
    do {
        nodes.clear();
        LOG_ALWAYS("Create tree %s",  base_dir.c_str());
        name_str << "-renamed";
        rv = __vsf_mkdirp(base_dir);
        if(rv != VSSI_SUCCESS) {
            LOG_ERROR("Make based dir %s failed %d",  base_dir.c_str(), rv);
            goto exit;
        }
        root.path = base_dir;
        root.level = 0;
        node_queue.push(root);
        node_stack.push(base_dir_renamed);

        // create dirs and files for the tree
        while(node_queue.size() != 0 && rv == 0) {
            test_dir_tree_node node = node_queue.front();
            node_queue.pop();
            if(node.level >= num_levels - 1 ) {
                for(int i = 0; i < num_per_levels; i++) {
                    name_str.str(std::string());
                    name_str << node.path << "/f" << myTid << std::setfill('0') << std::setw(32) << i;
                    std::string name_path = name_str.str(); 
                    if((rv = __vsf_write_file(name_path, num_blocks, blk_size)) != 0) break;
                    if((rv = __vsf_verify_file(name_path, num_blocks, blk_size)) != 0) break;
                    if((rv = __vsf_verify_file_blocks(name_path, num_blocks, blk_size)) != 0) break;
                    str_replace(name_path, base_dir, base_dir_renamed);
                    node_stack.push(name_path); // push for deletion
                    nodes.push_back(name_path);
                }
            } else {
                for(int i = 0; i < num_per_levels; i++) {
                    test_dir_tree_node child_node;
                    name_str.str(std::string());
                    name_str << node.path << "/d"<< myTid << std::setfill('0') << std::setw(32) << i;
                    std::string name_path = name_str.str();
                    child_node.path = name_path;
                    child_node.level = node.level + 1;
                    if((rv = __vsf_mkdir(name_path)) != 0 ) break; 
                    node_queue.push(child_node);
                    str_replace(name_path, base_dir, base_dir_renamed);
                    node_stack.push(name_path); // push for deletion
                    nodes.push_back(name_path);
                }
            }
        }
        if(rv != 0) {
            goto exit;
        }

        // rename the tree
        LOG_ALWAYS("Rename tree %s",  base_dir.c_str());
        VSSI_Rename2(dataset_handle, base_dir.c_str(),  
                   base_dir_renamed.c_str(), 0,
                   &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            LOG_ERROR("Rename %s failed %d",  base_dir.c_str(), rv);
            goto exit;
        }

        // look into the tree
        LOG_ALWAYS("Looking tree %s",  base_dir_renamed.c_str());
        for(int i = 0; i < (int)nodes.size(); i++) {
            VSSI_Dirent2* stats = NULL;
            std::string node_name = nodes[i];
            LOG_ALWAYS("Looking node %s",  node_name.c_str());
            VSSI_Stat2(dataset_handle, node_name.c_str(), &stats,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            free(stats);
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("Looking %s failed %d",  node_name.c_str(), rv);
                goto exit;
            }
        }

        // remove the tree
        LOG_ALWAYS("Remove tree %s",  base_dir_renamed.c_str());
        while(node_stack.size() > 0) {
            std::string node_name = node_stack.top();
            LOG_ALWAYS("Remove node %s",  node_name.c_str());
            VSSI_Remove(dataset_handle, node_name.c_str(),
                        &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("Remove %s failed %d",  node_name.c_str(), rv);
                goto exit;
            } 
            node_stack.pop();
        }
    } while(--iter > 0 || keep_going);

exit:
    VPLSem_Destroy(&(test_context.sem));
    thread_end();
    
    //
    // If -k mode, abort the tests as soon as one worker fails.  Otherwise the remaining
    // threads will keep running and the failure message may be lost.
    //
    if(keep_going && rv != 0) {
        LOG_ERROR("Worker thread failed, calling abort() to stop the run");
        VPLThread_Sleep(VPLTime_FromMillisec(1000));
        abort();
    }

    delete ap;

    return (VPLThread_return_t)rv;
}

#define ATTRS_SET_MASK  (VSSI_ATTR_READONLY | VSSI_ATTR_SYS | \
    VSSI_ATTR_HIDDEN | VSSI_ATTR_ARCHIVE)
#define ATTRS_CLR_MASK  (ATTRS_SET_MASK)

struct test_file_mode
{
    std::string path;
    std::string path_renamed;
    u32 ori_attrs;
    u32 set_attrs;
};

//
// File Modes and Attributes Test
//
static VPLThread_return_t vssi_stress_case_file_mode_attr(VPLThread_arg_t arg)
{
    std::string curTestName = TEST_FILE_MODE_ATTR;
    struct vssi_stress_worker_args *ap = (struct vssi_stress_worker_args *)arg;
    int rv = 0;
    int number_of_files;
    int max_blocks;
    int num_blocks;
    int blk_size;
    bool keep_going;
    int iter;
    int blk_rand;
    std::string base_dir;
    std::stringstream name_str;
    vssi_stress_test_context_t test_context;
    std::vector<test_file_mode> files;
    u32 myTid = __vsf_get_tid();
    
    thread_start();

    number_of_files = ap->file_mod_attr_number_of_files;
    base_dir = ap->dir_based;
    max_blocks = ap->file_io_max_blocks;
    blk_size = ap->file_io_block_size;
    keep_going = ap->keep_running_worker;
    iter = ap->iterations;
    blk_rand = (int) VPLMath_Rand();
    num_blocks = blk_rand % max_blocks;
    
    VPL_SET_UNINITIALIZED(&(test_context.sem));
    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        LOG_ERROR("Failed to create semaphore.");
        rv = VSSI_INVALID;
        goto exit;
    }
   
    rv = __vsf_mkdirp(base_dir);
    if(rv != VSSI_SUCCESS) {
        LOG_ERROR("Make based dir %s failed %d",  base_dir.c_str(), rv);
        goto exit;
    }

    do {
        // create files
        files.clear();
        for(int i = 0; i < number_of_files; i++) {
            name_str.str(std::string());
            name_str << base_dir << "/f" << myTid << std::setfill('0') << std::setw(32) << i;
            std::string name_path = name_str.str();
            if((rv = __vsf_write_file(name_path, num_blocks, blk_size)) != 0) break;
            if((rv = __vsf_verify_file(name_path, num_blocks, blk_size)) != 0) break;
            if((rv = __vsf_verify_file_blocks(name_path, num_blocks, blk_size)) != 0) break;
            VSSI_Dirent2* stats = NULL;
            test_file_mode file;
            name_str << "-renamed";
            file.path = name_path;
            file.path_renamed = name_str.str();
            VSSI_Stat2(dataset_handle, name_path.c_str(), &stats,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("Stat2 %s failed %d",  name_path.c_str(), rv);
                free(stats);
                goto exit;
            }
            file.ori_attrs = stats->attrs;
            LOG_ALWAYS("File: %s, ori attrs: "FMTu32,  name_path.c_str(), stats->attrs);
            free(stats);
            files.push_back(file);
        }
        if(rv != 0) {
            goto exit;
        }

        // rename files, and chmod
        for(int i = 0; i < (int)files.size(); i++) {
            VSSI_File fileHandle;
            u32 attrsFile = 0;
            u32 attrs = (u32)VPLMath_Rand() % ATTRS_SET_MASK;
            u32 attrs_mask = (u32)VPLMath_Rand() % ATTRS_CLR_MASK;
            files[i].set_attrs  = (files[i].ori_attrs & ~(attrs_mask & ATTRS_CLR_MASK)) | (attrs & (attrs_mask & ATTRS_SET_MASK));
            std::string name_path = files[i].path;
            std::string name_path_renamed = files[i].path_renamed;
            LOG_ALWAYS("Rename file %s",  name_path.c_str());
            VSSI_Rename2(dataset_handle, name_path.c_str(),  
                       name_path_renamed.c_str(), 0,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
                LOG_ERROR("Rename %s failed %d",  name_path.c_str(), rv);
                goto exit;
            }
            
            VSSI_OpenFile(dataset_handle, 
                       name_path_renamed.c_str(),
                       VSSI_FILE_OPEN_READ, attrsFile, &fileHandle,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
                LOG_ERROR("OpenFile %s failed %d",  name_path_renamed.c_str(), rv);
                goto exit;
            } 

            VSSI_ChmodFile(fileHandle,  
                       attrs, attrs_mask,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
                LOG_ERROR("ChmodFile %s failed %d",  name_path_renamed.c_str(), rv);
                goto exit;
            } 
            
            VSSI_CloseFile( 
                       fileHandle,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("CloseFile %s failed %d",  name_path_renamed.c_str(), rv);
                goto exit;
            } 

        }

        // read attrs back
        for(int i = 0; i < (int)files.size(); i++) {
            VSSI_Dirent2* stats = NULL;
            u32 set_attrs = files[i].set_attrs;
            std::string name_path_renamed = files[i].path_renamed;
            VSSI_Stat2(dataset_handle, name_path_renamed.c_str(), &stats,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("Stat2 %s failed %d",  name_path_renamed.c_str(), rv);
                free(stats);
                goto exit;
            }
            LOG_ALWAYS("Read attrs back, file: %s, set attrs: "FMTu32" attrs: "FMTu32, name_path_renamed.c_str(), set_attrs, stats->attrs);
            if(set_attrs != stats->attrs) {
                rv = -1;
                LOG_ERROR("Attrs is different for %s ", name_path_renamed.c_str());
                free(stats);
                goto exit;
            }
            free(stats);
        }
        

        // set to original
        // rename files, and chmod
        for(int i = 0; i < (int)files.size(); i++) {
            VSSI_File fileHandle;
            u32 attrsFile = 0;
            u32 attrs = files[i].ori_attrs;
            u32 attrs_mask = 0xffffffff;
            std::string name_path = files[i].path;
            std::string name_path_renamed = files[i].path_renamed;
            LOG_ALWAYS("Rename file %s",  name_path_renamed.c_str());
            VSSI_Rename2(dataset_handle, name_path_renamed.c_str(),  
                       name_path.c_str(), 0,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
                LOG_ERROR("Rename %s failed %d", name_path_renamed.c_str(), rv);
                goto exit;
            }
            
            VSSI_OpenFile(dataset_handle, 
                       name_path.c_str(),
                       VSSI_FILE_OPEN_READ, attrsFile, &fileHandle,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
                LOG_ERROR("OpenFile %s failed %d",  name_path_renamed.c_str(), rv);
                goto exit;
            } 

            VSSI_ChmodFile(fileHandle,  
                       attrs, attrs_mask,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
                LOG_ERROR("ChmodFile %s failed %d",  name_path_renamed.c_str(), rv);
                goto exit;
            } 
            
            VSSI_CloseFile( 
                       fileHandle,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("CloseFile %s failed %d",  name_path_renamed.c_str(), rv);
                goto exit;
            } 

        }

        // read attrs back
        for(int i = 0; i < (int)files.size(); i++) {
            VSSI_Dirent2* stats;
            u32 ori_attrs = files[i].ori_attrs;
            std::string name_path = files[i].path;
            VSSI_Stat2(dataset_handle, name_path.c_str(), &stats,
                       &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("Stat2 %s failed %d",  name_path.c_str(), rv);
                free(stats);
                goto exit;
            }
            LOG_ALWAYS("Read attrs back, file: %s, ori attrs: "FMTu32" attrs: "FMTu32, name_path.c_str(), ori_attrs, stats->attrs);
            if(ori_attrs != stats->attrs) {
                rv = -1;
                LOG_ERROR("Attrs is different for %s ", name_path.c_str());
                free(stats);
                goto exit;
            }
            free(stats);
        }

        // remove files
        for(int i = 0; i < (int)files.size(); i++) {
            std::string name_path = files[i].path;
            LOG_ALWAYS("Remove file %s",  name_path.c_str());
            VSSI_Remove(dataset_handle, name_path.c_str(),
                        &test_context, vssi_stress_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
            if(rv != VSSI_SUCCESS) {
                LOG_ERROR("Remove %s failed %d", name_path.c_str(), rv);
                goto exit;
            }
        }

    } while(--iter > 0 || keep_going);

exit:
    VPLSem_Destroy(&(test_context.sem));
    thread_end();

    //
    // If -k mode, abort the tests as soon as one worker fails.  Otherwise the remaining
    // threads will keep running and the failure message may be lost.
    //
    if(keep_going && rv != 0) {
        LOG_ERROR("Worker thread failed, calling abort() to stop the run");
        VPLThread_Sleep(VPLTime_FromMillisec(1000));
        abort();
    }

    delete ap;

    return (VPLThread_return_t)rv;
}

//
// The file for the byte range lock test consists of an array
// of the following structs, followed by the SHA1 hash of the 
// array of structs.
//
#define VSFL_MAX_ENTRIES 32
struct __vsl_lock_file_entry {
    u32 tid;
    u32 counter;
    VPLTime_t totalWait;
    VPLTime_t lastEvent;
    VPLTime_t maxWait;
};

static void __vsl_set_hash(u8 *vslRec, u32 len)
{
    CSL_ShaContext hashCtx;
    u8 hashVal[20];
    u32 hashLen = len - sizeof(hashVal);

    CSL_ResetSha(&hashCtx);
    CSL_InputSha(&hashCtx, vslRec, hashLen);
    CSL_ResultSha(&hashCtx, hashVal);
    memcpy(vslRec + hashLen, hashVal, sizeof(hashVal));
}

static bool __vsl_check_hash(u8 *vslRec, u32 len)
{
    CSL_ShaContext hashCtx;
    u8 hashVal[20];
    u32 hashLen = len - sizeof(hashVal);

    CSL_ResetSha(&hashCtx);
    CSL_InputSha(&hashCtx, vslRec, hashLen);
    CSL_ResultSha(&hashCtx, hashVal);

    return (memcmp(vslRec + hashLen, hashVal, sizeof(hashVal)) == 0);
}

//
// File Lock Stress Test
//
// This worker contends for a lock on a shared file, keeps track of statistics
// on how long it waits for the lock in the file itself.  The logic also tries
// to detect conflicting accesses to prove that byte range locking is correctly
// providing mutual exclusion.  The per worker stats should show that the locking
// code and the scheduler provide "fair and balanced" treatment.
//
static VPLThread_return_t vssi_stress_case_file_lock(VPLThread_arg_t arg)
{
    std::string curTestName = TEST_FILE_LOCK;
    struct vssi_stress_worker_args *ap = (struct vssi_stress_worker_args *)arg;
    int rv = 0;
    u32 myTid;
    std::string baseDir;
    std::string filePath;
    std::string fileName;
    int iter;
    int loops = 0;
    bool keep_going = false;
    vssi_stress_test_context_t test_context;
    VSSI_File fileHandle = NULL;
    u32 flags;
    u32 attrs = 0;
    VSSI_ByteRangeLock brLock;
    u32 lockFlags;
    u32 rdLen;
    u32 wrLen;
    u32 fileLen;
    int index;
    VPLTime_t delay;
    VPLTime_t curTime;
    VPLTime_t waitTime;
    struct __vsl_lock_file_entry *vslfp = NULL;
    struct __vsl_lock_file_entry *ventp = NULL;

    // Sync with other threads
    thread_start();

    myTid = __vsf_get_tid();
    baseDir = ap->dir_based;
    if((rv = __vsf_mkdirp(baseDir)) != 0) goto exit;
    
    keep_going = ap->keep_running_worker;
    iter = ap->iterations;
    delay = VPLTime_FromMillisec(ap->file_lock_sleep_time);
    fileName = ap->file_lock_file_name;
    filePath = baseDir + "/" + fileName;

    LOG_ALWAYS("filePath %s, tid %u, iter %d", filePath.c_str(), myTid, iter);

    fileLen = VSFL_MAX_ENTRIES * sizeof(struct __vsl_lock_file_entry) + 20;
    if((vslfp = (struct __vsl_lock_file_entry *)malloc(fileLen)) == NULL) {
        rv = VSSI_NOMEM;
        goto exit;
    }

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to create semaphore.");
        rv = VSSI_INVALID;
        goto exit;
    } 

    // Open and create the file
    flags = VSSI_FILE_OPEN_READ | VSSI_FILE_OPEN_WRITE | VSSI_FILE_OPEN_OPEN_ALWAYS;
    flags |= VSSI_FILE_SHARE_READ | VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(dataset_handle, filePath.c_str(), 
                  flags, attrs, &fileHandle,
                  &test_context, vssi_stress_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;
    if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
        LOG_ERROR("OpenFile %s failed %d", filePath.c_str(), rv);
        goto exit1;
    }
    rv = VSSI_SUCCESS;

    //
    // Grab write exclusive lock in blocking mode, update the file, repeat
    //
    while(1) {
        brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
        brLock.offset = 0;
        brLock.length = fileLen;
        lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_WAIT;

        VSSI_SetByteRangeLock(fileHandle, &brLock, lockFlags,
                              &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            LOG_ERROR("VSSI_SetByteRangeLock returned %d", rv);
            break;
        }

        //LOG_ALWAYS("tid %u has the lock (loop %d)", myTid, loops);

        rdLen = fileLen;
        VSSI_ReadFile(fileHandle, 0, &rdLen, (char *)vslfp,
                              &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            LOG_ERROR("VSSI_ReadFile returned %d", rv);
            break;
        }
        //
        // If this is the first one, initialize the file
        //
        if(rdLen == 0) {
            memset((void *)vslfp, 0, fileLen);
            vslfp->tid = myTid;
            __vsl_set_hash((u8 *)vslfp, fileLen);
        }
        else if(rdLen != fileLen) {
            LOG_ERROR("ReadFile of lockstate file returns %u, expected %u",
                rdLen, fileLen);
            rv = VSSI_INVALID;
            break;
        }

        if(!__vsl_check_hash((u8 *)vslfp, fileLen)) {
            LOG_ERROR("Lock file hash is incorrect!");
            rv = VSSI_INVALID;
            break;
        }

        // Find the entry for this thread or add it if necessary
        ventp = vslfp;
        for(index = 0; index < VSFL_MAX_ENTRIES; index++) {
            if(ventp->tid == myTid) {
                break;
            }
            // Entries are added contiguously and never deleted
            // so if we hit a free one, allocate it.
            if(ventp->tid == 0) {
                LOG_ALWAYS("Allocate slot %d for tid %u", index, myTid);
                ventp->tid = myTid;
                break;
            }
            ventp++;
        }
        if(index == VSFL_MAX_ENTRIES) {
            LOG_ERROR("Lock file thread array is full!");
            rv = VSSI_INVALID;
            break;
        }
        // Check that the record is consistent with what we expect
        // to detect incorrect mutual exclusion.
        if(loops != ventp->counter) {
            LOG_ERROR("Conflicting update to task loop counter: %u expect %u",
                ventp->counter, loops);
            rv = VSSI_INVALID;
            break;
        }
        ventp->counter++;
        curTime = VPLTime_GetTimeStamp();
        if(ventp->lastEvent != 0) {
            waitTime = curTime - ventp->lastEvent;
            if(waitTime > ventp->maxWait) {
                ventp->maxWait = waitTime;
            }
            ventp->totalWait += waitTime;
        }
        ventp->lastEvent = curTime;

        if((ventp->counter & 0x3f) == 0 && ventp->counter != 0) {
            LOG_ALWAYS("Tid %u: %u loops, avg wt "FMTu64"ms, max wt "FMTu64"ms",
                       myTid,
                       ventp->counter,
                       VPLTime_ToMillisec(ventp->totalWait / ventp->counter),
                       VPLTime_ToMillisec(ventp->maxWait));
        }

        //
        // Recompute hash of the file
        //
        __vsl_set_hash((u8 *)vslfp, fileLen);

        // Update the file
        wrLen = fileLen;
        VSSI_WriteFile(fileHandle, 0, &wrLen, (const char *)vslfp,
                              &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            LOG_ERROR("VSSI_WriteFile returned %d", rv);
            break;
        }
        if(wrLen != fileLen) {
            LOG_ERROR("VSSI_WriteFile short write: %d, exp %d", wrLen, fileLen);
            rv = VSSI_INVALID;
            break;
        }

        // Release the lock
        brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
        brLock.offset = 0;
        brLock.length = fileLen;
        lockFlags = VSSI_RANGE_UNLOCK|VSSI_RANGE_NOWAIT;

        VSSI_SetByteRangeLock(fileHandle, &brLock, lockFlags,
                              &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            LOG_ERROR("VSSI_SetByteRangeLock returned %d", rv);
            break;
        }

        // Wait for the specified delay time
        if(delay != 0) VPLThread_Sleep(delay);

        // Go around again?
        loops++;
        if(iter >= 0 && loops < iter) continue;
        if(!keep_going) break;
    }

    if(fileHandle != NULL) {
        VSSI_CloseFile(fileHandle,
                       &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        if(test_context.rv != VSSI_SUCCESS) {
            LOG_ERROR("CloseFile %s failed %d", filePath.c_str(), test_context.rv);
        }
    }
    (void) __vsf_remove_file(filePath);

exit1:
    VPLSem_Destroy(&(test_context.sem));
exit:
    if(vslfp != NULL) {
        // Find the entry for this thread and print the stats
        ventp = vslfp;
        for(index = 0; index < VSFL_MAX_ENTRIES; index++) {
            if(ventp->tid == myTid) {
                LOG_ALWAYS("Tid %u exiting: %u loops, avg wt "FMTu64"ms, max wt "FMTu64"ms",
                    myTid,
                    ventp->counter,
                    VPLTime_ToMillisec(ventp->totalWait / ventp->counter),
                    VPLTime_ToMillisec(ventp->maxWait));
                break;
            }
            if(ventp->tid == 0) {
                break;
            }
            ventp++;
        }
        free(vslfp);
    }
    thread_end();

    //
    // If -k mode, abort the tests as soon as one worker fails.  Otherwise the remaining
    // threads will keep running and the failure message may be lost.
    //
    if(keep_going && rv != 0) {
        LOG_ERROR("Worker thread failed, calling abort() to stop the run");
        VPLThread_Sleep(VPLTime_FromMillisec(1000));
        abort();
    }

    delete ap;

    return (VPLThread_return_t)rv;
}

static VPLThread_return_t vssi_stress_case_many_files(VPLThread_arg_t arg)
{
    std::string curTestName = TEST_MANY_FILES;
    struct vssi_stress_worker_args *ap = (struct vssi_stress_worker_args *)arg;
    int rv = 0;
    int rc, i, j, k;
    vssi_stress_test_context_t test_context;
    VSSI_File fileHandle1;
    u32 flags, attrs;
    std::string baseDir;
    std::string dirPath;
    std::string filePath;
    std::stringstream nameStr;
    u32 myTid;
    int iter;
    bool keep_going = false;

    thread_start();
    LOG_ALWAYS("vssi_stress_case_many_files()");

    baseDir = ap->dir_based;
    iter = ap->iterations;
    keep_going = ap->keep_running_worker;
    myTid = __vsf_get_tid();

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    if (VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rv++;
        goto exit;
    }

    for (i = 100; i < 200; i++) {
        for (j = 0; j < 500; j++) {
            int num_files;

            nameStr.str(std::string());
            nameStr << "/" << myTid
                    << "/" << std::setfill('0') << std::setw(8) << i
                    << "/" << std::setw(8) << j;
            dirPath = baseDir + nameStr.str();

            rc = __vsf_mkdirp(dirPath);
            if(rc != VSSI_SUCCESS) {
                rv++;
                continue;
            }

            if (j == 0) {
                num_files = (63 * 1024);
            } else {
                num_files = 600;
            }

            for (k = 0; k < num_files; k++) {
                flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
                flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
                attrs = 0;

                nameStr.str(std::string());
                nameStr << "/" << std::setfill('0') << std::setw(16) << k;
                filePath = dirPath + nameStr.str();
                if((k & 0xff) == 0) {
                    LOG_ALWAYS("About to Open %s", filePath.c_str());
                }

                VSSI_OpenFile(dataset_handle, filePath.c_str(), flags, attrs, &fileHandle1,
                            &test_context, vssi_stress_test_callback);
                VPLSem_Wait(&(test_context.sem));
                rc = test_context.rv;
                if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
                    LOG_ERROR("FAIL: VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_SUCCESS);
                    rv++;
                    continue;
                }

                VSSI_CloseFile(fileHandle1,
                   &test_context, vssi_stress_test_callback);
                VPLSem_Wait(&(test_context.sem));
                rc = test_context.rv;
                if(rc != VSSI_SUCCESS) {
                    LOG_ERROR("FAIL: VSSI_CloseFile returns %d.", rc);
                    rv++;
                }
            }

            LOG_ALWAYS("Created %d files", num_files);
        }

        LOG_ALWAYS("i = %d", i);
    }

    rv = 1; // force exit

    VPLSem_Destroy(&(test_context.sem));
 exit:
    LOG_ALWAYS("Test many_files result: %d", rv);
    thread_end();

    //
    // If -k mode, abort the tests as soon as one worker fails.  Otherwise the remaining
    // threads will keep running and the failure message may be lost.
    //
    if(keep_going && rv != 0) {
        LOG_ERROR("Worker thread failed, calling abort() to stop the run");
        VPLThread_Sleep(VPLTime_FromMillisec(1000));
        abort();
    }

    delete ap;

    return (VPLThread_return_t)rv;
}

static int usage(int argc, const char* argv[])
{
    // Dump original command

    printf("VSSIStressTest Options:\n");
    printf(" -u --userid                user id\n");
    printf(" -p --password              user password\n");
    printf(" -o --domain                lab domain\n");
    printf(" -W --worker-args           use -W \"WORKER_ARGS\" to specify the arguments passed into each worker, there could be mutiple -W \"WORKER_ARGS\"\n");

    printf("\nWORKER_ARGS:\n");
    printf(" -d --dir-based             base-dir in Orbe\n");
    printf(" -k --keep-running          keep running the worker\n");
    printf(" -n --iterations            run the worker for number of times\n");
    printf(" -N --num-of-test           number of workers to run\n");
    printf(" -t --testcase              specify testcase to run (FileIo, DirCreateDestruct, FileLock, FileModeAttr, ManyFiles)\n");
    printf(" -b --blocksize             (FileIo)file block size to be tested\n");
    printf(" -m --maxblocks             (FileIo)file max blocks to be tested\n");
    printf(" -D --dirs-levels           (DirCreateDestruct)directory tree depth\n");
    printf(" -W --dirs-num-per-level    (DirCreateDestruct)directory tree width\n");
    printf(" -a --file-mod-num-files    (FileModeAttr)number of files to be tested\n");
    printf(" -f --file-lock-filename    (FileLock)synced filename\n");
    printf(" -s --file-lock-sleeptime   (FileLock)worker sleep time\n");
    printf("\nExample:\n");
    printf("./dxshell -i 2 VSSIStressTest -u user@igware.com -p pAsSwOrD -o pc.igware.net -W \"-t FileIo -d file_io_dir\" -W \"-t DirCreateDestruct -N 2 -D 3 -W 2 -d dir_cre_des_dir\" -W \"-t FileModeAttr -N 3 -a 5 -d file_mode_dir\"\n\n");
    return 0;
}

static int master_args_parser(int argc, const char* argv[])
{
    int rv = 0;
    optind = 1;
    static struct option long_options[] = {
        {"worker-args", required_argument, 0, 'W'},
        {"userid", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"domain", required_argument, 0, 'o'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;

        int c = getopt_long(argc, (char* const*)argv, "W:u:p:o:",
                            long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'W':
            worker_args.push_back(optarg);
            break;
        case 'o':
            domain = optarg;
            break;
        case 'u':
            user_login_id = optarg;
            break;
        case 'p':
            password = optarg;
            break;
        default:
            usage(argc, argv);
            rv++;
            break;
        }
    }

    return rv;

}

static int worker_args_parser(const std::string& args_in, vssi_stress_worker_args& args_out)
{
    int rv = 0;
    // Convert args string into array of tokens for parsing by getopts.
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end;
    char** argv = NULL;
    do {
        start = args_in.find_first_not_of(" \t\n", start); // Skip leading whitepsace
        if(start == std::string::npos) { break; }
        end = args_in.find_first_of(" \t\n", start); // End at first whitespace.
        tokens.push_back(args_in.substr(start, end - start));
        start = end;
    } while(start != std::string::npos);

    argv = new char*[tokens.size() + 1];
    argv[0] = worker_str;
    for(size_t i = 0; i < tokens.size(); i++) {
        argv[i + 1] = (char*)(tokens[i].c_str());
    }

    optind = 1;

    memset((void*)(&args_out), 0, sizeof(args_out));

    args_out.keep_running_worker = false;
    args_out.iterations = -1;

    args_out.file_io_block_size = 4 * 1024;
    args_out.file_io_max_blocks = 10 * 1024;

    args_out.dir_create_des_levels = 1;
    args_out.dir_create_des_num_per_level = 1;

    args_out.file_mod_attr_number_of_files = 1;

    args_out.file_lock_sleep_time = 0;
    args_out.num_of_test = 1;

    static struct option long_options[] = {
        {"dir-based", required_argument, 0, 'd'},
        {"keep-running", no_argument, 0, 'k'},
        {"testcase", required_argument, 0, 't'},
        {"blocksize", required_argument, 0, 'b'},
        {"maxblocks", required_argument, 0, 'm'},
        {"dirs-levels", required_argument, 0, 'D'},
        {"dirs-num-per-level", required_argument, 0, 'W'},
        {"file-mod-num-files", required_argument, 0, 'a'},
        {"file-lock-filename", required_argument, 0, 'f'},
        {"file-lock-sleeptime", required_argument, 0, 's'},
        {"num-of-test", required_argument, 0, 'N'},
        {"iterations", required_argument, 0, 'n'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;

        int c = getopt_long(tokens.size() + 1, (char* const*)argv, "d:kt:b:m:D:W:a:f:s:N:n:",
                            long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'f':
            memcpy(args_out.file_lock_file_name, optarg, strlen(optarg)+1);
            break;
        case 's':
            args_out.file_lock_sleep_time = atoi(optarg);
            break;
        case 'D':
            args_out.dir_create_des_levels = atoi(optarg);
            break;
        case 'W':
            args_out.dir_create_des_num_per_level = atoi(optarg);
            break;
        case 'a':
            args_out.file_mod_attr_number_of_files = atoi(optarg);
            break;
        case 'b':
            args_out.file_io_block_size = atoi(optarg);
            break;
        case 'm':
            args_out.file_io_max_blocks = atoi(optarg);
            if(args_out.file_io_max_blocks == 0) {
                LOG_ERROR("Maxblocks must not be zero");
                rv++;
            }
            break;
        case 'd':
            memcpy(args_out.dir_based, optarg, strlen(optarg)+1);
            break;
        case 'k':
            args_out.keep_running_worker = true;
            break;
        case 't':
            if(!strcmp("FileIo", optarg)) {
                args_out.case_type = VSSI_STRESS_TEST_CASE_FILE_IO;
            } else if (!strcmp("DirCreateDestruct", optarg)) {
                args_out.case_type = VSSI_STRESS_TEST_CASE_DIR_CREATE_DEST;
            } else if (!strcmp("FileModeAttr", optarg)) {
                args_out.case_type = VSSI_STRESS_TEST_FILE_MODE_ATTR;
            } else if (!strcmp("FileLock", optarg)) {
                args_out.case_type = VSSI_STRESS_TEST_FILE_LOCK;
            } else if (!strcmp("ManyFiles", optarg)) {
                args_out.case_type = VSSI_STRESS_TEST_MANY_FILES;
            } else {
                args_out.case_type = VSSI_STRESS_TEST_NONE;
                LOG_ERROR("Unrecognized stress test type %s", optarg);
                rv++;
            }
            break;
        case 'N':
            args_out.num_of_test = atoi(optarg);
            break;
        case 'n':
            args_out.iterations = atoi(optarg);
            break;
        default:
            usage(tokens.size() + 1, (const char **)argv);
            rv++;
            break;
        }
    }

    delete [] argv;
    return rv;
}

static int vssi_stress_init()
{
    int rv = 0; // pass until failed.

//    VSSI_Session vssSession = NULL;
    vssi_stress_test_context_t test_context;
    u64 userId = 0;
    u64 deviceId = 0;
    u64 virtualDriveId = 0;
    u64 handle = 0;
    std::string ticket;
//    VSSI_RouteInfo vssRouteInfo;
    u64 datasetId = 0;
    ccd::ListUserStorageOutput listSnOut;
    ccd::ListUserStorageInput listSnIn;

//    vssRouteInfo.routes = NULL;
//    vssRouteInfo.num_routes = 0;

    LOG_ALWAYS("\n\n==== Launching Client CCD ====");
    {
        const char *testStr = "SetDomain";
        const char *testArg[] = { testStr, domain.c_str() };
        rv = set_domain(4, testArg);
        CHECK_AND_PRINT_RESULT("Prepare_CCD_Client", testStr, rv);
    }

    {
        const char *testStr = "StartCCD";
        const char *testArg[] = { testStr };
        rv = start_ccd(1, testArg);
        CHECK_AND_PRINT_RESULT("Prepare_CCD_Client", testStr, rv);
    }

    {
        const char *testStr = "StartClient";
        const char *testArg[] = { testStr, user_login_id.c_str(), password.c_str() };
        rv = start_client(3, testArg);
        if (rv < 0) {
            // If fail, try again before declaring failure
            rv = start_client(3, testArg);
        }
        CHECK_AND_PRINT_RESULT("Prepare_CCD_Client", testStr, rv);
    }

    VPLThread_Sleep(VPLTime_FromSec(5));
    
    //get user id
    rv = getDeviceId(&deviceId);
    LOG_ALWAYS("TC_RESULT = %s ;;; TC_NAME = Get_Device_Id",
               rv ? "FAIL" : "PASS");
    if(rv != 0) {
        goto exit;
    }

    //get device id
    rv = getUserId(userId);
    LOG_ALWAYS("TC_RESULT = %s ;;; TC_NAME = Get_User_Id",
                rv ? "FAIL" : "PASS");
    if(rv != 0) {
        goto exit;
    }

    LOG_ALWAYS("VALUE = "FMTu64" "FMTu64" ", 
                deviceId,userId);

    //discover virtual drive
    //use the first one virtual drive
    listSnIn.set_user_id(userId);
    listSnIn.set_only_use_cache(true);
    rv = CCDIListUserStorage(listSnIn, listSnOut);
    LOG_ALWAYS("TC_RESULT = %s ;;; TC_NAME = List User Storage",
               rv ? "FAIL" : "PASS");
    if(rv != 0) {
        goto exit;
    }

    rv = -1;
    for (int i = 0; i < listSnOut.user_storage_size(); i++) {

        LOG_ALWAYS("ListUserStorage[%d]:  %s (virtdrive %s)",
            i, listSnOut.user_storage(i).DebugString().c_str(), 
            listSnOut.user_storage(i).featurevirtdriveenabled() ? "true" : "false");

        if(listSnOut.user_storage(i).featurevirtdriveenabled() &&
           virtualDriveId == 0) {
            handle = listSnOut.user_storage(i).accesshandle();
            virtualDriveId = listSnOut.user_storage(i).storageclusterid();
            ticket = listSnOut.user_storage(i).devspecaccessticket();
            {
                std::ostringstream tohex;
                for (int buf_ind = 0; buf_ind < (int)ticket.length(); buf_ind++) {
                    tohex << std::hex << std::setfill('0') << 
                        std::setw(2) << 
                        std::nouppercase << 
                        ((int)ticket[buf_ind] & 0xff);
                }
                LOG_ALWAYS("Ticket value %s", tohex.str().c_str());
            }
            //local route info
            ccd::ListLanDevicesInput listLanDevIn;
            ccd::ListLanDevicesOutput listLanDevOut;
            listLanDevIn.set_user_id(userId);
            listLanDevIn.set_include_unregistered(true);
            listLanDevIn.set_include_registered_but_not_linked(true);
            listLanDevIn.set_include_linked(true);
            rv = CCDIListLanDevices(listLanDevIn, listLanDevOut);
            if(rv != 0) {
                LOG_ERROR("List Lan Devices result %d", rv);
                break;
            }

#ifdef NDEF
            int route_ind = 0;
            vssRouteInfo.num_routes = 0;
            vssRouteInfo.routes = new VSSI_Route[listSnOut.user_storage(i).storageaccess_size()+
                                                 listLanDevOut.infos_size()];
            memset(vssRouteInfo.routes, 0, sizeof(VSSI_Route)*(listSnOut.user_storage(i).storageaccess_size()+listLanDevOut.infos_size()));
            for(int j = 0; j < listLanDevOut.infos_size(); j++) {
                if(listLanDevOut.infos(j).device_id() == virtualDriveId) {
                    size_t len = strlen(listLanDevOut.infos(j).route_info().ip_v4_address().c_str()) + 1;
                    char* new_str = new char[len];
                    memcpy(new_str, listLanDevOut.infos(j).route_info().ip_v4_address().c_str(), len);
                    vssRouteInfo.routes[route_ind].server = new_str;
                    vssRouteInfo.routes[route_ind].port = listLanDevOut.infos(j).route_info().virtual_drive_port();
                    vssRouteInfo.routes[route_ind].type = vplex::vsDirectory::DIRECT_INTERNAL;
                    vssRouteInfo.routes[route_ind].proto =  vplex::vsDirectory::VS;
                    vssRouteInfo.routes[route_ind].cluster_id = virtualDriveId;
                    LOG_ALWAYS("server : %s.", vssRouteInfo.routes[route_ind].server);
                    LOG_ALWAYS("port : %u.", vssRouteInfo.routes[route_ind].port);
                    LOG_ALWAYS("type : %d.", vssRouteInfo.routes[route_ind].type);
                    LOG_ALWAYS("proto : %d.", vssRouteInfo.routes[route_ind].proto);
                    LOG_ALWAYS("cluster_id : "FMTu64".", vssRouteInfo.routes[route_ind].cluster_id);
                    route_ind++;
                }
            }              
#endif // NDEF
            for( int j = 0 ; j < listSnOut.user_storage(i).storageaccess_size() ; j++ ) {
                bool port_found = false;
                for( int k = 0 ; k < listSnOut.user_storage(i).storageaccess(j).ports_size() ; k++ ) {
                    if ( listSnOut.user_storage(i).storageaccess(j).ports(k).porttype() != 
                            vplex::vsDirectory::PORT_VSSI ) {
                        continue;
                    }
                    if ( listSnOut.user_storage(i).storageaccess(j).ports(k).port() == 0 ) {
                        continue;
                    }
//                    vssRouteInfo.routes[route_ind].port =
//                        listSnOut.user_storage(i).storageaccess(j).ports(k).port();
                    port_found = true;
                    break;
                }
                if ( !port_found ) {
                    continue;
                }
                size_t len = strlen(listSnOut.user_storage(i).storageaccess(j).server().c_str()) + 1;
                char* new_str = new char[len];
                memcpy(new_str, listSnOut.user_storage(i).storageaccess(j).server().c_str(), len);
#ifdef NDEF
                vssRouteInfo.routes[route_ind].server = new_str;
                vssRouteInfo.routes[route_ind].type = listSnOut.user_storage(i).storageaccess(j).routetype();
                vssRouteInfo.routes[route_ind].proto = listSnOut.user_storage(i).storageaccess(j).protocol();
                vssRouteInfo.routes[route_ind].cluster_id = listSnOut.user_storage(i).storageclusterid();
                LOG_ALWAYS("server : %s.", vssRouteInfo.routes[route_ind].server);
                LOG_ALWAYS("port : %u.", vssRouteInfo.routes[route_ind].port);
                LOG_ALWAYS("type : %d.", vssRouteInfo.routes[route_ind].type);
                LOG_ALWAYS("proto : %d.", vssRouteInfo.routes[route_ind].proto);
                LOG_ALWAYS("cluster_id : "FMTu64".", vssRouteInfo.routes[route_ind].cluster_id);
                route_ind++;
#endif //NDEF
            }
#ifdef NDEF
            if ( route_ind == 0 ) {
                LOG_ERROR("No VS routes found.");
                rv = -1;
            } else {
                vssRouteInfo.num_routes = route_ind;
            }
#endif //NDEF
            break;
        }
    }
    if(virtualDriveId == 0) {
        LOG_ERROR("Can't find virtual drive");
        rv = -1;
    }

    LOG_ALWAYS("TC_RESULT = %s ;;; TC_NAME = Find virtual drive and route info.",
               rv ? "FAIL" : "PASS");
    if(rv != 0) {
        goto exit;
    }

    //find associated datasets*
    {
        ccd::ListOwnedDatasetsInput listDstIn;
        ccd::ListOwnedDatasetsOutput listDstOut;
        listDstIn.set_user_id(userId);
        listDstIn.set_only_use_cache(true);
        rv = CCDIListOwnedDatasets(listDstIn, listDstOut);

        if(rv != CCD_OK) { 
            LOG_ERROR("CCDIListOwnedDatasets failed: %d", rv);
        } else {
            rv = -1;
            for(int i = 0; i < listDstOut.dataset_details_size(); i++) {
                if(listDstOut.dataset_details(i).clusterid() ==  virtualDriveId
                   //&& listDstOut.created_by_this_device(i)){
                   && listDstOut.dataset_details(i).datasettype() == vplex::vsDirectory::VIRT_DRIVE) {
                    datasetId = listDstOut.dataset_details(i).datasetid();
                    rv = 0; 
                    LOG_ALWAYS("datasetId : "FMTu64".", datasetId);
                    LOG_ALWAYS("contentType : %s.", listDstOut.dataset_details(i).contenttype().c_str());
                    LOG_ALWAYS("datasetName : %s.", listDstOut.dataset_details(i).datasetname().c_str());
                    LOG_ALWAYS("datasetLocation : %s.", listDstOut.dataset_details(i).datasetlocation().c_str());
                    LOG_ALWAYS("storageclustername : %s.", listDstOut.dataset_details(i).storageclustername().c_str());
                    LOG_ALWAYS("storageclusterhostname : %s.", listDstOut.dataset_details(i).storageclusterhostname().c_str());
                    break;
                }
            }
        }
    }
    LOG_ALWAYS("TC_RESULT = %s ;;; TC_NAME = Find Owned Datasets",
               rv ? "FAIL" : "PASS");
    if(rv != 0) {
        goto exit;
    }

    //vssi setup
//    rv = do_vssi_setup(deviceId);
    rv = VSSI_Init(12 /* test value */);
    LOG_ALWAYS("TC_RESULT = %s ;;; TC_NAME = VSSI_Setup.",
               rv ? "FAIL" : "PASS");
    if(rv != 0) {
        goto exit;
    }

#ifdef NDEF 
    //vssi session registration
    vssSession = VSSI_RegisterSession(handle, ticket.c_str());
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Session registration, session handle %d",
                        (int)vssSession);
    if(vssSession == 0) {
        rv = -1;
        goto exit;
    }
#endif //NDEF

    //do open
    VPL_SET_UNINITIALIZED(&(test_context.sem));
    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rv = -1;
    } 
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Create semphore result %d", rv);
    if(rv != 0) {
        goto exit;
    }

    {
        VSSI_OpenObjectTS(userId, datasetId, 
                         VSSI_READWRITE | VSSI_FORCE, &dataset_handle,
                         &test_context, vssi_stress_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Dataset Open result %d.", rv);
    
 exit:
#ifdef NDEF
    for(int i = 0; i < vssRouteInfo.num_routes; i++) {
        delete [] vssRouteInfo.routes[i].server;
    }
    delete [] vssRouteInfo.routes;
#endif // NDEF
    return rv;
}

static void vssi_stress_finalize()
{
    // Close object. Callback doesn't affect next operation.
    VSSI_CloseObject(dataset_handle, NULL, NULL);

    //do_vssi_cleanup();
    VSSI_Cleanup();

    LOG_ALWAYS("\n\n== Freeing client ==");
    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }
}

static int vssi_stress_worker(const std::string& args)
{
    int thr_count = 0;
    vssi_stress_worker_args parsed_args;

    //Init for thread based test
    if(worker_args_parser(args, parsed_args) != 0) {
        LOG_ERROR("args parsing failed");
        return thr_count;
    }

    VPLThread_fn_t startRoutine; 
    
    switch(parsed_args.case_type) {
        case VSSI_STRESS_TEST_CASE_FILE_IO :
            startRoutine = vssi_stress_case_file_io;
            break;
        case VSSI_STRESS_TEST_CASE_DIR_CREATE_DEST :
            startRoutine = vssi_stress_case_dir_create_dest;
            break;
        case VSSI_STRESS_TEST_FILE_MODE_ATTR :
            startRoutine = vssi_stress_case_file_mode_attr;
            break;
        case VSSI_STRESS_TEST_FILE_LOCK :
            startRoutine = vssi_stress_case_file_lock;
            break;
        case VSSI_STRESS_TEST_MANY_FILES :
            startRoutine = vssi_stress_case_many_files;
            break;
        case VSSI_STRESS_TEST_NONE :
        default:
            return thr_count;
    }

    for(int i = 0; i < parsed_args.num_of_test; i++) {
        VPLThread_t *thr = new VPLThread_t;
        int rv;
        std::ostringstream os;
        os << "vssi_stress_worker_thr_" << thr_count;
        vssi_stress_worker_args* thread_args = new vssi_stress_worker_args;
        memcpy(thread_args, &parsed_args, sizeof(parsed_args));

        memset(thr, 0, sizeof(VPLThread_t));
        rv = VPLThread_Create(thr,
                              startRoutine,
                              thread_args,
                              0,
                              os.str().c_str());

        if(rv == 0) {
            test_thrs_running.push_back(std::pair<VPLThread_t*,int>(thr,0));
            thr_count++;
        }
        else{
            delete thread_args;
            delete thr;
        }
    }

    return thr_count;
}

int dispatch_vssi_stress_test_cmd(int argc, const char* argv[])
{
    int rv = 0;
    int threads_created = 0;

    if(checkHelp(argc, argv)) {
        printf("%s\n", argv[0]);
        usage(argc, argv);
        return 0;
    }

    // Initialize PRNG
    (void) VPLMath_InitRand();

    // Setup thread sync mechanism
    VPLMutex_Init(&thread_state_mutex);
    VPLCond_Init(&thread_state_cond);

    if(master_args_parser(argc, argv) != 0) {
        rv = VSSI_INVALID;
        goto exit;
    }
   
    rv = vssi_stress_init();
    if(rv != 0) {
        rv = VSSI_INVALID;
        goto exit;
    }

    for(int i = 0; i < (int)worker_args.size(); i++) {
        rv = vssi_stress_worker(worker_args[i]);
        if(rv > 0) {
            threads_created += rv;
        } else {
            LOG_ERROR("Failed to start worker threads");
            rv = VSSI_INVALID;
            goto exit;
        }
    }
    rv = 0;

    // Wait for all the worker threads to be ready and then start them
    while(1) {
        VPLMutex_Lock(&thread_state_mutex);
        if(threads_running == threads_created) {
            LOG_ALWAYS("Release the hounds (all %d of them)!", threads_running);
            VPLCond_Broadcast(&thread_state_cond);
            VPLMutex_Unlock(&thread_state_mutex);
            break;
        }
        VPLMutex_Unlock(&thread_state_mutex);
        VPLThread_Yield();
    }

    VPLMutex_Lock(&thread_state_mutex);
    while (1) {
        if(threads_running == 0) {
            VPLMutex_Unlock(&thread_state_mutex);
            break;
        }
        VPLCond_TimedWait(&thread_state_cond, &thread_state_mutex, VPLTime_FromSec(600));
    }
    
    for(int i = 0; i < (int)test_thrs_running.size(); i++) {
        VPLThread_return_t return_value;
        VPLThread_Join(test_thrs_running[i].first, &return_value);
        test_thrs_running[i].second = (int)return_value;
        LOG_ALWAYS("test[%d]: returns %d", i, test_thrs_running[i].second);
        if(rv == 0 && test_thrs_running[i].second != 0) {
            rv = test_thrs_running[i].second;
        }
        delete test_thrs_running[i].first;
    }

    LOG_ALWAYS("num of test done: %d", test_thrs_running.size());

 exit:
    vssi_stress_finalize();
    LOG_ALWAYS("TC_RESULT = %s ;;; TC_NAME = VSSIStressTest_Master",
               rv ? "FAIL" : "PASS");
    return rv;
}
