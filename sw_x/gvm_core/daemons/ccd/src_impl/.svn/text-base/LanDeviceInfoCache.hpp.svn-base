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


#ifndef __LAN_DEVICE_INFO_CACHE_H__
#define __LAN_DEVICE_INFO_CACHE_H__


#include <list>
#include <string>

#include "ccdi.hpp"
#include "ccdi_rpc.pb.h"
#include "EventManagerPb.hpp"
#include "log.h"
#include "vpl_th.h"
#include "vplu_types.h"


// The caller of ListLanDevices() may not want to retrieve all
// devices.  For example, the AcerCloud Control Panel would not
// be interested in devices registered to other users.  The
// following filter options allow such filtering:
#define LAN_DEVICE_FILTER_UNREGISTERED                  0x1     // Un-registered devices
#define LAN_DEVICE_FILTER_REGISTERED_BUT_NOT_LINKED     0x2     // Devices registered but not linked to the current user
#define LAN_DEVICE_FILTER_LINKED                        0x4     // Devices linked to the current user


class LanDeviceInfoCache
{
public:
    static LanDeviceInfoCache& Instance(void)
    {
        static LanDeviceInfoCache instance;
        return instance;
    }

    s32 reportLanDevices(std::list<ccd::LanDeviceInfo> *infos);

    s32 listLanDevices(VPLUser_Id_t userId, u32 filterBitMask, std::list<ccd::LanDeviceInfo> *infos);

    s32 getLanDeviceByDeviceId(u64 deviceId, ccd::LanDeviceInfo *info);

private:
    std::list<ccd::LanDeviceInfo> cache;

    VPLMutex_t lock;
};


#endif // __LAN_DEVICE_INFO_CACHE_H__
