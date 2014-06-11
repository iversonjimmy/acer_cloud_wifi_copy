//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef SYNCCONFIGTEST_HPP_
#define SYNCCONFIGTEST_HPP_

#include <vpl_plat.h>

#include "SyncConfig.hpp"
#include "VcsTestSession.hpp"

#include <vplu_types.h>
#include <string>

#ifdef WIN32
#define WIN32_DRIVE_LETTER "C:"
#else
#define WIN32_DRIVE_LETTER ""
#endif

// SyncConfigWorkerTestClient (SCWTestClient) Will hold the test client state,
// including the local SyncDirectories and any sync policy.
struct SCWTestClient {
    SyncType syncType;
    SyncPolicy syncPolicy;
    std::string localPath;
    std::string serverPath;  // Should this go to SCWTestUserAcct?

    SCWTestClient()
    :  syncType(SYNC_TYPE_TWO_WAY),
       syncPolicy()
    {}

    void clear(){
        syncType = SYNC_TYPE_TWO_WAY;
        SyncPolicy defaultPolicy;
        syncPolicy = defaultPolicy;
        localPath.clear();
        serverPath.clear();
    }
};

// SyncConfigWorkerTestUserAcct (SCWTestUserAcct) Holds the acct state.
// In general, this is the state that is identical between two separate clients.
struct SCWTestUserAcct {
    std::string ias_central_url;
    int ias_central_port;
    std::string acctNamespace;
    std::string username;
    std::string password;
    std::string datasetName;  // Or/and use datasetType?

    // Derived or looked up from the above information in login sequence.
    VcsTestSession vcsTestSession;
    u64 datasetNum;  // Obtained from VSDS.

    void clear() {
        username.clear();
        password.clear();
        ias_central_url.clear();
        ias_central_port = 0;
        datasetNum = 0;
        acctNamespace.clear();
        vcsTestSession.clear();
    }

    std::string getDatasetName() const {
        return "Media MetaData VCS";
    }
    VcsDataset getDataset() const {
        return VcsDataset(datasetNum, VCS_CATEGORY_METADATA);
    }
};

/// Creates a SyncConfig instance (according to the specified parameters) and blocks until
/// it is in-sync (SyncConfig::GetSyncStatus() reports SYNC_CONFIG_STATUS_DONE and no errors)
/// or until we reach a hard-coded timeout.
/// Either way, the SyncConfig instance is destroyed before this function returns.
int SyncConfigWorkerSync(const SCWTestUserAcct& tua,
                         const SCWTestClient& tss);

#if defined(IOS) || defined(VPL_PLAT_IS_WINRT) || defined(ANDROID)
// For iOS and WinRT, transform main into syncConfigTest
int syncConfigTest(int argc, const char ** argv);
#endif

#endif // include guard
