/*
 *  Copyright 2011 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef __VSTEST_INFRA_HPP__
#define __VSTEST_INFRA_HPP__

#include "vplex_vs_directory.h"
#include <string>

template<class RequestT> void
setIasAbstractRequestFields(RequestT& request);

void vsTest_infra_init(void);

void vsTest_infra_destroy(void);

int userLogin(const std::string& ias_name, u16 port, 
              const std::string& user, const std::string& ns, 
              const std::string& pass,
              u64& uid, vplex::vsDirectory::SessionInfo& session,
              std::string* iasTicket = NULL);

int registerAsDevice(const std::string& ias_name, u16 port, 
                     const std::string& username, const std::string& password,
                     u64& testDeviceId);

int getAnsLoginBlob(const std::string& ias_name, u16 port, 
                    u64 sessionHandle,
                    const std::string& iasTicket,
                    u64 deviceId,
                    std::string& ansSessionKey,
                    std::string& ansLoginBlob);

int getOwnedTitles(VPLVsDirectory_ProxyHandle_t& proxy,
                   const vplex::vsDirectory::SessionInfo& session,
                   const vplex::vsDirectory::Localization& l10n,
                   std::vector<vplex::vsDirectory::TitleDetail>& ownedTitles);

int unsubscribeDataset(VPLVsDirectory_ProxyHandle_t& proxy,
                       const vplex::vsDirectory::SessionInfo& session,
                       u64 uid, u64 testDeviceId, u64 datasetId);

int updatePSNConnection(VPLVsDirectory_ProxyHandle_t& proxy,
                        u64 userId,
                        u64 clusterId,
                        const std::string& hostname,
                        u16 vssiPort,
                        u16 secureClearfiPort,
                        const vplex::vsDirectory::SessionInfo& session);

#endif // include guard
