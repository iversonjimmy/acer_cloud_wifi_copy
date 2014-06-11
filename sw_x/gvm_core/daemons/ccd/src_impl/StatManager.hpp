#ifndef __STAT_MANAGER_HPP__
#define __STAT_MANAGER_HPP__

#include "vplu_types.h"
#include <string>
#include "ccdi_rpc.pb.h"

#define STAT_EVENT_ID_APPFOREGROUND "app-foreground"
#define STAT_EVENT_ID_NATINFO       "nat-info"

int StatMgr_Start(u64 userId);

int StatMgr_Stop();

int StatMgr_AddEventBegin(const std::string& app_id, const std::string& event_id);

int StatMgr_AddEventEnd(const std::string& app_id, const std::string& event_id);

int StatMgr_NotifyConnectionChange();

#endif  //__STAT_MANAGER_HPP__
