//
//  Copyright (C) 2007-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include <stdlib.h> 
#include <string.h>
#include <errno.h>

#include <vpl_fs.h>

#include "vplTest.h"

#ifdef ANDROID
#define ROOT_PATH    "/data"
#elif defined(_WIN32)
#ifndef VPL_PLAT_IS_WINRT
// For now, you need to manually create this path first.
#define ROOT_PATH   "C:\\tmp"
#endif
#elif !defined(IOS)
#define ROOT_PATH    "/tmp"
#endif

#ifdef IOS
extern const char* getRootPath();
#elif defined(VPL_PLAT_IS_WINRT)
static void getRootPath(char** rootPath)
{
    // retrieve LocalFolder path in app container
    _VPLFS__GetLocalAppDataPath(rootPath);
}
static void releaseRootPath(char* rootPath)
{
    // should be called to properly release rootPath
    if (rootPath != NULL) {
        free(rootPath);
        rootPath = NULL;
    }
}
#endif

#if defined(IOS)
static const char* rootPath;
#elif defined(VPL_PLAT_IS_WINRT)
static char* rootPath;
#endif

#ifdef WIN32
static int verifyGetExtendedPathCase(const wchar_t *path, const wchar_t *expected)
{
    int failed = 0;
    wchar_t *resp = NULL;
    _VPLFS__GetExtendedPath(path, &resp, 0);
    if (wcscmp(resp, expected) != 0) {
        failed++;
    }
    if (failed) {
        wprintf(L"FAIL %s, expected %s, got %s\n", path, expected, resp);
    }
    free(resp);
    return failed;
}

static void testGetExtendedPath(void)
{
    int nFailed = 0;  // number of failures encountered

    nFailed += verifyGetExtendedPathCase(L"C:\\", L"\\\\?\\C:\\");  //simplest working case
    nFailed += verifyGetExtendedPathCase(L"C:/", L"\\\\?\\C:\\");  // forward slash becomes backslash
    nFailed += verifyGetExtendedPathCase(L"C:\\dir1/dir2", L"\\\\?\\C:\\dir1\\dir2");

    nFailed += verifyGetExtendedPathCase(L"C:/dir1//dir2\\\\dir3/\\dir4///", L"\\\\?\\C:\\dir1\\dir2\\dir3\\dir4\\");  // consecutive slashes are collapsed into one
    nFailed += verifyGetExtendedPathCase(L"C:\\dir1/./dir2\\.\\dir3/.\\dir4", L"\\\\?\\C:\\dir1\\dir2\\dir3\\dir4");  // reference to current directory eliminated

    if (nFailed > 0) {
        VPLTEST_FAIL("testGetExtendedPath");
    }
}

static void testGetKnownFolderPath(void)
{
    wchar_t *wpath = NULL;
    int err = _VPLFS__GetLocalAppDataWPath(&wpath);
    if (err != VPL_OK) {
        VPLTEST_FAIL("testGetKnownFolderPath");
    }
    free(wpath);
}
#endif // WIN32

static void testInvalidParameters(void)
{
    const char* badFileName = "badFileName";
#   if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
    char* badDirName;
    char* badPrefix;
    char* possibleDirName;
    char* nestedDirName;
    char* validFileName;
    char* filePrefix;
#   else
    const char* badDirName = ROOT_PATH"/badDirName";
    const char* badPrefix = ROOT_PATH"/badDirName/folder";
    const char* possibleDirName = ROOT_PATH"/testDir";
    const char* nestedDirName = ROOT_PATH"/testDir/otherDir";
    const char* validFileName = ROOT_PATH"/validFileName";
    const char* filePrefix = ROOT_PATH"/validFileName/folder";
#   endif
    VPLFS_dir_t dir;
    VPLFS_dirent_t dirEntry;
#   ifndef ANDROID
    size_t pos;
#   endif
    VPLFS_stat_t fileStat;

#   if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
    size_t bufSize = sizeof(char) * ((strlen(rootPath) + strlen("/badDirName")))+1;
    badDirName = (char*)malloc( bufSize );
    memset(badDirName, 0, bufSize);
    sprintf(badDirName, "%s%s", rootPath, "/badDirName");

    bufSize = sizeof(char) * ((strlen(rootPath) + strlen("/badDirName/folder")))+1;
    badPrefix = (char*)malloc( bufSize );
    memset(badPrefix, 0, bufSize);
    sprintf(badPrefix, "%s%s", rootPath, "/badDirName/folder");

    bufSize = sizeof(char) * ((strlen(rootPath) + strlen("/testDir")))+1;
    possibleDirName = (char*)malloc( bufSize );
    memset(possibleDirName, 0, bufSize);
    sprintf(possibleDirName, "%s%s", rootPath, "/testDir");

    bufSize = sizeof(char) * ((strlen(rootPath) + strlen("/testDir/otherDir")))+1;
    nestedDirName = (char*)malloc( bufSize );
    memset(nestedDirName, 0, bufSize);
    sprintf(nestedDirName, "%s%s", rootPath, "/testDir/otherDir");

    bufSize = sizeof(char) * ((strlen(rootPath) + strlen("/validFileName")))+1;
    validFileName = (char*)malloc( bufSize );
    memset(validFileName, 0, bufSize);
    sprintf(validFileName, "%s%s", rootPath, "/validFileName");

    bufSize = sizeof(char) * ((strlen(rootPath) + strlen("/validFileName/folder")))+1;
    filePrefix = (char*)malloc( bufSize );
    memset(filePrefix, 0, bufSize);
    sprintf(filePrefix, "%s%s", rootPath, "/validFileName/folder");
#   endif

    memset(&dir, 0, sizeof(dir));
    memset(&dirEntry, 0, sizeof(dirEntry));

    // Create the vaild file for NOTDIR checking.
    {
        FILE* validFile = fopen(validFileName, "w+");
        if(validFile == NULL) {
            VPLTEST_NONFATAL_ERROR("Could not create valid file %s for negative testing. Error code %d.",
                                   validFileName, errno);
        }
        else {
            fclose(validFile);
        }
    }

    // Open dir
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Opendir(possibleDirName, NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Opendir(badDirName, &dir), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Opendir("", &dir), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Opendir(badFileName, &dir), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Opendir(validFileName, &dir), VPL_ERR_NOTDIR);

    // Close dir
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Closedir(&dir), VPL_ERR_BADF);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Closedir(NULL), VPL_ERR_INVALID);

    // Read dir
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Readdir(&dir, &dirEntry), VPL_ERR_BADF);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Readdir(&dir, NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Readdir(NULL, &dirEntry), VPL_ERR_INVALID);

    // Rewind dir
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Rewinddir(&dir), VPL_ERR_BADF);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Rewinddir(NULL), VPL_ERR_INVALID);

#   ifndef ANDROID
    // Seek dir
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Seekdir(&dir, 0), VPL_ERR_BADF);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Seekdir(NULL, 0), VPL_ERR_INVALID);

    // Tell dir
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Telldir(&dir, &pos), VPL_ERR_BADF);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Telldir(NULL, &pos), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Telldir(&dir, NULL), VPL_ERR_INVALID);
#   endif

    // Make dir
    // Make sure that possibleDirName has been removed.
    VPLFS_Rmdir(possibleDirName);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Mkdir(possibleDirName), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Mkdir(possibleDirName), VPL_ERR_EXIST);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Mkdir(badPrefix), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Mkdir(""), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Mkdir(filePrefix), VPL_ERR_NOTDIR);

    // Remove dir
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Rmdir("."), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Rmdir(badDirName), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Rmdir(badPrefix), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Rmdir(""), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Rmdir(filePrefix), VPL_ERR_NOTDIR);
    // Path in Libraries can only be accessed by WinRT APIs
    // WinRT APIs are unable to identify VPL_ERR_NOTDIR & VPL_ERR_NOENT
    // when target's "parent" folder is not valid folder, temporary return VPL_ERR_NOENT
    //VPLTEST_CALL_AND_CHK_RV(VPLFS_Rmdir(filePrefix), VPL_ERR_NOENT);

    // Make a nested dir inside possible dir. Test VPL_ERR_NOT_EMPTY.
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Mkdir(nestedDirName), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Rmdir(possibleDirName), VPL_ERR_NOTEMPTY);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Rmdir(nestedDirName), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Rmdir(possibleDirName), VPL_OK);
    // Path in Libraries can only be accessed by WinRT APIs
    // WinRT API is able to do "rm -rf [dir]" action, returns VPL_OK as expected
    //VPLTEST_CALL_AND_CHK_RV(VPLFS_Rmdir(possibleDirName), VPL_OK);

    // Stat
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Stat(validFileName, NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Stat(badFileName, &fileStat), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Stat("", &fileStat), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Stat(filePrefix, &fileStat), VPL_ERR_NOTDIR);
    // Path in Libraries can only be accessed by WinRT APIs
    // WinRT APIs are unable to identify VPL_ERR_NOTDIR & VPL_ERR_NOENT
    // when target's "parent" folder is not valid folder, temporary return VPL_ERR_NOENT
    //VPLTEST_CALL_AND_CHK_RV(VPLFS_Stat(filePrefix, &fileStat), VPL_ERR_NOENT);

#ifdef WIN32
    // This may legitimately give a different result on Windows and Linux.
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Stat("/", &fileStat), VPL_ERR_NOENT);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Stat("\\", &fileStat), VPL_ERR_NOENT);
    // Win32 only tests
#   ifndef VPL_PLAT_IS_WINRT
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Stat("C:/", &fileStat), VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Stat("C:/Users", &fileStat), VPL_OK);
#   endif
#endif

    // Remove the valid file.
    remove(validFileName);
}

static int writeFile(const char* fileName, const char* str)
{
    int ret = -1;
    FILE* f = NULL;

    // Attempt to remove the file first. It might have been created already,
    // and is read-only.
    remove(fileName);

    f = fopen(fileName, "w+");
    if (f == NULL) {
        VPLTEST_LOG("writeFile() - Cannot open file %s for writing. Error code %d.",
                fileName, errno);
        goto _err;
    }

    ret = (int)fwrite(str, 1, strlen(str), f);
    if (ret != strlen(str)) {
        VPLTEST_LOG("writeFile() - Cannot write file %s.", fileName);
        goto _err;
    }

    ret = 0;
 _err:
    if (f != NULL) {
        if (fclose(f) != 0) {
            VPLTEST_LOG("writeFile() - Cannot close file %s.", fileName);
            ret = -1;
        }
        // Test committing the buffer cache to disk.
        VPLFS_Sync();
    }
    return ret;
}

static int doStatTest(void)
{
#   if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
    char* fileName = NULL;
    char* dirName = NULL;
#   else
    const char* fileName = ROOT_PATH"/test1";
    const char* dirName = ROOT_PATH"/dir1";
#   endif
    const char str[] = "Tim's Mad Dog!!!";
    VPLFS_stat_t fileStat;

#   if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
    size_t nameSize = sizeof(char) * ((strlen(rootPath) + strlen("/test1")))+1;
    fileName = (char*)malloc(nameSize);
    memset(fileName, 0, nameSize);
    sprintf(fileName,"%s%s", rootPath, "/test1");

    size_t dirSize = sizeof(char) * ((strlen(rootPath) + strlen("/dir1")))+1;
    dirName = (char*)malloc( dirSize );
    memset(dirName, 0, dirSize);
    sprintf(dirName, "%s%s", rootPath, "/dir1");
#   endif
    if (writeFile(fileName, str) < 0) {
        VPLTEST_LOG("writeFile(\"%s\") to file %s failed.", str, fileName);
        return -1;
    }

    if (VPLFS_Stat(fileName, &fileStat) < 0) {
        VPLTEST_LOG("Stat file %s failed.", fileName);
        return -1;
    }

    if (fileStat.size != strlen(str)) {
        VPLTEST_LOG("File size "FMTu_size_t" expected, "FMTu_VPLFS_file_size_t" found.",
                strlen(str), fileStat.size);
        return -1;
    }

    VPLTEST_CHK_EQUAL(fileStat.type, VPLFS_TYPE_FILE, "%d", "File type");

    if (VPLFS_Mkdir(dirName) < 0) {
        VPLTEST_LOG("Make directory %s failed.", dirName);
        return -1;
    }

    if (VPLFS_Stat(dirName, &fileStat) < 0) {
        VPLTEST_LOG("Stat directory %s failed.", dirName);
        return -1;
    }

    VPLTEST_CHK_EQUAL(fileStat.type, VPLFS_TYPE_DIR, "%d", "File type");

    if (VPLFS_Rmdir(dirName) < 0) {
        VPLTEST_LOG("Remove directory %s failed.", dirName);
        return -1;
    }

    return 0;
}

static int getDirEntryCount(const char* dirName)
{
    VPLFS_dir_t dir;
    VPLFS_dirent_t dirEntry;
    int count = 0;
    int ret = -1;

    ret = VPLFS_Opendir(dirName, &dir);
    if (ret != VPL_OK) {
        goto _err;
    }

    while (VPLFS_Readdir(&dir, &dirEntry) == VPL_OK) {
        count++;
    }

    // Test to make sure that there are no more entries in the directory.
    VPLTEST_CALL_AND_CHK_RV(VPLFS_Readdir(&dir, &dirEntry), VPL_ERR_MAX);

    ret = count;

    if (VPLFS_Closedir(&dir) < 0) {
        ret = -1;
    }

 _err:
    return ret;
}

#ifndef ANDROID
static int doSeekdirTest(void)
{
    VPLFS_dir_t dir;
    VPLFS_dirent_t dirEntry;
    int ret = -1;
    size_t pos1;
    size_t pos2;

#ifdef WIN32
    // Because we now use extended-length path in Win32, relative paths are no longer allowed.
    // FIXME: support current directory path that cannot be described using ASCII alone
#ifdef VPL_PLAT_IS_WINRT
    char *currentDirectory;
    _VPLFS__GetLocalAppDataPath(&currentDirectory);
#else
    char currentDirectory[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDirectory);
#endif
#else
    char currentDirectory[32] = ".";
#endif
    ret = VPLFS_Opendir(currentDirectory, &dir);
    if (ret != VPL_OK) {
        VPLTEST_LOG("VPLFS_Opendir failed. - %d.", ret);
        goto _err;
    }

    if ((ret = VPLFS_Telldir(&dir, &pos1)) != VPL_OK) {
        VPLTEST_LOG("VPLFS_Telldir failed. - %d.", ret);
        goto _err;
    }

    if ((ret = VPLFS_Readdir(&dir, &dirEntry)) != VPL_OK) {
        VPLTEST_LOG("VPLFS_Readdir failed. - %d.", ret);
        goto _err;
    }

    if (strcmp(dirEntry.filename, ".") != 0) {
        goto _err;
    }

    if ((ret = VPLFS_Telldir(&dir, &pos2)) != VPL_OK) {
        VPLTEST_LOG("VPLFS_Telldir failed. - %d.", ret);
        goto _err;
    }

    VPLFS_Readdir(&dir, &dirEntry);
    if (strcmp(dirEntry.filename, "..") != 0) {
        VPLTEST_LOG("Expected \"..\"");
        goto _err;
    }
    VPLTEST_CHK_EQUAL(dirEntry.type, VPLFS_TYPE_DIR, "%d", "File type for \"..\"");

    if ((ret = VPLFS_Seekdir(&dir, pos2)) != VPL_OK) {
        VPLTEST_LOG("VPLFS_Seekdir failed. - %d.", ret);
        goto _err;
    }

    VPLFS_Readdir(&dir, &dirEntry);
    if (strcmp(dirEntry.filename, "..") != 0) {
        VPLTEST_LOG("Expected \"..\"");
        goto _err;
    }
    VPLTEST_CHK_EQUAL(dirEntry.type, VPLFS_TYPE_DIR, "%d", "File type for \"..\"");

    if ((ret = VPLFS_Seekdir(&dir, pos1)) != VPL_OK) {
        VPLTEST_LOG("VPLFS_Seekdir failed. - %d.", ret);
        goto _err;
    }

    VPLFS_Readdir(&dir, &dirEntry);
    if (strcmp(dirEntry.filename, ".") != 0) {
        VPLTEST_LOG("Expected \".\"");
        goto _err;
    }
    VPLTEST_CHK_EQUAL(dirEntry.type, VPLFS_TYPE_DIR, "%d", "File type for \".\"");

    if ((ret = VPLFS_Rewinddir(&dir)) != VPL_OK) {
        VPLTEST_LOG("VPLFS_Rewinddir failed. - %d.", ret);
        goto _err;
    }

    VPLFS_Readdir(&dir, &dirEntry);
    if (strcmp(dirEntry.filename, ".") != 0) {
        VPLTEST_LOG("Expected \".\"");
        goto _err;
    }
    VPLTEST_CHK_EQUAL(dirEntry.type, VPLFS_TYPE_DIR, "%d", "File type for \".\"");

    ret = 0;
_err:
#ifdef VPL_PLAT_IS_WINRT
    if (currentDirectory != NULL)
        free (currentDirectory);
#endif
    if (VPLFS_Closedir(&dir) < 0) {
        ret = -1;
    }
    return ret;
}
#endif

static int doMkdirTest(void)
{
    int ret;
    int newCount;
#   if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
    int oldCount = getDirEntryCount(rootPath);
    size_t bufSize = sizeof(char) * ((strlen(rootPath) + strlen("/temp_dir1")))+1;
    char* testDirPath1 = (char*)malloc( bufSize );
    memset(testDirPath1, 0, bufSize);
    sprintf(testDirPath1, "%s%s", rootPath, "/temp_dir1");
#   else
    int oldCount = getDirEntryCount(ROOT_PATH);
    const char* rootPath = ROOT_PATH;
    const char* testDirPath1 = ROOT_PATH"/temp_dir1";
#   endif

    if (oldCount < 0) {
        VPLTEST_LOG("getDirEntryCount failed. < 0.");
        return -1;
    }

    ret = VPLFS_Mkdir(testDirPath1);
    if (ret != VPL_OK) {
        VPLTEST_LOG("VPLFS_Mkdir failed. - %d.", ret);
        return -1;
    }

    newCount = getDirEntryCount(rootPath);
    if (newCount < 0) {
        VPLTEST_LOG("newCount < 0. newCount(%d).", newCount);
        return -1;
    }

    if (oldCount + 1 != newCount) {
        VPLTEST_LOG("oldCount(%d) + 1 != newCount(%d).", oldCount, newCount);
        return -1;
    }

    if (VPLFS_Rmdir(testDirPath1) < 0) {
        VPLTEST_LOG("VPLFS_Rmdir failed.");
        return -1;
    }

    newCount = getDirEntryCount(rootPath);
    if (newCount < 0) {
        VPLTEST_LOG("newCount < 0 #2");
        return -1;
    }

    if (oldCount != newCount) {
        VPLTEST_LOG("oldCount != newCount.");
        return -1;
    }

    return 0;
}

static int doUnlinkTest(void)
{
    int newCount;
#   if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
    int oldCount = getDirEntryCount(rootPath);
    size_t bufSize = sizeof(char) * ((strlen(rootPath) + strlen("/test_file1")))+1;
    char* testFilePath1 = (char*)malloc( bufSize );
    memset(testFilePath1, 0, bufSize);
    sprintf(testFilePath1, "%s%s", rootPath, "/test_file1");
#   else
    int oldCount = getDirEntryCount(ROOT_PATH);
    const char* rootPath = ROOT_PATH;
    const char* testFilePath1 = ROOT_PATH"/test_file1";
#   endif

    if (oldCount < 0) {
        return -1;
    }

#   if defined(IOS) || defined(VPL_PLAT_IS_WINRT)
    if (writeFile(testFilePath1, "testtest") < 0) {
        return -1;
    }

    newCount = getDirEntryCount(rootPath);
#   else
    if (writeFile(ROOT_PATH"/test_file1", "testtest") < 0) {
        return -1;
    }

    newCount = getDirEntryCount(ROOT_PATH);
#   endif

    if (newCount < 0) {
        return -1;
    }

    if (oldCount + 1 != newCount) {
        return -1;
    }

    if (remove(testFilePath1) < 0) {
        return -1;
    }

    newCount = getDirEntryCount(rootPath);

    if (newCount < 0) {
        return -1;
    }

    if (oldCount != newCount) {
        return -1;
    }

    return 0;
}

void testVPLFS(void)
{
#   ifdef IOS
    rootPath = getRootPath();
#   elif defined(VPL_PLAT_IS_WINRT)
    getRootPath(&rootPath);
#   endif

#ifdef WIN32
    VPLTEST_LOG("testGetExtendedPath");
    testGetExtendedPath();

    VPLTEST_LOG("testGetKnownFolderPath");
    testGetKnownFolderPath();
#endif

    VPLTEST_LOG("testInvalidParameters.");
    testInvalidParameters();

    VPLTEST_LOG("doStatTest.");
    if (doStatTest()) {
        VPLTEST_FAIL("doStatTest.");
    }

#   ifndef ANDROID
    VPLTEST_LOG("doSeekdirTest.");
    if (doSeekdirTest()) {
        VPLTEST_FAIL("doSeekdirTest.");
    }
#   endif

    VPLTEST_LOG("doMkdirTest.");
    if (doMkdirTest()) {
        VPLTEST_FAIL("doMkdirTest.");
    }

    VPLTEST_LOG("doUnlinkTest.");
    if (doUnlinkTest()) {
        VPLTEST_FAIL("doUnlinkTest.");
    }

#   ifdef VPL_PLAT_IS_WINRT
    releaseRootPath(rootPath);
#   endif
}
