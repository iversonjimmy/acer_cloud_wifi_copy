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
/// ts_client.hpp
///
/// Tunnel Service Client Interface

#ifndef __TS_CLIENT_HPP__
#define __TS_CLIENT_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <string>
#include "ts_types.hpp"
#include "LocalInfo.hpp"

using namespace std;

#ifndef __TsRouteInfo_Declared
#define __TsRouteInfo_Declared
// This must agree with RouteInfo in gvm_core/daemons/ccd/src_impl/RouteManager.hpp.
// It is declared here because TS cannot reference files in gvm_core/daemons/ccd/src_impl/...
struct TsRouteInfo {
    std::string     directInternalAddr;
    std::string     directExternalAddr;
    u16             directPort;
    std::string     proxyAddr;
    u16             proxyPort;
    u64             vssiSessionHandle;
    std::string     vssiServiceTicket;
};
#endif

#ifdef IS_TS_WRAPPER
namespace TS_WRAPPER {
#elif defined(IS_TS)
namespace TS {
#endif

#ifdef __TS_EXT_CLIENT_HPP__
namespace TS_EXT {
/// Initialize the TS internal state variables
/// @param error_msg - returned error message on failure.
/// @return TS_OK on successful init.
///         TS error code on failure and error_msg set.
/// @note initializes internal state. Subsequent calls have no effect unless
/// #TS_Shutdown() was called to release all state.
/// @note TS spawns a thread to track and reap idle connections.
TSError_t TS_Init(string& error_msg);
#else // !__TS_EXT_CLIENT_HPP__
/// Initialize the TS internal state variables
/// @param userId - Current CCD user id
/// @param deviceId - Current CCD device id
/// @param instanceId - Current CCD instance id
/// @param getRouteInfo - Function to retrieve routeInfo
/// @param din_idle_timeout - Direct Internal Connection idle timeout
/// @param p2p_idle_timeout - P2P Connection idle timeout
/// @param prx_idle_timeout - Proxy Connection idle timeout
/// @param disabled_route_types - Bit mask to disallow certain route types for testing
/// @param error_msg - returned error message on failure.
/// @return TS_OK on successful init.
///         TS error code on failure and error_msg set.
/// @note initializes internal state. Subsequent calls have no effect unless
/// #TS_Shutdown() was called to release all state.
/// @note TS spawns a thread to track and reap idle connections.
TSError_t TS_Init(u64 userId,
                  u64 deviceId,
                  u32 instanceId,
                  s32 (*getRouteInfo)(u64 userId, u64 deviceId, TsRouteInfo *routeInfo),
                  VPLTime_t din_idle_timeout,
                  VPLTime_t p2p_idle_timeout,
                  VPLTime_t prx_idle_timeout,
                  int disabled_route_types,
                  Ts2::LocalInfo* local_info,
                  string& error_msg);

/// Get tunnel service port
/// @nparam port_out - output the port for TS external service
/// #note It should be called after TS is already initialized
TSError_t TS_GetPort(int& port_out);

#endif // !__TS_EXT_CLIENT_HPP__

/// Clean up TS internal state for system shutdown.
/// @note All internal state is cleared. Close all open tunnels before
/// calling.
void TS_Shutdown(void);

/// Open a tunnel to the requested user's device.
/// @param parms - set of parameters identify target and service.
/// @param ret_io_handle - return IO handle on success.
/// @param error_msg - returned error message on failure.
/// @return TS_OK on successful init.
///         TS error code on failure and error_msg set.
TSError_t TS_Open(const TSOpenParms_t& parms,
                  TSIOHandle_t& ret_io_handle,
                  string& error_msg);

/// Close a tunnel
/// @param io_handle - IO handle returned by TS_Open()
/// @param error_msg - returned error message on failure.
/// @return TS_OK on successful init.
///         TS error code on failure and error_msg set.
/// @note This call blocks until all unsent data has been received
///       or dropped by the server. It may return an error if the
///       data was unable to be sent.
TSError_t TS_Close(TSIOHandle_t& io_handle,
                   string& error_msg);

/// read data from a tunnel
/// @param io_handle - IO handle returned by TS_Open()
/// @param buffer - pointer to buffer to receive data from tunnel.
/// @param buffer_len - size of the receiving buffer. On success returns
///        the amount of data received.
/// @param error_msg - returned error message on failure.
/// @return TS_OK on successful init.
///         TS error code on failure and error_msg set.
/// @note this call blocks until data is received or an error occurs. It
///       does not wait for the entire buffer to be filled before returning.
TSError_t TS_Read(TSIOHandle_t io_handle,
                  char* buffer,
                  size_t& buffer_len,
                  string& error_msg);

/// write data to a tunnel
/// @param io_handle - IO handle returned by TS_Open()
/// @param buffer - pointer to buffer of data to write to tunnel.
/// @param buffer_len - size of the send buffer. 
/// @param error_msg - returned error message on failure.
/// @return TS_OK on successful init.
///         TS error code on failure and error_msg set.
/// @note this call blocks until data is received and acknowledged by
///       the remote server or until the operation fails.
TSError_t TS_Write(TSIOHandle_t io_handle,
                   const char* buffer,
                   size_t buffer_len,
                   string& error_msg);

/// Call this function to get address information for local listening socket for DIN connection.
/// @param addr - addr pointer to store information
TSError_t TS_GetServerLocalPort(int& port_out);

void TS_RefreshServerInfo(u64 userId, bool forceDropConns);
void TS_UpdateServerInfo(u64 userId, u64 deviceId, bool forceDropConns);
void TS_RemoveServerInfo(u64 deviceId);

#if defined(IS_TS) || defined(IS_TS_WRAPPER) || defined(__TS_EXT_CLIENT_HPP__)
}  // namespace guard
#endif

#endif // include guard
