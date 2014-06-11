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
#include "NotificationQ.hpp"
#include <ccdi_rpc.pb.h>
#include <vpl_th.h>
#include <vpl_lazy_init.h>
#include <vplu_format.h>
#include <vplu_types.h>
#include <deque>

#include <log.h>

static std::deque<ccd::SyncStateNotification> g_notifications;
static VPLLazyInitMutex_t g_notificationQMutex = VPLLAZYINITMUTEX_INIT;
static const u32 MAX_NOTIFICATIONS = 200;

void DownloadCompleteCb(u64 dataset_id, const char* full_path)
{
    ccd::SyncStateNotification notification;
    notification.mutable_file_added()->set_dataset_id(dataset_id);
    notification.mutable_file_added()->set_full_path(full_path);

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_notificationQMutex));
    while(g_notifications.size() >= MAX_NOTIFICATIONS) {
        if(g_notifications.front().has_file_added()) {
            LOG_ERROR("Dropping notification due to max queue exceeded: dset:"FMTx64" path:%s",
                      g_notifications.front().file_added().dataset_id(),
                      g_notifications.front().file_added().full_path().c_str());
        }
        g_notifications.pop_front();
    }

    g_notifications.push_back(notification);

    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_notificationQMutex));
}

void GetNotifications(ccd::GetSyncStateNotificationsOutput& response)
{
    GetNotificationsPage(MAX_NOTIFICATIONS, response);
}

void GetNotificationsPage(u32 count, ccd::GetSyncStateNotificationsOutput& response)
{

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_notificationQMutex));

    for(;!g_notifications.empty() && count > 0; count--) {
        (*response.add_notifications()) = g_notifications.front();
        g_notifications.pop_front();
    }

    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_notificationQMutex));
}
