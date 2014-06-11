/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND 
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef __VSSI_HTTP_H__
#define __VSSI_HTTP_H__

#include "vssi_internal.h"

#ifdef __cplusplus
extern "C" {
#endif 

// Default HTTP service port (CCS)
#define HTTP_SERVER_PORT 80

/// Associate each component of the HTTP object to the correct HTTP connection
/// @param object - Object pointer that needs connections
VSSI_Result VSSI_HTTPGetServerConnections(VSSI_ObjectState* object);

/// Send (queue) a read request.
/// @param object - Object pointer, used to get connection.
/// @param name - Object-file name for read
/// @param offset - Read offset from beginning of file
/// @param length - Length of read requested. Also out-param for length read.
/// @param buf - Buffer to receive read data.
/// @param callback - Callback to call when response received.
/// @param ctx - Context for callback.
VSSI_Result VSSI_HTTPRead(VSSI_ObjectState* object,
                          const char* name,
                          u64 offset, 
                          u32* length,
                          char* buf,
                          VSSI_Callback callback,
                          void* ctx);

/// Push pending commands to the server.
/// Must be called after queuing any command, and after a socket potentially
/// has finished with its current command.
void VSSI_SendHTTPCommands(VSSI_HTTPConnectionNode* connection);

/// Receive response on a ready HTTP connection socket
/// @param node The connection node with the ready socket
/// @param id Connection ID for the ready socket of this node.
void VSSI_ReceiveHTTPResponse(VSSI_HTTPConnectionNode* node, int id);

/// Indicate communication error for an HTTP socket.
/// @param node The connection node with communication error.
/// @param id Connection ID for the socket with communication error..
/// @result Error code to return for the lead pending command.
void VSSI_HTTPCommError(VSSI_HTTPConnectionNode* node, int id, int result);

#ifdef __cplusplus
}
#endif 

#endif // include guard
