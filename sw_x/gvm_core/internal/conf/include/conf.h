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
#ifndef __CONF_H__
#define __CONF_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CONF_DEFAULT_SIZE       16

/// Maximum string length (not counting null-terminator).
#define CONF_MAX_STR_LENGTH     255
/// Maximum buffer length.
#define CONF_MAX_LENGTH         (CONF_MAX_STR_LENGTH + 1)

#define CONF_OK                 0
#define CONF_ERROR_PARAMETER    -1
#define CONF_ERROR_INIT         -2
#define CONF_ERROR_NOT_FOUND    -3
#define CONF_ERROR_FOPEN        -4
#define CONF_ERROR_FSEEK        -5
#define CONF_ERROR_FILE_LENGTH  -6

typedef struct {
    char name[CONF_MAX_LENGTH];
    char value[CONF_MAX_LENGTH];
} CONFVariable;

/// "//" Denotes a single-line comment.
/// Pair format: <name> = <value>\n
/// Empty lines are acceptable.
/// Type conversion is up to the library user.
/// All GVM configs can be found in /conf

int CONFGetVariable(CONFVariable* out, const char* name);
int CONFInit(void);
int CONFLoad(const char* fileName);
int CONFPrint(void);
int CONFQuit(void);

#ifdef __cplusplus
}
#endif

#endif // include guard
