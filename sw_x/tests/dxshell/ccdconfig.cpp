#include <vplu_types.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include <log.h>
#include <vpl_plat.h>
#include <gvm_file_utils.h>

#include "common_utils.hpp"
#include "ccd_utils.hpp"
#include "dx_common.h"
#include "ccdconfig.hpp"
#include "TargetDevice.hpp"

static int getDefaultConfig(std::string &config)
{
    int rc = 0;
    std::string ccdConfTmplPath;
    char *confbuf = NULL;
    int confsize = 0;

    getCurDir(ccdConfTmplPath);
    ccdConfTmplPath.append(DIR_DELIM CCD_CONF_TMPL_FILE);
    confsize = Util_ReadFile(ccdConfTmplPath.c_str(), (void**)&confbuf, 0);
    if (confsize < 0) {
        rc = confsize;
        LOG_ERROR("Failed to read from %s: %d", ccdConfTmplPath.c_str(), rc);
        goto end;
    }
    config.assign(confbuf, confsize);

    updateConfig(config, "infraDomain", "cloud.acer.com");  // set default domain

    if (testInstanceNum) {
        std::ostringstream testInstanceNumSS;
        testInstanceNumSS << testInstanceNum;
        updateConfig(config, "testInstanceNum", testInstanceNumSS.str());
    }

 end:
    if (confbuf != NULL) {
        free(confbuf);
    }
    return rc;
}

int ccdconfig_set(const std::string &key, const std::string &value)
{
    int rc = 0;
    TargetDevice *target = NULL;
    std::string config;

    target = getTargetDevice();
    rc = target->pullCcdConfig(config);
    if (rc != 0) {
        LOG_ERROR("Failed to pull ccd.conf from target device: %d", rc);
        goto end;
    }

    if (config.empty()) {
        rc = getDefaultConfig(config);
        if (rc != 0) {
            LOG_ERROR("Failed to read the default ccd config file: %d", rc);
            goto end;
        }
    }

    updateConfig(config, key, value);

    rc = target->pushCcdConfig(config);
    if (rc != 0) {
        LOG_ERROR("Failed to push ccd.conf to target device: %d", rc);
        goto end;
    }

 end:
    if (target != NULL) {
        delete target;
    }
    return rc;
}

int ccdconfig_get(const std::string &key, std::string &val)
{
    int rc = 0;

    LOG_ERROR("NOT YET IMPLEMENTED");

    return rc;
}

int ccdconfig_pull(const std::string &filepath)
{
    int rc = 0;
    TargetDevice *target = NULL;
    std::string config;

    target = getTargetDevice();
    rc = target->pullCcdConfig(config);
    if (rc != 0) {
        LOG_ERROR("Failed to pull ccd.conf from target device: %d", rc);
        goto end;
    }

    if (config.empty())
        getDefaultConfig(config);

    rc = Util_WriteFile(filepath.c_str(), config.data(), config.size());
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to write ccd.conf to %s: %d", filepath.c_str(), rc);
        goto end;
    }

 end:
    if (target != NULL) {
        delete target;
    }
    return rc;
}

int ccdconfig_push(const std::string &filepath)
{
    int rc = 0;
    char *confbuf = NULL;
    int confsize;
    std::string config;
    TargetDevice *target = NULL;

    confsize = Util_ReadFile(filepath.c_str(), (void**)&confbuf, 0);
    if (confsize < 0) {
        rc = confsize;
        LOG_ERROR("Failed to read from %s: %d", filepath.c_str(), rc);
        goto end;
    }
    config.assign(confbuf, confsize);

    target = getTargetDevice();
    rc = target->pushCcdConfig(config);
    if (rc != 0) {
        LOG_ERROR("Failed to push %s as ccd.conf to target device: %d", filepath.c_str(), rc);
        goto end;
    }

 end:
    if (confbuf != NULL) {
        free(confbuf);
    }
    if (target != NULL) {
        delete target;
    }
    return rc;
}

int ccdconfig_reset()
{
    int rc = 0;
    TargetDevice *target = NULL;
    std::string config;

    target = getTargetDevice();

    rc = getDefaultConfig(config);
    if (rc != 0) {
        LOG_ERROR("Failed to read the default ccd config file: %d", rc);
        goto end;
    }

    rc = target->pushCcdConfig(config);
    if (rc != 0) {
        LOG_ERROR("Failed to push default ccd.conf to target device: %d", rc);
        goto end;
    }

 end:
    if (target != NULL) {
        delete target;
    }
    return rc;
}

//--------------------------------------------------
// dispatcher

static int ccdconfig_help(int argc, const char* argv[])
{
    // argv[0] == "CCDConfig" if called from dispatch_ccdconfig_cmd()
    // Otherwise, called from ccdconfig_*() handler function

    bool printAll = strcmp(argv[0], "CCDConfig") == 0 || strcmp(argv[0], "Help") == 0;

    if (printAll || strcmp(argv[0], "Set") == 0)
        std::cout << "CCDConfig Set key value" << std::endl;
    if (printAll || strcmp(argv[0], "Get") == 0)
        std::cout << "CCDConfig Get key" << std::endl;
    if (printAll || strcmp(argv[0], "Pull") == 0)
        std::cout << "CCDConfig Pull filepath" << std::endl;
    if (printAll || strcmp(argv[0], "Push") == 0)
        std::cout << "CCDConfig Push filepath" << std::endl;
    if (printAll || strcmp(argv[0], "Reset") == 0)
        std::cout << "CCDConfig Reset" << std::endl;

    return 0;
}

static int ccdconfig_reset(int argc, const char* argv[])
{
    if (argc > 1) {
        LOG_ERROR("Too many arguments.  Usage:");
        ccdconfig_help(argc, argv);
        return -1;
    }

    return ccdconfig_reset();
}

static int ccdconfig_set(int argc, const char* argv[])
{
    if (argc < 3) {
        LOG_ERROR("Too few arguments.  Usage:");
        ccdconfig_help(argc, argv);
        return -1;
    }

    return ccdconfig_set(argv[1], argv[2]);
}

static int ccdconfig_get(int argc, const char* argv[])
{
    if (argc < 2) {
        LOG_ERROR("Too few arguments.  Usage:");
        ccdconfig_help(argc, argv);
        return -1;
    }

    std::string val;
    return ccdconfig_get(argv[1], val);
}

static int ccdconfig_pull(int argc, const char* argv[])
{
    if (argc < 2) {
        LOG_ERROR("Too few arguments.  Usage:");
        ccdconfig_help(argc, argv);
        return -1;
    }

    std::string val;
    return ccdconfig_pull(argv[1]);
}

static int ccdconfig_push(int argc, const char* argv[])
{
    if (argc < 2) {
        LOG_ERROR("Too few arguments.  Usage:");
        ccdconfig_help(argc, argv);
        return -1;
    }

    std::string val;
    return ccdconfig_push(argv[1]);
}

class CCDConfigDispatchTable {
public:
    CCDConfigDispatchTable() {
        cmds["Set"]   = ccdconfig_set;
        cmds["Get"]   = ccdconfig_get;
        cmds["Pull"]  = ccdconfig_pull;
        cmds["Push"]  = ccdconfig_push;
        cmds["Reset"] = ccdconfig_reset;
        cmds["Help"]  = ccdconfig_help;
    }
    std::map<std::string, subcmd_fn> cmds;
};

static CCDConfigDispatchTable ccdConfigDispatchTable;

int dispatch_ccdconfig_cmd(int argc, const char* argv[])
{
    if (argc <= 1)
        return ccdconfig_help(argc, argv);
    else
        return dispatch(ccdConfigDispatchTable.cmds, argc - 1, &argv[1]);
}

