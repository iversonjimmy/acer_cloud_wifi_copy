//
//  Copyright 2012 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string>
#include <gvm_rm_utils.hpp>


using namespace std;


#define CCD_CONF_TEMPL_FILE_NAME    "ccd.conf.tmpl"
#define CCD_TEMP_DIR_PATH           "/temp"
#define CCD_SYNC_AGENT_DIR_PATH     "/temp/SyncAgent"
#define CCD_CONF_DIR_PATH           "/temp/SyncAgent/conf"
#define CCD_CONF_FILE_PATH          "/temp/SyncAgent/conf/ccd.conf"
#define CCD_DEVICE_CRED_PATH        "/temp/SyncAgent/cc"

#define CCD_DIR_PERM                (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)


void printUsage();


void
printUsage()
{
    printf("Usage: actool [-g domain userGroup] [-d]\n");
    printf("Use -g to set the domain and user group (optional)\n");
    printf("Use -d to use default configuration\n");
    printf("Only one of -g or -d can be specified\n");
}


int
main(int argc, const char *argv[])
{
    int rv = 0;
    string tempDirPath, syncAgentDirPath, confDirPath, ccdConfigFilePath;
    string devCredPath;
    string option;
    string domain;
    string group;
    string conf;
    FILE *fp = NULL;
    int fSize, nBytes, pos;
    char *buf = NULL;

    if (argc < 2) {
        printUsage();
        rv = -1;
        goto end;
    }
    option = argv[1];

    tempDirPath = getenv("HOME");
    tempDirPath.append(CCD_TEMP_DIR_PATH);

    syncAgentDirPath = getenv("HOME");
    syncAgentDirPath.append(CCD_SYNC_AGENT_DIR_PATH);

    confDirPath = getenv("HOME");
    confDirPath.append(CCD_CONF_DIR_PATH);

    ccdConfigFilePath = getenv("HOME");
    ccdConfigFilePath.append(CCD_CONF_FILE_PATH);

    if (option.compare("-g") == 0) {
        if (argc < 3) {
            printUsage();
            rv = -1;
            goto end;
        }
        domain = argv[2];
        group.clear();
        if (argc == 4) {
            group = argv[3];
        }

        fp = fopen(CCD_CONF_TEMPL_FILE_NAME, "r");
        if (fp == NULL) {
            printf("Failed to open CCD config template file\n");
            rv = -1;
            goto end;
        }

        fseek(fp, 0, SEEK_END);
        fSize = ftell(fp);
        rewind(fp);

        buf = (char *) malloc(fSize);
        if (buf == NULL) {
            printf("Failed to allocate %d bytes\n", fSize);
            rv = -1;
            goto end;
        }

        nBytes = fread(buf, sizeof(char), fSize, fp);
        if (nBytes != fSize) {
            printf("Failed to read CCD config template file\n");
            rv = -1;
            goto end;
        }
        conf = buf;

        free(buf);
        buf = NULL;
        fclose(fp);
        fp = NULL;

        pos = conf.find("${DOMAIN}");
        if (pos == string::npos) {
            printf("CCD config template not in expected format\n");
            rv = -1;
            goto end;
        }
        conf.replace(pos, sizeof("${DOMAIN}") - 1, domain);
        
        pos = conf.find("${GROUP}");
        if (pos == string::npos) {
            printf("CCD config template not in expected format\n");
            rv = -1;
            goto end;
        }
        conf.replace(pos, sizeof("${GROUP}") - 1, group);

        mkdir(tempDirPath.c_str(), CCD_DIR_PERM);
        mkdir(syncAgentDirPath.c_str(), CCD_DIR_PERM);
        mkdir(confDirPath.c_str(), CCD_DIR_PERM);

        fp = fopen(ccdConfigFilePath.c_str(), "w+t");
        if (fp == NULL) {
            printf("Failed to open CCD config file\n");
        }

        nBytes = fwrite(conf.c_str(), 1, conf.length(), fp);
        if (nBytes != (conf.length())) {
            printf("Failed to write CCD config file\n");
            rv = -1;
            goto end;
        }
    } else if (option.compare("-d") == 0) {
        remove(ccdConfigFilePath.c_str());
    } else {
        printUsage();
        rv = -1;
        goto end;
    }

    devCredPath = getenv("HOME");
    devCredPath.append(CCD_DEVICE_CRED_PATH);
    (void) Util_rmRecursive(devCredPath, tempDirPath);

end:
    if (fp != NULL) {
        fclose(fp);
    }
    if (buf != NULL) {
        free(buf);
    }

    return rv;
}
