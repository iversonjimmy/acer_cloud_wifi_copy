//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "autotest_basic.hpp"
#include "autotest_clouddoc.hpp"
#include "autotest_mediametadata.hpp"
#include "autotest_picstream.hpp"
#include "autotest_picstreamIndexOnly.hpp"
#include "autotest_photo_share.hpp"
#include "autotest_remotefile.hpp"
#include "autotest_streaming.hpp"
#include "autotest_transcode.hpp"

#include "dx_common.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"

#include "autotest.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

const char* AUTOTEST_STR = "AutoTest";

#define TAG_DXRC_HEAD           "<Config=dxshell_rc>"
#define TAG_DXRC_TAIL           "</Config=dxshell_rc>"
#define TAG_DEVICEINFO_HEAD     "<Config=device_info>"
#define TAG_DEVICEINFO_TAIL     "</Config=device_info>"

static std::map<std::string, subcmd_fn> do_g_autotest_cmds;

static int print_autotest_help()
{
    int rv = 0;
    std::map<std::string, subcmd_fn>::iterator it;

    for (it = do_g_autotest_cmds.begin(); it != do_g_autotest_cmds.end(); ++it) {
        const char *argv[2];
        argv[0] = (const char *)it->first.c_str();
        argv[1] = "Help";
        it->second(2, argv);
    }

    return rv;
}

static void autotest_init_commands()
{
    do_g_autotest_cmds["SdkBasicRelease"] = do_autotest_sdk_release_basic;
    do_g_autotest_cmds["SdkMediaMetadataRelease"] = do_autotest_sdk_release_mediametadata;
    do_g_autotest_cmds["SdkMediaMetadataDownloadFromAcs"] = do_autotest_sdk_mediametadata_download_from_acs;
    do_g_autotest_cmds["SdkPicStreamRelease"] = do_autotest_sdk_release_picstream;
    do_g_autotest_cmds["SdkPicStreamIndexOnly"] = do_autotest_sdk_release_picstream_index_only;
    do_g_autotest_cmds["SdkCloudDocRelease"] = do_autotest_sdk_release_clouddochttp;
    do_g_autotest_cmds["SdkRemoteFileRelease"] = do_autotest_sdk_release_remotefile;
    do_g_autotest_cmds["SdkRemoteFileVcs"] = do_autotest_sdk_release_remotefile_vcs;
    do_g_autotest_cmds["SdkTranscodeStreamingPositive"] = do_autotest_regression_streaming_transcode_positive;
    do_g_autotest_cmds["SdkTranscodeStreamingNegative"] = do_autotest_regression_streaming_transcode_negative;
    do_g_autotest_cmds["RegressionStreamingInternalDirect"] = do_autotest_regression_streaming_internal_direct;
    do_g_autotest_cmds["RegressionStreamingProxy"] = do_autotest_regression_streaming_proxy;
    do_g_autotest_cmds["PhotoShare"] = do_autotest_sdk_release_photo_share;
}

int autotest_commands(int argc, const char* argv[])
{
    int rv = 0;

    autotest_init_commands();

    if (argc == 1 || checkHelp(argc, argv)) {
        print_autotest_help();
        return rv;
    }

    if (do_g_autotest_cmds.find(argv[1]) != do_g_autotest_cmds.end()) {
        rv = do_g_autotest_cmds[argv[1]](argc-1, &argv[1]);
    } else {
        LOG_ERROR("Command %s %s not supported", argv[0], argv[1]);
        rv = -1;
    }

    if ( strcmp(argv[1], "DownloadUpdates") == 0 ) {
        LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=%s_%s (Bug 4722)\n", (rv == 0)? "PASS":"EXPECTED_TO_FAIL",  argv[0], argv[1]);
    }
    else {
        LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=%s_%s\n", (rv == 0)? "PASS":"FAIL",  argv[0], argv[1]);
        LOG_ALWAYS("TC_COMPLETE=%s ;;; TC_NAME=%s_%s\n", (rv == 0)? "PASS":"FAIL",  argv[0], argv[1]);
   }

    return rv;
}

int config_autotest(int argc, const char* argv[])
{
    int breakIdx = 0;
    int configIdx = 0;
    int remarkIdx = 0;
    std::string line;
    //std::fstream cfgfile ("./dxshellrc.cfg");
    std::fstream cfgfile;
    std::fstream writeFile;
    std::vector<std::string> otherResult;
    std::vector<std::string> configResult;
    std::vector<std::string> contentResult;
    VPLFS_stat_t stat;
    std::string curPath;
    getCurDir(curPath);
#ifdef WIN32
    curPath += "\\";
#else
    curPath += "/";
#endif
    curPath += std::string("dxshellrc.cfg");

    if (checkHelp(argc, argv)) {
        std::cout << "AutoTest " << argv[0] << " Configuration Usage: " << std::endl;
        std::cout << "  Add/Revise Device config: dxshell.exe AutoTest ConfigAutoTest <alias name> <OS> <username> <IP> <Controller IP>" << std::endl;
        return -1;
    }

    if (VPLFS_Stat(curPath.c_str(), &stat) != VPL_OK) {
        std::ofstream sampleFile (curPath.c_str());
        sampleFile << TAG_DXRC_HEAD << std::endl << TAG_DXRC_TAIL << std::endl
                   << TAG_DEVICEINFO_HEAD << std::endl << TAG_DEVICEINFO_TAIL << std::endl;

        sampleFile.close();
    }

    cfgfile.open("dxshellrc.cfg");

    if ( (argc == 2) && (strcmp(argv[1], "CleanUp") == 0)) {
        if (cfgfile.is_open()) {
            while (cfgfile.good())
            {
                std::getline(cfgfile,line);                    

                configIdx = line.find(TAG_DXRC_HEAD);
                if (configIdx != std::string::npos) {
                    configResult.push_back(line);
                    while (cfgfile.good())
                    {
                        std::getline(cfgfile,line);
                        remarkIdx = line.find("#");
                        if (remarkIdx != std::string::npos) {
                            configResult.push_back(line);
                        }
                        else {
                            breakIdx = line.find(TAG_DXRC_TAIL);
                            if (breakIdx == std::string::npos) {
                                continue;
                            }
                            else {
                                break;
                            }    
                        }
                    }
                }
                otherResult.push_back(line);        
            }
        }
        else {
            std::cout << "[Error]Unable to open dxshellrc.cfg" << std::endl;    
            return -1;    
        }

        for(std::vector<std::string>::iterator iter=configResult.begin();iter!=configResult.end();++iter)
        {
            contentResult.push_back(*iter);
        }
            
        for(std::vector<std::string>::iterator iter=otherResult.begin();iter!=otherResult.end();++iter)
        {
            contentResult.push_back(*iter);
        }
    }
    else if ( (argc == 3) && (strcmp(argv[1], "CleanUp") == 0)) {      
        if (cfgfile.is_open()) {
            while (cfgfile.good())
            {
                std::getline(cfgfile,line);           

                configIdx = line.find(TAG_DXRC_HEAD);
                if (configIdx != std::string::npos) {
                    configResult.push_back(line);
                    while (cfgfile.good())
                    {
                        std::getline(cfgfile,line);
                        breakIdx = line.find(TAG_DXRC_TAIL);
                        if (breakIdx == std::string::npos) {
                            configResult.push_back(line);
                        }
                        else {
                            break;
                        }                
                    }
                }
                otherResult.push_back(line);        
            }
        }
        else {
            std::cout << "[Error]Unable to open dxshellrc.cfg" << std::endl;    
            return -1;    
        }

        int index = 0;

        for(std::vector<std::string>::iterator iter=configResult.begin();iter!=configResult.end();++iter)
        {
            std::string configData = *iter;

            index = configData.find(",");
            std::string deviceName = configData.substr(0, index);
            if (strcmp(deviceName.c_str(), argv[2]) != 0)
            {
                contentResult.push_back(configData);
            }
        }

        for(std::vector<std::string>::iterator iter=otherResult.begin();iter!=otherResult.end();++iter)
        {
            contentResult.push_back(*iter);
        }

    }
    else if ( ( (argc == 5) && ( (strcmp(argv[2], OS_WINDOWS) == 0) || (strcmp(argv[2], OS_WINDOWS_RT) == 0))) || 
              ( (argc == 5) && ( (strcmp(argv[2], OS_LINUX) == 0)))  || 
              ( (argc == 5) && ( (strcmp(argv[2], OS_ORBE) == 0)))  || 
              ( (argc == 7) && ( (strcmp(argv[2], OS_ANDROID) == 0)))  || 
              ( (argc == 7) && ( (strcmp(argv[2], OS_iOS) == 0))) ){
        
        int index = 0;
        int count = 0;
        bool haveUpdate = false;
        std::string newData = "";        

        if (cfgfile.is_open()) {
            while (cfgfile.good())
            {
                std::getline(cfgfile,line);    
            
                std::string configStartInfo = "<Config=dxshell_rc>";
                std::string configStopInfo = "</Config=dxshell_rc>";

                configIdx = line.find(configStartInfo);
                if (configIdx != std::string::npos) {
                    configResult.push_back(line);
                    while (cfgfile.good())
                    {        
                        getline(cfgfile,line);  
                        breakIdx = line.find(configStopInfo);
                        if (breakIdx == std::string::npos) {
                            configResult.push_back(line);
                        }
                        else {
                            break;
                        }    
                    }
                }
                otherResult.push_back(line);        
            }
        }
        else {
            std::cout << "[Error]Unable to open dxshellrc.cfg" << std::endl;    
            return -1;    
        }    

        for(std::vector<std::string>::iterator iter=configResult.begin();iter!=configResult.end();++iter)
        {
            ++count;
            std::string configData = *iter;

            index = configData.find(",");
            std::string deviceName = configData.substr(0, index);
            if (strcmp(configData.c_str(), "") != 0) {
                if (strcmp(deviceName.c_str(), argv[1]) != 0) {
                    contentResult.push_back(configData);
                }
                else {
                    haveUpdate = true;
                    for (int i = 0; i < argc - 1; i++) 
                    {
                        newData = newData + argv[i + 1] + ",";
                    }

                    contentResult.push_back(newData);
                }
            }
        }
        
        if (!haveUpdate) {
            newData = "";
            for (int i = 0; i < argc - 1; i++) 
            {
                newData = newData + argv[i + 1] + ",";
            }

            contentResult.push_back(newData);
        }

        
        for(std::vector<std::string>::iterator iter=otherResult.begin();iter!=otherResult.end();++iter)
        {
            contentResult.push_back(*iter);
        }

    }
    else {
        LOG_ERROR("Unexpected number of arguments.  Received %d, expected at least 2 args. Please refer help for details", argc);
        return -1;        
    }
    
    cfgfile.close();
    writeFile.open("./dxshellrc.cfg", std::ios::out);

    for(std::vector<std::string>::iterator iter=contentResult.begin();iter!=contentResult.end();++iter)
    {
        writeFile << *iter << std::endl;
    }
    writeFile.close();

    return 0;
}

int config_device(int argc, const char* argv[])
{
    int rv = 0;
    int breakIdx = 0;
    int configIdx = 0;
    int deviceIdx = 0;
    std::string line;
    std::fstream cfgfile;
    std::fstream writeFile;
    std::vector<std::string> configResult;
    std::vector<std::string> deviceResult;
    std::vector<std::string> contentResult;
    VPLFS_stat_t stat;
    std::string cmdAliasName;
    std::string cmdDevName;
    std::string curPath;
    std::string configStartInfo = std::string(TAG_DXRC_HEAD);
    std::string configStopInfo = std::string(TAG_DXRC_TAIL);
    std::string deviceStartInfo = std::string(TAG_DEVICEINFO_HEAD);
    std::string deviceStopInfo = std::string(TAG_DEVICEINFO_TAIL);
    if (argc != 3 || checkHelp(argc, argv)) {
        std::cout << "AutoTest " << argv[0] << " Configuration Usage: " << std::endl;
        std::cout << "  Add/Revise Device config: dxshell.exe AutoTest ConfigDevice <alias name> <machine name>" << std::endl;
        if(checkHelp(argc, argv)) {
            return 0;
        } else {
            return -1;
        }
    }

    getCurDir(curPath);
#ifdef WIN32
    curPath += "\\";
#else
    curPath += "/";
#endif
    curPath += std::string("dxshellrc.cfg");

    if (VPLFS_Stat(curPath.c_str(), &stat) != VPL_OK) {
        std::ofstream sampleFile (curPath.c_str());
        sampleFile << TAG_DXRC_HEAD << std::endl << TAG_DXRC_TAIL << std::endl
                   << TAG_DEVICEINFO_HEAD << std::endl << TAG_DEVICEINFO_TAIL << std::endl;

        sampleFile.close();
    }

    cmdAliasName = std::string(argv[1]);
    cmdDevName = std::string(argv[2]);

    cfgfile.open("dxshellrc.cfg");

    do {
        if (!cfgfile.is_open()) {
            std::cout << "Open config file failed" << std::endl;
            rv = 1;
            break;
        }
        
        std::string::size_type index = 0;
        bool update = false;
        std::string newData;

        while (cfgfile.good()) {
            std::getline(cfgfile, line);    
            

            configIdx = line.find(configStartInfo);
            if (configIdx != std::string::npos) {
                configResult.push_back(line);
                while (cfgfile.good()) {
                    getline(cfgfile, line);
                    configResult.push_back(line);
                    breakIdx = line.find(configStopInfo);
                    if (breakIdx != std::string::npos) {
                        break;
                    }
                }
            }

            while (cfgfile.good()) {
                std::getline(cfgfile, line);
                deviceIdx = line.find(deviceStartInfo);
                if (deviceIdx != std::string::npos) {
                    deviceResult.push_back(line);
                    while (cfgfile.good()) {
                        std::getline(cfgfile,line);
                        deviceResult.push_back(line);
                        breakIdx = line.find(deviceStopInfo);
                        if (breakIdx != std::string::npos) {
                            break;
                        }
                    }
                }
            }
        }
        
        for (std::vector<std::string>::iterator iter=configResult.begin();iter != configResult.end(); ++iter) {
            contentResult.push_back(*iter);
        }

        for (std::vector<std::string>::iterator iter=deviceResult.begin();iter != deviceResult.end();++iter) {
            std::string deviceData = *iter;
            index = deviceData.find(",");
            if (index == std::string::npos) {
                continue;
            }

            std::string aliasName = deviceData.substr(0, index);
            if (aliasName.compare(cmdAliasName) == 0) {
                update = true;
                break;
            }
        }

        for (std::vector<std::string>::iterator iter=deviceResult.begin(); iter != deviceResult.end(); ++iter) {
            std::string deviceData = *iter;

            if (deviceData.compare(deviceStopInfo) == 0) {
                if (!update) {
                    newData = cmdAliasName + std::string(",") + cmdDevName;
                    contentResult.push_back(newData);
                }
                contentResult.push_back(deviceData);
                break;
            }

            index = deviceData.find(",");
            if (index == std::string::npos) {
                contentResult.push_back(deviceData);
                continue;
            }

            std::string aliasName = deviceData.substr(0, index);
            if (aliasName.compare(cmdAliasName) != 0) {
                contentResult.push_back(deviceData);
                continue;
            }

            newData = cmdAliasName + std::string(",") + cmdDevName;
            contentResult.push_back(newData);
        }

        cfgfile.close();
        writeFile.open("./dxshellrc.cfg", std::ios::out);

        for (std::vector<std::string>::iterator iter=contentResult.begin(); iter != contentResult.end(); ++iter) {
            writeFile << *iter << std::endl;
        }
        writeFile.close();
    } while (false);

    return rv;
}
