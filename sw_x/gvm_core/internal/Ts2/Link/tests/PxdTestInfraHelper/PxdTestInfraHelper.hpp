//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef __PXDTESTINFRAHELPER_HPP__
#define __PXDTESTINFRAHELPER_HPP__
#include "vplex_vs_directory.h"
#include <string>
#include <sstream>
#include <stack>

#include "vplex_ias.hpp"
#include "vplex_ias_service_types.pb.h"
#include "vplex_serialization.h"
#include "vplex_trace.h"
#include "vpl_th.h"

#include "csltypes.h"
#include "cslsha.h"

#include "gvm_configuration.h"
#include "ccd_features.h"

namespace PxdTest {
class InfraHelper {
public:
    InfraHelper(const std::string& _username,
                const std::string& _password,
                const std::string& _deviceInfo,
                const std::string& _ns = "acer",
                const std::string& _ans_name = "ans-c100.pc-int.igware.net",
                const std::string& _pxd_name = "pxd1-c100.pc-int.igware.net",
                const std::string& _vsds_name = "www-c100.pc-int.igware.net",
                const u16 _vsds_port = 443,
                const std::string& _ias_name = "www-c100.pc-int.igware.net",
                const u16 _ias_port = 443);
    InfraHelper();
    ~InfraHelper();

    // Connect to infra to setup a small CCD instance
    int ConnectInfra(u64& _userId, u64& _deviceId);

    // ANS credential
    int GetAnsLoginBlob(std::string& ansSessionKey,
                        std::string& ansLoginBlob);

    // Pxd credential helper group
    int GetPxdLoginBlob(std::string inst_id,
                        std::string ansLoginBlob,
                        std::string& pxdSessionKey,
                        std::string& pxdLoginBlob);

    int GetCCDLoginBlob(std::string inst_id,
                        u64 server_user_id,
                        u64 server_device_id,
                        std::string server_inst_id,
                        std::string ansLoginBlob,
                        std::string& ccdSessionKey,
                        std::string& ccdLoginBlob);

    int GetCCDServerKey(std::string inst_id,
                        std::string& ccdServerKey);

    int GetUserStorage(u64 device_id,
                       vplex::vsDirectory::UserStorage &userStorage);

    const std::string &GetPxdSvrName() const;
    const std::string &GetAnsSvrName() const;

private:
    const std::string username;
    const std::string password;
    const std::string device_info;
    const std::string ns;
    const std::string ans_name;
    const std::string pxd_name;
    const std::string vsds_name;
    const u16 vsds_port;
    const std::string ias_name;
    const u16 ias_port;


    VPLMutex_t infra_mutex;
    std::stack<VPLIas_ProxyHandle_t> iasproxies;
    std::stack<VPLVsDirectory_ProxyHandle_t> vsdsproxies;
    vplex::vsDirectory::SessionInfo session;
    std::string iasTicket;

    u64 userId;
    u64 deviceId;


    // CCD instance setup functions group
    int userLogin();

    int registerAsDevice();

    int linkDevice();
};
}

#endif /* __PXDTESTINFRAHELPER_HPP__ */
