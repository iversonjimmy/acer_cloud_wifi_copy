/*
 *               Copyright (C) 2011, iGware Inc.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 */

#ifndef __SW_UPDATE_HPP__
#define __SW_UPDATE_HPP__

#include "base.h"
#include <string>

using namespace std;

int SWUpdate_Init(void);
void SWUpdate_Quit(void);
int SWUpdateCheck(const std::string& app_guid, const std::string& app_version, bool update_cache, u64& update_mask, u64& app_size, std::string& latest_app_version, std::string& change_log, std::string& latest_ccd_version, bool& isAutoUpdateDisabled, bool& isQA, bool &isInfraDownload, const char* userGroup);
int SWUpdateBeginDownload(const std::string& app_guid, const std::string& app_version, u64& handle);
int SWUpdateGetDownloadProgress(u64 handle, u64& tot_xfer_size, u64& tot_xferred, ccd::SWUpdateDownloadState_t& state);
int SWUpdateEndDownload(u64 handle, const std::string& file_loe);
int SWUpdateCancelDownload(u64 handle);
int SWUpdateSetCcdVersion(const std::string& guid, const std::string& version);

#endif // __SW_UPDATE_HPP__
