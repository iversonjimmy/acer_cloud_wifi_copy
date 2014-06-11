//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef VCSTESTSESSION_HPP_02_18_2013
#define VCSTESTSESSION_HPP_02_18_2013

#include "vcs_util.hpp"
#include <string>

struct VcsTestSession {
    u64 userId;
    std::string username;

    // for debug, encoded base64 of the binary byte array sessionServiceTicket
    std::string sessionEncodedBase64;

    // urlPrefix includes protocol (scheme name), domain, and port
    // For example, https://www-c100.pc-int.igware.net:443
    std::string urlPrefix;

    // For example, www-c100.pc-int.igware.net (does not include protocol or port)
    std::string serverHostname;

    u64 sessionHandle;

    // sessionServiceTicket - binary byte array (no encoding) representing the
    // serviceTicket.
    std::string sessionServiceTicket;

    void clear() {
        userId = 0;
        username.clear();
        sessionEncodedBase64.clear();
        urlPrefix.clear();
        serverHostname.clear();
        sessionHandle = 0;
        sessionServiceTicket.clear();
    }
    VcsTestSession(){ clear(); }
};

int vcsGetTestSession(const std::string& domain,
                      int domainPort,
                      const std::string& ns,
                      const std::string& user,
                      const std::string& password,
                      VcsTestSession& vcsTestSession_out);

int vcsGetDatasetIdFromName(const VcsTestSession& vcsTestSession,
                            const std::string& datasetName,
                            u64& datasetId_out);

int saveVcsTestSessionFile(const std::string& sessionFilepath,
                           const VcsTestSession& session);

int readVcsTestSessionFile(const std::string& sessionFilepath,
                           bool printLog,
                           VcsTestSession& session_out);

void getVcsSessionFromVcsTestSession(const VcsTestSession& vcsTestSession,
                                     VcsSession& vcsSession_out);

#endif /* VCSTESTSESSION_HPP_02_18_2013 */
