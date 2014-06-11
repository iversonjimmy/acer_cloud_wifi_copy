#include <vplex_file.h>
#include <vpl_fs.h>
#include <vplu_sstr.hpp>
#include <dx_remote_agent_util.h>
#include "dx_remote_agent_util_linux.h"
#include <string>
#include <sstream>
#include "ccd_core.h"
#include "gvm_file_utils.h"
#include "log.h"
#include <dirent.h>
#include <cerrno>

#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <set>

#include "common_utils.hpp"


#define MAX_PATH    128
#define PATH_MAX_LENGTH         1024
static std::string dx_ccd_name = "";

unsigned short getFreePort()
{
    // Just use port 0 for autoselect.
    return 0;
}

int recordPortUsed(unsigned short port) 
{
    int rv = 0;
    char szModule[MAX_PATH];
    VPLFile_handle_t fd;

    //GetModuleFileNameA(NULL, szModule, MAX_PATH);
    std::string strModule(szModule);
    std::string portFile = strModule.substr(0, strModule.find_last_of('\\') + 1);
    portFile.clear();
    portFile.append("port");
    std::string ccdName = get_dx_ccd_name();
    if (ccdName.length() > 0) {
        portFile.append("_");
        portFile.append(ccdName);
    }

    portFile.append(".txt");
    std::string portToBind;
    std::stringstream ss;
    ss << "port=";
    ss << port;
    portToBind = ss.str();
    ss.str(""); // clear buffer
    ss.clear(); // clear flag

    if(VPLFile_CheckAccess(portFile.c_str(), VPLFILE_CHECKACCESS_EXISTS) == VPL_OK) {
        // File found. Delete it.
        rv = VPLFile_Delete(portFile.c_str());
        if(rv != VPL_OK) {
            LOG_ERROR("Failed to delete %s.", portFile.c_str());
            goto exit;
        }
    }


    fd = VPLFile_Open(portFile.c_str(), VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY, 0777);
    if(! VPLFile_IsValidHandle(fd)) {
        LOG_ERROR("Failed to open %s.", portFile.c_str());
        rv = -1;
        goto exit;
    }
    else {
        rv = VPLFile_Write(fd, portToBind.c_str(), portToBind.size());
        if(rv != portToBind.size()) {
            LOG_ERROR("Failed to write to %s. (%d)", portFile.c_str(), rv);
            goto exit;
        }
        else {
            rv = VPLFile_Close(fd);
            if(rv != VPL_OK) {
                LOG_ERROR("Failed to close to %s. (%d)", portFile.c_str(), rv);
                goto exit;
            }
        }
    }

 exit:
    return rv;
}

//closeFds()
static int closeFds()
{
    int fd = -1;
    int rv = 0;
    DIR* directory = NULL;
    struct dirent* entry = NULL;
    char path[PATH_MAX_LENGTH];

    snprintf(path, sizeof(path), "/proc/%d/fd", (int)getpid());

    directory = opendir(path);
    if (directory == NULL) {
        LOG_ERROR("opendir %s failed: %s", path, strerror(errno));
        goto out;
    }

    while ((entry = readdir(directory)) != NULL) {
        if (strncmp(".",  entry->d_name, 2) == 0 ||
        strncmp("..", entry->d_name, 3) == 0) {
            continue;
        }

        fd = atoi(entry->d_name);
        if (fd > KMSG_FILENO) {
            close(fd);
        }
    }

    closedir(directory);
out:
    directory = NULL;
    entry = NULL;

    return rv;
}

//startCcd()
int startccd(const char* titleId)
{
    int rv = 0;
    std::string tmpPath;

    rv = get_cc_folder(tmpPath);
    if (rv != 0) {
        LOG_ERROR("Fail to get ccd app data path: rv %d", rv);
        return rv;
    }

    LOG_INFO("Launching ccd process....");
    bool valgrindFound = false;
    if (VPLFile_CheckAccess("/usr/bin/valgrind", VPLFILE_CHECKACCESS_EXISTS) == VPL_OK) {
        LOG_INFO("Found /usr/bin/valgrind....");
        valgrindFound = true;
    }

    std::vector<char*> argv;
    std::string logFileArg; // This must have same scope as argv, since argv holds a pointer to its data.
    if(valgrindFound){
        //prepend some valgrind argv
        //refer to ccd_utils_linux.cpp::startCcdWithPath()

        std::string logFile = "valgrind.log";
        logFileArg = SSTR("--log-file=" << logFile);
        LOG_INFO("logFile: %s", logFile.c_str());

        argv.push_back((char*)"/usr/bin/valgrind");
        argv.push_back((char*)"-v");
        argv.push_back((char*)"--suppressions=valgrind.supp");
        argv.push_back((char*)"--gen-suppressions=all");
        argv.push_back((char*)"--leak-check=full");
        argv.push_back((char*)"--track-origins=yes");
        argv.push_back((char*)logFileArg.c_str());
        argv.push_back((char*)"./ccd");
        argv.push_back((char*)tmpPath.c_str());
        argv.push_back((char*)"");
        argv.push_back((char*)titleId);
        argv.push_back(NULL);

        // To get useful results from valgrind, we need to set CCD_FULL_SHUTDOWN.
        if (setenv("CCD_FULL_SHUTDOWN", "1", 1) != 0) {
            LOG_ERROR("***\n*** FAILED to setenv(\"CCD_FULL_SHUTDOWN\"): %s\n***", strerror(errno));
            exit(127);
        }
    }else{
        argv.push_back((char*)"./ccd");
        argv.push_back((char*)tmpPath.data());
        argv.push_back((char*)"");
        argv.push_back((char*)titleId);
        argv.push_back(NULL);
    }
    
    {
        pid_t pid;
        pid = fork();
        if (pid == 0) {
            closeFds();
            int nullout = open("/dev/null", O_WRONLY);
            fflush(stdout);
            fflush(stderr);
            dup2(nullout, STDOUT_FILENO);
            dup2(nullout, STDERR_FILENO);
            close(nullout);
            execv(argv[0], &argv[0]);
            abort();
        }
    }

    return 0;

}

//called by DxRemoteOSAgent
int startccd(int testInstanceNum, const char* titleId)
{
    int rv = 0;
//    #ifdef WIN32
    std::string CcdConfPath;
    //char *ccdConfigPath = NULL;
    char *fileContentBuf = NULL;
    int fileSize = -1;
    std::string config;

    // 1. get ccd config path
    rv = get_cc_folder(CcdConfPath);
    if (rv != 0) {
        LOG_ERROR("Get local appdata wpath failed (%d).", rv);
        goto err;
    }
    CcdConfPath.append("conf/ccd.conf");

    // 2. open the ccd config file
    rv = VPLFile_CheckAccess(CcdConfPath.c_str(), VPLFILE_CHECKACCESS_EXISTS);
    if (rv != VPL_OK) {
        LOG_ERROR("Config file %s doesn't exist! rv = %d", CcdConfPath.c_str(), rv);
        goto err;
    }

    //read conf
    fileSize = Util_ReadFile(CcdConfPath.c_str(), (void**)&fileContentBuf, 0);
    if (fileSize < 0) {
        LOG_ERROR("Fail to read %s, rv = %d", CcdConfPath.c_str(), fileSize);
        rv = fileSize;
        goto err;
    }
    config.assign(fileContentBuf, fileSize);

    // 3. update testInstanceNum to the ccd config
    {
        std::stringstream ss;
        ss << testInstanceNum;
        LOG_ALWAYS("Updating testInstanceNum to %d", testInstanceNum);
        updateConfig(config, "testInstanceNum", ss.str());
    }

    // 4. write back to the config file
    rv = Util_WriteFile(CcdConfPath.c_str(), config.data(), config.size());
    if (rv != VPL_OK) {
        LOG_ERROR("Fail to writeback config to %s, rv = %d", CcdConfPath.c_str(), rv);
        goto err;
    }

    // call startccd() at the end
    startccd(titleId);

 err:
/*
    if (ccdConfigPath != NULL) {
        free(ccdConfigPath);
        ccdConfigPath = NULL;
    }
*/
    if (fileContentBuf != NULL) {
        free(fileContentBuf);
        fileContentBuf = NULL;
    }
//    #endif
    return rv;
}

int stopccd()
{
    int rv = 0;

    std::string ccdName = get_dx_ccd_name();

/*
    if (!ccdName.empty()) {
        ccdi::client::CCDIClient_SetOsUserId(ccdName.c_str());
    }
*/
    ccd::UpdateSystemStateInput request;
    request.set_do_shutdown(true);
    ccd::UpdateSystemStateOutput response;
    rv = CCDIUpdateSystemState(request, response);

    if (rv != CCD_OK) {
        LOG_ERROR("CCDIUpdateSystemState failed: %d", rv);
    }

    return rv;
}

int clean_cc()
{
    return 0;
}

//getCcdAppDataPath
int get_cc_folder(std::string &path)
{
    int rv = -1;
    char *s = getenv("HOME");
    
    path.assign(s);
    path.append("/temp/SyncAgent_");
    
    path += get_dx_ccd_name();
    path += "/";

    LOG_ALWAYS("%s", path.c_str());
    rv = Util_CreatePath(path.c_str(), true);
    if (rv != VPL_OK) {
        LOG_ERROR("Fail to create directory %s, rv = %d", path.c_str(), rv);
        //return -1;
    }

    return 0;

}

int get_user_folder(std::string &path)
{
    int rv = -1;
    get_cc_folder(path);

    path += "dxshell_pushfiles";

    LOG_ALWAYS("%s, %s", __func__, path.c_str());
    rv = Util_CreatePath(path.c_str(), true);
    if (rv != VPL_OK) {
        LOG_ERROR("Fail to create directory %s, rv = %d", path.c_str(), rv);
        //return -1;
    }

    return 0;
}

int startprocess(const std::string& command)
{
    return 0;
}

void set_dx_ccd_name(std::string ccd_name)
{
    dx_ccd_name = ccd_name;
}

std::string get_dx_ccd_name()
{
    return dx_ccd_name;
}



