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
#include <conf.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TEST_BADFILE    "/tmp/conftest_badfile"
#define TEST_FILE0      "/tmp/conftest_file0"
#define TEST_FILE1      "/tmp/conftest_file1"

static const char* testFile0 = {
        "// \"//\" Denotes a single-line comment.\n"
        "// Pair format: <name> = <value>\\n\n"
        "// Empty lines are acceptable.\n"
        "// Type conversion is up to the library user.\n"
        "// All GVM configs can be found in /conf\n"
        "\n"
        "myvar = 38\n"
        "duplicate = 68.93\n"
};

static const char* testFile1 = {
        "// \"//\" Denotes a single-line comment.\n"
        "// Pair format: <name> = <value>\\n\n"
        "// Empty lines are acceptable.\n"
        "// Type conversion is up to the library user.\n"
        "// All GVM configs can be found in /conf\n"
        "\n"
        "myvar2 = myvar2value\n"
        "duplicate = 15\n"
};

unsigned char
isIntEqual(const char* message, int expected, int actual)
{
    unsigned char success = expected == actual;

    if (success) {
        LOG_INFO("PASS: %s, %d, %d", message, expected, actual);
    } else {
        LOG_ERROR("FAIL: %s, %d, %d", message, expected, actual);
    }

    return success;
}

unsigned char
isStringEqual(const char* message, const char* expected, const char* actual)
{
    unsigned char success = strcmp(expected, actual) == 0;

    if (success) {
        LOG_INFO("PASS: %s, %s, %s", message, expected, actual);
    } else {
        LOG_ERROR("FAIL: %s, %s, %s", message, expected, actual);
    }

    return success;
}

int
main(int argc, char** argv)
{
    int rv = 0;
    FILE* file = NULL;
    unsigned char success = 1;
    CONFVariable var;

    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    // setup

    file = fopen(TEST_FILE0, "w");
    fwrite(testFile0, strlen(testFile0), 1, file);
    fclose(file);

    file = fopen(TEST_FILE1, "w");
    fwrite(testFile1, strlen(testFile1), 1, file);
    fclose(file);

    file = NULL;

    // CONFInit not called

    rv = CONFGetVariable(NULL, NULL);
    success &= isIntEqual("CONFGetVariable(NULL, NULL)", CONF_ERROR_PARAMETER, rv);

    rv = CONFGetVariable(&var, NULL);
    success &= isIntEqual("CONFGetVariable(&var, NULL)", CONF_ERROR_PARAMETER, rv);

    rv = CONFGetVariable(NULL, "junk");
    success &= isIntEqual("CONFGetVariable(NULL, junk)", CONF_ERROR_PARAMETER, rv);

    rv = CONFGetVariable(&var, "junk");
    success &= isIntEqual("CONFGetVariable(&var, junk)", CONF_ERROR_INIT, rv);

    rv = CONFLoad(NULL);
    success &= isIntEqual("CONFLoad(NULL)", CONF_ERROR_PARAMETER, rv);

    rv = CONFLoad("junk");
    success &= isIntEqual("CONFLoad(junk)", CONF_ERROR_INIT, rv);

    rv = CONFPrint();
    success &= isIntEqual("CONFPrint()", CONF_ERROR_INIT, rv);

    rv = CONFQuit();
    success &= isIntEqual("CONFQuit()", CONF_ERROR_INIT, rv);

    // CONFInit called

    rv = CONFInit();
    success &= isIntEqual("CONFInit()", CONF_OK, rv);

    rv = CONFLoad(TEST_BADFILE);
    success &= isIntEqual("CONFLoad(TEST_BADFILE)", CONF_ERROR_FOPEN, rv);

    rv = CONFLoad(TEST_FILE0);
    success &= isIntEqual("CONFLoad(TEST_FILE0)", CONF_OK, rv);

    rv = CONFGetVariable(&var, "junk");
    success &= isIntEqual("CONFGetVariable(&var, junk)", CONF_ERROR_NOT_FOUND, rv);

    rv = CONFGetVariable(&var, "myvar");
    success &= isIntEqual("CONFGetVariable(&var, myvar)", CONF_OK, rv);
    success &= isStringEqual("CONFGetVariable(&var, myvar)", "myvar", var.name);
    success &= isStringEqual("CONFGetVariable(&var, myvar)", "38", var.value);

    rv = CONFGetVariable(&var, "duplicate");
    success &= isIntEqual("CONFGetVariable(&var, duplicate)", CONF_OK, rv);
    success &= isStringEqual("CONFGetVariable(&var, duplicate)", "duplicate", var.name);
    success &= isStringEqual("CONFGetVariable(&var, duplicate)", "68.93", var.value);

    rv = CONFPrint();
    success &= isIntEqual("CONFPrint()", CONF_OK, rv);

    rv = CONFLoad(TEST_FILE1);
    success &= isIntEqual("CONFLoad(TEST_FILE1)", CONF_OK, rv);

    rv = CONFGetVariable(&var, "myvar2");
    success &= isIntEqual("CONFGetVariable(&var, myvar2)", CONF_OK, rv);
    success &= isStringEqual("CONFGetVariable(&var, myvar2)", "myvar2", var.name);
    success &= isStringEqual("CONFGetVariable(&var, myvar2)", "myvar2value", var.value);

    rv = CONFGetVariable(&var, "duplicate");
    success &= isIntEqual("CONFGetVariable(&var, duplicate)", CONF_OK, rv);
    success &= isStringEqual("CONFGetVariable(&var, duplicate)", "duplicate", var.name);
    success &= isStringEqual("CONFGetVariable(&var, duplicate)", "15", var.value);

    rv = CONFPrint();
    success &= isIntEqual("CONFPrint()", CONF_OK, rv);

    // clean up

    rv = CONFQuit();
    success &= isIntEqual("CONFQuit()", CONF_OK, rv);

    unlink(TEST_FILE0);
    unlink(TEST_FILE1);

    LOG_INFO("success = %d", success);

    return success;
}
