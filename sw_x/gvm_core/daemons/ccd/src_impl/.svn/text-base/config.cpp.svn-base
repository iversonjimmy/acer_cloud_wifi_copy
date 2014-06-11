//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "config.h"
#include "vplex_strings.h"
#ifdef VPL_PLAT_IS_WINRT
#include "vpl_fs.h"
#include "vplex_file.h"
#endif
#ifdef IOS
#include "KeychainFileReader-C-Interface.h"
#endif
CCDConfig __ccdConfig;

#  define DEFAULT_PRX_CONN_IDLE_TIMEOUT (5*60)
#  define DEFAULT_P2P_CONN_IDLE_TIMEOUT (5*60)
#  define DEFAULT_DEX_CONN_IDLE_TIMEOUT (5*60)
#  define DEFAULT_DIN_CONN_IDLE_TIMEOUT (5*60)

#if defined(CLOUDNODE) || (defined(WIN32) && !defined(VPL_PLAT_IS_WINRT))
#define DEFAULT_ENABLE_TS (CONFIG_ENABLE_TS_USE_HTTPSVC_CCD|CONFIG_ENABLE_TS_INIT_TS_IN_SN|CONFIG_ENABLE_TS_INIT_TS_EXT)
#else
#define DEFAULT_ENABLE_TS (CONFIG_ENABLE_TS_USE_HTTPSVC_CCD|CONFIG_ENABLE_TS_INIT_TS_IN_SN)
#endif

#ifdef ANDROID
static std::string s_brandName;
#endif

#define DEFAULT_ATTEMPT_PRX_DELAY 4

#define DEFAULT_HTTPSVC_OPT CONFIG_HTTPSVC_OPT_TRY_FUNCALL_SN

#if CCD_ENABLE_IOT_SDK_HTTP_API
#define DEFAULT_CCD_HTTP_SERVICE_LISTENING_PORT         17200
#else
#define DEFAULT_CCD_HTTP_SERVICE_LISTENING_PORT         0
#endif

// (name, default_value)
#define INT_CONFIG_PARAMS \
    ROW(debugLog, 0) \
    ROW(debugInfraHttp, 1) \
    ROW(debugIasHttp, 1) \
    ROW(debugNusHttp, 1) \
    ROW(debugVsdsHttp, 1) \
    ROW(debugVcsHttp, 1) \
    ROW(debugAcsHttp, 0) \
    ROW(enableLogFiles, 1) \
    ROW(writeToSystemLog, 0) \
    ROW(mediaMetadataUploadVirtualSync, 1) \
    ROW(mediaMetadataSyncDownloadFromArchiveDevice, 1) \
    ROW(mediaMetadataThumbVirtDownload, 1) \
    ROW(vsdsContentPort, 443) \
    ROW(iasPort, 443) \
    ROW(nusPort, 443) \
    ROW(ansPort, 443) \
    ROW(clearfiPort, 16960) \
    ROW(clearfiMode, 0) \
    ROW(clearfiUserServerPort, DEFAULT_CCD_HTTP_SERVICE_LISTENING_PORT) \
    ROW(secureClearfiUserServerPort, 0) \
    ROW(storageNodeClientPort, 0) \
    ROW(syncPendingCommitNumberThreshold, 200) \
    ROW(syncPendingCommitBytesThreshold, (1024*1024)) \
    ROW(testInstanceNum, 0) \
    ROW(serveDataKeepAwakeTimeSec, (600)) \
    ROW(sleepPacketInterval, 0) \
    ROW(useThumbnailCache, 1) \
    ROW(ioacProtocolOffloadMode, 7) \
    ROW(ioacArpOffloadForceOnIntel, 0) \
    ROW(ioacDontCheckWakeReason, 0) \
    ROW(maxTotalLogSize, (10*1024*1024)) \
    ROW(prxConnIdleTimeout, DEFAULT_PRX_CONN_IDLE_TIMEOUT) \
    ROW(p2pConnIdleTimeout, DEFAULT_P2P_CONN_IDLE_TIMEOUT) \
    ROW(dexConnIdleTimeout, DEFAULT_DEX_CONN_IDLE_TIMEOUT) \
    ROW(dinConnIdleTimeout, DEFAULT_DIN_CONN_IDLE_TIMEOUT) \
    ROW(enableVssiNewDatasetPath, 0) \
    ROW(syncUpRetryInterval, 900) \
    ROW(syncDownRetryInterval, 900) \
    ROW(uploadThreadPoolSize, 4) \
    ROW(downloadAndUploadUsesUploadThreadPool, 0) \
    ROW(downloadThreadPoolSize, 4) \
    ROW(enableTs, DEFAULT_ENABLE_TS) \
    ROW(statMgrReportInterval, 86400) \
    ROW(statMgrMaxEvent, 100) \
    ROW(httpSvcOpt, DEFAULT_HTTPSVC_OPT) \
    ROW(ts2MaxSegmentSize, 64*1024) \
    ROW(ts2MaxWindowSize, (128*1024)) \
    ROW(ts2SendBufSize, 0) \
    ROW(ts2AttemptPrxP2PDelaySec, DEFAULT_ATTEMPT_PRX_DELAY) \
    ROW(ts2UsePrxDelaySec, 0) \
    ROW(ts2RetransmitIntervalMs, 5000) \
    ROW(ts2MinRetransmitIntervalMs, 100) \
    ROW(ts2MaxRetransmitRound, 8) \
    ROW(ts2EventsTimeoutMs, -1000) \
    ROW(ts2DevAidParam, 0) \
    ROW(ts2PktDropParam, 30) \
    ROW(ts2TsOpenTimeoutMs, 30000) \
    ROW(ts2TsReadTimeoutMs, 30000) \
    ROW(ts2TsWriteTimeoutMs, 30000) \
    ROW(ts2TsCloseTimeoutMs, 30000) \
    ROW(ts2FinWaitTimeoutMs, 4000) \
    ROW(ts2PacketSendTimeoutMs, 18000) \
    ROW(ts2TestNetworkEnv, 0) \
    ROW(syncboxSyncConfigErrPollInterval, 0) \

// (name, default_value)
#define STRING_CONFIG_PARAMS \
    ROW(infraDomain, "cloud.acer.com") \
    ROW(clearfiUserServerName, "0.0.0.0") \
    ROW(tagEditPath, "TagEdit.exe") \
    ROW(userGroup, "") \
    ROW(serverServicePortRange, "17000-17100")

static void
Config_Parse(void)
{
    CONFVariable var;

#define ROW(name_, default_value_) \
    if (CONFGetVariable(&var, #name_) == CONF_OK) { \
        __ccdConfig.name_ = atoi(var.value); \
    }
    INT_CONFIG_PARAMS;
#undef ROW

#define ROW(name_, default_value_) \
    if (CONFGetVariable(&var, #name_) == CONF_OK) { \
        VPLString_SafeStrncpy(__ccdConfig.name_, ARRAY_SIZE_IN_BYTES(__ccdConfig.name_), var.value); \
    }
    STRING_CONFIG_PARAMS;
#undef ROW
}

static void
Config_SetDefaults(void)
{
#define ROW(name_, default_value_)  __ccdConfig.name_ = default_value_;
    INT_CONFIG_PARAMS;
#undef ROW

#define ROW(name_, default_value_)  VPLString_SafeStrncpy(__ccdConfig.name_, ARRAY_SIZE_IN_BYTES(__ccdConfig.name_), default_value_);
    STRING_CONFIG_PARAMS;
#undef ROW
}

#ifdef VPL_PLAT_IS_WINRT
static void
Config_MoveCcdConf(Platform::String^ data, const char* local_app_data_path)
{
    //file read success, try to create conf folder
    std::string szPath = "";
    szPath.append(local_app_data_path);
    szPath.append("/conf");
    if(VPLFile_CheckAccess(szPath.c_str(), VPLFILE_CHECKACCESS_EXISTS) != 0) {
        VPLDir_Create(szPath.c_str(), 0777);
    }
    //create conf file
    szPath.append("/ccd.conf");
    VPLFile_handle_t newConfFile = VPLFile_Open(szPath.c_str(), VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_READWRITE, 0777);
    if( VPLFile_IsValidHandle(newConfFile) ) {
        //prepare data
        char* dataA = NULL;
        _VPL__wstring_to_utf8_alloc(data->Data(),&dataA);
        //write conf data to file
        VPLFile_Write(newConfFile,dataA,strlen(dataA));
        VPLFile_Close(newConfFile);
        if(dataA != NULL)
            free(dataA);
    }
}

static void 
Config_DeleteCcdConf(const char* local_app_data_path) 
{
    std::string confPath = "";
    confPath.append(local_app_data_path);
    confPath.append("/conf/ccd.conf");
    VPLFile_Delete(confPath.c_str());
}

static void 
CCDConfig_CheckCcdConf(const char* local_app_data_path) 
{
    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        return;
    }

    //check if ccd.conf file created in Music Library folder
    Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFolder^>^ getSyncAgentAction = nullptr;
    try {
        getSyncAgentAction = Windows::Storage::KnownFolders::MusicLibrary->GetFolderAsync(_VPL__SharedAppFolder);
    }
    catch(Platform::Exception^ e){
        Config_DeleteCcdConf(local_app_data_path);
        return;
    }

    getSyncAgentAction->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Storage::StorageFolder^>(
        [local_app_data_path,&completedEvent] (Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFolder^>^ op, Windows::Foundation::AsyncStatus status) {
            //get "AcerCloud" folder in Music Library
            try {
                auto getConfFolderAction = op->GetResults()->GetFolderAsync(L"conf");
                getConfFolderAction->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Storage::StorageFolder^>(
                    [local_app_data_path,&completedEvent] (Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFolder^>^ op, Windows::Foundation::AsyncStatus status) {
                        //get "conf" folder in "AcerCloud"
                        try {
                            auto getFileAction = op->GetResults()->GetFileAsync(L"ccd.conf");
                            getFileAction->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Windows::Storage::StorageFile^>(
                                [local_app_data_path,&completedEvent] (Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFile^>^ op, Windows::Foundation::AsyncStatus status) {
                                    //get ccd.conf file
                                    try {
                                        auto readAction = Windows::Storage::FileIO::ReadTextAsync(op->GetResults());
                                        readAction->Completed = ref new Windows::Foundation::AsyncOperationCompletedHandler<Platform::String^>(
                                            [local_app_data_path,&completedEvent] (Windows::Foundation::IAsyncOperation<Platform::String^>^ op, Windows::Foundation::AsyncStatus status) {
                                                //raed data from ccd.conf file
                                                try {
                                                    Config_MoveCcdConf(op->GetResults(),local_app_data_path);
                                                    SetEvent(completedEvent);
                                                }
                                                catch(Platform::Exception^ e){
                                                    //if exception, ccd.conf NOT exist in Music Library -> use default
                                                    //delete old ccd.conf 
                                                    Config_DeleteCcdConf(local_app_data_path);
                                                    SetEvent(completedEvent);
                                                }
                                            }
                                        );
                                    }
                                    catch(Platform::Exception^ e){
                                        //if exception, ccd.conf NOT exist in Music Library -> use default
                                        //delete old ccd.conf 
                                        Config_DeleteCcdConf(local_app_data_path);
                                        SetEvent(completedEvent);
                                    }
                                }
                            );
                        }
                        catch(Platform::Exception^ e){
                            //if exception, ccd.conf NOT exist in Music Library -> use default
                            //delete old ccd.conf 
                            Config_DeleteCcdConf(local_app_data_path);
                            SetEvent(completedEvent);
                        }
                    }
                );
            }
            catch(Platform::Exception^ e){
                //if exception, ccd.conf NOT exist in Music Library -> use default
                //delete old ccd.conf 
                Config_DeleteCcdConf(local_app_data_path);
                SetEvent(completedEvent);
            }
        }
    );

    WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
    CloseHandle(completedEvent);
}
#endif

#ifdef ANDROID
void CCDConfig_SetBrandName(const char* brandName)
{
    if (brandName != NULL) {
        s_brandName.assign(brandName);
    } else {
        s_brandName.assign("AcerCloud");
    }
}
#endif

int
CCDConfig_Init(const char* local_app_data_path)
{
    int rv = CCD_OK;
#ifdef ANDROID
    std::string androidConfPath;
#endif

    Config_SetDefaults();

    rv = CONFInit();
    if (rv < 0) {
        goto out;
    }

    {
        // path = %LOCAL_APP_DATA%/conf/common.conf & %LOCAL_APP_DATA%/conf/ccd.conf
        // 17 = strlen("/conf/common.conf")
        // 1 = null terminal (\0)
        char path[LOCAL_APP_DATA_MAX_LENGTH+17+1];
        if (local_app_data_path != NULL) {
            VPLString_SafeStrncpy(path, ARRAY_SIZE_IN_BYTES(path), local_app_data_path);
        } else {
            VPLString_SafeStrncpy(path, ARRAY_SIZE_IN_BYTES(path), GVM_DEFAULT_LOCAL_APP_DATA_PATH);
        }
        // Remove any trailing slash(es) and leave len set to the index of the null-terminator.
        int len = strlen(path);
        while ((len > 0) && (path[len - 1] == '/')) {
            len--;
            path[len] = '\0';
        }

#ifdef VPL_PLAT_IS_WINRT
        // due to WinRT file access limitation, actool_winRT will generate ccd.conf in Music Library
        // so we check if ccd.conf exists in Music Library
        // if yes, move it to local_app_data_path/conf folder
        // if no (which means use default), we delete ccd.conf in local_app_data_path/conf folder
        CCDConfig_CheckCcdConf(local_app_data_path);        
#endif
        
#ifdef IOS
        if (getenv("IGNORE_ACTOOL_KEYCHAIN") == NULL) {
            // IOS use keychain to pass the ccd.conf from actool
            IOS_CCDConfig_CheckCcdConf(local_app_data_path);
        } else {
            LOG_INFO("Do not get ccd.conf for keychain.");
        }
#endif

        VPLString_SafeStrncpy(path + len, ARRAY_SIZE_IN_BYTES(path) - len, "/conf/common.conf");
        CONFLoad(path);

        VPLString_SafeStrncpy(path + len, ARRAY_SIZE_IN_BYTES(path) - len, "/conf/ccd.conf");
        CONFLoad(path);

#ifdef ANDROID
        androidConfPath.append("/sdcard/AOP/").append(s_brandName.c_str()).append("/conf/ccd.conf");
        LOG_INFO("androidConfPath: %s", androidConfPath.c_str());
        CONFLoad(androidConfPath.c_str());
#endif

        Config_Parse();
    }

out:
    CONFQuit();

    return rv;
}

void
CCDConfig_Print(void)
{
    LOG_INFO("CCD CONFIG");

#define ROW(name_, default_value_)  LOG_INFO(#name_" = %d", __ccdConfig.name_);
    INT_CONFIG_PARAMS;
#undef ROW

#define ROW(name_, default_value_)  LOG_INFO(#name_" = \"%s\"", __ccdConfig.name_);
    STRING_CONFIG_PARAMS;
#undef ROW

    LOG_INFO(" ");
}
