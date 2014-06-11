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


#include "cache.h"
#include "config.h"
#include "LanDeviceInfoCache.hpp"


s32 LanDeviceInfoCache::reportLanDevices(std::list<ccd::LanDeviceInfo> *infos)
{
    s32 rv = 0;
    ccd::CcdiEvent *event;
    ccd::EventLanDevicesChange *change;
    VPLTime_t timestamp;

    // Copy the new device infos into the cache.  It's simpler to
    // just do a plain copy.  SDD should only call this function
    // when there's a change.  The function update_stream_servers()
    // also check for changes before modifying the connection pool,
    // so it makes sense to not duplicate the check here
    VPLMutex_Lock(&this->lock);
    cache = *infos;
    VPLMutex_Unlock(&this->lock);

    LocalServers_GetHttpService().update_stream_servers(/*force_drop_conns*/false);

    // Send an event to notify control panel of the change
    event = new ccd::CcdiEvent();
    if (event == NULL) {
        LOG_ERROR("Failed to allocate an event for change notification");
        goto end;
    }
    change = event->mutable_lan_devices_change();

    timestamp = VPLTime_GetTimeStamp();
    change->set_timestamp(timestamp);

    EventManagerPb_AddEvent(event);

end:
    return rv;
}


s32 LanDeviceInfoCache::listLanDevices(VPLUser_Id_t userId, u32 filterBitMask, std::list<ccd::LanDeviceInfo> *infos)
{
    s32 rv = 0;
    ccd::ListLinkedDevicesInput req;
    ccd::ListLinkedDevicesOutput resp;
    std::list<ccd::LanDeviceInfo>::iterator it;
    bool linked;

    if (userId == 0) {
        VPLMutex_Lock(&this->lock);
        *infos = cache;
        VPLMutex_Unlock(&this->lock);

        goto end;
    }

    // Need to retrieve the linked devices to filter out those
    // that are not linked to the current user
    req.set_user_id(userId);
    req.set_only_use_cache(true);
    rv = CCDIListLinkedDevices(req, resp);
    if (rv < 0) {
        LOG_ERROR("CCDIListLinkedDevices failed, rv=%d\n", rv);
        goto end;
    }

    // Go through the cache and apply the filter
    VPLMutex_Lock(&this->lock);
    for (it = this->cache.begin(); it != this->cache.end(); it++) {
        if (!it->has_device_id() || (it->device_id() == 0)) {
            if (filterBitMask & LAN_DEVICE_FILTER_UNREGISTERED) {
                infos->push_back(*it);
            }
            continue;
        }

        linked = false;
        for (s32 i = 0; i < resp.devices_size(); i++) {
            if (it->device_id() == resp.devices(i).device_id()) {
                linked = true;
                break;
            }
        }

        if ((filterBitMask & LAN_DEVICE_FILTER_LINKED) && linked) {
            infos->push_back(*it);
            continue;
        }


        if ((filterBitMask & LAN_DEVICE_FILTER_REGISTERED_BUT_NOT_LINKED) && !linked) {
            infos->push_back(*it);
            continue;
        }
    }
    VPLMutex_Unlock(&this->lock);

end:
    return rv;
}


s32 LanDeviceInfoCache::getLanDeviceByDeviceId(u64 deviceId, ccd::LanDeviceInfo *info)
{
    s32 rv = CCD_ERROR_NOT_FOUND;
    std::list<ccd::LanDeviceInfo>::iterator it;

    // Go through the cache and find the device ID
    VPLMutex_Lock(&this->lock);
    for (it = this->cache.begin(); it != this->cache.end(); it++) {
        if (it->has_device_id() && (it->device_id() == deviceId)) {
            *info = *it;
            rv = 0; // Found
            break;
        }
    }
    VPLMutex_Unlock(&this->lock);

    return rv;
}
