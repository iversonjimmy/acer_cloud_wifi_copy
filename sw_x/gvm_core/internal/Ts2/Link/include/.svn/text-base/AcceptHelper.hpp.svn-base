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

#ifndef __TS2_LINK_ACCEPT_HELPER_HPP__
#define __TS2_LINK_ACCEPT_HELPER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_socket.h>
#include <vpl_th.h>

#include "RouteType.hpp"
#include "LocalInfo.hpp"

#include <string>

/*
  AcceptHelper helps with accepting a connection request.
  It is expected to be created when a connection request is pending.
  It will deliver an authenticated socket via the callback function.
  Callback function will be called once per socket, 
  and then once more (with ROUTE_TYPE_INVALID) to indicate the end.

  Designed to be used only once.
 */

namespace Ts2 {
namespace Link {

class AcceptHelper;

typedef void (*AcceptedSocketHandler)(VPLSocket_t socket, RouteType routeType, 
                                      u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                      const std::string *sessionKey,
                                      AcceptHelper *acceptHelper, void *context);

class AcceptHelper {
public:
    AcceptHelper(AcceptedSocketHandler acceptedSocketHandler, void *acceptedSocketHandler_context,
                 LocalInfo *localInfo);
    virtual ~AcceptHelper();

    // Accept a connection request, using a background thread.
    // When connection is established (and authenticated), the socket will be delivered by the callback function.
    virtual int Accept() = 0;

    // Block until all work is done.
    // When this function returns successfully, it is safe to destroy this object.
    virtual int WaitDone() = 0;

    // Force tear down any outgoing socket attempts
    // Caller also needs to call WaitDone() after this call to make sure every things are ok
    virtual void ForceClose() = 0;

protected:
    VPL_DISABLE_COPY_AND_ASSIGN(AcceptHelper);

    AcceptedSocketHandler acceptedSocketHandler;
    void *acceptedSocketHandler_context;
    LocalInfo *const localInfo;

    mutable VPLMutex_t mutex;

    std::string sessionKey;
};
} // end namespace Link
} // end namespace Ts2

#endif // incl guard
