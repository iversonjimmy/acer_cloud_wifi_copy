#include <vpl_plat.h>
#include "vcs_test.hpp"
#include "vcs_v1_util.hpp"
#include "vcs_util.hpp"
#include "cJSON2.h"
#include "ccdi.hpp"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "gvm_misc_utils.h"
#include "gvm_file_utils.hpp"
#include "scopeguard.hpp"
#include "dx_common.h"
#include "protobuf_file_reader.hpp"
#include "protobuf_file_writer.hpp"
#include "vplex_serialization.h"
#include "vplex_http2.hpp"
#include "vplex_http_util.hpp"
#include "vplex_user.h"
#include "vplex_vs_directory.h"

#include <cslsha.h>

#include <vplex_ias.hpp>


#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>

#include "log.h"

static const u32 FILE_SIZE_TAB = 30;

struct VcsTestSession {
    u64 userId;
    std::string username;

    // for debug, encoded base64 of the binary byte array sessionServiceTicket
    std::string sessionEncodedBase64;

    // urlPrefix includes protocol (scheme name), domain, and port
    // For example, https://www-c100.pc-int.igware.net:443
    std::string urlPrefix;

    // For example, www-c100.pc-int.igware.net (does not include protocol or port)
    // Useful to create VSDS proxy (not used here).
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

static std::string GetVcsTestSessionPath() {
    std::string toReturn;
    char testInstanceNumStr[32];
    snprintf(testInstanceNumStr, sizeof(testInstanceNumStr), "%d", testInstanceNum);
    toReturn = getDxshellTempFolder();
    toReturn += "/";
    toReturn += "VcsTestSession-";
    toReturn += testInstanceNumStr;
    toReturn += ".bin";
    return toReturn;
}

// read by readVcsTestSessionFile
static int saveVcsTestSessionFile(const std::string& sessionFilepath,
                                  const VcsTestSession& session)
{
    int rv = 0;
    int rc = VPLFile_Delete(sessionFilepath.c_str());
    if(rc != 0) {
        LOG_WARN("VPLFile_Delete %s:%d, File may not exist.  Not critical, continue.",
                 sessionFilepath.c_str(), rc);
    }
    rc = Util_CreatePath(sessionFilepath.c_str(), false);
    if(rc != 0){
        LOG_ERROR("Util_CreatePath %s:%d", sessionFilepath.c_str(), rc);
        return rc;
    }
    {
        ProtobufFileWriter writer;
        rv = writer.open(sessionFilepath.c_str(), (VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR));
        if (rv != 0) {
            LOG_ERROR("Failed to open \"%s\" for writing: %d", sessionFilepath.c_str(), rv);
            return rv;
        }
        google::protobuf::io::CodedOutputStream tempStream(writer.getOutputStream());

        tempStream.WriteVarint64(session.userId);
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session userId:"FMTu64, session.userId);
            return -1;
        }

        tempStream.WriteVarint32(session.username.size());
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session username prepend size:%d", session.username.size());
            return -2;
        }
        tempStream.WriteString(session.username);
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session username:%s", session.username.c_str());
            return -3;
        }

        tempStream.WriteVarint32(session.sessionEncodedBase64.size());
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session sessionEncodedBase64 prepend size:%d", session.sessionEncodedBase64.size());
            return -4;
        }
        tempStream.WriteString(session.sessionEncodedBase64);
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session sessionEncodedBase64:%s", session.sessionEncodedBase64.c_str());
            return -5;
        }

        tempStream.WriteVarint32(session.urlPrefix.size());
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session urlPrefix prepend size:%d", session.urlPrefix.size());
            return -6;
        }
        tempStream.WriteString(session.urlPrefix);
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session urlPrefix:%s", session.urlPrefix.c_str());
            return -7;
        }

        tempStream.WriteVarint64(session.sessionHandle);
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs sessionHandle");
            return -8;
        }

        tempStream.WriteVarint32(session.sessionServiceTicket.size());
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session prepend sessionServiceTicket size:%d", session.sessionServiceTicket.size());
            return -9;
        }
        tempStream.WriteString(session.sessionServiceTicket);
        if(tempStream.HadError()) {
            LOG_ERROR("Write sessionServiceTicket failed");
            return -10;
        }

        tempStream.WriteVarint32(session.serverHostname.size());
        if(tempStream.HadError()) {
            LOG_ERROR("Write Vcs session prepend serverHostname size:%d", session.serverHostname.size());
            return -11;
        }
        tempStream.WriteString(session.serverHostname);
        if(tempStream.HadError()) {
            LOG_ERROR("Write serverHostname failed");
            return -12;
        }
    }
    LOG_ALWAYS("Saved session to %s", sessionFilepath.c_str());
    return rv;
}

// See saveVcsTestSessionFile for what this function is reading.
static int readVcsTestSessionFile(const std::string& sessionFilepath,
                                  bool printLog,
                                  VcsTestSession& session_out) {
    int rc;
    VPLFS_stat_t statBuf;
    rc = VPLFS_Stat(sessionFilepath.c_str(), &statBuf);
    if(rc != 0) {
        LOG_ERROR("Stat of %s failed:%d", sessionFilepath.c_str(), rc);
        return rc;
    }
    if(statBuf.type != VPLFS_TYPE_FILE) {
        LOG_ERROR("Is not file:%s", sessionFilepath.c_str());
        return -20;
    }
    {
        ProtobufFileReader reader;
        rc = reader.open(sessionFilepath.c_str(), true);
        if(rc != 0) {
            LOG_ERROR("Could not open %s:%d", sessionFilepath.c_str(), rc);
            return rc;
        }

        u32 prependLength;
        google::protobuf::io::CodedInputStream tempStream(reader.getInputStream());

        if(!tempStream.ReadVarint64(&session_out.userId)) {
            LOG_ERROR("Failed to read userId");
            return -1;
        }

        if(!tempStream.ReadVarint32(&prependLength)) {
            LOG_ERROR("Failed to read prepend username");
            return -2;
        }
        if(!tempStream.ReadString(&session_out.username, prependLength)) {
            LOG_ERROR("Failed to read username");
            return -3;
        }

        if(!tempStream.ReadVarint32(&prependLength)) {
            LOG_ERROR("Failed to read prepend sessionEncodedBase64");
            return -4;
        }
        if(!tempStream.ReadString(&session_out.sessionEncodedBase64, prependLength)) {
            LOG_ERROR("Failed to read sessionEncodedBase64");
            return -5;
        }

        if(!tempStream.ReadVarint32(&prependLength)) {
            LOG_ERROR("Failed to read prepend urlPrefix");
            return -6;
        }
        if(!tempStream.ReadString(&session_out.urlPrefix, prependLength)) {
            LOG_ERROR("Failed to read urlPrefix");
            return -7;
        }

        if(!tempStream.ReadVarint64(&session_out.sessionHandle)) {
            LOG_ERROR("Failed to read sessionHandle");
            return -8;
        }

        if(!tempStream.ReadVarint32(&prependLength)) {
            LOG_ERROR("Failed to read prepend sessionServiceTicket");
            return -9;
        }
        if(!tempStream.ReadString(&session_out.sessionServiceTicket, prependLength)) {
            LOG_ERROR("Failed to read sessionServiceTicket");
            return -10;
        }

        if(!tempStream.ReadVarint32(&prependLength)) {
            LOG_ERROR("Failed to read prepend serverHostname");
            return -11;
        }
        if(!tempStream.ReadString(&session_out.serverHostname, prependLength)) {
            LOG_ERROR("Failed to read serverHostname");
            return -12;
        }
    }

    if(printLog) {
        LOG_ALWAYS("Read session state from %s\n"
                   "         username:%s\n"
                   "         userId:"FMTu64"\n"
                   "         serverHostname:%s\n"
                   "         urlPrefix:%s\n"
                   "         sessionHandle:"FMTs64"\n"
                   "         Session file created on "FMTu64,
                   sessionFilepath.c_str(),
                   session_out.username.c_str(),
                   session_out.userId,
                   session_out.serverHostname.c_str(),
                   session_out.urlPrefix.c_str(),
                   session_out.sessionHandle,
                   (u64)statBuf.ctime);
    }
    return 0;
}

static void getVcsSessionFromVcsTestSession(const VcsTestSession& vcsTestSession,
                                            VcsSession& vcsSession_out)
{
    vcsSession_out.userId = vcsTestSession.userId;
    // TODO: Bug 11568: Set deviceId here?
    // vcsSession_out.deviceId = ?;
    vcsSession_out.urlPrefix = vcsTestSession.urlPrefix;
    vcsSession_out.sessionHandle = vcsTestSession.sessionHandle;
    vcsSession_out.sessionServiceTicket = vcsTestSession.sessionServiceTicket;
}

static std::string makeMessageId()
{
    std::stringstream stream;
    VPLTime_t currTimeMillis = VPLTIME_TO_MILLISEC(VPLTime_GetTime());
    stream << "vsTest-" << currTimeMillis;
    return std::string(stream.str());
}

template<class RequestT>
static void
setIasAbstractRequestFields(RequestT& request)
{
    request.mutable__inherited()->set_version("2.0");
    request.mutable__inherited()->set_country("US");
    request.mutable__inherited()->set_language("en");
    request.mutable__inherited()->set_region("US");
    request.mutable__inherited()->set_messageid(makeMessageId());
}

//------------------------------------------------------------
// code from linkedDevices.cpp
// code from vsTest_infra.cpp (with minor modifications)
static int userLogin(const std::string& ias_name,
                     u16 port,
                     const std::string& user,
                     const std::string& ns,
                     const std::string& pass,
                     u64& clusterId_out,
                     u64& uid_out,
                     u64& sessionHandle_out,
                     std::string& serviceTicket_out)
{
    int rc, rv = 0;
    VPLIas_ProxyHandle_t iasproxy;
    vplex::ias::LoginRequestType loginReq;
    vplex::ias::LoginResponseType loginRes;
    sessionHandle_out = 0;
    serviceTicket_out.clear();

    // Testing VSDS login operation
    // Use the VPLIAS API to log in to infrastructure.
    // Relying on VPL unit testing to cover testing for login.
    rc = VPLIas_CreateProxy(ias_name.c_str(), port, &iasproxy);
    if (rc != 0) {
        LOG_ERROR("VPLIas_CreateProxy returned %d.", rv);
        rv=rc;
        goto fail;
    }

    setIasAbstractRequestFields(loginReq);
    loginReq.set_username(user);
    loginReq.set_namespace_(ns);
    loginReq.set_password(pass);
    rc = VPLIas_Login(iasproxy, VPLTIME_FROM_SEC(30), loginReq, loginRes);
    if(rc != 0) {
        LOG_ERROR("FAIL: Failed login: %d", rc);
        rv = rc;
        goto fail;
    }
    else {
        VPLUser_SessionSecret_t session_secret;
        std::string service = "Virtual Storage";
        const size_t const_encode_len = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(sizeof(session_secret));
        char secret_data_encoded[const_encode_len];
        size_t encode_len = const_encode_len;
        VPLUser_ServiceTicket_t ticket_data;
        CSL_ShaContext context;

        sessionHandle_out = loginRes.sessionhandle();

        memcpy(session_secret, loginRes.sessionsecret().data(),
               VPL_USER_SESSION_SECRET_LENGTH);
        // Compute service ticket like so:
        // ticket = SHA1(base64(session_secret) + "Virtual Storage")
        VPL_EncodeBase64(session_secret, sizeof(session_secret),
                         secret_data_encoded, &encode_len, false, false);
        CSL_ResetSha(&context);
        CSL_InputSha(&context, secret_data_encoded, encode_len - 1); // remove terminating NULL
        CSL_InputSha(&context, service.data(), service.size());
        CSL_ResultSha(&context, (unsigned char*)ticket_data);
        serviceTicket_out.assign(ticket_data, VPL_USER_SERVICE_TICKET_LENGTH);
        uid_out = loginRes.userid();
        clusterId_out = loginRes.storageclusterid();
    }

 fail:
    rc = VPLIas_DestroyProxy(iasproxy);
    if(rc != 0) {
        LOG_ERROR("FAIL: Failed to destroy IAS proxy: %d", rc);
        if(rv == 0) {
            rv=rc;
        }
    }

    return rv;
}

static const int DEFAULT_VCS_GET_SESSION_PORT = 443;

// This function is identical to the one in sw_x/tests/SyncConfigWorker/VcsTestSession.cpp
static int vcsGetTestSession(const std::string& domain,
                             int domainPort,
                             const std::string& ns,
                             const std::string& user,
                             const std::string& password,
                             VcsTestSession& vcsTestSession_out)
{
    vcsTestSession_out.clear();

    u64 userId_out = 0;
    u64 sessionHandle_out = 0;
    u64 clusterId_out = 0;
    std::string sessionServiceTicket_out;
    // See CCD_GET_INFRA_CENTRAL_HOSTNAME in sw_x/gvm_core/daemons/ccd/src_impl/query.h
    std::string ias_url = std::string("www.")+domain;
    std::string vcs_url;
    int rv = 0;
    int rc;

    rc = userLogin(ias_url,
                   domainPort,
                   user,
                   std::string("acer"),
                   password,
                   clusterId_out,
                   userId_out,
                   sessionHandle_out,
                   sessionServiceTicket_out);
    if(rc != 0) {
        LOG_ERROR("userLogin failed (%s, %s, %s): %d",
                  ias_url.c_str(), user.c_str(), password.c_str(),
                  rc);
        rv = rc;
        return rv;
    }

    {   // Figure out vcs_url
        char clusterIdBuf[64];
        snprintf(clusterIdBuf, ARRAY_SIZE_IN_BYTES(clusterIdBuf), FMTs64, clusterId_out);
        vcs_url = std::string("www-c")+
                  std::string(clusterIdBuf)+
                  std::string(".")+
                  domain;
    }

    vcsTestSession_out.userId = userId_out;
    vcsTestSession_out.username = user;
    vcsTestSession_out.sessionHandle = sessionHandle_out;
    vcsTestSession_out.sessionServiceTicket = sessionServiceTicket_out;

    {  // Encode the session into base64
        char* ticket64;
        rc = Util_EncodeBase64(sessionServiceTicket_out.data(),
                               sessionServiceTicket_out.size(),
                               &ticket64, NULL, VPL_FALSE, VPL_FALSE);
        if (rc < 0) {
            LOG_ERROR("error %d when generating service ticket", rv);
            rv = rc;
            return rv;
        }
        ON_BLOCK_EXIT(free, ticket64);
        vcsTestSession_out.sessionEncodedBase64.assign(ticket64);
    }

    vcsTestSession_out.serverHostname.assign(vcs_url);

    // Always use http secure.  For now, assuming https is always used.
    vcsTestSession_out.urlPrefix.assign("https://");
    vcsTestSession_out.urlPrefix.append(vcs_url);
    vcsTestSession_out.urlPrefix.append(":443");

    return rv;
}

#define print_cjson_obj(obj) \
    do{ LOG_ALWAYS("cjson objType:%d, name:%s", obj->type, obj->string); } while(0)

#define FMT_cJSON2_OBJ "cjson Name:%s, Type:%d"
#define VAL_cJSON2_OBJ(s)  (s)->string, (s)->type

static void printVcsGetDirResponse(VcsGetDirResponse& vcsGetDirResponse,
                                   u64 pageOffset,
                                   u64 pageSize)
{
    printf("Files:\n");
    for(u32 i=0; i<vcsGetDirResponse.files.size(); ++i) {
        std::string tabSpaces(" ");

        if(vcsGetDirResponse.files[i].name.size() < FILE_SIZE_TAB) {
            for(u32 j=0;j<FILE_SIZE_TAB-vcsGetDirResponse.files[i].name.size();++j) {
                tabSpaces.append(" ");
            }
        }

        printf(" %s%s (compId:"FMTu64",revision:"FMTu64", size:"FMTu64", lastChangedNanoSecs:"FMTu64", createDateNanoSecs:"FMTu64", noAcs:%d)\n",
               vcsGetDirResponse.files[i].name.c_str(),
               tabSpaces.c_str(),
               vcsGetDirResponse.files[i].compId,
               vcsGetDirResponse.files[i].latestRevision.revision,
               vcsGetDirResponse.files[i].latestRevision.size,
               vcsGetDirResponse.files[i].lastChanged,
               vcsGetDirResponse.files[i].createDate,
               vcsGetDirResponse.files[i].latestRevision.noAcs);
    }
    printf("Directories:\n");
    for(u32 i=0; i<vcsGetDirResponse.dirs.size(); ++i) {

        std::string tabSpaces(" ");
        if(vcsGetDirResponse.dirs[i].name.size() < FILE_SIZE_TAB) {
            for(u32 j=0;j<FILE_SIZE_TAB-vcsGetDirResponse.dirs[i].name.size();++j) {
                tabSpaces.append(" ");
            }
        }

        printf(" %s%s (compId:"FMTu64", version:"FMTu64")\n",
               vcsGetDirResponse.dirs[i].name.c_str(),
               tabSpaces.c_str(),
               vcsGetDirResponse.dirs[i].compId,
               vcsGetDirResponse.dirs[i].version);
    }
    printf("===numEntries:"FMTs64", version:"FMTs64", pageOffset:"FMTu64", pageSize:"FMTu64"\n",
           vcsGetDirResponse.numOfFiles, vcsGetDirResponse.currentDirVersion,
           pageOffset, pageSize);
}

#define DEFAULT_NAMESPACE "acer"

int cmd_vcs_start_session(int argc, const char* argv[])
{
    int rv;
    const std::string DEFAULT_DOMAIN = "pc-int.igware.net";
    std::string domain = DEFAULT_DOMAIN;
    std::string user;
    std::string password;
    int currArg = 1;

    if(!checkHelp(argc, argv)) {
        if(argc > currArg+1 && (strcmp(argv[currArg], "-d")==0)) {
            domain = argv[currArg+1];
            currArg += 2;
        }

        if(argc > currArg+1 && (strcmp(argv[currArg], "-u")==0)) {
            user = argv[currArg+1];
            currArg += 2;
        }

        if(argc > currArg+1 && (strcmp(argv[currArg], "-p")==0)) {
            password = argv[currArg+1];
            currArg += 2;
        }
    }

    if(checkHelp(argc, argv) || (currArg != argc) || user.empty() || password.empty()) {
        if(!checkHelp(argc, argv)) {
            LOG_ERROR("Invalid sub-command. (args must appear in the same order)");
            if(currArg != argc) {
                LOG_ERROR("Wrong number of commands: expected %d, got %d", currArg, argc);
            }
            if(user.empty() || password.empty()) {
                LOG_ERROR("No user/password");
            }
        }

        printf("%s %s -d domain (default:%s) -u username -p password\n",
                VCS_CMD_STR, argv[0],
                DEFAULT_DOMAIN.c_str());
        return 0;
    }

    std::string sessionFilepath = GetVcsTestSessionPath();
    int rc = VPLFile_Delete(sessionFilepath.c_str());
    if(rc != 0) {
        LOG_WARN("VPLFile_Delete %s:%d, File may not exist.  May be expected, continue.",
                 sessionFilepath.c_str(), rc);
    }

    VcsTestSession vcsTestSession;
    rv = vcsGetTestSession(domain,
                           DEFAULT_VCS_GET_SESSION_PORT,
                           std::string(DEFAULT_NAMESPACE),
                           user,
                           password,
                           vcsTestSession);
    if(rv != 0) {
        LOG_ERROR("vcs_get_test_session:%d", rv);
        return rv;
    }

    rc = saveVcsTestSessionFile(sessionFilepath, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Error saving test session:%d", rc);
        rv = rc;
    }

    return rv;
}

static void printFiles(const VcsGetDirResponse& folderOut,
                       int level)
{
    std::string indentSpaces(" ");
    for(int i=0; i<level; ++i) {
        indentSpaces.append("  ");
    }

    for(u32 i=0; i<folderOut.files.size(); ++i) {
        std::string tabSpaces(" ");
        if(folderOut.files[i].name.size() < FILE_SIZE_TAB) {
            for(u32 j=0;j<FILE_SIZE_TAB-folderOut.files[i].name.size();++j) {
                tabSpaces.append(" ");
            }
        }

        printf("%s%s %s(compId:"FMTu64", rev:"FMTu64", size:"FMTu64", noAcs:%d)\n",
                indentSpaces.c_str(),
                folderOut.files[i].name.c_str(),
                tabSpaces.c_str(),
                folderOut.files[i].compId,
                folderOut.files[i].latestRevision.revision,
                folderOut.files[i].latestRevision.size,
                folderOut.files[i].latestRevision.noAcs);
    }
}

static VcsDataset getVcsDataset(u64 datasetId)
{
    // TODO: bug 12928: assuming "metadata" for now.
    return VcsDataset (datasetId, VCS_CATEGORY_METADATA);
}

static const u64 VCS_GET_DIR_PAGE_SIZE = 500;

int cmd_vcs_tree(int argc, const char* argv[])
{
    int rv = 0;
    u64 datasetId;
    VcsFolder folder;
    std::string service;

    if(checkHelp(argc, argv) || (argc != 3)) {
        printf("%s %s <datasetIdDecimal> <serverPath>\n", VCS_CMD_STR, argv[0]);
        return 0;
    }

    folder.clear();
    datasetId = atol(argv[1]);
    folder.name = argv[2];

    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rv = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rv != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rv);
        return rv;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VcsDataset vcsDataset = getVcsDataset(datasetId);

    std::vector< std::pair<VcsFolder, int> > directories;
    directories.push_back(std::pair<VcsFolder, int>(folder, 0));
    bool isRootFolder = true;

    LOG_ALWAYS("Tree printout");
    while(directories.size()>0) {
        VcsFolder folder;
        int level;
        VcsGetDirResponse folderOut;

        folder = directories.back().first;
        level = directories.back().second;
        directories.pop_back();

        if(isRootFolder) {
            isRootFolder = false;
            u64 compId;
            VPLHttp2 httpHandleCompId;
            int rc;
            rc = vcs_get_comp_id(vcsSession,
                                 httpHandleCompId,
                                 vcsDataset,
                                 folder.name,
                                 false,
                                 compId);
            if(rc == 0) {
                folder.compId = compId;
            }else{
                LOG_ERROR("vcs_get_comp_id:%d", rc);
            }
        }

        u64 filesSeen = 0;
        bool performOncePerDir = false;
        do {
            VPLHttp2 httpHandle;
            rv = vcs_read_folder_paged(vcsSession,
                                 httpHandle,
                                 vcsDataset,
                                 folder.name,
                                 folder.compId,
                                 (filesSeen + 1), // VCS starts at 1 instead of 0 for some reason
                                 VCS_GET_DIR_PAGE_SIZE,
                                 false,
                                 folderOut);
            if(rv != 0) {
                LOG_ERROR("vcs_read_folder:%d, dset:"FMTu64", folder:%s",
                          rv, datasetId, folder.name.c_str());
                continue;
            }

            std::string spaces(" ");
            for(int i=0; i<level; ++i) {
                spaces.append("  ");
            }
            if(!performOncePerDir) {
                performOncePerDir = true;
                printf("%s%s - numOfFiles:"FMTu64",version:"FMTu64",compId:"FMTu64
                       ",pageIndex:"FMTu64",pageMax:"FMTu64"\n",
                       spaces.c_str(),
                       folder.name.c_str(),
                       folderOut.numOfFiles,
                       folderOut.currentDirVersion,
                       folder.compId,
                       filesSeen, VCS_GET_DIR_PAGE_SIZE);
            } else {
                printf("%s(Next pageIndex:"FMTu64",pageMax:"FMTu64",numFiles:"FMTu64",version:"FMTu64")\n",
                       spaces.c_str(),
                       filesSeen, VCS_GET_DIR_PAGE_SIZE,
                       folderOut.numOfFiles,
                       folderOut.currentDirVersion);
            }
            printFiles(folderOut, level+1);

            for(u32 i=0; i<folderOut.dirs.size(); ++i) {
                VcsFolder vcsFolder = folderOut.dirs[i];
                std::string noTrailingSlashFolderName;
                Util_trimTrailingSlashes(folder.name, noTrailingSlashFolderName);
                vcsFolder.name = noTrailingSlashFolderName+std::string("/")+vcsFolder.name;
                directories.push_back(std::pair<VcsFolder, int>(vcsFolder, level+1));
            }

            // Get ready to process the next page of vcs_read_folder results.
            u64 entriesInCurrRequest = folderOut.files.size() + folderOut.dirs.size();
            filesSeen += entriesInCurrRequest;
            if(entriesInCurrRequest == 0 && filesSeen < folderOut.numOfFiles) {
                LOG_ERROR("No entries in getDir(%s) response:"FMTu64"/"FMTu64,
                          folder.name.c_str(),
                          filesSeen, folderOut.numOfFiles);
                break;
            }
        } while (filesSeen < folderOut.numOfFiles);
    }
    return rv;
}

int cmd_vcs_get_dir(int argc, const char* argv[])
{
    int rv=0;
    u64 datasetId;
    std::string folder;
    u64 compId;

    if(checkHelp(argc, argv) || (argc != 4)) {
        printf("%s %s <datasetIdDecimal> <serverPath> <compId> (pageSize "FMTu64")\n",
               VCS_CMD_STR, argv[0], VCS_GET_DIR_PAGE_SIZE);
        return 0;
    }
    datasetId = atol(argv[1]);
    folder = argv[2];
    compId = atol(argv[3]);
    VcsGetDirResponse folderOut;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rv = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rv != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rv);
        return rv;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    u64 filesSeen = 0;
    do {
        VPLHttp2 httpHandle;
        rv = vcs_read_folder_paged(vcsSession,
                                   httpHandle,
                                   getVcsDataset(datasetId),
                                   folder,
                                   compId,
                                   (filesSeen + 1), // VCS starts at 1 instead of 0 for some reason
                                   VCS_GET_DIR_PAGE_SIZE,
                                   true,
                                   folderOut);
        if(rv != 0) {
            LOG_ERROR("vcs_read_folder_paged:%d, dset:"FMTu64", folder:%s",
                      rv, datasetId, folder.c_str());
            return rv;
        }
        printVcsGetDirResponse(folderOut, filesSeen, VCS_GET_DIR_PAGE_SIZE);

        // Get ready to process the next page of vcs_read_folder results.
        u64 entriesInCurrRequest = folderOut.files.size() + folderOut.dirs.size();
        filesSeen += entriesInCurrRequest;
        if(entriesInCurrRequest == 0 && filesSeen < folderOut.numOfFiles) {
            LOG_ERROR("No entries in getDir(%s) response:"FMTu64"/"FMTu64,
                      folder.c_str(),
                      filesSeen, folderOut.numOfFiles);
            break;
        }
    } while (filesSeen < folderOut.numOfFiles);

    return rv;
}

int cmd_vcs_get_dir_paged(int argc, const char* argv[])
{
    int rv=0;
    u64 datasetId;
    std::string folder;
    u64 compId;
    u64 pageIndex;
    u64 pageMax;

    if(checkHelp(argc, argv) || (argc != 6)) {
        printf("%s %s <datasetIdDecimal> <serverPath> <compId> <pageIndex> <pageMax>\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    datasetId = atol(argv[1]);
    folder = argv[2];
    compId = atol(argv[3]);
    pageIndex = atol(argv[4]);
    pageMax = atol(argv[5]);
    VcsGetDirResponse folderOut;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rv = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rv != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rv);
        return rv;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;
    rv = vcs_read_folder_paged(vcsSession,
                               httpHandle,
                               getVcsDataset(datasetId),
                               folder,
                               compId,
                               pageIndex+1,  // VCS starts at 1 instead of 0 for some reason
                               pageMax,
                               true,
                               folderOut);
    if(rv != 0) {
        LOG_ERROR("vcs_read_folder:%d, dset:"FMTu64", folder:%s",
                  rv, datasetId, folder.c_str());
        return rv;
    }

    printVcsGetDirResponse(folderOut, pageIndex, pageMax);
    return rv;
}

int cmd_vcs_post_acs_url(int argc, const char* argv[])
{
    int rv=0;
    u64 datasetId;
    std::string path;
    u64 compId;
    u64 revisionId;
    std::string acs_access_url;

    if(checkHelp(argc, argv) || (argc != 6)) {
        printf("%s %s <datasetIdDecimal> <path> <compId> <pageIndex> <pageMax>\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    datasetId = atol(argv[1]);
    path = argv[2];
    compId = atol(argv[3]);
    revisionId = atol(argv[4]);
    acs_access_url = argv[5];
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rv = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rv != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rv);
        return rv;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;
    rv = vcs_post_acs_access_url_virtual(vcsSession,
                                         httpHandle,
                                         getVcsDataset(datasetId),
                                         path,
                                         compId,
                                         revisionId,
                                         acs_access_url,
                                         true);

    if(rv != 0) {
        LOG_ERROR("vcs_post_acs_access_url_virtual:%d, dset:"FMTu64", folder:%s",
                  rv, datasetId, path.c_str());
        return rv;
    }
    return rv;
}

int cmd_vcs_delete_file(int argc, const char* argv[])
{
    if(checkHelp(argc, argv) || (argc != 5)) {
        printf("%s %s <datasetIdDecimal> <serverPath> <compId> <revision>\n", VCS_CMD_STR, argv[0]);
        return 0;
    }

    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];
    u64 compId = atol(argv[3]);
    u64 revision = atol(argv[4]);
    int rc;

    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;  // Single use
    rc = vcs_delete_file(vcsSession,
                         httpHandle,
                         getVcsDataset(datasetId),
                         serverPath,
                         compId,
                         revision,
                         true);
    if(rc != 0) {
        LOG_ERROR("vcs_delete_file:%d", rc);
        return rc;
    }
    return 0;
}

int cmd_vcs_delete_dir(int argc, const char* argv[])
{
    if(checkHelp(argc, argv) || ((argc != 5) && (argc != 4))) {
        printf("%s %s <datasetIdDecimal> <serverPath> <compId> [datasetVersion]\n", VCS_CMD_STR, argv[0]);
        return 0;
    }

    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];
    u64 compId = atol(argv[3]);
    u64 datasetVersion = 0;
    bool hasDatasetVersion = false;
    if(argc == 5) {
        datasetVersion = atol(argv[4]);
        hasDatasetVersion = true;
    }
    int rc;

    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;  // Single use
    rc = vcs_delete_dir(vcsSession,
                        httpHandle,
                        getVcsDataset(datasetId),
                        serverPath,
                        compId,
                        false,    // not recursive
                        hasDatasetVersion, datasetVersion,
                        true);
    if(rc != 0) {
        LOG_ERROR("vcs_delete_file:%d", rc);
        return rc;
    }
    return 0;
}

int cmd_vcs_rm_dash_rf(int argc, const char* argv[])
{
    if(checkHelp(argc, argv) || ((argc != 5) && (argc != 4))) {
        printf("%s %s <datasetIdDecimal> <serverPath> <compId> [datasetVersion]\n", VCS_CMD_STR, argv[0]);
        return 0;
    }

    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];
    u64 compId = atol(argv[3]);
    u64 datasetVersion = 0;
    bool hasDatasetVersion = false;
    if(argc == 5) {
        datasetVersion = atol(argv[4]);
        hasDatasetVersion = true;
    }
    int rc;

    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;  // Single use
    rc = vcs_delete_dir(vcsSession,
                        httpHandle,
                        getVcsDataset(datasetId),
                        serverPath,
                        compId,
                        true,    // recursive
                        hasDatasetVersion, datasetVersion,
                        true);
    if(rc != 0) {
        LOG_ERROR("vcs_delete_file:%d", rc);
        return rc;
    }
    return 0;
}

struct VSDS_dataset {
    std::string dset_name;
    u64 dset_id;
};

// copied from sw_x/tests/sync_config/syncConfig/VcsTestSession.cpp
static int vcsGetDatasetIdFromName(const VcsTestSession& vcsTestSession,
        ::google::protobuf::RepeatedPtrField< ::vplex::vsDirectory::DatasetDetail >& datasets_out)
{
    // TODO: will this work if we don't link the device?
    int rv = 0;
    int rc;
    datasets_out.Clear();

    // Create VSDS proxy.
    VPLVsDirectory_ProxyHandle_t vsds_proxyHandle;
    rc = VPLVsDirectory_CreateProxy(vcsTestSession.serverHostname.c_str(),
                                    DEFAULT_VCS_GET_SESSION_PORT,
                                    &vsds_proxyHandle);
    if(rc != 0) {
        LOG_ERROR("VPLVsDirectory_CreateProxy:%d", rc);
        return rc;
    }

    // Get user's datasets.
    vplex::vsDirectory::SessionInfo* req_session;
    vplex::vsDirectory::ListOwnedDataSetsInput listDatasetReq;
    vplex::vsDirectory::ListOwnedDataSetsOutput listDatasetResp;

    req_session = listDatasetReq.mutable_session();
    req_session->set_sessionhandle(vcsTestSession.sessionHandle);
    req_session->set_serviceticket(vcsTestSession.sessionServiceTicket);
    listDatasetReq.set_userid(vcsTestSession.userId);
    // http://intwww/wiki/index.php/VSDS#ListOwnedDataSets
    // version needs to be "3.0" or greater for "Media MetaData VCS" dataset.
    listDatasetReq.set_version("5.0");
    //listDatasetReq.set_deviceclass("GVM");
    rc = VPLVsDirectory_ListOwnedDataSets(vsds_proxyHandle, VPLTIME_FROM_SEC(30),
                                          listDatasetReq, listDatasetResp);
    if(rc != 0) {
        LOG_ERROR("FAIL:ListOwnedDatasets query returned %d, detail:%d:%s",
                  rc, listDatasetResp.error().errorcode(),
                  listDatasetResp.error().errordetail().c_str());
        rv = rc;
        goto exit;
    }

    // Find the wanted datasetId
    LOG_INFO("User %s:"FMTu64" has %d datasets.",
             vcsTestSession.username.c_str(),
             vcsTestSession.userId,
             listDatasetResp.datasets_size());

    datasets_out.CopyFrom(listDatasetResp.datasets());

 exit:
    rc = VPLVsDirectory_DestroyProxy(vsds_proxyHandle);
    if(rc != 0) {
        LOG_ERROR("VPLVsDirectory_DestroyProxy:%d.  Continuing.", rc);
    }

    return rv;
}

int cmd_vcs_print_session_info(int argc, const char* argv[])
{
    if(checkHelp(argc, argv)) {
        printf("%s %s\n", VCS_CMD_STR, argv[0]);
        return 0;
    }
    if(argc != 1) {
        LOG_ERROR("SessionInfo takes no arguments");
        return -1;
    }
    int rc;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }

    ::google::protobuf::RepeatedPtrField< ::vplex::vsDirectory::DatasetDetail > datasets_out;
    rc = vcsGetDatasetIdFromName(vcsTestSession, datasets_out);
    if(rc != 0) {
        LOG_ERROR("vcsGetDatasetIdFromName:%d", rc);
        return rc;
    }

    LOG_ALWAYS("Read session state from %s\n"
               "         username:%s\n"
               "         userId:"FMTu64"\n"
               "         serverHostname:%s\n"
               "         urlPrefix:%s\n"
               "         sessionHandle:"FMTs64,
               sessionFilepath.c_str(),
               vcsTestSession.username.c_str(),
               vcsTestSession.userId,
               vcsTestSession.serverHostname.c_str(),
               vcsTestSession.urlPrefix.c_str(),
               vcsTestSession.sessionHandle);

    std::stringstream outstream;
    for(int dsetIndex=0; dsetIndex<datasets_out.size(); dsetIndex++)
    {
        outstream << "   ";
        outstream << std::setw(20) << datasets_out.Get(dsetIndex).datasetname().c_str();
        outstream << std::setw(13) << datasets_out.Get(dsetIndex).datasetid();
        outstream << "  ";
        outstream << std::setw(35) << datasets_out.Get(dsetIndex).storageclusterhostname().c_str();
        outstream << std::setw(7) << datasets_out.Get(dsetIndex).storageclusterport();
        outstream << std::endl;
    }

    LOG_ALWAYS("%d Datasets - dsetName, dsetId (decimal), storageClusterName\n%s",
               datasets_out.size(), outstream.str().c_str());
    return 0;
}

int cmd_vcs_make_dir(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 4)) {
        printf("%s %s <datasetIdDecimal> <serverPath> <parentCompId>\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];
    u64 parentCompId = atol(argv[3]);

    int rc;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;  // Single use
    VcsMakeDirResponse makeDirResponse;
    rc = vcs_make_dir(vcsSession,
                      httpHandle,
                      getVcsDataset(datasetId),
                      serverPath,
                      parentCompId,
                      8, 9, 10,  // Junk values for info
                      true,
                      makeDirResponse);
    if(rc != 0) {
        LOG_ERROR("vcs_make_dir:%d", rc);
        return rc;
    }
    LOG_ALWAYS("New compId of (%s): "FMTu64,
               makeDirResponse.name.c_str(),
               S64_TO_U64(makeDirResponse.compId));
    return rv;
}

static void printFileMetadataResponse(const VcsFileMetadataResponse& response)
{
    LOG_ALWAYS("File Metadata Response:\n"
               " name:%s\n"
               " compId:"FMTu64"\n"
               " originDevice:"FMTu64"\n"
               " lastChangedNanoSecs:"FMTu64"\n"
               " createDateNanoSecs:"FMTu64"\n"
               " numRevisions:"FMTu64,
               response.name.c_str(),
               response.compId,
               response.originDevice,
               response.lastChanged,
               response.createDate,
               response.numOfRevisions);
    for(std::vector<VcsFileRevision>::const_iterator revisionIt = response.revisionList.begin();
        revisionIt != response.revisionList.end();
        ++revisionIt)
    {
        const VcsFileRevision& currRevision = *revisionIt;
        LOG_ALWAYS("File Metadata Response Revision Continued:\n"
                   "    revision:"FMTu64"\n"
                   "    size:"FMTu64"\n"
                   "    previewUri:%s\n"
                   "    downloadUrl:%s\n"
                   "    lastChanged:"FMTu64"\n"
                   "    updateDevice:"FMTu64,
                   currRevision.revision,
                   currRevision.size,
                   currRevision.previewUri.c_str(),
                   currRevision.downloadUrl.c_str(),
                   VPLTime_ToSec(currRevision.lastChangedSecResolution),
                   currRevision.updateDevice);
    }
}

int cmd_vcs_get_file_metadata(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 4)) {
        printf("%s %s <datasetIdDecimal> "
               "<serverPath (someroot/parentPath/nameOfFile.txt)> <compId>\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];
    u64 compId = atol(argv[3]);

    int rc;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        rv = rc;
        return rv;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;  // Single use
    VcsFileMetadataResponse response;
    rc = vcs_get_file_metadata(vcsSession,
                               httpHandle,
                               getVcsDataset(datasetId),
                               serverPath,
                               compId,
                               true,
                               /*OUT*/response);
    if(rc!=0){
        LOG_ERROR("vcs_get_file_metadata:%d", rc);
        rv = rc;
        return rv;
    }

    printFileMetadataResponse(response);
    return rv;
}

int cmd_vcs_file_put_deferred(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 6)) {
        printf("%s %s <datasetIdDecimal> <serverPath (someroot/parentPath/nameOfFile.txt)> "
               "<compId> <uploadRevision> <absLocalPath> "
               "\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];
    u64 compId = atol(argv[3]);
    u64 uploadRevision = atol(argv[4]);
    std::string absLocalPath = argv[5];

    int rc;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    // Get the 3rd party storage URL (accessUrl) from VCS GET accessinfo (pass method=PUT).
    VcsAccessInfo accessInfoResp;
    {
        VPLHttp2 httpHandle;
        rc = vcs_access_info_for_file_put(vcsSession,
                                          httpHandle,
                                          getVcsDataset(datasetId),
                                          true,
                                          accessInfoResp);
    }
    if (rc != 0) {
        LOG_ERROR("vcs_access_info_for_file_put:%d", rc);
        rv = rc;
        goto end;
    }
    // Upload file to 3rd party storage.
    {
        VPLHttp2 httpHandle;
        rc = vcs_s3_putFileHelper(accessInfoResp,
                                  httpHandle,
                                  NULL,
                                  NULL,
                                  absLocalPath,
                                  true);
    }
    if (rc != 0) {
        LOG_ERROR("vcs_s3_putFileHelper:%d", rc);
        rv = rc;
    }

    LOG_ALWAYS("Post access URL: %s\n"
               "    path: %s,\n"
               "    compId: "FMTu64",\n"
               "    uploadRevision: "FMTu64,
               accessInfoResp.accessUrl.c_str(),
               serverPath.c_str(),
               compId,
               uploadRevision);
    {
        VPLHttp2 httpHandle;
        rv = vcs_post_acs_access_url_virtual(vcsSession,
                                             httpHandle,
                                             getVcsDataset(datasetId),
                                             serverPath,
                                             compId,
                                             uploadRevision,
                                             accessInfoResp.accessUrl,
                                             true);

        if(rv != 0) {
            LOG_ERROR("vcs_post_acs_access_url_virtual:%d, dset:"FMTu64", folder:%s",
                      rv, datasetId, serverPath.c_str());
            return rv;
        }
    }
    LOG_ALWAYS("Post access URL: %s\n"
               "    path: %s,\n"
               "    compId: "FMTu64",\n"
               "    uploadRevision: "FMTu64,
               accessInfoResp.accessUrl.c_str(),
               serverPath.c_str(),
               compId,
               uploadRevision);
 end:
    return rv;
}

int cmd_vcs_file_put_virtual(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 8)) {
        printf("%s %s <datasetIdDecimal> <serverPath (someroot/parentPath/nameOfFile.txt)> <parentPathCompId> "
               "<compId (0 for new)> <uploadRevision (1 for new)> <unixTimeSec> <fileSizeBytes> "
               "\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];
    u64 parentCompId = atol(argv[3]);
    u64 compId = atol(argv[4]);
    u64 uploadRevision = atol(argv[5]);
    u64 vplTime = VPLTime_FromSec(atol(argv[6]));
    u64 fileSize = atol(argv[7]);

    int rc;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    {
        VPLHttp2 handleHttpPostFilemetadata;
        VcsFileMetadataResponse response_out;
        rc = vcs_post_file_metadata(vcsSession,
                                    handleHttpPostFilemetadata,
                                    getVcsDataset(datasetId),
                                    serverPath,
                                    parentCompId,
                                    compId!=0,       // compId is unknown for new files
                                    compId,          // Only valid when hasCompId==true
                                    uploadRevision,  // lastKnownRevision+1, (for example, 1 for new files)
                                    vplTime,  // infoLastChanged
                                    vplTime,  // infoCreateDate
                                    fileSize,
                                    "VCS_TEST_CONTENT_HASH",
                                    "VCS_TEST_CLIENT_GEN_HASH",
                                    12345678901ULL,  // infoUpdateDeviceId
                                    false,
                                    std::string(""),
                                    true,
                                    response_out);
        if(rc != 0) {
            LOG_ERROR("vcs_post_file_metadata:%d", rc);
        }

        printFileMetadataResponse(response_out);
    }
    return rv;
}

int cmd_vcs_access_info_for_file_put(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 7)) {
        printf("%s %s <datasetIdDecimal> <serverPath (someroot/parentPath/nameOfFile.txt)> <parentPathCompId> "
               "<compId (0 for new)> <uploadRevision (1 for new)> <localSrcFile>\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];
    u64 parentCompId = atol(argv[3]);
    u64 compId = atol(argv[4]);
    u64 uploadRevision = atol(argv[5]);
    std::string localSrcFile = argv[6];

    int rc;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;  // Single use
    VcsAccessInfo accessInfoResponse;
    rc = vcs_access_info_for_file_put(vcsSession,
                                      httpHandle,
                                      getVcsDataset(datasetId),
                                      true,
                                      accessInfoResponse);
    if(rc != 0) {
        LOG_ERROR("vcs_access_info_for_file_put:%d", rc);
        return rc;
    }

    LOG_ALWAYS("AccessInfoForPutFile:\n"
               "   accessUrl:%s\n"
               "   LocationName:%s\n"
               "   Header:",
               accessInfoResponse.accessUrl.c_str(),
               accessInfoResponse.locationName.c_str());
    for(u32 headerIdx=0; headerIdx<accessInfoResponse.header.size();++headerIdx)
    {
        printf("      %s:%s\n",
               accessInfoResponse.header[headerIdx].headerName.c_str(),
               accessInfoResponse.header[headerIdx].headerValue.c_str());
    }

    {
        LOG_ALWAYS("Uploading file:%s", localSrcFile.c_str());
        VPLHttp2 handleUpload;
        rc = vcs_s3_putFileHelper(accessInfoResponse,
                                  handleUpload,
                                  NULL,
                                  NULL,
                                  localSrcFile,
                                  true);
        if(rc != 0) {
            LOG_ERROR("vcs_s3_putFileHelper:%d", rc);
            return rc;
        }
    }
    LOG_ALWAYS("Upload complete. Posting file metadata.");

    {
        VPLHttp2 handleHttpPostFilemetadata;
        VcsFileMetadataResponse response_out;
        VPLFS_stat_t statBuf;
        rc = VPLFS_Stat(localSrcFile.c_str(), &statBuf);
        if(rc != 0) {
            LOG_ERROR("Could not stat:%d, %s", rc, localSrcFile.c_str());
        }
        rc = vcs_post_file_metadata(vcsSession,
                                    handleHttpPostFilemetadata,
                                    getVcsDataset(datasetId),
                                    serverPath,
                                    parentCompId,
                                    compId!=0,       // compId is unknown for new files
                                    compId,          // Only valid when hasCompId==true
                                    uploadRevision,  // lastKnownRevision+1, (for example, 1 for new files)
                                    statBuf.vpl_mtime,  // infoLastChanged
                                    statBuf.vpl_ctime,  // infoCreateDate
                                    (u64)statBuf.size,
                                    "VCS_TEST_CONTENT_HASH",
                                    "VCS_TEST_CLIENT_GEN_HASH",
                                    12345678901ULL,  // infoUpdateDeviceId
                                    true,
                                    accessInfoResponse.accessUrl,
                                    true,
                                    response_out);
        if(rc != 0) {
            LOG_ERROR("vcs_post_file_metadata:%d", rc);
        }

        printFileMetadataResponse(response_out);
    }
    return rv;
}


static void printPostFileMetadataBatchResponse(
        const VcsBatchFileMetadataResponse& postFileMetadataBatchResponse)
{
    LOG_ALWAYS("===PostFileMetadataBatchResponse===");
    printf(" entriesFailed:"FMTu64",entriesReceived:"FMTu64"\n",
           postFileMetadataBatchResponse.entriesFailed,
           postFileMetadataBatchResponse.entriesReceived);
    //postFileMetadataBatchResponse.
    for(std::vector<VcsBatchFileMetadataResponse_Folder>::const_iterator
            folderIter = postFileMetadataBatchResponse.folders.begin();
        folderIter != postFileMetadataBatchResponse.folders.end();
        ++folderIter)
    {
        printf("   parentFolderCompId("FMTu64")\n", folderIter->folderCompId);

        for(std::vector<VcsBatchFileMetadataResponse_File>::const_iterator
                fileIter = folderIter->files.begin();
            fileIter != folderIter->files.end();
            ++fileIter)
        {
            fileIter->name;

            printf("     %s - (compId:"FMTu64", success:%d, numRevision:"FMTu64
                   ", lastChangedNano:"FMTu64", createDateNano:"FMTu64", errCode:%d\n",
                   fileIter->name.c_str(),
                   fileIter->compId,
                   fileIter->success?1:0,
                   fileIter->numOfRevisions,
                   fileIter->lastChanged,
                   fileIter->createDate,
                   fileIter->errCode);

            for(std::vector<VcsBatchFileMetadataResponse_RevisionList>::const_iterator
                    revIter = fileIter->revisionList.begin();
                revIter != fileIter->revisionList.end();
                ++revIter)
            {
                printf("          rev:"FMTu64", size:"FMTu64", lastChanged:"FMTu64
                       ", updateDevice:"FMTu64", previewUri:%s, noAcs:%d\n",
                       revIter->revision,
                       revIter->size,
                       VPLTime_ToSec(revIter->lastChangedSecResolution),
                       revIter->updateDevice,
                       revIter->previewUri.c_str(),
                       revIter->noACS?1:0);
            }
        }
    }
}

int cmd_vcs_create_batch_post_file_metadata(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 2)) {
        printf("%s %s <pfm_batchId (identifier, make up a number/suffix)> \n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }

    std::string filename = std::string("pfm_batch_")+argv[1]+".csv";

    VPLFile_handle_t batchFile = VPLFILE_INVALID_HANDLE;

    batchFile = VPLFile_Open(filename.c_str(),
                             VPLFILE_OPENFLAG_CREATE |
                                 VPLFILE_OPENFLAG_TRUNCATE |
                                 VPLFILE_OPENFLAG_WRITEONLY,
                             0777);
    if(!VPLFile_IsValidHandle(batchFile)) {
        LOG_ERROR("VPLFile_Open(%s):%d", filename.c_str(), batchFile);
        return -1;
    }

    // define PFM_BATCH_FILE_FORMAT
    std::string toWrite;
    toWrite = "# Barebones CSV file (does not escape characters, '#' in first column to comment)\n"
              "# parentPathCompId, serverPath (dset relatvive), serverFilename, "
              "compId (0 for new), uploadRevision (1 for new), "
              "uploadMode(normal/pureVirtual/virtual), localSrcFile\n";

    int rc = VPLFile_Write(batchFile, toWrite.c_str(), toWrite.size());
    if(rc != toWrite.size()) {
        LOG_ERROR("VPLFile_Write(%s):%d != %d", filename.c_str(), rc, toWrite.size());
        rv = rc;
    }
    toWrite = "# EXAMPLES:\n"
              "#673859646,/igware_dir_1,file1.txt,0,1,normal,/home/igware/dxshell/file1.txt\n"
              "#673859646,/igware_dir_1,file2.txt,0,1,normal,/home/igware/dxshell/file2.txt\n"
              "#745411455,/igware_dir_2,file1.txt,0,1,normal,/home/igware/dxshell/file1.txt\n";
    rc = VPLFile_Write(batchFile, toWrite.c_str(), toWrite.size());
    if(rc != toWrite.size()) {
        LOG_ERROR("VPLFile_Write(%s):%d != %d", filename.c_str(), rc, toWrite.size());
        if (rv==0) {rv = rc;}
    }

    rc = VPLFile_Close(batchFile);
    if (rc != 0) {
        LOG_ERROR("VPLFile_Close:%d", rc);
    }

    LOG_ALWAYS("\n =============================="
               "\n   ==== Created ./%s file to create a batch uploads.  Please append batch"
               "\n   ==== upload entries into the file, and call \"./dxshell Vcs BatchFileUpload\" "
               "\n   ==== to read the batchFile and actually upload the specified files in a batch."
               "\n ==============================",
               filename.c_str());
    return rv;
}

static int uploadFile(VcsSession& vcsSession,
                      u64 datasetId,
                      const std::string& localSrcFile,
                      VcsAccessInfo& accessInfoResponse_out)
{
    int rc;
    VPLHttp2 httpHandle;  // Single use

    rc = vcs_access_info_for_file_put(vcsSession,
                                      httpHandle,
                                      getVcsDataset(datasetId),
                                      true,
                                      accessInfoResponse_out);
    if(rc != 0) {
        LOG_ERROR("vcs_access_info_for_file_put:%d", rc);
        return rc;
    }

    LOG_ALWAYS("AccessInfoForPutFile:\n"
               "   accessUrl:%s\n"
               "   LocationName:%s\n"
               "   Header:",
               accessInfoResponse_out.accessUrl.c_str(),
               accessInfoResponse_out.locationName.c_str());
    for(u32 headerIdx=0; headerIdx<accessInfoResponse_out.header.size();++headerIdx)
    {
        printf("      %s:%s\n",
               accessInfoResponse_out.header[headerIdx].headerName.c_str(),
               accessInfoResponse_out.header[headerIdx].headerValue.c_str());
    }

    {
        LOG_ALWAYS("Uploading file:%s", localSrcFile.c_str());
        VPLHttp2 handleUpload;
        rc = vcs_s3_putFileHelper(accessInfoResponse_out,
                                  handleUpload,
                                  NULL,
                                  NULL,
                                  localSrcFile,
                                  true);
        if(rc != 0) {
            LOG_ERROR("vcs_s3_putFileHelper:%d", rc);
            return rc;
        }
    }
    LOG_ALWAYS("Upload(%s) complete.", localSrcFile.c_str());
    return rc;
}

static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

int cmd_vcs_batch_post_file_metadata(int argc, const char* argv[])
{
    int rv = 0;
    int rc;
    if(checkHelp(argc, argv) || (argc != 3)) {
        printf("%s %s <datasetIdDecimal> <pfm_batchId (suffix of file created by 'VCS BatchFileUploadCreate')> \n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    u64 datasetId = atol(argv[1]);
    std::string filename = std::string("pfm_batch_")+argv[2]+".csv";

    {
        VPLFS_stat_t stat_buf;
        rc = VPLFS_Stat(filename.c_str(), &stat_buf);
        if (rc != 0) {
            LOG_ERROR("VPLFS_Stat(%s):%d", filename.c_str(), rc);
            return rc;
        }
    }

    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    std::fstream batchFile;
    batchFile.open(filename.c_str());
    if(!batchFile.is_open()) {
        LOG_ERROR("fstream open(%s)", filename.c_str());
        return -1;
    }

    std::vector<VcsBatchFileMetadataRequest> postFileMetadataBatchReq;

    // parse PFM_BATCH_FILE_FORMAT
    u32 lineNum = 0;
    while (batchFile.good())
    {
        std::string line;
        std::getline(batchFile,line);
        lineNum++;

        if (line.empty()) {
            continue;
        }
        if (line[0] == '#') { // simple comment support
            continue;
        }
        std::vector<std::string> entries;
        split(line, ',', /*OUT*/ entries);

        const static u32 ENTRIES_EXPECTED = 7;
        if (entries.size() != ENTRIES_EXPECTED) {
            LOG_ERROR("(%s,Line %d) unexpected number of entries %d. Expecting %d",
                      filename.c_str(), lineNum, entries.size(), ENTRIES_EXPECTED);
            continue;
        }

        // parse PFM_BATCH_FILE_FORMAT
        u64 parentPathCompId = VPLConv_strToU64(entries[0].c_str(), NULL, 10);
        std::string serverPath = entries[1];
        std::string serverFilename = entries[2];
        u64 compId = VPLConv_strToU64(entries[3].c_str(), NULL, 10);
        u64 uploadRevision = VPLConv_strToU64(entries[4].c_str(), NULL, 10);
        std::string uploadMode = entries[5];
        std::string localSrcFile = entries[6];

        if (uploadMode != "normal") {
            LOG_ERROR("(%s,Line %d) uploadMode(%s) not supported."
                      " Only \"normal\" currently supported. Quitting.",
                      filename.c_str(), lineNum, uploadMode.c_str());
            return -2;
        }

        VPLFS_stat_t stat_buf;
        rc = VPLFS_Stat(localSrcFile.c_str(), &stat_buf);
        if (rc != 0) {
            LOG_ERROR("(%s,Line %d) VPLFS_Stat(%s):%d",
                      filename.c_str(), lineNum, localSrcFile.c_str(), rc);
            continue;;
        }

        VcsAccessInfo accessInfoResponse;
        rc = uploadFile(vcsSession,
                        datasetId,
                        std::string("file1.txt"),
                        /*OUT*/accessInfoResponse);
        if (rc != 0) {
            LOG_ERROR("(%s,Line %d) uploadFile:%d", filename.c_str(), lineNum, rc);
        }

        VcsBatchFileMetadataRequest pfmRequest;
        pfmRequest.folderCompId = parentPathCompId;
        pfmRequest.folderPath = serverPath;
        pfmRequest.fileName = serverFilename;
        pfmRequest.size = static_cast<u64>(stat_buf.size);
        pfmRequest.updateDevice = 12345678901ULL;
        pfmRequest.hasAccessUrl = true;
        pfmRequest.accessUrl = accessInfoResponse.accessUrl;
        pfmRequest.lastChanged = stat_buf.vpl_mtime;
        pfmRequest.createDate = stat_buf.vpl_ctime;
        pfmRequest.hasContentHash = true;
        pfmRequest.contentHash = "VCS_TEST_CONTENT_HASH";
        pfmRequest.uploadRevision = uploadRevision;
        if (compId != 0) {
            pfmRequest.hasCompId = true;
            pfmRequest.compId = compId;
        }

        postFileMetadataBatchReq.push_back(pfmRequest);
    }

    VPLHttp2 handleHttpBatchPostFilemetadata;
    VcsBatchFileMetadataResponse postFileMetadataBatchResponse;
    rc = vcs_batch_post_file_metadata(vcsSession,
                                      handleHttpBatchPostFilemetadata,
                                      getVcsDataset(datasetId),
                                      true,
                                      postFileMetadataBatchReq,
                                      postFileMetadataBatchResponse);
    if (rc != 0) {
        LOG_ERROR("vcs_batch_post_file_metadata:%d", rc);
        rv = rc;
    }

    LOG_ALWAYS("Printing:");
    printPostFileMetadataBatchResponse(postFileMetadataBatchResponse);
    return rv;
}

int cmd_vcs_access_info_for_file_get(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 5)) {
        printf("%s %s <datasetIdDecimal> <serverPath> <compId> <revision>\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];
    u64 compId = atol(argv[3]);
    u64 revision = atol(argv[4]);

    int rc;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;  // Single use
    VcsAccessInfo accessInfoResponse;
    rc = vcs_access_info_for_file_get(vcsSession,
                                      httpHandle,
                                      getVcsDataset(datasetId),
                                      serverPath,
                                      compId,
                                      revision,
                                      true,
                                      accessInfoResponse);
    if(rc != 0) {
        LOG_ERROR("vcs_access_info_for_file_get:%d", rc);
        return rc;
    }

    LOG_ALWAYS("AccessInfoForGetFile:\n"
               "   accessUrl:%s\n"
               "   LocationName:%s\n"
               "   Header:",
               accessInfoResponse.accessUrl.c_str(),
               accessInfoResponse.locationName.c_str());
    for(u32 headerIdx=0; headerIdx<accessInfoResponse.header.size();++headerIdx)
    {
        printf("      %s:%s\n",
               accessInfoResponse.header[headerIdx].headerName.c_str(),
               accessInfoResponse.header[headerIdx].headerValue.c_str());
    }

    return rv;
}

int cmd_vcs_get_comp_id(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 3)) {
        printf("%s %s <datasetIdDecimal> <serverPath>\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];

    int rc;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;  // Single use
    u64 compId_out = 0;
    rc = vcs_get_comp_id(vcsSession,
                         httpHandle,
                         getVcsDataset(datasetId),
                         serverPath,
                         true,
                         compId_out);
    if(rc != 0) {
        LOG_ERROR("vcs_get_comp_id:%d", rc);
        return rc;
    }
    LOG_ALWAYS("CompId of (%s): "FMTu64, serverPath.c_str(), compId_out);
    return rv;
}

int cmd_vcs_get_dataset_info(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 2)) {
        printf("%s %s <datasetIdDecimal>\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }

    u64 datasetId = atol(argv[1]);

    int rc;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;  // Single use
    u64 currentVersion_out = 0;
    rc = vcs_get_dataset_info(vcsSession,
                              httpHandle,
                              getVcsDataset(datasetId),
                              true,
                              currentVersion_out);
    if(rc != 0) {
        LOG_ERROR("vcs_get_dataset_info:%d", rc);
        return rc;
    }
    LOG_ALWAYS("DatasetVersion: "FMTu64, currentVersion_out);
    return rv;
}

static void printVcsV1FileMetadataResponse(const VcsV1_postFileMetadataResponse& response)
{
    LOG_ALWAYS("File Metadata Response:\n"
               " name:%s\n"
               " compId:"FMTu64"\n"
               " originDevice:"FMTu64"\n"
               " numRevisions:"FMTu64,
               response.name.c_str(),
               response.compId,
               response.originDevice,
               response.numOfRevisions);
    for(std::vector<VcsV1_revisionListEntry>::const_iterator revisionIt = response.revisionList.begin();
        revisionIt != response.revisionList.end();
        ++revisionIt)
    {
        const VcsV1_revisionListEntry& currRevision = *revisionIt;
        LOG_ALWAYS("File Metadata Response Revision Continued:\n"
                   "    revision:"FMTu64"\n"
                   "    size:"FMTu64"\n"
                   "    previewUri:%s\n"
                   "    updateDevice:"FMTu64,
                   currRevision.revision,
                   currRevision.size,
                   currRevision.previewUri.c_str(),
                   currRevision.updateDevice);
    }
}

int cmd_vcs_v1_access_info_for_file_put(int argc, const char* argv[])
{
    int rv = 0;
    if(checkHelp(argc, argv) || (argc != 7)) {
        printf("%s %s <datasetIdDecimal> <serverPath (someroot/parentPath/nameOfFile.txt)> <localSrcFile>\n",
               VCS_CMD_STR, argv[0]);
        return 0;
    }
    u64 datasetId = atol(argv[1]);
    std::string serverPath = argv[2];
    //u64 parentCompId = atol(argv[3]);
    //u64 compId = atol(argv[4]);
    //u64 uploadRevision = atol(argv[5]);
    std::string localSrcFile = argv[3];

    int rc;
    VcsTestSession vcsTestSession;
    std::string sessionFilepath = GetVcsTestSessionPath();
    rc = readVcsTestSessionFile(sessionFilepath, true, vcsTestSession);
    if(rc != 0) {
        LOG_ERROR("Cannot read VCS session file:%s, %d.  Please call \"Vcs StartSession\"",
                  GetVcsTestSessionPath().c_str(), rc);
        return rc;
    }
    VcsSession vcsSession;
    getVcsSessionFromVcsTestSession(vcsTestSession, vcsSession);

    VPLHttp2 httpHandle;  // Single use
    VcsAccessInfo accessInfoResponse;
    rc = VcsV1_getAccessInfo(vcsSession,
                              httpHandle,
                              getVcsDataset(datasetId),
                              true,
                              accessInfoResponse);
    if(rc != 0) {
        LOG_ERROR("vcs_access_info_for_file_put:%d", rc);
        return rc;
    }

    LOG_ALWAYS("AccessInfoForPutFile:\n"
               "   accessUrl:%s\n"
               "   LocationName:%s\n"
               "   Header:",
               accessInfoResponse.accessUrl.c_str(),
               accessInfoResponse.locationName.c_str());
    for(u32 headerIdx=0; headerIdx<accessInfoResponse.header.size();++headerIdx)
    {
        printf("      %s:%s\n",
               accessInfoResponse.header[headerIdx].headerName.c_str(),
               accessInfoResponse.header[headerIdx].headerValue.c_str());
    }

    {
        LOG_ALWAYS("Uploading file:%s", localSrcFile.c_str());
        VPLHttp2 handleUpload;
        rc = vcs_s3_putFileHelper(accessInfoResponse,
                                  handleUpload,
                                  NULL,
                                  NULL,
                                  localSrcFile,
                                  true);
        if(rc != 0) {
            LOG_ERROR("vcs_s3_putFileHelper:%d", rc);
            return rc;
        }
    }
    LOG_ALWAYS("Upload complete. Posting file metadata.");

    {
        VPLHttp2 handleHttpPostFilemetadata;
        VcsV1_postFileMetadataResponse response;
        VPLFS_stat_t statBuf;
        rc = VPLFS_Stat(localSrcFile.c_str(), &statBuf);
        if(rc != 0) {
            LOG_ERROR("Could not stat:%d, %s", rc, localSrcFile.c_str());
        }
        rc = VcsV1_share_postFileMetadata(vcsSession,
                                          handleHttpPostFilemetadata,
                                          getVcsDataset(datasetId),
                                          serverPath,
                                          statBuf.vpl_mtime,  // infoLastChanged
                                          statBuf.vpl_ctime,  // infoCreateDate
                                          (u64)statBuf.size,
                                          true,
                                          "THIS_IS_OPAQUE_METADATA",
                                          false,
                                          "VCS_TEST_CLIENT_GEN_HASH",
                                          12345678901ULL,  // infoUpdateDeviceId
                                          accessInfoResponse.accessUrl,
                                          true,
                                          /*OUT*/ response);
        if(rc != 0) {
            LOG_ERROR("vcs_post_file_metadata:%d", rc);
        }

        printVcsV1FileMetadataResponse(response);
    }
    return rv;
}

int cmd_vcspic_put_preview(int argc, const char* argv[])
{
    return 0;
}
