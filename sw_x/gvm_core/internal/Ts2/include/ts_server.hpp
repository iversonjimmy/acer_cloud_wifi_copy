/*
 *  Copyright 2013 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

///
/// ts_server.hpp
///
/// Tunnel Service Server Interface
///
#ifndef __TS_SERVER_HPP__
#define __TS_SERVER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <string>
#include <list>
#include "ts_client.hpp"

#if defined(IS_TS_WRAPPER)
namespace TS_WRAPPER {
#elif defined(IS_TS)
namespace TS {
#endif

/// Initialize the TS internal server state variables
/// @param userId - user id for this server
/// @param deviceId - device id for this server
/// @param instanceId - instance id for this server
/// @param error_msg - returned error message on failure.
/// @return TS_OK on successful init.
///         TS error code on failure and error_msg set.
/// @note initializes internal state. Subsequent calls have no effect unless
/// #TS_ServerShutdown() was called to release all state.
/// @note TS spawns a thread to service new the connection port.
TSError_t TS_ServerInit(u64 userId,
                        u64 deviceId,
                        u32 instanceId,
                        Ts2::LocalInfo* local_info,
                        string& error_msg);

/// Clean up TS internal server state for system shutdown.
/// @note All internal state is cleared.
void TS_ServerShutdown(void);

/// Register a set of services to the specified handler
/// @param parms - service information and callback
/// @param ret_service_handle - handle identifying the registered service
/// @param error_msg - returned error message on failure.
/// @return TS_OK on success and ret_service_handle set.
///         TS error code on failure and error_msg set.
TSError_t TS_RegisterService(TSServiceParms_t& parms,
                             TSServiceHandle_t& ret_service_handle,
                             string& error_msg);

/// Deregister a set of services associated with the service handle
/// @param service_handle - handle returned by TS_RegisterService()
/// @param error_msg - returned error message on failure.
/// @return TS_OK on success
///         TS error code on failure and error_msg set.
TSError_t TS_DeregisterService(TSServiceHandle_t& service_handle,
                               string& error_msg);

/// These functions are defined to prevent name collisions between
/// client and server libraries.

/// read data from a tunnel (server side)
/// @param io_handle - IO handle returned by TS_Open()
/// @param buffer - pointer to buffer to receive data from tunnel.
/// @param buffer_len - size of the receiving buffer. On success returns
///        the amount of data received.
/// @param error_msg - returned error message on failure.
/// @return TS_OK on successful init.
///         TS error code on failure and error_msg set.
/// @note this call blocks until data is received or an error occurs. It
///       does not wait for the entire buffer to be filled before returning.
TSError_t TSS_Read(TSIOHandle_t io_handle,
                  char* buffer,
                  size_t& buffer_len,
                  string& error_msg);

/// write data to a tunnel (server side)
/// @param io_handle - IO handle returned by TS_Open()
/// @param buffer - pointer to buffer of data to write to tunnel.
/// @param buffer_len - size of the send buffer. 
/// @param error_msg - returned error message on failure.
/// @return TS_OK on successful init.
///         TS error code on failure and error_msg set.
/// @note this call blocks until all data has been received and
///       acknowledged by the other side of the connection.
TSError_t TSS_Write(TSIOHandle_t io_handle,
                   const char* buffer,
                   size_t buffer_len,
                   string& error_msg);

/// Close a tunnel (server side)
/// @param io_handle - IO handle returned by TS_Open()
/// @param error_msg - returned error message on failure.
/// @return TS_OK on successful init.
///         TS error code on failure and error_msg set.
/// @note This call blocks until all unsent data has been received
///       or dropped by the other side of the connection.
///       It may return an error if the data was unable to be sent.
TSError_t TSS_Close(TSIOHandle_t& io_handle,
                    string& error_msg);

#if defined(IS_TS) || defined(IS_TS_WRAPPER)
}  // namespace guard
#endif
#endif // include guard
