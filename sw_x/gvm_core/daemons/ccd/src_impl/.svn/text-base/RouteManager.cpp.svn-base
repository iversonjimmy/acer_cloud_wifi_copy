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
#include "ccd_features.h"
#include "LanDeviceInfoCache.hpp"
#include "RouteManager.hpp"
#include "HttpService.hpp"


s32 RouteManager::getRouteInfo(u64 userId, u64 deviceId, struct RouteInfo *routeInfo)
{
    s32 rv = 0;
    ccd::LanDeviceInfo lanDeviceInfo;

    // Aggregate routes from VSDS and LAN discovery
    routeInfo->directInternalAddr.clear();
    routeInfo->directExternalAddr.clear();
    routeInfo->directPort = 0;
    routeInfo->proxyAddr.clear();
    routeInfo->proxyPort = 0;
    routeInfo->vssiSessionHandle = 0;
    routeInfo->vssiServiceTicket.clear();

    {
        // Get the storage node address by userID from cache
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("ROUTE-MGR: Failed to obtain lock");
            goto end;
        }

        CachePlayer *user = cache_getUserByUserId(userId);
        if (user == NULL) {
            LOG_ERROR("ROUTE-MGR["FMT_VPLUser_Id_t"] not signed-in", userId);
            rv = CCD_ERROR_NOT_SIGNED_IN;
            goto end;
        }

        const google::protobuf::RepeatedPtrField<vplex::vsDirectory::UserStorage>& cachedUserStorages =
        user->_cachedData.details().cached_user_storage();
        const vplex::vsDirectory::UserStorage* currCachedUserStorage = Util_FindInUserStorageList(deviceId, cachedUserStorages);
        if (currCachedUserStorage == NULL) {
            LOG_DEBUG("ROUTE-MGR["FMTu64"]: No CachedUserStorage available:"FMTu64", %d",
                      userId, deviceId, cachedUserStorages.size());
        } else {
            if (currCachedUserStorage->storageaccess_size() <= 0) {
                LOG_DEBUG("ROUTE-MGR["FMTu64"]: No StorageAccess available:"FMTu64", %d",
                          userId, deviceId, currCachedUserStorage->storageaccess_size());
            } else {
                LOG_DEBUG("ROUTE-MGR["FMTu64"]: StorageAccess success",
                          userId);
                
                for (int storageAccessIdx = 0; storageAccessIdx < currCachedUserStorage->storageaccess_size(); storageAccessIdx++) {
                    const vplex::vsDirectory::StorageAccess &cachedUserStorageAccess = currCachedUserStorage->storageaccess().Get(storageAccessIdx);
                    switch (cachedUserStorageAccess.routetype()) {
                        case vplex::vsDirectory::DIRECT_INTERNAL:
                            routeInfo->directInternalAddr.assign(cachedUserStorageAccess.server());
                            LOG_DEBUG("ROUTE-MGR["FMT_VPLUser_Id_t"]: DIRECT_INTERNAL address: %s",
                                      userId, cachedUserStorageAccess.server().c_str());
                            if (cachedUserStorageAccess.ports_size() > 0) {
                                for (int portIdx = 0; portIdx < cachedUserStorageAccess.ports().size(); portIdx++) {
                                    if (cachedUserStorageAccess.ports().Get(portIdx).porttype() == vplex::vsDirectory::PORT_CLEARFI_SECURE) {
                                        // Use PORT_CLEARFI_SECURE
                                        routeInfo->directPort = cachedUserStorageAccess.ports().Get(portIdx).port();
                                        LOG_DEBUG("ROUTE-MGR["FMT_VPLUser_Id_t"]: DIRECT_INTERNAL port: %d",
                                                  userId, cachedUserStorageAccess.ports().Get(portIdx).port());
                                        // Force the loop to stop since we've got what we need
                                        break;
                                    }
                                }
                            }
                            break;
                        case vplex::vsDirectory::DIRECT_EXTERNAL:
                            routeInfo->directExternalAddr.assign(cachedUserStorageAccess.server());
                            LOG_DEBUG("ROUTE-MGR["FMT_VPLUser_Id_t"]: DIRECT_EXTERNAL address: %s",
                                      userId, cachedUserStorageAccess.server().c_str());
                            // Only update the port when it's not set by the DIRECT_INTERNAL
                            if ((cachedUserStorageAccess.ports_size() > 0) && (routeInfo->directPort == 0)) {
                                // Currently we assign PORT_CLEARFI_SECURE for DIRECT_EXTERNAL too
                                for (int portIdx = 0; portIdx < cachedUserStorageAccess.ports().size(); portIdx++) {
                                    if (cachedUserStorageAccess.ports().Get(portIdx).porttype() == vplex::vsDirectory::PORT_CLEARFI_SECURE) {
                                        // Use PORT_CLEARFI_SECURE
                                        routeInfo->directPort = cachedUserStorageAccess.ports().Get(portIdx).port();
                                        LOG_DEBUG("ROUTE-MGR["FMT_VPLUser_Id_t"]: DIRECT_EXTERNAL port: %d",
                                                  userId, cachedUserStorageAccess.ports().Get(portIdx).port());
                                        // Force the loop to stop since we've got what we need
                                        break;
                                    }
                                }
                            }
                            break;
                        case vplex::vsDirectory::PROXY:
                            routeInfo->proxyAddr.assign(cachedUserStorageAccess.server());
                            LOG_DEBUG("ROUTE-MGR["FMT_VPLUser_Id_t"]: PROXY address: %s",
                                      userId, cachedUserStorageAccess.server().c_str());
                            if (cachedUserStorageAccess.ports_size() > 0) {
                                routeInfo->proxyPort = cachedUserStorageAccess.ports().Get(0).port();
                                LOG_DEBUG("ROUTE-MGR["FMT_VPLUser_Id_t"]: PROXY port: %d",
                                          userId, cachedUserStorageAccess.ports().Get(0).port());
                            }
                            break;
                        case vplex::vsDirectory::INVALID_ROUTE:
                        default:
                            LOG_WARN("ROUTE-MGR["FMT_VPLUser_Id_t"] unused routetype:%d", userId, cachedUserStorageAccess.routetype());
                            break;
                    }
                    
                }
            }
            
            if (currCachedUserStorage->has_accesshandle()) {
                routeInfo->vssiSessionHandle = currCachedUserStorage->accesshandle();
            }
            if (currCachedUserStorage->has_devspecaccessticket()) {
                routeInfo->vssiServiceTicket = currCachedUserStorage->devspecaccessticket();
            }
        }
    }

    // LAN discovered route
    rv = LanDeviceInfoCache::Instance().getLanDeviceByDeviceId(deviceId, &lanDeviceInfo);
    if ((rv < 0) || !lanDeviceInfo.route_info().has_ip_v4_address() || !lanDeviceInfo.route_info().has_media_server_port() || (lanDeviceInfo.route_info().media_server_port() == 0)) {
        if (rv == CCD_ERROR_NOT_FOUND) {
            rv = 0; // Not really an error
        } else {
            goto end;
        }
    } else {
        // The LAN discovered route is more up-to-date than the VSDS
        // route, so overwrite internal direct with the LAN discovered
        // route
        routeInfo->directInternalAddr.assign(lanDeviceInfo.route_info().ip_v4_address());
        routeInfo->directPort = lanDeviceInfo.route_info().media_server_port();
    }

end:
    return rv;
}
