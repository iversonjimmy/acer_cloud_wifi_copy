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

#ifndef NOTIFICATIONQ_HPP_07012011
#define NOTIFICATIONQ_HPP_07012011

#include <ccdi_rpc.pb.h>
#include <vplu_types.h>

void DownloadCompleteCb(u64 dataset_id, const char* full_path);

void GetNotifications(ccd::GetSyncStateNotificationsOutput& response);

void GetNotificationsPage(u32 count,
                          ccd::GetSyncStateNotificationsOutput& response);

#endif /* NOTIFICATIONQ_HPP_ */
