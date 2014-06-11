/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */
#include <log.h>
#include <gvm_utils.h>
#include <vpl_error.h>

#include <conf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

static char __confInit = 0;
static int __confFilled = 0;
static int __confSize = CONF_DEFAULT_SIZE;
static CONFVariable* __confVars = NULL;

static inline char *
safe_strtok(char *str, const char *delim, char **saveptr)
{
#ifdef WIN32
    // according to http://msdn.microsoft.com/en-us/library/2c8d19sb.aspx
    // strtok() uses TLS to parse the string so it's thread safe
    UNUSED(saveptr);
    return strtok(str, delim);
#else
    return strtok_r(str, delim, saveptr);
#endif
}

static int
CONFCompare(const void* p1, const void* p2)
{
    CONFVariable* var1 = (CONFVariable*) p1;
    CONFVariable* var2 = (CONFVariable*) p2;
    return strcmp(var1->name, var2->name);
}

static int
CONFOverWriteDuplicate(CONFVariable* var)
{
    int i;
    int rv = CONF_ERROR_NOT_FOUND;

    if (var == NULL) {
        LOG_ERROR("var = NULL");
        rv = CONF_ERROR_PARAMETER;
        goto out;
    }

    for (i = 0; i < __confFilled; i++) {
        if (strncmp(var->name, __confVars[i].name, CONF_MAX_LENGTH) == 0) {
            memcpy(&__confVars[i], var, sizeof(CONFVariable));
            rv = CONF_OK;
            goto out;
        }
    }

out:
    return rv;
}

int
CONFGetVariable(CONFVariable* out, const char* name)
{
    int i;
    int rv = CONF_OK;
    CONFVariable var;
    CONFVariable* foundVar = NULL;

    LOG_DEBUG("CONFGetVariable %s", name);

    if (out == NULL || name == NULL) {
        LOG_ERROR("out = NULL | name = NULL");
        rv = CONF_ERROR_PARAMETER;
        goto out;
    }

    if (!__confInit) {
        LOG_ERROR("libconf not initialized");
        rv = CONF_ERROR_INIT;
        goto out;
    }

    memset(&var, 0, sizeof(var));
    strncpy(var.name, name, CONF_MAX_LENGTH);

    for (i = 0; i < CONF_MAX_LENGTH; i++) {
        var.name[i] = tolower(var.name[i]);
    }

    foundVar = bsearch(&var, __confVars, __confFilled, sizeof(CONFVariable), CONFCompare);
    if (foundVar == NULL) {
        rv = CONF_ERROR_NOT_FOUND;
        goto out;
    }

    memcpy(out, foundVar, sizeof(CONFVariable));

out:
    foundVar = NULL;

    return rv;
}

int
CONFInit(void)
{
    LOG_DEBUG("CONFInit");

    if (__confInit) {
        goto out;
    }

    __confFilled = 0;
    __confInit = 1;
    __confSize = CONF_DEFAULT_SIZE;
    __confVars = (CONFVariable*)malloc(__confSize * sizeof(CONFVariable));
    memset(__confVars, 0, __confSize * sizeof(CONFVariable));

out:
    return CONF_OK;
}

int
CONFLoad(const char* fileName)
{
    int i;
    int rv = CONF_OK;
    char* line = NULL;
    int length = 0;
    char* buffer = NULL;
    void* vbuffer = NULL;
    char* savePtr = NULL;

    LOG_DEBUG("CONFLoad %s", fileName);

    if (fileName == NULL) {
        LOG_ERROR("fileName = NULL");
        rv = CONF_ERROR_PARAMETER;
        goto out;
    }

    if (__confInit == 0) {
        LOG_ERROR("libconf not initialized");
        rv = CONF_ERROR_INIT;
        goto out;
    }

    LOG_INFO("Reading configuration from %s", fileName);
    length = Util_ReadFile(fileName, &vbuffer, 1);
    if (length < 0) {
        if (length == VPL_ERR_NOENT) {
            LOG_INFO("%s does not exist; skipping", fileName);
        } else {
            LOG_ERROR("Util_ReadFile(%s) failed: %d", fileName, length);
        }
        rv = length;
        goto out;
    }

    buffer = vbuffer;
    // Add a null-terminator so strtok doesn't read past the end of the buffer.
    buffer[length] = '\0';
    
    line = safe_strtok(buffer, "\n", &savePtr);
    while (line != NULL) {
        CONFVariable var;
        size_t lineLen = strlen(line);
        
        if (lineLen < 2 || (line[0] == '/' && line[1] == '/')) {
            goto skip;
        }

        memset(&var, 0, sizeof(var));
        sscanf(line, "%"VPL_STRING(CONF_MAX_STR_LENGTH)"s = %"VPL_STRING(CONF_MAX_STR_LENGTH)"s\n",
                var.name, var.value);
        LOG_DEBUG("name,value = '%s','%s'", var.name, var.value);

        // Check if the length of name and value are less than buffer size.
        // 3 is the length of ' = ' in configuration format.
        // Still regard as success but put an error in log.
        // Add 3 (instead of subtracting 3) to avoid wrapping around if lineLen == strlen(var.name),
        // which can happen if there is no ' = ' in the line.
        if ( (lineLen - strlen(var.name) > CONF_MAX_STR_LENGTH + 3) ||
             (lineLen - strlen(var.value) > CONF_MAX_STR_LENGTH + 3)   ) {
            LOG_ERROR("Configuration exceeds buffer size: '%s'", line);
        }

        for (i = 0; i < CONF_MAX_LENGTH; i++) {
            var.name[i] = tolower(var.name[i]);
        }

        // overwrite duplicate entries
        if (CONFOverWriteDuplicate(&var) == CONF_ERROR_NOT_FOUND) {
            memcpy(&__confVars[__confFilled], &var, sizeof(var));
            __confFilled++;
        }

        // grow array
        if (__confFilled >= __confSize) {
            CONFVariable* tempVars;
            __confSize *= 2;
            tempVars = __confVars;
            __confVars = (CONFVariable*)malloc(__confSize * sizeof(CONFVariable));
            memset(__confVars, 0, __confSize * sizeof(CONFVariable));
            memcpy(__confVars, tempVars, __confFilled * sizeof(CONFVariable));
            free(tempVars);
        }

skip:
        line = safe_strtok(NULL, "\n", &savePtr);
    }

    qsort(__confVars, __confFilled, sizeof(CONFVariable), CONFCompare);

out:
    free(buffer);

    return rv;
}

int
CONFPrint(void)
{
    int rv = CONF_OK;

    LOG_DEBUG("CONFPrint");

    if (__confInit == 0) {
        LOG_ERROR("libconf not initialized");
        rv = CONF_ERROR_INIT;
        goto out;
    }

    {
        int i;
        for (i = 0; i < __confFilled; i++) {
            LOG_INFO("%s = %s", __confVars[i].name, __confVars[i].value);
        }
    }

out:
    return rv;
}

int
CONFQuit(void)
{
    int rv = CONF_OK;

    LOG_DEBUG("CONFQuit");

    if (__confInit == 0) {
        LOG_ERROR("libconf not initialized");
        rv = CONF_ERROR_INIT;
        goto out;
    }

    __confFilled = 0;
    __confSize = 0;
    free(__confVars);
    __confVars = NULL;
    __confInit = 0;
    
out:
    return rv;
}
