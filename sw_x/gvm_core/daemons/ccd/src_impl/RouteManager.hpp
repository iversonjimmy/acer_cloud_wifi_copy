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


#ifndef __ROUTE_MANAGER_H__
#define __ROUTE_MANAGER_H__


#include <string>


struct RouteInfo {
    std::string     directInternalAddr;
    std::string     directExternalAddr;
    u16             directPort;
    std::string     proxyAddr;
    u16             proxyPort;
    u64             vssiSessionHandle;
    std::string     vssiServiceTicket;
};


class RouteManager
{
public:
    static RouteManager& Instance(void)
    {
        static RouteManager instance;
        return instance;
    }

    s32 getRouteInfo(u64 userId, u64 deviceId, RouteInfo *routeInfo);
};


#endif // __ROUTE_MANAGER_H__
