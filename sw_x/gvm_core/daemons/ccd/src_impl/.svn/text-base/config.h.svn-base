//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __CCD_CONFIG_H__
#define __CCD_CONFIG_H__

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_CLEARFI_MODE_DISABLE_DIN 1
#define CONFIG_CLEARFI_MODE_DISABLE_DEX 2
#define CONFIG_CLEARFI_MODE_DISABLE_P2P 4
#define CONFIG_CLEARFI_MODE_DISABLE_PRX 8

#define CONFIG_ENABLE_TS_USE_HTTPSVC_CCD 1
#define CONFIG_ENABLE_TS_ALWAYS_USE_VSSI 2
#define CONFIG_ENABLE_TS_INIT_TS_IN_SN   4
#define CONFIG_ENABLE_TS_ALWAYS_USE_TS   8
#define CONFIG_ENABLE_TS_INIT_TS_EXT    16

#define CONFIG_TS2_DEV_AID_PARAM__DROP_TS_PACKETS 1
#define CONFIG_TS2_DEV_AID_PARAM__DUP_TS_PACKETS  2

#define CONFIG_SERVER_SERVICE_PORT_RANGE_MAX 32

/// Whether or not to allow a HTTP request destined to the local device to be satisfied by
/// a function call to Sn (StorageNode).
#define CONFIG_HTTPSVC_OPT_TRY_FUNCALL_SN 1

typedef struct CCDConfig_ {

    /// If true, enables #LOG_LEVEL_DEBUG (see log.h).
    u8   debugLog;

    //---------------------------
    // Sets if the HTTP traffic is logged.
    // 0 for false.
    // 1 for true.
    //---------------------------
    /// Requests made via CCDIInfraHttp().
    s8 debugInfraHttp;
    /// SOAP calls to IAS
    // TODO: Currently, this is always treated as true (see Bug 11687).
    s8 debugIasHttp;
    /// SOAP calls to NUS.
    // TODO: Currently, this is always treated as true (see Bug 11687).
    s8 debugNusHttp;
    /// SOAP calls to VSDS.
    // TODO: Currently, this is always treated as true (see Bug 11687).
    s8 debugVsdsHttp;
    /// Calls to VCS (REST + JSON).
    s8 debugVcsHttp;
    /// Calls to ACS
    s8 debugAcsHttp;
#if 0
    /// Binary uploads/downloads to/from ACS.
    s8 debugAcsHttp;
#endif
    //---------------------------

    /// On Android: if false, turns off logging to CCD log files.
    /// On other platforms: has no effect.
    s8 enableLogFiles;

    /// On Android: if true, adds all CCD logging to the logcat logs.
    /// On other platforms: has no effect.
    s8 writeToSystemLog;

    /// (Only relevant if the local device is the Media Server) Allows the Media Server to post
    /// file metadata to VCS before uploading the file to ACS (Virtual Sync).
    /// 0 => Normal sync (upload file, then post to VCS).  Old behavior.
    /// 1 => Hybrid sync (post to VCS, then upload file, then update VCS).  New behavior.
    /// 2 => Pure virtual sync (post to VCS only).  For testing only.
    s8 mediaMetadataUploadVirtualSync;

    /// If this is enabled, Media Metadata download SyncConfigs will download files directly
    /// from the Archive Storage Device (Media Server) whenever possible.
    s8 mediaMetadataSyncDownloadFromArchiveDevice;

    /// If enabled, use Pure Virtual Sync to download the componentId/revision information for
    /// media thumbnails.  Then when CCD gets an HTTP request for a thumbnail that it doesn't have,
    /// it can download that thumbnail from ACS without needing to look up the componentId/revision
    /// in VCS.
    s8 mediaMetadataThumbVirtDownload;

    int  vsdsContentPort;
    int  iasPort;
    int  nusPort;
    int  ansPort;
    int  clearfiPort;

    /// [Testing Option] Bitmask to control routes attempted.
    /// Possible values are some bit-OR of CONFIG_CLEARFI_MODE_*
    int  clearfiMode;

    /// [DEPRECATED] - No longer consulted in 2.6 or later.
    /// User provided streaming server port number
    int  clearfiUserServerPort;
    int  secureClearfiUserServerPort; /// Same, but using a secure tunnel

    /// Range of ports to try to use first for server services.
    /// Services include VSSI, VSSI Secure Tunnel, and TS.
    char serverServicePortRange[CONFIG_SERVER_SERVICE_PORT_RANGE_MAX];

    /// The number of seconds between sleep packets sent to ANS (for Wake-On-WiFi feature).
    /// 0 (not currently the default) indicates that we should use whatever value ANS tells us.
    int  sleepPacketInterval;

    /// Determines whether thumbnail requests should be satisfied from the local cache.
    int useThumbnailCache;

    int  storageNodeClientPort;

    /// Set the max number of pending uploads to accrue before sending a commit.
    int syncPendingCommitNumberThreshold;

    /// Set the max number of pending upload bytes to accrue before sending a commit.
    int syncPendingCommitBytesThreshold;

    /// For testing purposes only.
    /// This allows multiple independent CCD instances under the same OS user on a single host.
    /// Each instance is expected to be a separate virtual device.
    /// This should always be 0 in production.
    int  testInstanceNum;

    /// Number of seconds to delay system sleep when vss_server serves data.
    int serveDataKeepAwakeTimeSec;

    /// The domain that all infrastructure URLs will be based on.
    char infraDomain[HOSTNAME_MAX_LENGTH];

    /// The local IP address for the CCD Local HTTP interface to listen on.
    /// See http://www.ctbg.acer.com/wiki/index.php/CCD_Local_HTTP_Server
    char clearfiUserServerName[HOSTNAME_MAX_LENGTH];

    /// User provided tag edit program path
    char tagEditPath[TAGEDIT_PATH_MAX_LENGTH];

    /// To specify the user's group for checking for updates
    char userGroup[USERGROUP_MAX_LENGTH];

    /// < 0 - Turn off
    /// 0  - Search for free offload slot 
    /// > 0 - Fix offload id to use  
    int ioacProtocolOffloadMode;

    /// Force ARP offload on Intel adapter
    /// Currently, APR offload is disabled for Intel adapter, due to a bug in its driver code.
    /// This option can be used to force ARP offload on Intel adapter.
    /// This will be temporary until the issue is resolved.
    int ioacArpOffloadForceOnIntel;

    /// Don't try to determine the reason for wakeup.
    /// It appears Realtek driver has problems doing so on Win8.
    /// This option is added to allow relief on such environments.
    /// This will be temporary until the issue is resolved.
    int ioacDontCheckWakeReason;

    /// The maximum total log size in bytes.
    /// If 0, the size is infinite, with max number of logs 3 (still daily rollover).
    int maxTotalLogSize;

    /// Time in seconds after which an idle connection will be closed.
    /// 0 means no timeout
    int prxConnIdleTimeout;  // proxy
    int p2pConnIdleTimeout;  // P2P
    int dexConnIdleTimeout;  // external direct
    int dinConnIdleTimeout;  // internal direct

    /// Enable call path from VSSI to new dataset implementations.
    /// By default, this is disabled, per management, to encourage new apps to use the new HTTP API for dataset access.
    int enableVssiNewDatasetPath;

    /// SyncUp retry interval
    int syncUpRetryInterval;
    /// SyncDown retry interval
    int syncDownRetryInterval;

    /// Set to 0 to disable thread pool.  Otherwise, this is the size of
    /// the general thread pool for uploads.
    /// Note that a setting of 1 can actually be worse than a setting of 0, since it will upload
    /// at the same rate, but use an extra thread (and thread synchronization overhead).
    /// May be shared with downloads if downloadAndUploadUsesUploadThreadPool is enabled.
    int uploadThreadPoolSize;

    /// If enabled (nonzero), downloadThreadPool will be the same as the uploadThreadPool,
    /// with the size being set by uploadThreadPoolSize.  downloadThreadPoolSize
    /// will be ignored if this is enabled.
    int downloadAndUploadUsesUploadThreadPool;

    /// Set to 0 to disable thread pool.  Otherwise, this is the size of
    /// the general thread pool for downloads.
    /// Note that a setting of 1 can actually be worse than a setting of 0, since it will download
    /// at the same rate, but use an extra thread (and thread synchronization overhead.
    int downloadThreadPoolSize;

    /// [Development Option] Bitmask of HttpSvc/TS components to enable.
    /// Possible values are some bit-OR of CONFIG_ENABLE_TS_*
    /// TS Integration: temporary until we get this working on all platforms.
    int enableTs;

    /// Client Statistics reporting period, in seconds. (default 24 hours = 86400 seconds)
    int statMgrReportInterval;

    /// Client Statistics maximum number of events (per event type) within reporting period. (default 100)
    int statMgrMaxEvent;

    /// Options for HttpSvc.
    /// Possible values are some bit-OR of CONFIG_HTTPSVC_OPT_*.
    int httpSvcOpt;

    /// Ts2 options.
    int ts2MaxSegmentSize;
    int ts2MaxWindowSize;
    int ts2SendBufSize;

    /// Time in seconds after which Ts2 would start to talk to PXD (Proxy Daemon)
    int ts2AttemptPrxP2PDelaySec;

    /// Time in seconds after which Ts2 would start to use PRX (Proxy Connection, if it's available) to relay data
    int ts2UsePrxDelaySec;

    /// Time in millisec after which Ts2 will retransmit the packet.
    int ts2RetransmitIntervalMs;

    /// Lowerbound of time in millisec after which Ts2 will retransmit the packet.
    int ts2MinRetransmitIntervalMs;

    /// Upperbound of exponential backoff factor after which Ts2 will retransmit the packet.
    int ts2MaxRetransmitRound;

    /// Timeout in millisec to get CcdiEvents
    int ts2EventsTimeoutMs;

    /// [Testing Option] Bitmask to elicit specific behavior from Ts2.
    /// Possible values are some bit-OR of CONFIG_TS2_DEV_AID_PARAM__*
    int ts2DevAidParam;
    /// This value shows for every specific number of packet transmission, packet drop happens once
    int ts2PktDropParam;

    /// Ts2 options
    /// Timeout to use for each TS_* calls.  Unit is milliseconds.
    int ts2TsOpenTimeoutMs;
    int ts2TsReadTimeoutMs;
    int ts2TsWriteTimeoutMs;
    int ts2TsCloseTimeoutMs;

    /// Ts2 option
    /// Timeout waiting for FIN or FIN-ACK.  Unit is milliseconds.
    int ts2FinWaitTimeoutMs;

    /// Ts2 option
    /// Timeout waiting for a packet to be sent out.  Unit is milliseconds.
    int ts2PacketSendTimeoutMs;

    /// Ts2 testing option
    /// For test and simulate network environment in ccd (default = 0)
    int ts2TestNetworkEnv;

    /// sync_config options
    int syncboxSyncConfigErrPollInterval;         // 0 - use default settings; else, interval in secs
} CCDConfig;

extern CCDConfig __ccdConfig;

int  CCDConfig_Init(const char* storageRoot);
void CCDConfig_Print(void);

#ifdef ANDROID
void CCDConfig_SetBrandName(const char* brandName);
#endif

#ifdef __cplusplus
}
#endif

#endif // include guard
