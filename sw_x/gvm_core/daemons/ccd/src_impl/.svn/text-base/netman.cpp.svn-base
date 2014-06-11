#define DEBUG_NETMAN 1
#define EVEN_MORE_DEBUG_NETMAN 0

#define UM_NDIS620 1

#include "netman.hpp"

#if defined(_MSC_VER) && !defined(VPL_PLAT_IS_WINRT)

#include "vplex_plat.h"
#include "vplex_assert.h"
#include "vplex_socket.h"
#include "vpl_lazy_init.h"
#include "vpl_fs.h"
#include "vplex_file.h"
#include "log.h"
#include "ans_connection.hpp"
#include "AutoLocks.hpp"
#include "config.h"
#include "EventManagerPb.hpp"

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <mstcpip.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>

#include <ntddndis.h>
#if NDIS_SUPPORT_NDIS620 == 0
#error Need to use a version of the Windows SDK with NDIS_SUPPORT_NDIS620 support
#endif

#undef REQUIRE_NLO        // define this for NLO support;

#include <win32/NlmEventSink.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <limits.h>
#include <strsafe.h>
#include <powrprof.h>
#include <string>
#include <map>
#include <queue>
#include <set>

// Include the QCA API (which wasn't tested with C++ apparently).
extern "C" {
#include "customer.h"
}

//-----------------------------------------
// ndis 6.30 specific;
// copied here from win8 wdk;
//-----------------------------------------
#ifndef NDIS_SUPPORT_NDIS630
#define NDIS_SUPPORT_NDIS630 1

#define NDIS_HEADER_TYPE NDIS_OBJECT_TYPE_DEFAULT

#define NDIS_PM_WAKE_PACKET_INDICATION_SUPPORTED                0x00000001
#define NDIS_PM_SELECTIVE_SUSPEND_SUPPORTED                     0x00000002
#define NDIS_PM_WOL_PACKET_WILDCARD_SUPPORTED                   0x00100000
#define NDIS_WLAN_WAKE_ON_NLO_DISCOVERY_SUPPORTED                 0x00000001
#define NDIS_WLAN_WAKE_ON_AP_ASSOCIATION_LOST_SUPPORTED           0x00000002
#define NDIS_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR_SUPPORTED           0x00000004
#define NDIS_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST_SUPPORTED        0x00000008
#define NDIS_PM_WAKE_ON_LINK_CHANGE_ENABLED                     0x00000001
#define NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_ENABLED                0x00000002
#define NDIS_PM_SELECTIVE_SUSPEND_ENABLED                       0x00000010
#define NDIS_WLAN_WAKE_ON_NLO_DISCOVERY_ENABLED                 0x00000001
#define NDIS_WLAN_WAKE_ON_AP_ASSOCIATION_LOST_ENABLED           0x00000002
#define NDIS_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR_ENABLED           0x00000004
#define NDIS_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST_ENABLED        0x00000008

#define NDIS_PM_CAPABILITIES_REVISION 2
#define NDIS_PM_PARAMETERS_REVISION 2
#define NDIS_PM_WAKE_REASON_REVISION 1

#define MAX_PROTOCOL_OFFLOAD_ID 10

// ndis 6.30 defs;

typedef struct _NDIS630_PM_CAPABILITIES {
    NDIS_OBJECT_HEADER Header;
    ULONG Flags;
    ULONG SupportedWoLPacketPatterns;
    ULONG NumTotalWoLPatterns;
    ULONG MaxWoLPatternSize;
    ULONG MaxWoLPatternOffset;
    ULONG MaxWoLPacketSaveBuffer;
    ULONG SupportedProtocolOffloads;
    ULONG NumArpOffloadIPv4Addresses;
    ULONG NumNSOffloadIPv6Addresses;
    NDIS_DEVICE_POWER_STATE MinMagicPacketWakeUp;
    NDIS_DEVICE_POWER_STATE MinPatternWakeUp;
    NDIS_DEVICE_POWER_STATE MinLinkChangeWakeUp;
#if NDIS_SUPPORT_NDIS630
    ULONG SupportedWakeUpEvents;
    ULONG MediaSpecificWakeUpEvents;
#endif
} NDIS630_PM_CAPABILITIES;

typedef struct _NDIS630_PM_PARAMETERS {
    NDIS_OBJECT_HEADER Header;
    ULONG EnabledWoLPacketPatterns;
    ULONG EnabledProtocolOffloads;
    ULONG WakeUpFlags;
#if NDIS_SUPPORT_NDIS630
    ULONG MediaSpecificWakeUpEvents;
#endif
} NDIS630_PM_PARAMETERS;

#endif // NDIS_SUPPORT_NDIS630
//-----------------------------------------

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))

static void HEAP_FREE(void* ptr) {
    HeapFree(GetProcessHeap(), 0, ptr);
}

#define MAC_HDR_LEN  6

#define IP_HDR_LEN_WORDS  5
#define IP_HDR_LEN_BYTES  (4 * IP_HDR_LEN_WORDS)

#define ALL_HDRS_LEN  34

#define EXTRA_INTEL_HDRS_LEN 8

static const DOT11_MAC_ADDRESS MAGIC_PKT_FIXED_PATTERN = { 0xAC, 0xE4, 0xC1, 0x07, 0xDC, 0xCD };

#define WAKEUP_SECRET_SIZE 6
/// The temporary secret for the wakeup packet.
/// Also known as "magic packet extension" or "SecureOn password".
struct NetMan_WakeupPktSecret_t {
    u8 ext[WAKEUP_SECRET_SIZE];
};

struct NetMan_MacAddr_t {
    DOT11_MAC_ADDRESS mac;
};
#define MacAddr_t_AssignFromArray(dst_, src_) \
    ASSERT_EQUAL_SIZE_COMPILE_TIME(ARRAY_SIZE_IN_BYTES(dst_.mac), ARRAY_SIZE_IN_BYTES(src_)); \
    memcpy(dst_.mac, src_, ARRAY_SIZE_IN_BYTES(dst_.mac));

struct ProgramChipContext {

    ccd::IoacAdapterStatus newStatus;

    /// The keep-alive packet that will be periodically sent to the server.
    /// 000-005: (6 bytes) MAC addr of access point
    /// 006-025: (20 bytes) IP header
    /// 026-033: (8 bytes) UDP header
    /// 034-...: (keepAlivePayloadLen) Payload (specified by the infrastructure)
    /// ...-...: (8 bytes) 0s for now
    /// ...-...: (6 bytes) MAC addr of card to wake
    u8 keepAlivePacket[ATH_CUSTOMER_KEEPALIVE_MAX_PACKET_SIZE + KEEPALIVE_EXTRA_BUFFER_FOR_INTEL];

    /// The actual size of keepAlivePacket (it varies based on the payload).
    /// Includes ALL_HDRS_LEN.
    size_t keepAlivePacketLen;

    /// Secret required to wakeup the machine.
    NetMan_WakeupPktSecret_t secret;

    u64 sleepSetupAsyncId;

    std::string serverHostname;

    /// In host byte order.
    VPLNet_port_t serverPortHbo;

    u32 keepAliveInterval_ms;

    //------------------------------
};

struct WakeableNetAdapter {

    WakeableNetAdapter(CUSTOMER_CONTROL_SET& ioacInfo) :
        ioacInfo(ioacInfo),
        foundWifiInfo(false)
    {}

    NetMan_MacAddr_t localMacAddr;
    VPLNet_addr_t localIpAddr;
    CUSTOMER_CONTROL_SET& ioacInfo;
    bool foundWifiInfo;
    
    bool isValid() const
    {
        switch (ioacInfo.nType) {
        case CONNECTION_WIRED:
            return true;
        case CONNECTION_WIRELESS:
            return foundWifiInfo;
        default:
            return false;
        }
    }
    
    int getWakeupPacketType() const
    {
        if ((ioacInfo.nPacketFmt & MAGIC_PACKET_TYPE_ACER_SHORT) != 0) {
            // First choice is MAGIC_PACKET_TYPE_ACER_SHORT (avoids revealing the local MAC address).
            return ioac_intel;
        } else if ((ioacInfo.nPacketFmt & MAGIC_PACKET_TYPE_EXTENDED) != 0) {
            // Next choice is MAGIC_PACKET_TYPE_EXTENDED.
            return ioac_proto;
        } else {
            return ioac_none;
        }
    }
    
    WakeableNetAdapter& WakeableNetAdapter::operator=(const WakeableNetAdapter &rhs)
    {
      if (this != &rhs) {
        localMacAddr = rhs.localMacAddr;
        localIpAddr = rhs.localIpAddr;
        ioacInfo = rhs.ioacInfo;
        foundWifiInfo = rhs.foundWifiInfo;
      }
      return *this;
    }
};

static void allowOsSleep(bool allow);

#define MAX_KA  2
#define NUM_KEEPALIVES_TO_USE  1

static int
xlatCustomerError(CUSTOMER_RETURN_CODE rc)
{
    switch (rc) {
    case CUSTOMER_CONTROL_OK:
        return VPL_OK;
    case CUSTOMER_CONTROL_FAILURE:
        LOG_WARN("Unspecified CUSTOMER_CONTROL error, GetLastError()="FMT_DWORD, GetLastError());
        return CCD_ERROR_IOAC_LIB_FAIL;
    case CUSTOMER_CONTROL_BAD_PARAMETER:
        return CCD_ERROR_INTERNAL;
    default:
        LOG_WARN("Unknown CUSTOMER_CONTROL return code: "FMTenum, rc);
        return CCD_ERROR_IOAC_LIB_FAIL;
    }
}

//-----------------------------------------

// GUID comparison class
// This is used in maps where the key is of type GUID.
struct GUIDCompare {
    bool operator() (const GUID &guid1, const GUID &guid2) const {
        return memcmp((void*)&guid1, (void*)&guid2, sizeof(GUID)) < 0;
    }
};

//-----------------------------------------

struct IoacSleepSetup {
    u64 asyncId;
    std::string wakeupKey;
    std::string serverHostname;
    VPLNet_port_t serverPort;
    u32 packetIntervalSec;
    std::string payload;
};

//-----------------------------------------

enum WorkerCommand {
    WCMD_INVALID = 0,
    WCMD_QUIT,               // no arg
    WCMD_CHECK_WAKE_REASON,  // no arg
    WCMD_UNPROGRAM_ALL,      // no arg
    WCMD_SCAN,               // no arg
    WCMD_REQUEST_SLEEP_SETUP, // no arg
    WCMD_PROGRAM,            // arg is u64 value of asyncId
    WCMD_CHECK_FOR_ENABLE_IOAC,// arg is u64 value of deviceId
    WCMD_DISABLE_IOAC,       // no arg
};

struct WorkerTask {
    //WorkerTask() : cmd(WCMD_INVALID) {}
    WorkerTask(WorkerCommand _cmd) : cmd(_cmd) {}
    WorkerTask(WorkerCommand _cmd, void *_arg) : cmd(_cmd) { arg.vp = _arg; }
    WorkerTask(WorkerCommand _cmd, u64 _arg) : cmd(_cmd) { arg.u64 = _arg; }
    WorkerCommand cmd;
    union {
        void* vp;
        u64 u64;
    } arg;
};

class WorkerTaskQueue {
public:
    WorkerTaskQueue() {
        VPLMutex_Init(&mutex);
        VPLSem_Init(&sem, VPLSEM_MAX_COUNT, 0);
    }
    ~WorkerTaskQueue() {
        VPLMutex_Destroy(&mutex);
        VPLSem_Destroy(&sem);
    }
    void add(WorkerCommand cmd) {
        WorkerTask t(cmd);
        add(t);
    }
    void add(WorkerCommand cmd, void *arg) {
        WorkerTask t(cmd, arg);
        add(t);
    }
    void add(WorkerCommand cmd, u64 arg) {
        WorkerTask t(cmd, arg);
        add(t);
    }
    void add(WorkerTask task) {
        {
            MutexAutoLock lock(&mutex);
            q.push(task);
        }
        VPLSem_Post(&sem);
    }
    bool empty() const { 
        MutexAutoLock lock (&mutex);
        return q.empty();
    }
    void wait() {
        VPLSem_Wait(&sem);
        VPLSem_Post(&sem);
    }
    WorkerTask next() {
        VPLSem_Wait(&sem);
        MutexAutoLock lock(&mutex);
        WorkerTask t = q.front();
        q.pop();
        return t;
    }
private:
    std::queue<WorkerTask> q;
    VPLSem_t sem;
    mutable VPLMutex_t mutex;
};
static WorkerTaskQueue wTaskQ;

#define OBJECT_ID_ENABLED_IOAC_DEVICE_ID "enabled_ioac_device_id"

static bool hasGlobalAccessDataPath = false;
static std::string globalAccessDataPath;
//------------------------------

#if DEBUG_NETMAN

class MacAddrStr {
 public:
    MacAddrStr(const NetMan_MacAddr_t& mac)
    {
        for(int i = 0; i < sizeof(DOT11_MAC_ADDRESS); i++) {
            snprintf(str + (i * 3), 4, "%02x:", mac.mac[i]);
        }
        str[17] = '\0';
    }
    const char* c_str() { return str; }

 private:
    VPL_DISABLE_COPY_AND_ASSIGN(MacAddrStr);

    // (2 * 6) bytes for hex digits + 5 bytes for colon-separators + 1 byte for '\0' + 1 extra for snprintf to be happy.
    char str[19];
};

static const char*
wlanStateStr(WLAN_INTERFACE_STATE state)
{
    switch(state) {
    case wlan_interface_state_not_ready:
        return "interface not ready";
    case wlan_interface_state_connected:
        return "connected";
    case wlan_interface_state_ad_hoc_network_formed:
        return "ad-hoc formed";
    case wlan_interface_state_disconnecting:
        return "disconnecting";
    case wlan_interface_state_disconnected:
        return "disconnected";
    case wlan_interface_state_associating:
        return "associating";
    case wlan_interface_state_discovering:
        return "discovering network";
    case wlan_interface_state_authenticating:
        return "authenticating";
    }
    return "unknown";
}

static const char*
wlanType(DOT11_BSS_TYPE type)
{
    switch(type) {
    case dot11_BSS_type_infrastructure:
        return "infrastructure";
    case dot11_BSS_type_independent:
        return "independent";
    }
    return "other";
}

static const char*
wlanAuth(DOT11_AUTH_ALGORITHM auth)
{
    switch(auth) {
    case DOT11_AUTH_ALGO_80211_OPEN:
        return "802.11 open";
    case DOT11_AUTH_ALGO_80211_SHARED_KEY:
        return "802.11 shared key";
    case DOT11_AUTH_ALGO_WPA:
        return "wpa";
    case DOT11_AUTH_ALGO_WPA_PSK:
        return "wpa-psk";
    case DOT11_AUTH_ALGO_WPA_NONE:
        return "wpa-none";
    case DOT11_AUTH_ALGO_RSNA:
        return "wpa2";
    case DOT11_AUTH_ALGO_RSNA_PSK:
        return "wpa2-psk";
    }
    return "unknown auth";
}

static const char*
wlanCipher(DOT11_CIPHER_ALGORITHM cipher)
{
    switch(cipher) {
    case DOT11_CIPHER_ALGO_NONE:
        return "none";
    case DOT11_CIPHER_ALGO_WEP40:
        return "wep-40";
    case DOT11_CIPHER_ALGO_TKIP:
        return "tkip";
    case DOT11_CIPHER_ALGO_CCMP:
        return "aes/ccmp";
    case DOT11_CIPHER_ALGO_WEP104:
        return "wep-104";
    case DOT11_CIPHER_ALGO_WEP:
        return "wep";
    }
    return "unknown cipher";
}

#if 0

void
PrintKaMac(UCHAR *p, int off, int size)
{
    assert(size == 6);
    printf("   mac\n");
    printf("    [%02d:%02d] %02x:%02x:%02x:%02x:%02x:%02x\n",
        off, off + size - 1,
        p[0], p[1], p[2], p[3], p[4], p[5]);
}

void
PrintKaIp(UCHAR *p, int off, int size)
{
    assert(size == 20);
    p += off;
    printf("   ip\n");
    printf("    [%02d] version/ihl %02x\n", off+0, p[0]);
    printf("    [%02d] tos %02x\n", off+1, p[1]);
    printf("    [%02d:%02d] length %02x%02x\n", off+2,off+3, p[2],p[3]);
    printf("    [%02d:%02d] id %02x%02x\n", off+4,off+5, p[4],p[5]);
    printf("    [%02d:%02d] flags/frag %02x%02x\n", off+6,off+7, p[6],p[7]);
    printf("    [%02d] ttl %02x\n", off+8, p[8]);
    printf("    [%02d] proto %02x\n", off+9, p[9]);
    printf("    [%02d:%02d] chk %02x%02x\n", off+10,off+11, p[10],p[11]);
    printf("    [%02d:%02d] src %02x%02x%02x%02x\n",
        off+12,off+15, p[12],p[13],p[14],p[15]);
    printf("    [%02d:%02d] dst %02x%02x%02x%02x\n",
        off+16,off+19, p[16],p[17],p[18],p[19]);
}

void
PrintKaUdp(UCHAR *p, int off, int size)
{
    assert(size == 8);
    p += off;
    printf("   udp\n");
    printf("    [%02d:%02d] sport %02x%02x\n", off+0,off+1, p[0],p[1]);
    printf("    [%02d:%02d] dport %02x%02x\n", off+2,off+3, p[2],p[3]);
    printf("    [%02d:%02d] len %02x%02x\n", off+4,off+5, p[4],p[5]);
    printf("    [%02d:%02d] chk %02x%02x\n", off+6,off+7, p[6],p[7]);
}

void
PrintKaData(UCHAR *p, int off, int size)
{
    printf("   data\n");
    printf("    [%02d:%02d] '%s'\n", off+0, off+size-1, p + off);
}

const char*
NdisDevStateStr(NDIS_DEVICE_POWER_STATE ps)
{
    switch(ps) {
    case NdisDeviceStateUnspecified:
        return("not supported");
    case NdisDeviceStateD0:
        return("from D0");
    case NdisDeviceStateD1:
        return("from D1");
    case NdisDeviceStateD2:
        return("from D2");
    case NdisDeviceStateD3:
        return("from D3");
    }
    return("??");
}

const char*
WowPmWakeReason(g_t *g, DWORD *err)
{
#ifdef XXX
    CUSTOMER_RETURN_CODE rc;
    DWORD br;
    NDIS_PM_WAKE_REASON pm_wr;
    ULONG x;

    // get wakeup reason;

    pm_wr.Header.Type = NDIS_HEADER_TYPE;
    pm_wr.Header.Revision = NDIS_PM_PARAMETERS_REVISION;
    pm_wr.Header.Size = sizeof(pm_wr);

    printf("  NDIS_PM_PARAMETERS.Header.Revision %d\n",
        pm_wr.Header.Revision);
    printf("  NDIS_PM_PARAMETERS.Header.Size %d\n",
        pm_wr.Header.Size);

    br = 0;
    rc = CustomerControlDll(IOCTL_CUSTOMER_NDIS_QUERY_PM_PARAMETERS,
        &pm_wr, sizeof(pm_wr),
        &pm_wr, sizeof(pm_wr), &br);

    if(CustomerError(rc, err))
        return("cannot get ndis pm wake reason");

    // report enabled patterns;

    x = pm_wr.EnabledWoLPacketPatterns;
#else
    printf("  XXX need NDIS_PM_WAKE_REASON api\n");
#endif
    return(NULL);
}

#endif

#endif // DEBUG_NETMAN
//-----------------------------------------

static void postIoacStatusChangeEvent()
{
    ccd::IoacOverallStatus status = NetMan_GetIoacOverallStatus();
    LOG_INFO("IOAC status is now: %s", status.ShortDebugString().c_str());
    
    // Post an event.
    {
        ccd::CcdiEvent* event = new ccd::CcdiEvent();
        event->mutable_ioac_status_change()->set_status_summary(status.summary());
        EventManagerPb_AddEvent(event);
        // event will be freed by EventManagerPb.
    }
}

/// Caller must call #HEAP_FREE() on the result.
static IP_ADAPTER_ADDRESSES*
getIpAdapterAddresses()
{
    // Allocate a 15 KB buffer to start with.
    ULONG outBufLen = 15000;
    // Try multiple times, to avoid race condition inherent in the API design.
    int tries = 3;

    do {
        IP_ADAPTER_ADDRESSES* pAddresses = (IP_ADAPTER_ADDRESSES*)MALLOC(outBufLen);
        if (pAddresses == NULL) {
            LOG_ERROR("Alloc(%lu) failed", outBufLen);
            return NULL;
        }
        DWORD err = GetAdaptersAddresses(AF_INET,
                GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST,
                NULL /*reserved*/,
                pAddresses, &outBufLen);
        if (err == NO_ERROR) {
            return pAddresses;
        } else {
            HEAP_FREE(pAddresses);
            pAddresses = NULL;
            if (err == ERROR_BUFFER_OVERFLOW) {
                // Will try again with new value of outBufLen.
            } else {
                LOG_ERROR("%s failed: %d", "GetAdaptersAddresses", VPLError_XlatWinErrno(err));
                return NULL;
            }
        }
        tries--;
    } while (tries > 0);
    LOG_ERROR("Ran out of tries for %s", "GetAdaptersAddresses");
    return NULL;
}

static void
findAdapters(const IP_ADAPTER_ADDRESSES* addrs, int ioacNicCount, CUSTOMER_CONTROL_SET* ccsArray[], std::map<GUID, WakeableNetAdapter, GUIDCompare>& adapters_out)
{
    for (const IP_ADAPTER_ADDRESSES* curr = addrs; curr; curr = curr->Next) {
        LOG_INFO("AdapterName=%s", curr->AdapterName);
#if DEBUG_NETMAN
        {
            char strFriendly[256] = "";
            _VPL__wstring_to_utf8(curr->FriendlyName, -1, strFriendly, ARRAY_SIZE_IN_BYTES(strFriendly));
            LOG_DEBUG("  FriendlyName=%s", strFriendly);
        }
        {
            char strDesc[256] = "";
            _VPL__wstring_to_utf8(curr->Description, -1, strDesc, ARRAY_SIZE_IN_BYTES(strDesc));
            LOG_DEBUG("  Description=%s", strDesc);
        }
#endif
        GUID currGuid;
        {
            NETIO_STATUS temp_rv = ConvertInterfaceLuidToGuid(&curr->Luid, &currGuid);
            if (temp_rv != 0) {
                LOG_WARN("ConvertInterfaceLuidToGuid returned "FMT_DWORD, temp_rv);
                continue;
            }
            LOG_INFO("  GUID=%s", WinGuidToStr(currGuid).c_str());
        }
        
        CUSTOMER_CONTROL_SET* ioacInfo;
        for (int i = 0; i < ioacNicCount; i++) {
            if (IsEqualGUID(ccsArray[i]->nId, currGuid)) {
                LOG_INFO("  IOAC_NIC[%d]", i);
                ioacInfo = ccsArray[i];
                goto isIoac;
            }
        }
        LOG_INFO("  Not IOAC");
        continue;
    isIoac:
        
        WakeableNetAdapter newEntry(*ioacInfo);
        
        if (curr->PhysicalAddressLength != ARRAY_SIZE_IN_BYTES(newEntry.localMacAddr.mac)) {
            LOG_INFO("  PhysicalAddressLength was %lu", curr->PhysicalAddressLength);
            continue;
        }
        memcpy(newEntry.localMacAddr.mac, curr->PhysicalAddress, ARRAY_SIZE_IN_BYTES(newEntry.localMacAddr.mac));
        LOG_INFO("  MAC=%s", MacAddrStr(newEntry.localMacAddr).c_str());

        // Multiple ip addresses can sit on each interface.
        u32 numIpAddrs = 0;
        for(IP_ADAPTER_UNICAST_ADDRESS* ipp = curr->FirstUnicastAddress; ipp != NULL; ipp = ipp->Next) {
            SOCKADDR* sap = ipp->Address.lpSockaddr;
            if (sap == NULL) {
                LOG_ERROR("  SOCKADDR was NULL");
            } else {
                newEntry.localIpAddr = ((SOCKADDR_IN*)sap)->sin_addr.S_un.S_addr;
                LOG_INFO("  Local IP address["FMTu32"]: "FMT_VPLNet_addr_t, numIpAddrs, VAL_VPLNet_addr_t(newEntry.localIpAddr));
                numIpAddrs++;
            }
        }
        if (numIpAddrs < 1) {
            // TODO: report as a different case
            LOG_WARN("  No IP addresses found for adapter");
            continue;
        } else if (numIpAddrs > 1) {
            LOG_INFO("  More than 1 IP address found for adapter; using last one");
        }

        adapters_out.insert(std::pair<GUID, WakeableNetAdapter>(ioacInfo->nId, newEntry));

    } // foreach IP_ADAPTER_ADDRESSES element
}

static void
wlanCheckIfs(HANDLE hWlan, WLAN_INTERFACE_INFO_LIST* iflist, std::map<GUID, WakeableNetAdapter, GUIDCompare>& connectedIoacAdapters)
{
    for(int wlanIdx = 0; wlanIdx < (int)iflist->dwNumberOfItems; wlanIdx++) {
        WLAN_INTERFACE_INFO* ifinfo = iflist->InterfaceInfo + wlanIdx;

        LOG_INFO("wlan i/f %d: GUID=%s", wlanIdx, WinGuidToStr(ifinfo->InterfaceGuid).c_str());

#if DEBUG_NETMAN
        {
            char strDesc[256] = "";
            _VPL__wstring_to_utf8(ifinfo->strInterfaceDescription, -1, strDesc, ARRAY_SIZE_IN_BYTES(strDesc));
            LOG_DEBUG("  strInterfaceDescription=\"%s\", isState=%s", strDesc, wlanStateStr(ifinfo->isState));
        }
#endif

        WakeableNetAdapter* currConnectedIoacAdapter = NULL;
        std::map<GUID, WakeableNetAdapter, GUIDCompare>::iterator it = connectedIoacAdapters.find(ifinfo->InterfaceGuid);
        if (it != connectedIoacAdapters.end()) {
            currConnectedIoacAdapter = &it->second;
            goto isConnIoac;
        }
        LOG_INFO("  wlan i/f %d is not a connected IOAC adapter", wlanIdx);
        continue;
    isConnIoac:

        if (ifinfo->isState != wlan_interface_state_connected) {
            LOG_INFO("  wlan i/f %d not currently connected", wlanIdx);
            continue;
        }

        WLAN_CONNECTION_ATTRIBUTES* cinfo;
        {
            WLAN_OPCODE_VALUE_TYPE valueType = wlan_opcode_value_type_invalid;
            DWORD cinfosz;
            DWORD err = WlanQueryInterface(hWlan,
                    &ifinfo->InterfaceGuid,
                    wlan_intf_opcode_current_connection,
                    NULL,
                    &cinfosz, (PVOID*)&cinfo,
                    &valueType);
            if (err != ERROR_SUCCESS) {
                LOG_ERROR("  wlan i/f %d: %s failed: %d", wlanIdx, "WlanQueryInterface", err);
                continue;
            }
        }

#if DEBUG_NETMAN
        {
            char profileName[256] = "";
            _VPL__wstring_to_utf8(cinfo->strProfileName, -1, profileName, ARRAY_SIZE_IN_BYTES(profileName));
            LOG_DEBUG("  isState=%s, profileName=%s", wlanStateStr(cinfo->isState), profileName);
        }
#endif

        {
            WLAN_ASSOCIATION_ATTRIBUTES* cattr = &cinfo->wlanAssociationAttributes;
#if DEBUG_NETMAN
            LOG_DEBUG("  ssid=%.*s, type=%s",
                    // ucSSID is not null-terminated, so we need to pass the length
                    (int)cattr->dot11Ssid.uSSIDLength, (const char*)cattr->dot11Ssid.ucSSID,
                    wlanType(cattr->dot11BssType));
#endif
        }

        {
#if DEBUG_NETMAN
            WLAN_SECURITY_ATTRIBUTES* sattr = &cinfo->wlanSecurityAttributes;
            LOG_DEBUG("  802.1x=%s %s, %s",
                sattr->bOneXEnabled? "yes" : "no",
                wlanAuth(sattr->dot11AuthAlgorithm),
                wlanCipher(sattr->dot11CipherAlgorithm));
#endif
            // TODO: any checks here?
            //if (sattr->dot11CipherAlgorithm != DOT11_CIPHER_ALGO_TKIP) {
            //
            //}
        }
        LOG_INFO("  wlan i/f %d appears usable", wlanIdx);
        currConnectedIoacAdapter->foundWifiInfo = true;
    }
}

static u16
getIpHdrChecksum(const void *ip, int len)
{
    long sum = 0;
    const u16 *p = static_cast<const u16*>(ip);

    for (; len > 1; len -= 2) {
        sum += *p++;
    }
    if (len > 0)
        sum += (u16) *(const u8*)p;
    while(sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return static_cast<u16>(~sum);
}

static void
keepAlivePacket_construct(u8* keepAlivePacket, size_t keepAlivePacketLen,
        const NetMan_MacAddr_t& firstHopMac,
        VPLNet_addr_t sourceIp, VPLNet_port_t sourcePort,
        VPLNet_addr_t destIp, VPLNet_port_t destPort)
{
    // 000-005: (6 bytes) MAC addr of the first hop (gateway)
    ASSERT_EQUAL_SIZE_COMPILE_TIME(ARRAY_SIZE_IN_BYTES(firstHopMac.mac), MAC_HDR_LEN);
    memcpy(&keepAlivePacket[0], firstHopMac.mac, ARRAY_SIZE_IN_BYTES(firstHopMac.mac));

    // 006-025: (20 bytes) IP header
    {
        u8* ipHdr = &keepAlivePacket[MAC_HDR_LEN];

        // 00: (4 bits) Version, (4 bits) Length of header in 32 bit words
        ipHdr[0] = 0x40 /* IPv4*/ + IP_HDR_LEN_WORDS;

        // 01: (8 bits) Differentiated Services
        ipHdr[1] = 0;

        // 02-03: (16 bits) Total length of IP packet (omit the first 6 bytes for MAC addr).
        {
            u16 tempLen = VPLConv_hton_u16(keepAlivePacketLen - MAC_HDR_LEN);
            memcpy(&ipHdr[2], &tempLen, 2);
        }

        // 04-05: (16 bits) Identification
        ipHdr[4] = 0;
        ipHdr[5] = 0;

        // 06-07: (3 bits) flags, (13 bits) frag offset
        // Do not fragment, frag off =0
        ipHdr[6] = 0x40;
        ipHdr[7] = 0;

        // 08: (8 bits) TTL
        ipHdr[8] = 100;

        // 09: (8 bits) Protocol
        // UDP
        ipHdr[9] = 17;

        // 10-11: (16 bits) Header checksum
        // Temporarily set to 0 for purposes of computing it.
        ipHdr[10] = 0;
        ipHdr[11] = 0;

        // 12-15: (32 bits) Source IP address
        ASSERT_EQUAL_SIZE_COMPILE_TIME(sizeof(sourceIp), 4);
        memcpy(&ipHdr[12], &sourceIp, 4); // sourceIp is already in host byte order.

        // 16-19: (32 bits) Destination IP address
        ASSERT_EQUAL_SIZE_COMPILE_TIME(sizeof(destIp), 4);
        memcpy(&ipHdr[16], &destIp, 4); // destIp is already in host byte order.

        // No options.

        // Now go back and fill in the checksum.
        {
            u16 sum = getIpHdrChecksum(ipHdr, IP_HDR_LEN_BYTES);
            // Due to using the one's complement sum, the result is already in the desired byte order.
            memcpy(&ipHdr[10], &sum, 2);
        }
    }

    // 026-033: (8 bytes) UDP header
    {
        u8* udpHdr = &keepAlivePacket[MAC_HDR_LEN + IP_HDR_LEN_BYTES];

        // 00-01: (16 bits) Source port
        {
            u16 tempPort = VPLConv_hton_u16(sourcePort);
            ASSERT_EQUAL_SIZE_COMPILE_TIME(sizeof(sourcePort), 2);
            memcpy(&udpHdr[0], &tempPort, 2);
        }

        // 02-03: (16 bits) Dest port
        {
            u16 tempPort = VPLConv_hton_u16(destPort);
            ASSERT_EQUAL_SIZE_COMPILE_TIME(sizeof(destPort), 2);
            memcpy(&udpHdr[2], &tempPort, 2);
        }

        // 04-05: (16 bits) Length of UDP header + data
        {
            u16 tempLen = VPLConv_hton_u16(keepAlivePacketLen - MAC_HDR_LEN - IP_HDR_LEN_BYTES);
            memcpy(&udpHdr[4], &tempLen, 2);
        }

        // 06-07: (16 bits) Checksum (can set to 0 to omit).
        udpHdr[6] = 0;
        udpHdr[7] = 0;
    }

    // 034-...: Payload (filled in previously).
}

//------------------------------

static void
openIoacLib(CUSTOMER_CONTROL_SET& ccs)
{
    int rv;
    rv = xlatCustomerError(ccs.beginProgram());
    if (rv != 0) {
        LOG_WARN("pfbeginProgram failed: %d", rv);
    }

    rv = xlatCustomerError(ccs.beginSession());
    if (rv != 0) {
        LOG_WARN("pfbeginSession failed: %d", rv);
    }
}

static void
closeIoacLib(CUSTOMER_CONTROL_SET& ioacInfo)
{
    int rv;
    rv = xlatCustomerError(ioacInfo.closeSession());
    if (rv != 0) {
        LOG_WARN("pfendSession failed: %d", rv);
    }

    rv = xlatCustomerError(ioacInfo.closeProgram());
    if (rv != 0) {
        LOG_WARN("pfendProgram failed: %d", rv);
    }
}

//------------------------------

static void disableSleepKeepAlives_DllAlreadyLoaded(CUSTOMER_CONTROL_SET& ccs)
{
    u32 numToDisable = MAX_KA;
    for(u32 n = 0; n < MAX_KA; n++) {
        // disable the keepalive
        {
            CUSTOMER_KEEPALIVE_SET ka;
            ka.nId = n;
            ka.nMSec = 0;
            ka.nSize = 0;
            DWORD dummyBytesReturned = 0;
            int rv = xlatCustomerError(ccs.CustomerControl(IOCTL_CUSTOMER_KEEPALIVE_SET,
                    &ka, sizeof(ka), NULL, 0, &dummyBytesReturned));
            if (rv != 0) {
                LOG_WARN("%s failed: %d", "Disable keepalive", rv);
                continue;
            }
        }
        // check that it is reported as disabled
        {
            CUSTOMER_KEEPALIVE_QUERY kaq;
            kaq.nId = n;
            kaq.nRet = TRUE;
            DWORD bytesReturned = 0;
            int rv = xlatCustomerError(ccs.CustomerControl(IOCTL_CUSTOMER_KEEPALIVE_QUERY,
                    &kaq, sizeof(kaq), &kaq, sizeof(kaq), &bytesReturned));
            if (rv != 0) {
                LOG_WARN("%s failed: %d", "Query keep-alive", rv);
                continue;
            }
            if (bytesReturned != sizeof(kaq)) {
                LOG_WARN("Query keep-alive returned wrong size: "FMT_DWORD, bytesReturned);
                continue;
            }
            if (kaq.nRet != FALSE) {
                LOG_WARN("Keep-alive does not appear to be disabled");
                continue;
            }
        }
        numToDisable--;
    }
    if (numToDisable != 0) {
        LOG_WARN("Could not disable all keep-alives");
    }
}

static void disableSleepKeepAlives(CUSTOMER_CONTROL_SET& ccs)
{
    openIoacLib(ccs);
    disableSleepKeepAlives_DllAlreadyLoaded(ccs);
    closeIoacLib(ccs);
}

static void disableWakeUpPattern(CUSTOMER_CONTROL_SET& ccs)
{
    int rv;
    openIoacLib(ccs);
    // To disable wakeup.
    {
        DWORD bytesReturned = 0;
        rv = xlatCustomerError(ccs.CustomerControl(IOCTL_CUSTOMER_WAKEUP_DISABLE,
                                                   NULL, 0, NULL, NULL, &bytesReturned));
        if (rv != 0) {
            LOG_ERROR("Failed to disable wakeup secret, rv = %d", rv);
            goto end;
        }
    }
    // To query and check wakeup been disabled.
    {
        u32 ret = FALSE;
        DWORD bytesReturned = 0;
        rv = xlatCustomerError(ccs.CustomerControl(IOCTL_CUSTOMER_WAKEUP_QUERY,
                                                   NULL, 0, &ret, sizeof(ret), &bytesReturned));

        if (rv != 0) {
            LOG_WARN("Failed to query wakeup secret: %d", rv);
        } else if (ret == TRUE) {
            LOG_WARN("Wakeup secret does not seem to be disabled");
        }
    }
end:
    closeIoacLib(ccs);
}
//------------------------------

int DiskCache_SetData(std::string key, std::string value)
{
    int rv = VPL_OK;
    u32 write_bytes = 0;
    std::string obj_path;
    VPLFile_handle_t handle;

    if (!hasGlobalAccessDataPath) {
        return IOAC_UNAVAILABLE_GLOBAL_ACCESSDATA_PATH;
    }

    rv = VPLDir_Create(globalAccessDataPath.c_str(), 0777);
    if (rv != VPL_OK && rv != VPL_ERR_EXIST) {
        goto end;
    }

    obj_path.assign(globalAccessDataPath);
    obj_path.append("\\");
    obj_path.append(key);

    handle = VPLFile_Open(obj_path.c_str(), VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0);
    if ( !VPLFile_IsValidHandle(handle) ) {
        rv = VPL_ERR_FAIL;
        goto end;
    }

    write_bytes = VPLFile_Write(handle, value.c_str(), value.length());
    if (write_bytes != value.length()) {
        rv = VPL_ERR_FAIL;
    }

    rv = VPLFile_Close(handle);

end:

    return rv;
}

static int DiskCache_HasData(std::string key, bool& has_data)
{
    int rv = VPL_OK;
    VPLFS_stat_t state;
    std::string obj_path;

    if (!hasGlobalAccessDataPath) {
        return IOAC_UNAVAILABLE_GLOBAL_ACCESSDATA_PATH;
    }

    obj_path.assign(globalAccessDataPath);
    obj_path.append("\\");
    obj_path.append(key);

    rv = VPLFS_Stat(obj_path.c_str(), &state);
    if (rv == VPL_OK && state.size > 0) {
        has_data = TRUE;
    } else {
        has_data = FALSE;
    }

    return VPL_OK;
}

static int DiskCache_GetData(std::string key, std::string& value)
{
    int rv = VPL_OK;

    value.clear();

    if (!hasGlobalAccessDataPath) {
        return IOAC_UNAVAILABLE_GLOBAL_ACCESSDATA_PATH;
    }

    std::string obj_path;
    obj_path.assign(globalAccessDataPath);
    obj_path.append("\\");
    obj_path.append(key);

    {
        void* buf;
        int bytes_read = Util_ReadFile(obj_path.c_str(), /*out*/ &buf, 0);
        if (bytes_read < 0) {
            rv = bytes_read;
            goto end;
        }
        ON_BLOCK_EXIT(free, buf);
        value.assign((char*)buf, bytes_read);
    }

end:
    return rv;
}

static int DiskCache_DeleteData(std::string key)
{
    int rv = VPL_OK;
    u32 bytesRead = 0;
    std::string obj_path;

    if (!hasGlobalAccessDataPath) {
        return IOAC_UNAVAILABLE_GLOBAL_ACCESSDATA_PATH;
    }

    obj_path.assign(globalAccessDataPath);
    obj_path.append("\\");
    obj_path.append(key);

    rv = VPLFile_Delete(obj_path.c_str());

    return rv;
}


// Data structure used to hold any information associated with configuring IOAC on an adapter.
// Used as a singleton.
class IoacAdapterStateDB {

    // Data structure used to hold any data needed to enable IOAC on a particular adapter.
    class IoacParams {
    public:
        IoacParams()
            : asyncId(0),
              keepAliveInterval_ms(31 * VPLTIME_MILLISEC_PER_SEC), requested(false), received(false) {}

        u64 asyncId;
        bool requested;  // whether sleep data has been requested or not
        bool received;  // whether sleep data has been received or not

        u8 keepAlivePayload[ATH_CUSTOMER_KEEPALIVE_MAX_PACKET_SIZE + KEEPALIVE_EXTRA_BUFFER_FOR_INTEL - ALL_HDRS_LEN];
        size_t keepAlivePayloadLen;

        /// Secret required to wakeup the machine.
        NetMan_WakeupPktSecret_t secret;

        std::string serverHostname;
        /// In host byte order.
        VPLNet_port_t serverPortHbo;
        u32 keepAliveInterval_ms;

        void populatePCContext(ProgramChipContext& c);
    };

    // Data structure used to hold any data associated with a particular adapter.
    struct AdapterState {
        AdapterState(const GUID &guid) {
            status.set_guid(WinGuidToStr(guid));
        }
        ccd::IoacAdapterStatus status;
        IoacParams params;
    };

public:
    IoacAdapterStateDB() {
        VPLMutex_Init(&mutex);
    }
    ~IoacAdapterStateDB() {
        VPLMutex_Destroy(&mutex);
    }
    void clear();
    ccd::IoacOverallStatus getOverallStatus();

    // report UDP ping test status for the given asyncId
    void reportPingTestStarted(u64 asyncId);
    void reportPingTestProgress(u64 asyncId, int numSent, int numSucceeded, bool isDone);

    void reportIoacAdapters(const std::map<GUID, WakeableNetAdapter, GUIDCompare> &connectedIoacAdapters);

    // returns true iff asyncId is valid (i.e., known and current, not stale nor obsolete)
    bool asyncIdIsValid(u64 asyncId);

    // report sleep setup for given asyncId
    bool reportSleepSetup(u64 asyncId, const IoacSleepSetup &sleepSetup);

    // prepare device state for next sleep data
    void prepareNextSleepSetup(const GUID &guid, u64 asyncId);

    // set the overall status summary
    void setOverallStatus(const ccd::IoacStatusSummary_t summary);
    // set the overall status summary only if the new summary is at least as high in rank as the current one
    void upgradeOverallStatus(const ccd::IoacStatusSummary_t summary);

    void updateOverallStatus();

    // report adapter status for the adapter with the given asyncId
    void reportAdapterStatus(u64 asyncId, const ccd::IoacStatusSummary_t summary);
    void reportAdapterStatus(u64 asyncId, const ccd::IoacAdapterStatus &status);

    // report adapter status for the adapter with the given guid
    void reportAdapterStatus(const GUID &guid, const ccd::IoacStatusSummary_t summary);
    void reportAdapterStatus(const GUID &guid, const ccd::IoacAdapterStatus &status);

    // populate pcc for the given asyncId
    // returns true if successful 
    // returns false if asyncId is not known 
    bool populatePCContext(u64 asyncId, ProgramChipContext &pcc);

    // report outcome of chip programming
    void reportChipProgramResult(u64 asyncId, const ProgramChipContext &pcc, bool success);

    bool getGuidByAsyncId(u64 asyncId, GUID &guid);

    void warnSleepNotSupported(bool notSupported = true);

    void getAsyncIdListToProgram(std::vector<u64>& asyncIdList);

private:
    VPLMutex_t mutex;

    std::map<GUID, AdapterState*, GUIDCompare> adapterstate;

    /// Last known status of the IOAC functionality.
    // In the current implementation, update to the "adapters" field will deferred until a call to getOverallStatus().
    ccd::IoacOverallStatus overallStatus;  

    // Find the AdapterState object with the given asyncId.
    // Returns adapterstate.end() if not found.
    std::map<GUID, AdapterState*, GUIDCompare>::iterator findAdapterByAsyncId(u64 asyncId);

    // Report adapter status to the AdapterState object pointed to by the iterator.
    void reportAdapterStatus(std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it, const ccd::IoacStatusSummary_t summary);
    void reportAdapterStatus(std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it, const ccd::IoacAdapterStatus &status);
};

static IoacAdapterStateDB ioacAdapterStateDB;

//------------------------------

struct PingTestContext {
    VPLSocket_addr_t destSockAddr;
    VPLNet_addr_t localIpAddr;
    u64 asyncId;
};

static bool startUdpPingTest(u64 asyncId, VPLNet_addr_t localIpAddr, const VPLSocket_addr_t* destSockAddr);

static int
callCustControl(const WakeableNetAdapter& adapter, DWORD dwIoControlCode, LPVOID lpInBuffer,
        DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned)
{
    LOG_INFO("Calling CustControl("FMT_DWORD")", dwIoControlCode);
    CUSTOMER_RETURN_CODE rc = adapter.ioacInfo.CustomerControl(dwIoControlCode,
            lpInBuffer,
            nInBufferSize,
            lpOutBuffer,
            nOutBufferSize,
            lpBytesReturned);
    LOG_INFO("CustControl returned %d", rc);
    return xlatCustomerError(rc);
}

static int
ProbePmPol(const WakeableNetAdapter& adapter, NDIS_PM_PROTOCOL_OFFLOAD *pol, ULONG id)
{
    DWORD br = 0;

    // get protocol offload;
    pol->Header.Type = NDIS_HEADER_TYPE;
    pol->Header.Revision = NDIS_PM_PROTOCOL_OFFLOAD_REVISION_1;
    pol->Header.Size = sizeof(*pol);
    pol->Flags = 0;
    pol->NextProtocolOffloadOffset = 0;
    pol->ProtocolOffloadType = NdisPMProtocolOffloadIdIPv4ARP;
    pol->ProtocolOffloadId = id;

    *((ULONG *)pol) = id;
    int rv = callCustControl(adapter, IOCTL_CUSTOMER_NDIS_QUERY_PM_PROTOCOL_OFFLOAD, 
         pol, sizeof(*pol), pol, sizeof(*pol), &br);

    if (rv != 0) {
        LOG_ERROR("Query protocol offload error %d ", rv);
    } else if (br == 0) {
        LOG_ERROR("Query protocol offload returns no info");
        rv = -1;
    }

    return rv;
}

/*
 * setup custom wow features;
 */
static bool
WowHwSetup(WakeableNetAdapter& adapter, ProgramChipContext& context)
{
    openIoacLib(adapter.ioacInfo);

    ccd::IoacAdapterStatus& adapterStatus = context.newStatus;
    adapterStatus.set_guid(WinGuidToStr(adapter.ioacInfo.nId));

    LOG_DEBUG("Querying NDIS_PM_CAPABILITIES");
    {
        NDIS630_PM_CAPABILITIES pm_caps;
        memset(&pm_caps, 0, sizeof(pm_caps));
        pm_caps.Header.Type = NDIS_HEADER_TYPE;
        pm_caps.Header.Revision = NDIS_PM_CAPABILITIES_REVISION;
        pm_caps.Header.Size = sizeof(pm_caps);
#if DEBUG_NETMAN
        LOG_DEBUG("NDIS_PM_CAPABILITIES.Header.Revision %d", pm_caps.Header.Revision);
        LOG_DEBUG("NDIS_PM_CAPABILITIES.Header.Size %d", pm_caps.Header.Size);
#endif
        DWORD dummyBytesReturned = 0;
        int rv = callCustControl(adapter, IOCTL_CUSTOMER_NDIS_QUERY_PM_CAPABILITIES,
                &pm_caps, sizeof(pm_caps), &pm_caps, sizeof(pm_caps), &dummyBytesReturned);
        if (rv != 0) {
            LOG_ERROR("cannot query ndis capabilities: %d", rv);
            goto fail;
        }

        // report enabled patterns;

#if DEBUG_NETMAN
        LOG_DEBUG("Flags 0x%x", pm_caps.Flags);
        LOG_DEBUG("    wake packet indication %s""supported",
            (pm_caps.Flags & NDIS_PM_WAKE_PACKET_INDICATION_SUPPORTED)? "" : "not ");
        LOG_DEBUG("    selective suspend %s""supported",
            (pm_caps.Flags & NDIS_PM_SELECTIVE_SUSPEND_SUPPORTED)? "" : "not ");
        {
            DWORD x = pm_caps.SupportedWoLPacketPatterns;
            LOG_DEBUG("SupportedWoLPacketPatterns 0x%x", x);
            LOG_DEBUG("    bitmap patterns %s""supported",
                (x & NDIS_PM_WOL_BITMAP_PATTERN_SUPPORTED)? "" : "not ");
            LOG_DEBUG("    magic packet %s""supported",
                (x & NDIS_PM_WOL_MAGIC_PACKET_SUPPORTED)? "" : "not ");
            LOG_DEBUG("    ipv4 tcp syn %s""supported",
                (x & NDIS_PM_WOL_IPV4_TCP_SYN_SUPPORTED)? "" : "not ");
            LOG_DEBUG("    ipv6 tcp syn %s""supported",
                (x & NDIS_PM_WOL_IPV6_TCP_SYN_SUPPORTED)? "" : "not ");
            LOG_DEBUG("    ipv4 destination address wildcard %s""supported",
                (x & NDIS_PM_WOL_IPV4_DEST_ADDR_WILDCARD_SUPPORTED)? "" : "not ");
            LOG_DEBUG("    ipv6 destination address wildcard %s""supported",
                (x & NDIS_PM_WOL_IPV6_DEST_ADDR_WILDCARD_SUPPORTED)? "" : "not ");
            LOG_DEBUG("    eapol request id %s""supported",
                (x & NDIS_PM_WOL_EAPOL_REQUEST_ID_MESSAGE_SUPPORTED)? "" : "not ");
            LOG_DEBUG("    packet wildcard %s""supported",
                (x & NDIS_PM_WOL_PACKET_WILDCARD_SUPPORTED)? "" : "not ");
        }
#endif
        if (!(pm_caps.SupportedWoLPacketPatterns & NDIS_PM_WOL_MAGIC_PACKET_SUPPORTED)) {
            LOG_INFO("magic packet is not supported");
            adapterStatus.mutable_warnings()->set_magic_packet_not_supported(true);
        }

#if DEBUG_NETMAN
        LOG_DEBUG("   NumTotalWoLPatterns %lu", pm_caps.NumTotalWoLPatterns);
        LOG_DEBUG("   MaxWoLPatternSize %lu", pm_caps.MaxWoLPatternSize);
        LOG_DEBUG("   MaxWoLPatternOffset %lu", pm_caps.MaxWoLPatternOffset);
        LOG_DEBUG("   MaxWoLPacketSaveBuffer %lu", pm_caps.MaxWoLPacketSaveBuffer);
#endif

        {
            DWORD x = pm_caps.SupportedProtocolOffloads;
#if DEBUG_NETMAN
            LOG_DEBUG("   SupportedProtocolOffloads 0x%x", x);
#endif
            if ((x & NDIS_PM_PROTOCOL_OFFLOAD_ARP_SUPPORTED) == 0) {
                LOG_INFO("NDIS_PM_PROTOCOL_OFFLOAD_ARP_SUPPORTED capability is missing");
                adapterStatus.mutable_warnings()->set_offload_arp_not_supported(true);
            }
            if ((x & NDIS_PM_PROTOCOL_OFFLOAD_NS_SUPPORTED) == 0) {
                LOG_INFO("NDIS_PM_PROTOCOL_OFFLOAD_NS_SUPPORTED capability is missing");
                adapterStatus.mutable_warnings()->set_offload_ns_not_supported(true);
            }
            if ((x & NDIS_PM_PROTOCOL_OFFLOAD_80211_RSN_REKEY_SUPPORTED) == 0) {
                LOG_INFO("NDIS_PM_PROTOCOL_OFFLOAD_80211_RSN_REKEY_SUPPORTED capability is missing");
                adapterStatus.mutable_warnings()->set_offload_80211_rsn_rekey_not_supported(true);
            }
        }

#if 0
        x = pm_caps.NumArpOffloadIPv4Addresses;
        LOG_DEBUG("   NumArpOffloadIPv4Addresses %d\n", x);
        if(x < 2)
            supported = 0;

        x = pm_caps.NumNSOffloadIPv6Addresses;
        LOG_DEBUG("   NumNSOffloadIPv6Addresses %d\n", x);
        if(x < 2)
            supported = 0;

        x = pm_caps.MinMagicPacketWakeUp;
        LOG_DEBUG("   MinMagicPacketWakeUp 0x%x\n", x);
        LOG_DEBUG("    magic packet wakeup %s\n", NdisDevStateStr((NDIS_DEVICE_POWER_STATE)x));

        x = pm_caps.MinPatternWakeUp;
        LOG_DEBUG("   MinPatternWakeUp 0x%x\n", x);
        LOG_DEBUG("    pattern wakeup %s\n", NdisDevStateStr((NDIS_DEVICE_POWER_STATE)x));

        x = pm_caps.MinLinkChangeWakeUp;
        LOG_DEBUG("   MinLinkChangeWakeUp 0x%x\n", x);
        LOG_DEBUG("    link change wakeup %s\n", NdisDevStateStr((NDIS_DEVICE_POWER_STATE)x));

        x = pm_caps.SupportedWakeUpEvents;
        LOG_DEBUG("   SupportedWakeUpEvents 0x%x\n", x);

        x = pm_caps.MediaSpecificWakeUpEvents;
        LOG_DEBUG("   MediaSpecificWakeUpEvents 0x%x\n", x);
        LOG_DEBUG("    nlo discovery %ssupported\n",
            (x & NDIS_WLAN_WAKE_ON_NLO_DISCOVERY_SUPPORTED)? "" : "not ");
        LOG_DEBUG("    ap lost %ssupported\n",
            (x & NDIS_WLAN_WAKE_ON_AP_ASSOCIATION_LOST_SUPPORTED)? "" : "not ");
        LOG_DEBUG("    gtk handshake error %ssupported\n",
            (x & NDIS_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR_SUPPORTED)? "" : "not ");
        LOG_DEBUG("    4way handshake %ssupported\n",
            (x & NDIS_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST_SUPPORTED)? "" : "not ");
        mask = NDIS_WLAN_WAKE_ON_AP_ASSOCIATION_LOST_SUPPORTED
    #ifdef REQUIRE_NLO
            | NDIS_WLAN_WAKE_ON_NLO_DISCOVERY_SUPPORTED
    #endif // REQUIRE_NLO
            | NDIS_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR_SUPPORTED
            | NDIS_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST_SUPPORTED;
        if((x & mask) != mask)
            supported = 0;
#endif
    }

// Intel driver is broken wrt ARP offload, so skip this part of the code for Intel driver until the driver is fixed.
// TODO: Restore this part of the code for Intel driver after it is fixed.  Bug 1466.
  if (adapter.getWakeupPacketType() != ioac_intel || __ccdConfig.ioacArpOffloadForceOnIntel) {
    LOG_DEBUG("Setting NDIS_PM_PROTOCOL_OFFLOAD ARP");
    if (__ccdConfig.ioacProtocolOffloadMode >= 0)
    {
        DWORD dummyBytesReturned = 0;
        struct _NDIS_PM_PROTOCOL_OFFLOAD::_PROTOCOL_OFFLOAD_PARAMETERS::_IPV4_ARP_PARAMETERS *ap;
        NDIS_PM_PROTOCOL_OFFLOAD pm_pol;
        int offloadId = -1;

        if (__ccdConfig.ioacProtocolOffloadMode == 0) {
            NDIS_PM_PROTOCOL_OFFLOAD pm_pol_check;
            for (int id = 0; id <= MAX_PROTOCOL_OFFLOAD_ID; id++) {
                LOG_INFO("Querying offload ID %d", id);
		if (ProbePmPol(adapter, &pm_pol_check, id) == 0) {
                    if (pm_pol_check.ProtocolOffloadType == NdisPMProtocolOffloadIdMaximum) {
                        LOG_INFO("    Maximum offload id reached");
                        break; 
                    } else if (pm_pol_check.ProtocolOffloadType == NdisPMProtocolOffloadIdUnspecified) {
                        LOG_INFO("    Offload slot has unspecified type, can be used");
                        offloadId = id;
                        break;
                    } else {
                        LOG_INFO("    Offload slot is used for type %d", pm_pol_check.ProtocolOffloadType);
                    }
                }
            }
        } else {
            offloadId = __ccdConfig.ioacProtocolOffloadMode;
        }

        if (offloadId >= 0) {
            memset(&pm_pol, 0, sizeof(pm_pol));

            // set arp offload;
            pm_pol.Header.Type = NDIS_HEADER_TYPE;
            pm_pol.Header.Revision = NDIS_PM_PROTOCOL_OFFLOAD_REVISION_1;
            pm_pol.Header.Size = sizeof(pm_pol);

            // set arp parameters; 
            pm_pol.ProtocolOffloadType = NdisPMProtocolOffloadIdIPv4ARP;
            pm_pol.Priority = NDIS_PM_PROTOCOL_OFFLOAD_PRIORITY_NORMAL; 
            pm_pol.ProtocolOffloadId = offloadId; 
            LOG_INFO("  NDIS_PM_PROTOCOL_OFFLOAD.Header.Revision %d", pm_pol.Header.Revision);
            LOG_INFO("  NDIS_PM_PROTOCOL_OFFLOAD.Header.Size %d", pm_pol.Header.Size);
            LOG_INFO("  NDIS_PM_PROTOCOL_OFFLOAD.Type %d", pm_pol.ProtocolOffloadType);
            LOG_INFO("  NDIS_PM_PROTOCOL_OFFLOAD.ProtocolOffloadId %d", pm_pol.ProtocolOffloadId);

            ap = &pm_pol.ProtocolOffloadParameters.IPv4ARPParameters;

            memcpy(ap->HostIPv4Address, &adapter.localIpAddr, sizeof(ap->HostIPv4Address));
            memcpy(ap->MacAddress, adapter.localMacAddr.mac, sizeof(ap->MacAddress));

            int rv = callCustControl(adapter, IOCTL_CUSTOMER_NDIS_SET_PM_PROTOCOL_OFFLOAD,
                    &pm_pol, sizeof(pm_pol), &pm_pol, sizeof(pm_pol), &dummyBytesReturned);

            // Just log warning, keep going
            if (rv != 0) {
                LOG_WARN("%s failed: %d", "Offload ARP", rv);
                adapterStatus.mutable_warnings()->set_offload_arp_failed(true);
            }
        } else {
            LOG_WARN("Offload ARP failed to find offload slot");
        }
    }
  }

    LOG_DEBUG("Disable any existing keepalive packets");
    disableSleepKeepAlives_DllAlreadyLoaded(adapter.ioacInfo);

    LOG_DEBUG("Enable keepalive packet (len="FMTu32")", context.keepAlivePacketLen);
    {
        u32 numToEnable = NUM_KEEPALIVES_TO_USE;
        for(u32 n = 0; n < NUM_KEEPALIVES_TO_USE; n++) {
            // enable the keepalive
            {
                CUSTOMER_KEEPALIVE_SET ka;
                ka.nId = n;
                ka.nMSec = context.keepAliveInterval_ms;
                ka.nSize = context.keepAlivePacketLen;
                memcpy(ka.bPacket, context.keepAlivePacket, ka.nSize);
                if (adapter.getWakeupPacketType() == ioac_intel) {
                    // need to insert source MAC and UDP code (0800) at offset 6
                    memmove(ka.bPacket + MAC_HDR_LEN + EXTRA_INTEL_HDRS_LEN,
                            ka.bPacket + MAC_HDR_LEN,
                            ka.nSize - MAC_HDR_LEN);
                    ka.nSize += EXTRA_INTEL_HDRS_LEN;
                    memcpy(ka.bPacket + MAC_HDR_LEN, adapter.localMacAddr.mac, MAC_HDR_LEN);
                    ka.bPacket[MAC_HDR_LEN  + MAC_HDR_LEN    ] = 0x08;
                    ka.bPacket[MAC_HDR_LEN  + MAC_HDR_LEN + 1] = 0x00;
                }
                DWORD dummyBytesReturned = 0;
                int rv = callCustControl(adapter, IOCTL_CUSTOMER_KEEPALIVE_SET,
                        &ka, sizeof(ka), NULL, 0, &dummyBytesReturned);
                if (rv != 0) {
                    LOG_WARN("%s failed: %d", "Enable keepalive", rv);
                    adapterStatus.mutable_warnings()->set_enable_keepalive_failed(true);
                    continue;
                }
            }
            // check that it is reported as enabled
            {
                CUSTOMER_KEEPALIVE_QUERY kaq;
                kaq.nId = n;
                kaq.nRet = FALSE;
                DWORD bytesReturned = 0;
                int rv = callCustControl(adapter, IOCTL_CUSTOMER_KEEPALIVE_QUERY,
                        &kaq, sizeof(kaq), &kaq, sizeof(kaq), &bytesReturned);
                if (rv != 0) {
                    LOG_WARN("%s failed: %d", "Query keep-alive", rv);
                    continue;
                }
                if (bytesReturned != sizeof(kaq)) {
                    LOG_WARN("Query keep-alive returned wrong size: "FMT_DWORD, bytesReturned);
                    continue;
                }
                if (kaq.nRet != TRUE) {
                    LOG_WARN("Keep-alive does not appear to be enabled");
                    continue;
                }
            }
            numToEnable--;
        }
        if (numToEnable != 0) {
            LOG_WARN("Could not enable all keep-alives");
            goto fail;
        }
    }

    if (adapter.getWakeupPacketType() == ioac_intel) {
        CUSTOMER_FIXEDPATTERN_SET fixedPatternReq;
        ASSERT_EQUAL_SIZE_COMPILE_TIME(ARRAY_SIZE_IN_BYTES(fixedPatternReq.bPattern), ARRAY_SIZE_IN_BYTES(MAGIC_PKT_FIXED_PATTERN));
        memcpy(fixedPatternReq.bPattern, MAGIC_PKT_FIXED_PATTERN, ARRAY_SIZE_IN_BYTES(fixedPatternReq.bPattern));
        DWORD dummyBytesReturned = 0;
        int rv = callCustControl(adapter, IOCTL_CUSTOMER_FIXEDPATTERN_SET,
                &fixedPatternReq, sizeof(fixedPatternReq), NULL, 0, &dummyBytesReturned);
        if (rv != 0) {
            LOG_WARN("Failed to set fixed pattern: %d", rv);
            goto failed_to_set_wakeup_match;
        }
    }

    LOG_DEBUG("Setting wakeup secret ("FMTxx8 FMTx8 FMTxx8 FMTxx8 FMTxx8 FMTxx8")",
            context.secret.ext[0], context.secret.ext[1], context.secret.ext[2],
            context.secret.ext[3], context.secret.ext[4], context.secret.ext[5]);
    {
        CUSTOMER_WAKEUP_MATCH_SET match;
        ASSERT_EQUAL_SIZE_COMPILE_TIME(ARRAY_SIZE_IN_BYTES(match.bPattern), ARRAY_SIZE_IN_BYTES(context.secret.ext));
        memcpy(match.bPattern, context.secret.ext, ARRAY_SIZE_IN_BYTES(match.bPattern));
        DWORD dummyBytesReturned = 0;
        int rv = callCustControl(adapter, IOCTL_CUSTOMER_WAKEUP_MATCH_SET,
                &match, sizeof(match), NULL, 0, &dummyBytesReturned);
        if (rv != 0) {
            LOG_WARN("Failed to set wakeup secret: %d", rv);
            goto failed_to_set_wakeup_match;
        }
    }

    LOG_DEBUG("Confirming wakeup secret");
    {
        u32 ret = FALSE;
        DWORD bytesReturned = 0;
        int rv = callCustControl(adapter, IOCTL_CUSTOMER_WAKEUP_QUERY,
                NULL, 0, &ret, sizeof(ret), &bytesReturned);
      // latest woc.cpp has these checks commented out, so do the same here for the time being
      if (adapter.getWakeupPacketType() != ioac_intel) {
        if (rv != 0) {
            LOG_WARN("%s failed: %d", "Wakeup secret query", rv);
            goto failed_to_set_wakeup_match;
        }
        if (bytesReturned != sizeof(ret)) {
            LOG_WARN("Wakeup secret query returned wrong size: "FMT_DWORD, bytesReturned);
            goto failed_to_set_wakeup_match;
        }
        if (ret != TRUE) {
            LOG_WARN("Wakeup secret does not seem to be enabled");
            goto failed_to_set_wakeup_match;
        }
      }
    }

    LOG_INFO("Wake-on-wifi setup successful");
    closeIoacLib(adapter.ioacInfo);
    return true;

failed_to_set_wakeup_match:
    LOG_INFO("Disable sending keep-alive packets.");
    disableSleepKeepAlives_DllAlreadyLoaded(adapter.ioacInfo);

fail:
    closeIoacLib(adapter.ioacInfo);
    return false;
}

static void
scanIoacHardware(HANDLE hWlan, ccd::IoacOverallStatus& newStatus, std::map<GUID, WakeableNetAdapter, GUIDCompare>& connectedIoacAdapters)
{
    connectedIoacAdapters.clear();
    {
        // EnumIoacNetAdapter(NULL) causes global variables to be updated, so only do this from the
        // single netman worker thread.
        int ioacNicCount = EnumIoacNetAdapter(NULL);
        LOG_INFO("IOAC-capable adapters: %d", ioacNicCount);
        
        char** ioacGuidStrs = new char*[ioacNicCount];
        ON_BLOCK_EXIT(deleteArray<char*>, ioacGuidStrs);
        
        ASSERT_EQUAL(EnumIoacNetAdapter(ioacGuidStrs), ioacNicCount, "%d");
        
        CUSTOMER_CONTROL_SET** ioacNics = new CUSTOMER_CONTROL_SET*[ioacNicCount];
        ON_BLOCK_EXIT(deleteArray<CUSTOMER_CONTROL_SET*>, ioacNics);
        
        for (int i = 0; i < ioacNicCount; i++) {
            CUSTOMER_CONTROL_SET* curr = ioacNics[i] = LoadCustCtlSet(ioacGuidStrs[i]);
            const char* typeStr;
            switch (curr->nType) {
            case CONNECTION_WIRED:
                typeStr = "Wired";
                break;
            case CONNECTION_WIRELESS:
                typeStr = "Wireless";
                break;
            default:
                typeStr = "Unknown";
                break;
            }
            LOG_INFO("IOAC_NIC[%d]: %s, GUID=%s, %s, packetFmt=%x",
                    i, ioacGuidStrs[i], WinGuidToStr(curr->nId).c_str(), typeStr, curr->nPacketFmt);
        }

        {
            IP_ADAPTER_ADDRESSES* ipAdapterAddrs = getIpAdapterAddresses();
            if (ipAdapterAddrs == NULL) {
                LOG_ERROR("Unable to get adapters");
                newStatus.set_summary(ccd::IOAC_STATUS_SUMMARY_FAIL_NO_HARDWARE);
                goto done;
            }
            ON_BLOCK_EXIT(HEAP_FREE, ipAdapterAddrs);

            findAdapters(ipAdapterAddrs, ioacNicCount, ioacNics, connectedIoacAdapters);
            LOG_INFO(FMTu_size_t" connected IOAC interface(s) found", connectedIoacAdapters.size());
            if (connectedIoacAdapters.size() < 1) {
                newStatus.set_summary(ccd::IOAC_STATUS_SUMMARY_FAIL_NO_HARDWARE);
                goto done;
            }
        }

        int numWifiIoac = 0;
        std::map<GUID, WakeableNetAdapter, GUIDCompare>::iterator it;
        for (it = connectedIoacAdapters.begin(); it != connectedIoacAdapters.end(); it++) {
            if (it->second.ioacInfo.nType == CONNECTION_WIRELESS) {
                numWifiIoac++;
            }
        }
        
        if (numWifiIoac > 0) {
            LOG_DEBUG("Checking wireless network interfaces");
            {
                WLAN_INTERFACE_INFO_LIST *iflist;
                DWORD err = WlanEnumInterfaces(hWlan, NULL, &iflist);
                if (err != ERROR_SUCCESS) {
                    s32 rv = VPLError_XlatWinErrno(err);
                    LOG_ERROR("%s failed: %d", "WlanEnumInterfaces", rv);
                } else {
                    LOG_DEBUG(FMT_DWORD" total wlan interface(s) found", iflist->dwNumberOfItems);
                    wlanCheckIfs(hWlan, iflist, connectedIoacAdapters);
                    WlanFreeMemory(iflist);
                }
            }
        }
    }
 done:
    ;
}

static void
requestSleepSetup(WakeableNetAdapter &wakeableNetAdapter,
                  u64 asyncId, ccd::IoacAdapterStatus &newStatus)
{
    if (wakeableNetAdapter.isValid()) {
        int wakeupPacketType = wakeableNetAdapter.getWakeupPacketType();
        LOG_INFO("Requesting ANS sleep setup for %s, type=%d, asyncId="FMTu64,
                 WinGuidToStr(wakeableNetAdapter.ioacInfo.nId).c_str(),
                 wakeupPacketType, asyncId);
        if (wakeupPacketType == ioac_proto) {
            VPL_BOOL requestOk = ANSConn_RequestSleepSetup(wakeupPacketType, asyncId,
                                                           wakeableNetAdapter.localMacAddr.mac,
                                                           ARRAY_SIZE_IN_BYTES(wakeableNetAdapter.localMacAddr.mac));
            newStatus.set_summary(requestOk ? ccd::IOAC_STATUS_SUMMARY_GETTING_INFO : ccd::IOAC_STATUS_SUMMARY_INACTIVE);
        } else if (wakeupPacketType == ioac_intel) {
            VPL_BOOL requestOk = ANSConn_RequestSleepSetup(wakeupPacketType, asyncId,
                                                           MAGIC_PKT_FIXED_PATTERN,
                                                           ARRAY_SIZE_IN_BYTES(MAGIC_PKT_FIXED_PATTERN));
            newStatus.set_summary(requestOk ? ccd::IOAC_STATUS_SUMMARY_GETTING_INFO : ccd::IOAC_STATUS_SUMMARY_INACTIVE);
        } else {
            LOG_ERROR("Hardware doesn't support any of the known packet types!");
        }
    }
}

static bool 
programIoacHardware(WakeableNetAdapter &wakeableNetAdapter,
                    ProgramChipContext &context)
{
    bool success = false;

    LOG_DEBUG("Resolving server IP address for \"%s\"", context.serverHostname.c_str());
    VPLNet_addr_t destIp;
    {
        // TODO: do we need to somehow make this use the specific network card's DNS?
        destIp = VPLNet_GetAddr(context.serverHostname.c_str());
        if (destIp == VPLNET_ADDR_INVALID) {
            LOG_ERROR("Unable to resolve \"%s\"", context.serverHostname.c_str());
            context.newStatus.set_summary(ccd::IOAC_STATUS_SUMMARY_FAIL_RESOLVE_SERVER);
            goto out;
        }
        LOG_DEBUG("Resolved \"%s\" to "FMT_VPLNet_addr_t, context.serverHostname.c_str(), VAL_VPLNet_addr_t(destIp));
    }

    LOG_DEBUG("Checking power capabilities");
    {
        SYSTEM_POWER_CAPABILITIES pwrcaps;
        if (!GetPwrCapabilities(&pwrcaps)) {
            s32 rv = VPLError_XlatWinErrno(GetLastError());
            LOG_WARN("%s failed: %d", "GetPwrCapabilities", rv);
            ioacAdapterStateDB.warnSleepNotSupported();
            goto skip_pwrcaps;
        }
        if (pwrcaps.SystemS3 || pwrcaps.SystemS4) {
            LOG_INFO("System supports sleep: %s%s", (pwrcaps.SystemS3 ? "S3 " : ""), (pwrcaps.SystemS4 ? "S4 " : ""));
        } else {
            LOG_WARN("System does not appear to support sleep");
            ioacAdapterStateDB.warnSleepNotSupported();
        }
    }
 skip_pwrcaps:

    WakeableNetAdapter& curr = wakeableNetAdapter;
    if (curr.isValid()) {
        LOG_INFO("Setting up IOAC using asyncId="FMTu64, context.sleepSetupAsyncId);
        NetMan_MacAddr_t firstHopMacAddr;
        {
            MIB_IPFORWARDROW bestRoute;
            DWORD err = GetBestRoute(destIp, curr.localIpAddr, &bestRoute);
            if (err != ERROR_SUCCESS) {
                LOG_INFO("GetBestRoute failed: %d", VPLError_XlatWinErrno(err));
                goto out;
            }
            VPLNet_addr_t nextHop = bestRoute.dwForwardNextHop;
            LOG_INFO("Next hop IP: "FMT_VPLNet_addr_t, VAL_VPLNet_addr_t(nextHop));
            // Note that bestRoute.dwForwardNextHop can be INADDR_ANY (0) if the server
            // is on the local network.  We don't actually care about this case in CCD though.
            ULONG macLen = ARRAY_SIZE_IN_BYTES(firstHopMacAddr.mac);
            err = SendARP(nextHop, curr.localIpAddr, firstHopMacAddr.mac, &macLen);
            if (err != ERROR_SUCCESS) {
                LOG_INFO("SendARP failed: %d", VPLError_XlatWinErrno(err));
                goto out;
            }
            LOG_INFO("Next hop MAC: %s", MacAddrStr(firstHopMacAddr).c_str());
        }
        const u16 localPort = 22222; // totally arbitrary
        keepAlivePacket_construct(context.keepAlivePacket, context.keepAlivePacketLen,
                                  firstHopMacAddr, curr.localIpAddr, localPort, destIp, context.serverPortHbo);
        success = WowHwSetup(curr, context);
        if (success) {
            VPLSocket_addr_t destSockAddr = { PF_INET, destIp, VPLNet_port_hton(context.serverPortHbo) };
            if (startUdpPingTest(context.sleepSetupAsyncId, curr.localIpAddr, &destSockAddr)) {
                context.newStatus.set_summary(ccd::IOAC_STATUS_SUMMARY_TESTING);
            }
        }
    }

 out:
    return success;
}


//-----------------------------------------

typedef enum _NDIS_PM_WAKE_REASON_TYPE {
    NdisWakeReasonUnspecified = 0x0000,
    NdisWakeReasonPacket = 0x0001,
    NdisWakeReasonMediaDisconnect = 0x0002,
    NdisWakeReasonMediaConnect = 0x0003,
    NdisWakeReasonWlanNLODiscovery = 0x1000,
    NdisWakeReasonWlanAPAssociationLost = 0x1001,
    NdisWakeReasonWlanGTKHandshakeError = 0x1002,
    NdisWakeReasonWlan4WayHandshakeRequest = 0x1003,
    NdisWakeReasonWwanRegisterState = 0x2000,
    NdisWakeReasonWwanSMSReceive = 0x2001,
    NdisWakeReasonWwanUSSDReceive = 0x2002,
    NdisWakeReasonAcerMagicPacket = 0xF001,
    NdisWakeReasonChipWakeTimer = 0xF002,
} NDIS_PM_WAKE_REASON_TYPE, *PNDIS_PM_WAKE_REASON_TYPE;

static const char* NdisWakeReason(NDIS_PM_WAKE_REASON_TYPE wr)
{
    switch(wr) {
    case NdisWakeReasonUnspecified:
        return("unspecified");
    case NdisWakeReasonPacket:
        return("magic packet");
    case NdisWakeReasonMediaDisconnect:
        return("media disconnect");
    case NdisWakeReasonMediaConnect:
        return("media connect");
    case NdisWakeReasonWlanNLODiscovery:
        return("NLO discovery");
    case NdisWakeReasonWlanAPAssociationLost:
        return("AP association lost");
    case NdisWakeReasonWlanGTKHandshakeError:
        return("GTK rekey error");
    case NdisWakeReasonWlan4WayHandshakeRequest:
        return("4-way handshake");
    case NdisWakeReasonWwanRegisterState:
        return("WwanRegisterState");
    case NdisWakeReasonWwanSMSReceive:
        return("WwanSMSReceive");
    case NdisWakeReasonWwanUSSDReceive:
        return("WwanUSSDReceive");
    case NdisWakeReasonAcerMagicPacket:
        return("acer magic packet");
    case NdisWakeReasonChipWakeTimer:
        return("ChipWakeTimer");
    }
    return("unknown wake reason");
}

typedef struct _NDIS630_PM_WAKE_REASON {
    NDIS_OBJECT_HEADER Header;
    ULONG Flags;
    NDIS_PM_WAKE_REASON_TYPE WakeReason;
    ULONG InfoBufferOffset;
    ULONG InfoBufferSize;
} NDIS630_PM_WAKE_REASON;

static void checkWakeReason(const std::map<GUID, WakeableNetAdapter, GUIDCompare>& connectedIoacAdapters)
{
    // Temporary. Bug 2151.
    if (__ccdConfig.ioacDontCheckWakeReason)
        return;

    if (connectedIoacAdapters.size() < 1) {
        LOG_INFO("No active IOAC adapters; cannot get wake reason");
    } else {
        std::map<GUID, WakeableNetAdapter, GUIDCompare>::const_iterator it;
        for (it = connectedIoacAdapters.begin(); it != connectedIoacAdapters.end(); it++) {
	    openIoacLib(it->second.ioacInfo);
            struct {
                NDIS630_PM_WAKE_REASON wr;
                UCHAR buf[16];
            } pm;
            pm.wr.Header.Type = NDIS_HEADER_TYPE;
            pm.wr.Header.Revision = NDIS_PM_WAKE_REASON_REVISION;
            pm.wr.Header.Size = sizeof(pm.wr);
            pm.wr.Flags = 0;
            pm.wr.InfoBufferOffset = sizeof(pm.wr);
            pm.wr.InfoBufferSize = sizeof(pm.buf);
            DWORD bytesReturned = 0;
            int rv = callCustControl(it->second, IOCTL_CUSTOMER_NDIS_QUERY_PM_WAKE_REASON,
                    &pm.wr, sizeof(pm.wr), &pm.wr, sizeof(pm.wr), &bytesReturned);
            if (rv != 0) {
                LOG_ERROR("IOAC: Failed to get wake reason from %s: %d", WinGuidToStr(it->first).c_str(), rv);
            } else {
                NDIS_PM_WAKE_REASON_TYPE wrt = pm.wr.WakeReason;
                LOG_INFO("IOAC: Wake reason for %s: 0x%x (%s)", WinGuidToStr(it->first).c_str(), wrt, NdisWakeReason(wrt));
            }
	    closeIoacLib(it->second.ioacInfo);
        }
    }
}

//-----------------------------------------

struct NetMan_Wlan_t
{
    HANDLE hWlan; // wlan handle;
    DWORD negotiatedVer; // negotiated version;

    NetMan_Wlan_t() : hWlan(INVALID_HANDLE_VALUE) {}
};

//------------------------------

static VPLTHREAD_FN_DECL workerThreadFn(void* arg);

class IoacWorker {
public:
    IoacWorker() : threadSpawned(false), lastAsyncId(0) {
        VPLMutex_Init(&mutex);
    }
    ~IoacWorker() {
        VPLMutex_Destroy(&mutex);
    }
    bool isRunning() const {
        MutexAutoLock lock(&mutex);
        return threadSpawned;
    }
    s32 spawnThread() {
        MutexAutoLock lock(&mutex);
        if (threadSpawned)
            return CCD_ERROR_ALREADY;
        s32 rv = VPLDetachableThread_Create(&thread, ::workerThreadFn, NULL, NULL, NULL);
        if (rv != VPL_OK) {
            LOG_ERROR("%s failed: %d", "VPLDetachableThread_Create", rv);
            return rv;
        }
        threadSpawned = true;
        return rv;
    }
    s32 joinThread() {
        {
            MutexAutoLock lock(&mutex);
            if (!threadSpawned)
                return CCD_ERROR_ALREADY;
        }
        s32 rv = VPLDetachableThread_Join(&thread);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "VPLDetachableThread_Join", rv);
            return rv;
        }
        {
            MutexAutoLock lock(&mutex);
            threadSpawned = false;
        }
        return rv;
    }
    u64 getNextAsyncId() {
        MutexAutoLock lock(&mutex);
        return ++lastAsyncId;
    }
private:
    mutable VPLMutex_t mutex;
    bool threadSpawned;
    VPLDetachableThreadHandle_t thread;
    u64 lastAsyncId;

    //static VPLTHREAD_FN_DECL workerFunc(void *arg);
};

static IoacWorker ioacWorker;

/// Must hold this when accessing any other static field in this file
static VPLLazyInitMutex_t s_mutex = VPLLAZYINITMUTEX_INIT;

//-------------------------------------

void IoacAdapterStateDB::IoacParams::populatePCContext(ProgramChipContext& c)
{
    c.newStatus.Clear();
    memcpy(&c.keepAlivePacket[ALL_HDRS_LEN], keepAlivePayload, keepAlivePayloadLen);
    c.keepAlivePacketLen = ALL_HDRS_LEN + keepAlivePayloadLen;
    c.secret = secret;
    c.sleepSetupAsyncId = asyncId;
    c.serverHostname = serverHostname;
    c.serverPortHbo = serverPortHbo;
    c.keepAliveInterval_ms = keepAliveInterval_ms;
}

void IoacAdapterStateDB::clear()
{
    MutexAutoLock lock(&mutex);
    std::map<GUID, AdapterState*, GUIDCompare>::iterator it;
    for (it = adapterstate.begin(); it != adapterstate.end(); it++) {
        delete it->second;
        it->second = NULL;
    }
    adapterstate.clear();

    overallStatus.Clear();
}

ccd::IoacOverallStatus IoacAdapterStateDB::getOverallStatus()
{
    MutexAutoLock lock(&mutex);

    // TODO - don't clear every time, modify if present, and create only if necessary
    overallStatus.clear_adapters();
    std::map<GUID, AdapterState*, GUIDCompare>::const_iterator it;
    for (it = adapterstate.begin(); it != adapterstate.end(); it++) {
        ccd::IoacAdapterStatus *adapterStatus = overallStatus.add_adapters();
        *adapterStatus = it->second->status;
    }
    return overallStatus;
}

void IoacAdapterStateDB::reportIoacAdapters(const std::map<GUID, WakeableNetAdapter, GUIDCompare> &connectedIoacAdapters)
{
    // We won't duplicate the same information inside IoacAdapterStateDB.
    // However, we'll use this opportunity to prune any device states that are no longer needed (because the adapter disappeared)

    std::map<GUID, AdapterState*, GUIDCompare>::iterator it;
    for (it = adapterstate.begin(); it != adapterstate.end(); it++) {
        if (connectedIoacAdapters.find(it->first) == connectedIoacAdapters.end()) {
            delete it->second;
            it = adapterstate.erase(it);  // MS-ism
        }
    }
}

void IoacAdapterStateDB::prepareNextSleepSetup(const GUID &guid, u64 asyncId)
{
    MutexAutoLock lock(&mutex);
    if (adapterstate[guid] == NULL) {
        adapterstate[guid] = new AdapterState(guid);
    }
    adapterstate[guid]->params.asyncId = asyncId;
    adapterstate[guid]->params.requested = false;
    adapterstate[guid]->params.received = false;
}

static int getIoacStatusSummaryRank(ccd::IoacStatusSummary_t summary)
{
    int rank = 0;
    switch (summary) {
    case ccd::IOAC_STATUS_SUMMARY_GOOD:
        rank = 40;
        break;
    case ccd::IOAC_STATUS_SUMMARY_TESTING:
        rank = 30;
        break;
    case ccd::IOAC_STATUS_SUMMARY_GETTING_INFO:
        rank = 20;
        break;
    case ccd::IOAC_STATUS_SUMMARY_UPDATING:
        rank = 10;
        break;
    default:
        rank = 0;
    }
    return rank;
}

void IoacAdapterStateDB::setOverallStatus(const ccd::IoacStatusSummary_t summary)
{
    MutexAutoLock lock(&mutex);
    overallStatus.set_summary(summary);
    ::postIoacStatusChangeEvent();
}

void IoacAdapterStateDB::upgradeOverallStatus(const ccd::IoacStatusSummary_t summary)
{
    // Overall status summary will be changed only if the "rank" of the new summary
    // is at least as high as the current overall status summary.
    // Note that we allow the status to change within the same rank.
    // This is so that at rank 0, the most recent error will be reflected 
    // in the overall status summary.

    MutexAutoLock lock(&mutex);
    if (getIoacStatusSummaryRank(summary) >= getIoacStatusSummaryRank(overallStatus.summary())) {
        setOverallStatus(summary);
    }
}

void IoacAdapterStateDB::updateOverallStatus()
{
    MutexAutoLock lock(&mutex);
    ccd::IoacStatusSummary_t summary = ccd::IOAC_STATUS_SUMMARY_INACTIVE;
    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it;
    for (it = adapterstate.begin(); it != adapterstate.end(); it++) {
        if (getIoacStatusSummaryRank(it->second->status.summary()) >= getIoacStatusSummaryRank(summary)) {
            summary = it->second->status.summary();
        }
    }

    if (summary != overallStatus.summary()) {
        setOverallStatus(summary);
    }
}

// IoacAdapterStateDB-internal use only
void IoacAdapterStateDB::reportAdapterStatus(std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it, const ccd::IoacStatusSummary_t summary)
{
    if (it != adapterstate.end()) {
        it->second->status.set_summary(summary);

        updateOverallStatus();
    }
}

// IoacAdapterStateDB-internal use only
void IoacAdapterStateDB::reportAdapterStatus(std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it, const ccd::IoacAdapterStatus &status)
{
    if (it != adapterstate.end()) {
        reportAdapterStatus(it, status.summary());
#define check_and_set(field)                                            \
        if (status.warnings().field())                                  \
            it->second->status.mutable_warnings()->set_##field(true)
        check_and_set(magic_packet_not_supported);
        check_and_set(offload_arp_not_supported);
        check_and_set(offload_ns_not_supported);
        check_and_set(offload_80211_rsn_rekey_not_supported);
        check_and_set(offload_arp_failed);
        check_and_set(enable_keepalive_failed);
#undef check_and_set
    }
}

void IoacAdapterStateDB::reportAdapterStatus(u64 asyncId, const ccd::IoacStatusSummary_t summary)
{
    MutexAutoLock lock(&mutex);
    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it = findAdapterByAsyncId(asyncId);
    if (it != adapterstate.end()) {
        reportAdapterStatus(it, summary);
        if (summary == ccd::IOAC_STATUS_SUMMARY_GETTING_INFO) {
            it->second->params.requested = true;
        }
    }
    else {
        LOG_DEBUG("Unknown asyncId ("FMTu64")", asyncId);
    }
}

void IoacAdapterStateDB::reportAdapterStatus(u64 asyncId, const ccd::IoacAdapterStatus &status)
{
    MutexAutoLock lock(&mutex);
    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it = findAdapterByAsyncId(asyncId);
    if (it != adapterstate.end()) {
        reportAdapterStatus(it, status);
        if (status.summary() == ccd::IOAC_STATUS_SUMMARY_GETTING_INFO) {
            it->second->params.requested = true;
        }
    }
    else {
        LOG_DEBUG("Unknown asyncId ("FMTu64")", asyncId);
    }
}

void IoacAdapterStateDB::reportAdapterStatus(const GUID &guid, const ccd::IoacStatusSummary_t summary)
{
    MutexAutoLock lock(&mutex);
    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it = adapterstate.find(guid);
    if (it != adapterstate.end()) {
        reportAdapterStatus(it, summary);
    }
    else {
        LOG_DEBUG("Unknown guid (%s)", WinGuidToStr(guid).c_str());
    }
}

void IoacAdapterStateDB::reportAdapterStatus(const GUID &guid, const ccd::IoacAdapterStatus &status)
{
    MutexAutoLock lock(&mutex);
    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it = adapterstate.find(guid);
    if (it != adapterstate.end()) {
        reportAdapterStatus(it, status);
    }
    else {
        LOG_DEBUG("Unknown guid (%s)", WinGuidToStr(guid).c_str());
    }
}

// for IoacAdapterStateDB-internal use only
std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator IoacAdapterStateDB::findAdapterByAsyncId(u64 asyncId)
{
    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it;
    for (it = adapterstate.begin(); it != adapterstate.end(); it++) {
        if (it->second->params.asyncId == asyncId)
            return it;
    }
    return adapterstate.end();
}

void IoacAdapterStateDB::reportPingTestStarted(u64 asyncId)
{
    LOG_DEBUG("Ping test ("FMTu64") started", asyncId);

    MutexAutoLock lock(&mutex);
    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it = findAdapterByAsyncId(asyncId);
    if (it != adapterstate.end()) {
        reportAdapterStatus(it, ccd::IOAC_STATUS_SUMMARY_TESTING);
    }
    else {
        LOG_DEBUG("Unknown asyncId "FMTu64, asyncId);
    }
}

void IoacAdapterStateDB::reportPingTestProgress(u64 asyncId, int numSent, int numSucceeded, bool isDone)
{
    LOG_DEBUG("Ping test ("FMTu64") progress %d/%d (%s)", asyncId, numSucceeded, numSent, isDone ? "done" : "in progress");

    MutexAutoLock lock(&mutex);
    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it = findAdapterByAsyncId(asyncId);
    if (it != adapterstate.end()) {
        if (numSucceeded > 0) {
            reportAdapterStatus(it, ccd::IOAC_STATUS_SUMMARY_GOOD);
        }
        else if (isDone && numSucceeded == 0) {
            reportAdapterStatus(it, ccd::IOAC_STATUS_SUMMARY_UDP_FILTERED);
        }
    }
    else {
        LOG_DEBUG("Unknown asyncId "FMTu64, asyncId);
    }
}

bool IoacAdapterStateDB::asyncIdIsValid(u64 asyncId)
{
    MutexAutoLock lock(&mutex);
    return findAdapterByAsyncId(asyncId) != adapterstate.end();
}

bool IoacAdapterStateDB::reportSleepSetup(u64 asyncId, const IoacSleepSetup &sleepSetup)
{
    MutexAutoLock lock(&mutex);

    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it = findAdapterByAsyncId(asyncId);
    if (it == adapterstate.end()) {
        LOG_INFO("Unknown asyncId "FMTu64, asyncId);
        return false;
    }

    LOG_INFO("Received IOAC params (asyncId="FMTu64")", asyncId);

    memcpy(it->second->params.secret.ext, sleepSetup.wakeupKey.data(), ARRAY_SIZE_IN_BYTES(it->second->params.secret.ext));
    LOG_INFO("Wakeup secret is "FMTxx8 FMTx8 FMTxx8 FMTxx8 FMTxx8 FMTxx8,
             it->second->params.secret.ext[0], it->second->params.secret.ext[1], it->second->params.secret.ext[2],
             it->second->params.secret.ext[3], it->second->params.secret.ext[4], it->second->params.secret.ext[5]);

    it->second->params.serverHostname = sleepSetup.serverHostname;
    it->second->params.serverPortHbo = sleepSetup.serverPort;

    u32 packetIntervalSec = sleepSetup.packetIntervalSec;
    if (__ccdConfig.sleepPacketInterval > 0) {
        LOG_INFO("Overriding sleep packet interval ("FMTu32") from config (%d)",
                 sleepSetup.packetIntervalSec, __ccdConfig.sleepPacketInterval);
        packetIntervalSec = static_cast<u32>(__ccdConfig.sleepPacketInterval);
    }
    it->second->params.keepAliveInterval_ms = VPLTIME_MILLISEC_PER_SEC * packetIntervalSec;

    it->second->params.keepAlivePayloadLen = sleepSetup.payload.size();
    memcpy(it->second->params.keepAlivePayload, sleepSetup.payload.data(), sleepSetup.payload.size());

    LOG_INFO("Keep-alive server is %s:"FMT_VPLNet_port_t", payload is "FMTu_size_t" bytes, interval is "FMTu32"ms",
             it->second->params.serverHostname.c_str(), it->second->params.serverPortHbo, it->second->params.keepAlivePayloadLen, it->second->params.keepAliveInterval_ms);

    it->second->params.received = true;

    return true;
}

bool IoacAdapterStateDB::populatePCContext(u64 asyncId, ProgramChipContext &pcc)
{
    bool result = false;
    MutexAutoLock lock(&mutex);

    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it = findAdapterByAsyncId(asyncId);
    if (it != adapterstate.end()) {
        it->second->params.populatePCContext(pcc);
        result = true;
    }
    return result;
}

void IoacAdapterStateDB::reportChipProgramResult(u64 asyncId, const ProgramChipContext &pcc, bool success)
{
    MutexAutoLock lock(&mutex);

    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it = findAdapterByAsyncId(asyncId);
    if (it != adapterstate.end()) {
        reportAdapterStatus(it, pcc.newStatus);
        if (!success && it->second->params.received) {
            // Situation: Programming failed and there is no outstanding sleepdata request.
            // Action: Drop the state for this adapter.
            delete it->second;
            adapterstate.erase(it);
        }
    }
    else {
        LOG_INFO("Unknown asyncId "FMTu64, asyncId);
    }
}

bool IoacAdapterStateDB::getGuidByAsyncId(u64 asyncId, GUID &guid)
{
    MutexAutoLock lock(&mutex);
    bool result = false;
    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it = findAdapterByAsyncId(asyncId);
    if (it != adapterstate.end()) {
        guid = it->first;
        result = true;
    }
    return result;
}

void IoacAdapterStateDB::warnSleepNotSupported(bool notSupported)
{
    MutexAutoLock lock(&mutex);
    overallStatus.set_warn_sleep_not_supported(notSupported);
}

void IoacAdapterStateDB::getAsyncIdListToProgram(std::vector<u64>& asyncIdList)
{
    MutexAutoLock lock(&mutex);
    std::map<GUID, IoacAdapterStateDB::AdapterState*, GUIDCompare>::iterator it;
    asyncIdList.clear();
    for (it = adapterstate.begin(); it != adapterstate.end(); it++) {
        if (it->second->params.received) {
            asyncIdList.push_back(it->second->params.asyncId);
        }
    }
}

//------------------------------

/// Will cast arg to a PingTestContext* and delete it before exiting.
static VPLTHREAD_FN_DECL udpPingTestThreadFn(void* arg)
{
    int numSent = 0;
    int numSucceeded = 0;

    // This thread is responsible for freeing the PingTestContext; use auto_ptr to ensure that it happens.
    std::auto_ptr<PingTestContext> pingContext(static_cast<PingTestContext*>(arg));
    ioacAdapterStateDB.reportPingTestStarted(pingContext->asyncId);

    VPLSocket_t udpSock = VPLSocket_CreateUdp(pingContext->localIpAddr, 0);
    if (VPLSocket_Equal(udpSock, VPLSOCKET_INVALID)) {
        LOG_ERROR("Failed to create UDP socket");
        goto done;
    }
    LOG_INFO("Attempting ANS ping (asyncId="FMTu64") from "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t" to "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t,
            pingContext->asyncId,
            VAL_VPLNet_addr_t(pingContext->localIpAddr), VPLNet_port_ntoh(VPLSocket_GetPort(udpSock)),
            VAL_VPLNet_addr_t(pingContext->destSockAddr.addr), VPLNet_port_ntoh(pingContext->destSockAddr.port));

    // TODO: make into config variables
#define MAX_NUM_PINGS 4
#define MIN_NUM_PINGS 1

    for (int i = 0; i < MAX_NUM_PINGS; i++) {
        // End now if pingContext->asyncId is no longer relevant.
        if (!ioacAdapterStateDB.asyncIdIsValid(pingContext->asyncId)) {
            LOG_INFO("Canceling obsolete ping test ("FMTu64")", pingContext->asyncId);
            goto obsolete;
        }

        LOG_INFO("Sending ANS ping i=%d (asyncId="FMTu64")", i, pingContext->asyncId);
        char buf[10] = "ping";
        *((u32*)&buf[4]) = VPLConv_hton_u32(TRUNCATE_TO_U32(pingContext->asyncId));
        *((u16*)&buf[8]) = VPLConv_hton_u16(TRUNCATE_TO_U16(i));
        VPLSocket_addr_t localSockAddr = { VPL_PF_INET, pingContext->localIpAddr, 0 };
        int rv = VPLSocket_SendTo(udpSock, buf, sizeof(buf), &pingContext->destSockAddr, sizeof(pingContext->destSockAddr));
        if (rv != sizeof(buf)) {
            LOG_ERROR("Failed to send ANS ping packet: %d", rv);
            continue;
        }
        numSent++;
        VPLSocket_poll_t socketList;
        socketList.socket = udpSock;
        socketList.events = VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_RDPRI;
        VPLTime_t timeout = VPLTime_FromSec(5);
        rv = VPLSocket_Poll(&socketList, 1, timeout);
        if (rv == 1) {
            char buf2[11];
            VPLSocket_addr_t senderAddr;
            rv = VPLSocket_RecvFrom(udpSock, buf2, sizeof(buf2), &senderAddr, sizeof(senderAddr));
            if (rv < 0) {
                LOG_WARN("VPLSocket_RecvFrom returned %d", rv);
            } else {
                bool correctSize = (rv == sizeof(buf));
                LOG_INFO("Response from "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t", size=%d (%scorrect)",
                        VAL_VPLNet_addr_t(senderAddr.addr), VPLNet_port_ntoh(senderAddr.port),
                        rv, (correctSize ? "" : "in"));
                if (correctSize){
                    u32 recvAsyncId32 = VPLConv_ntoh_u32(*((u32*)&buf2[4]));
                    u32 recvCount = VPLConv_ntoh_u16(*((u16*)&buf2[8]));
                    bool correctNums = ((recvAsyncId32 == TRUNCATE_TO_U32(pingContext->asyncId)) && (recvCount == i));
                    LOG_INFO("asyncId32="FMTu32", count="FMTu16", (%scorrect)",
                            recvAsyncId32, recvCount, correctNums ? "" : "in");
                    if (correctNums) {
                        numSucceeded++;
                        if (numSucceeded >= MIN_NUM_PINGS) {
                            break;
                        } else {
                            // Report partial progress.
                            ioacAdapterStateDB.reportPingTestProgress(pingContext->asyncId, numSent, numSucceeded, false);
                        }
                    }
                }
            }
        } else if (rv == 0) {
            LOG_INFO("UDP ping timed-out");
        } else {
            LOG_WARN("VPLSocket_Poll returned %d", rv);
        }
    }
 done:
    // Report the final result.
    ioacAdapterStateDB.reportPingTestProgress(pingContext->asyncId, numSent, numSucceeded, true);
 obsolete:
    return VPLTHREAD_RETURN_VALUE;
}

static bool startUdpPingTest(u64 asyncId, VPLNet_addr_t localIpAddr, const VPLSocket_addr_t* destSockAddr)
{
    LOG_INFO("Starting ANS ping test ("FMTu64") for "FMT_VPLNet_addr_t,
             asyncId, VAL_VPLNet_addr_t(localIpAddr));
    PingTestContext* pingContext = new PingTestContext();
    pingContext->destSockAddr = *destSockAddr;
    pingContext->localIpAddr = localIpAddr;
    pingContext->asyncId = asyncId;
    // Upon success, udpPingTestThreadFn becomes responsible for freeing pingContext.
    int rv = Util_SpawnThread(udpPingTestThreadFn, pingContext, UTIL_DEFAULT_THREAD_STACK_SIZE, VPL_FALSE, NULL);
    if (rv != 0) {
        LOG_ERROR("Failed to create ANS ping thread: %d", rv);
        delete pingContext;
        return false;
    }
    return true;
}

//-------------------------------------

#if 0

// per server data;

struct _svr {
    int sidx;            // server index;
    char *name;            // name of server;
    struct hostent *he;
    char *hostip;        // host ip string;
    SOCKET s[2];        // tcp/udp sockets;
    SOCKADDR_IN sa[2];
    SOCKADDR_IN bsa;    // bound interface address;
    SOCKADDR_IN srcv;
    UCHAR ka[88];        // keepalive packet;
    UCHAR ext[6];        // magic packet extension;
    UCHAR mac[6];        // wifi mac;
    CHAR macstr[20];    // wifi mac string;
    SOCKADDR_IN fwd;    // dst/gw ip address;
    UCHAR fwdmac[6];    // dst/gw mac address;
    CHAR cmd[80];        // send buffer;
    CHAR rep[200];        // reply buffer;
    int rsz;            // size of reply;
};

struct _g {
    IN_ADDR ifa[2];            // wired/wlan interface address;
    int sidx;                // server index;
    int sleep;                // do sleep/wake;
};


// get wow pm parameters;

char *
unusedWowPmParams(g_t *g, DWORD *err)
{
    CUSTOMER_RETURN_CODE rc;
    DWORD br;
    NDIS630_PM_PARAMETERS pm_params;
    ULONG x, mask;
    int enabled;

    enabled = 1;

    // get wakeup reason;

    pm_params.Header.Type = NDIS_HEADER_TYPE;
    pm_params.Header.Revision = NDIS_PM_PARAMETERS_REVISION;
    pm_params.Header.Size = sizeof(pm_params);

    printf("  NDIS_PM_PARAMETERS.Header.Revision %d\n",
        pm_params.Header.Revision);
    printf("  NDIS_PM_PARAMETERS.Header.Size %d\n",
        pm_params.Header.Size);

    br = 0;
    rc = CustomerControlDll(IOCTL_CUSTOMER_NDIS_QUERY_PM_PARAMETERS,
        &pm_params, sizeof(pm_params),
        &pm_params, sizeof(pm_params), &br);

    if(CustomerError(rc, err))
        return("cannot get ndis pm parameters");

    // report enabled patterns;

    x = pm_params.EnabledWoLPacketPatterns;
    printf("   enabled packet patterns 0x%x\n", x);
    printf("    bitmap pattern %sabled\n",
        (x & NDIS_PM_WOL_BITMAP_PATTERN_ENABLED)? "en" : "dis");
    printf("    magic packet %sabled\n",
        (x & NDIS_PM_WOL_MAGIC_PACKET_ENABLED)? "en" : "dis");
    printf("    eapol request id %sabled\n",
        (x & NDIS_PM_WOL_EAPOL_REQUEST_ID_MESSAGE_ENABLED)? "en" : "dis");
    printf("    ipv4 tcp syn %sabled\n",
        (x & NDIS_PM_WOL_IPV4_TCP_SYN_ENABLED)? "en" : "dis");
    printf("    ipv6 tcp syn %sabled\n",
        (x & NDIS_PM_WOL_IPV6_TCP_SYN_ENABLED)? "en" : "dis");
    printf("    ipv4 destination addr wildcard %sabled\n",
        (x & NDIS_PM_WOL_IPV4_DEST_ADDR_WILDCARD_ENABLED)? "en" : "dis");
    printf("    ipv6 destination addr wildcard %sabled\n",
        (x & NDIS_PM_WOL_IPV6_DEST_ADDR_WILDCARD_ENABLED)? "en" : "dis");
    if( !(x & NDIS_PM_WOL_MAGIC_PACKET_ENABLED))
        enabled = 0;

    // report enabled offloads;

    x = pm_params.EnabledProtocolOffloads;
    printf("   enabled protocol offloads 0x%x\n", x);
    printf("    arp offload %sabled\n",
        (x & NDIS_PM_PROTOCOL_OFFLOAD_ARP_ENABLED)? "en" : "dis");
    printf("    ns offload %sabled\n",
        (x & NDIS_PM_PROTOCOL_OFFLOAD_NS_ENABLED)? "en" : "dis");
    printf("    gtk refresh offload %sabled\n",
        (x & NDIS_PM_PROTOCOL_OFFLOAD_80211_RSN_REKEY_ENABLED)? "en" : "dis");
    mask = NDIS_PM_PROTOCOL_OFFLOAD_ARP_ENABLED
        | NDIS_PM_PROTOCOL_OFFLOAD_NS_ENABLED
        | NDIS_PM_PROTOCOL_OFFLOAD_80211_RSN_REKEY_ENABLED;
    if((x & mask) != mask)
        enabled = 0;

    // report wakeup flags;

    x = pm_params.WakeUpFlags;
    printf("   wakeup flags 0x%x\n", x);
    printf("    wake on link change %sabled\n",
        (x & NDIS_PM_WAKE_ON_LINK_CHANGE_ENABLED)? "en" : "dis");
    printf("    wake on media disconnect %sabled\n",
        (x & NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_ENABLED)? "en" : "dis");
    printf("    wake on selective suspend %sabled\n",
        (x & NDIS_PM_SELECTIVE_SUSPEND_ENABLED)? "en" : "dis");

    // report media-specific flags;

    x = pm_params.MediaSpecificWakeUpEvents;
    printf("   media-specific wake events 0x%x\n", x);
    printf("    wake on nlo discovery %sabled\n",
        (x & NDIS_WLAN_WAKE_ON_NLO_DISCOVERY_ENABLED)? "en" : "dis");
    printf("    wake on ap lost %sabled\n",
        (x & NDIS_WLAN_WAKE_ON_AP_ASSOCIATION_LOST_ENABLED)? "en" : "dis");
    printf("    wake on gtk error %sabled\n",
        (x & NDIS_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR_ENABLED)? "en" : "dis");
    printf("    wake on 4way handshake %sabled\n",
        (x & NDIS_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST_ENABLED)? "en" : "dis");
    mask = NDIS_WLAN_WAKE_ON_AP_ASSOCIATION_LOST_ENABLED
#ifdef REQUIRE_NLO
        | NDIS_WLAN_WAKE_ON_NLO_DISCOVERY_ENABLED
#endif // REQUIRE_NLO
        | NDIS_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR_ENABLED
        | NDIS_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST_ENABLED;
    if((x & mask) != mask)
        enabled = 0;

    if( !enabled)
        return("required ndis pm parameters not enabled");

    return(NULL);
}

void unusedGetSystemPowerStatus()
{
    SYSTEM_POWER_STATUS pwrst;
    if( !GetSystemPowerStatus(&pwrst)) {
        printf("cannot get power state\n");
        return;
    }
    printf(" Power state\n");
    if(pwrst.ACLineStatus)
        printf("  AC\n");
    if(pwrst.BatteryFlag & 128)
        printf("  no battery\n");
    else {
        if(pwrst.BatteryLifePercent <= 100)
            printf("  battery %d%%\n", pwrst.BatteryLifePercent);
        if(pwrst.BatteryFlag & 8)
            printf("  battery charging\n");
        if(pwrst.BatteryFlag & 4)
            printf("  battery critical!\n");
        else if(pwrst.BatteryFlag & 2)
            printf("  battery low!\n");
    }
}

/*
 * do one test;
 */
int
unusedTest(g_t *g)
{
    char *msg;
    int sidx, nka;
    svr_t *svr;
    DWORD err;
    int poff[5];

    printf(" Checking wow parameters\n");
    msg = WowPmParams(g, &err);
    if(msg) {
        printf("%s: %s: error 0x%x\n", g->cmd, msg, err);
        return(0);
    }

    {
        // sleep the system;
        // we cannot suspend if this fails;

        printf(" Going to sleep\n");
        if( !SetSuspendState(0, 0, TRUE)) {
            err = GetLastError();
            printf("%s: cannot sleep system: 0x%x\n", g->cmd, err);
            return(0);
        }

        // back from sleep;
        // try keep display from blanking;

        SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
        SetThreadExecutionState(ES_CONTINUOUS);

        // getting wake reason;

        printf(" Wakeup\n");
        msg = WowPmWakeReason(g, &err);
        if(msg) {
            printf("%s: %s: error 0x%x\n", g->cmd, msg, err);
            return(0);
        }
    }
}
#endif

#if EVEN_MORE_DEBUG_NETMAN
static const char*
NotificationSrcToStr(DWORD source, DWORD code, const char** codeStr_out)
{
    *codeStr_out = "???";
    switch(source) {
    case WLAN_NOTIFICATION_SOURCE_NONE:
        return("none");
    case WLAN_NOTIFICATION_SOURCE_ALL:
        return("all");
    case WLAN_NOTIFICATION_SOURCE_ACM: {
        static const char* acm_strings[] = { // first element is code==1
            "autoconf_enabled",
            "autoconf_disabled",
            "background_scan_enabled",
            "background_scan_disabled",
            "bss_type_change",
            "power_setting_change",
            "scan_complete",
            "scan_fail",
            "connection_start",
            "connection_complete",
            "connection_attempt_fail",
            "filter_list_change",
            "interface_arrival",
            "interface_removal",
            "profile_change",
            "profile_name_change",
            "profiles_exhausted",
            "network_not_available",
            "network_available",
            "disconnecting",
            "disconnected",
            "adhoc_network_state_change",
        };
        if ((code > 0) && (code <= ARRAY_ELEMENT_COUNT(acm_strings))) {
            *codeStr_out = acm_strings[code - 1];
        }
        return("autoconf");
    }
    case WLAN_NOTIFICATION_SOURCE_HNWK:
        return("hosted-network");
    case WLAN_NOTIFICATION_SOURCE_IHV:
        return("ihv");
    case WLAN_NOTIFICATION_SOURCE_ONEX:
        return("802.1x");
    case WLAN_NOTIFICATION_SOURCE_MSM: {
        static const char* msm_strings[] = { // first element is code==1
            "associating",
            "associated",
            "authenticating",
            "connected",
            "roaming_start",
            "roaming_end",
            "radio_state_change",
            "signal_quality_change",
            "disassociating",
            "disconnected",
            "peer_join",
            "peer_leave",
            "adapter_removal",
            "adapter_operation_mode_change",
        };
        if ((code > 0) && (code <= ARRAY_ELEMENT_COUNT(msm_strings))) {
            *codeStr_out = msm_strings[code - 1];
        }
        return("media-specific");
    }
    case WLAN_NOTIFICATION_SOURCE_SECURITY:
        return("security");
    }
    return("unknown");
}
#else
static const char*
NotificationSrcToStr(DWORD source)
{
    switch(source) {
    case WLAN_NOTIFICATION_SOURCE_NONE:
        return("none");
    case WLAN_NOTIFICATION_SOURCE_ALL:
        return("all");
    case WLAN_NOTIFICATION_SOURCE_ACM:
        return("autoconf");
    case WLAN_NOTIFICATION_SOURCE_HNWK:
        return("hosted-network");
    case WLAN_NOTIFICATION_SOURCE_IHV:
        return("ihv");
    case WLAN_NOTIFICATION_SOURCE_ONEX:
        return("802.1x");
    case WLAN_NOTIFICATION_SOURCE_MSM:
        return("msm");
    case WLAN_NOTIFICATION_SOURCE_SECURITY:
        return("security");
    }
    return("unknown");
}
#endif

// TODO: It's not yet clear which thread calls this callback; determine if we can safely access the global state from in here.
//       It looks like a completely new thread!
/// A #WLAN_NOTIFICATION_CALLBACK.
VOID WINAPI
WlanNotifyCb(WLAN_NOTIFICATION_DATA *data, VOID *ctx)
{
#if EVEN_MORE_DEBUG_NETMAN
    const char* codeStr;
    const char* sourceStr = NotificationSrcToStr(data->NotificationSource, data->NotificationCode, &codeStr);
    LOG_INFO("WLAN notification: source=%s, code="FMT_DWORD" (%s)",
            sourceStr, data->NotificationCode, codeStr);
#else
    // FIXME - too chatty
#if be_chatty
    LOG_INFO("WLAN notification: source=%s, code="FMT_DWORD,
            NotificationSrcToStr(data->NotificationSource), data->NotificationCode);
#endif
#endif
    switch(data->NotificationSource) {
    case WLAN_NOTIFICATION_SOURCE_ACM: //
        // Auto configuration module notification.
        // NotificationCode is #WLAN_NOTIFICATION_ACM.
        break;
    case WLAN_NOTIFICATION_SOURCE_HNWK:
        // Wireless Hosted Network notification (supported on Windows 7 and on Windows Server 2008 R2 with the Wireless LAN Service installed).
        // NotificationCode is #WLAN_HOSTED_NETWORK_NOTIFICATION_CODE.
        break;
    case WLAN_NOTIFICATION_SOURCE_IHV:
        // Independent hardware vendor (IHV) notification.
        // NotificationCode is IHV-specific.
        break;
    case WLAN_NOTIFICATION_SOURCE_ONEX:
        // 802.1X module notification.
        // NotificationCode is #ONEX_NOTIFICATION_TYPE.
        break;
    case WLAN_NOTIFICATION_SOURCE_MSM:
        // Media specific module (MSM) notification.
        // NotificationCode is #WLAN_NOTIFICATION_MSM.
        break;
    case WLAN_NOTIFICATION_SOURCE_SECURITY:
        // Security notification.
        // No notifications are currently defined for WLAN_NOTIFICATION_SOURCE_SECURITY.
        break;
    default:
        break;
    }
}

s32
NetMan_SetWakeOnWlanData(
        u64 asyncId,
        const void* wakeupKey,
        size_t wakeupKeyLen,
        const char* serverHostname,
        VPLNet_port_t serverPort,
        u32 packetIntervalSec,
        const void* payload,
        size_t payloadLen)
{
    bool set_to_ioac_adapters = false;

    if (!ioacWorker.isRunning()) {
        return CCD_ERROR_NOT_RUNNING;
    }
    if (wakeupKeyLen != WAKEUP_SECRET_SIZE) {
        LOG_ERROR("wakeupKeyLen was "FMTu_size_t, wakeupKeyLen);
        return CCD_ERROR_PARAMETER;
    }
    if (payloadLen > (ATH_CUSTOMER_KEEPALIVE_MAX_PACKET_SIZE - ALL_HDRS_LEN)) {
        LOG_ERROR("payload too large: "FMTu_size_t, payloadLen);
        return CCD_ERROR_PARAMETER;
    }

    IoacSleepSetup sleepSetup;
    sleepSetup.asyncId = asyncId;
    sleepSetup.wakeupKey.assign((const char*)wakeupKey, wakeupKeyLen);
    sleepSetup.serverHostname.assign(serverHostname);
    sleepSetup.serverPort = serverPort;
    sleepSetup.packetIntervalSec = packetIntervalSec;
    sleepSetup.payload.assign((const char*)payload, payloadLen);

    if (ioacAdapterStateDB.reportSleepSetup(asyncId, sleepSetup)) {
        int rv;
        u64 current_device_id;
        bool enabled = FALSE;

        // get current device_id
        rv = ESCore_GetDeviceGuid(&current_device_id);
        if (rv < 0) {
            LOG_WARN("ESCore_GetDeviceGuid failed: %d", rv);
            set_to_ioac_adapters = true;
            goto end;
        }

        // To check current device_id with enabled_ioac_device_id.
        rv = NetMan_IsUserEnabledIOAC(current_device_id, enabled);
        if (rv == IOAC_UNAVAILABLE_GLOBAL_ACCESSDATA_PATH) {
            LOG_WARN("Didn't set globalAccessDataPath to ccd, skip checking disk cache.");
            set_to_ioac_adapters = true;
            goto end;
        }else if (rv < 0) {
            LOG_WARN("NetMan_IsUserEnabledIOAC failed: %d", rv);
            set_to_ioac_adapters = true;
            goto end;
        }

        set_to_ioac_adapters = enabled;
    }

end:
    if (set_to_ioac_adapters) {
        allowOsSleep(false);
        wTaskQ.add(WCMD_SCAN);
        wTaskQ.add(WCMD_PROGRAM, asyncId);
    } else {
        LOG_INFO("Current user didn't enable IOAC, skip to configure to IOAC adapters. (asyncId = %llu)", asyncId);
        ioacAdapterStateDB.setOverallStatus(ccd::IOAC_STATUS_SUMMARY_INACTIVE);
    }

    return VPL_OK;
}

s32 NetMan_ClearWakeOnWlanData()
{
    if (!ioacWorker.isRunning()) {
        return CCD_ERROR_NOT_RUNNING;
    }

    wTaskQ.add(WCMD_UNPROGRAM_ALL);
    allowOsSleep(false);

    return VPL_OK;
}

void NetMan_CheckAndClearCache()
{
    int rv;
    u64 current_device_id;
    bool enabled = FALSE;

    // get current device_id
    rv = ESCore_GetDeviceGuid(&current_device_id);
    if (rv < 0) {
        LOG_WARN("ESCore_GetDeviceGuid failed: %d", rv);
        goto end;
    }

    // To check current device_id with enabled_ioac_device_id.
    rv = NetMan_IsUserEnabledIOAC(current_device_id, enabled);
    if (rv == IOAC_UNAVAILABLE_GLOBAL_ACCESSDATA_PATH) {
        LOG_WARN("Didn't set globalAccessDataPath to ccd, skip to delete disk cache.");
        goto end;

    } else if (rv < 0) {
        LOG_WARN("NetMan_IsUserEnabledIOAC failed: %d", rv);
        goto end;
    }

    if (enabled) {
        LOG_INFO("To clear the disk cache.");
        rv = DiskCache_DeleteData(OBJECT_ID_ENABLED_IOAC_DEVICE_ID);
        if (rv != VPL_OK) {
            LOG_ERROR("DiskCache_DeleteData failed: %d", rv);
        }
    }
end:
    return;
}

/// Connect to the WLAN service if needed.
static void connectWlanService(NetMan_Wlan_t& wlan)
{
    if (wlan.hWlan != INVALID_HANDLE_VALUE) {
        // Nothing to do.
        goto skip_wlan;
    }

    // Open a connection to the wlan service:
    {
        ASSERT_EQUAL(wlan.hWlan, INVALID_HANDLE_VALUE, FMT0xPTR);
        const DWORD REQUESTED_WLAN_VERSION = WLAN_API_VERSION_2_0;
        DWORD err = WlanOpenHandle(REQUESTED_WLAN_VERSION, NULL, &wlan.negotiatedVer, &wlan.hWlan);
        if (err != ERROR_SUCCESS) {
            // This probably happens when there are no wireless cards on the machine.
            if (err == ERROR_SERVICE_NOT_ACTIVE) {
                LOG_INFO("Wireless service has not been started (no WiFi cards available)");
            } else {
                LOG_ERROR("%s failed: %d", "WlanOpenHandle", VPLError_XlatWinErrno(err));
            }
            goto skip_wlan;
        }
        if (wlan.negotiatedVer != REQUESTED_WLAN_VERSION) {
            LOG_WARN("Wanted Wlan version "FMT_DWORD", but got "FMT_DWORD,
                    REQUESTED_WLAN_VERSION, wlan.negotiatedVer);
        }
    }

    // Register for WLAN notification callbacks:
    {
        // Note: "Do not call WlanRegisterNotification from the DllMain function in an application DLL. This could cause a deadlock."
        DWORD err = WlanRegisterNotification(wlan.hWlan,
            WLAN_NOTIFICATION_SOURCE_ALL,
            TRUE, // If set to TRUE, a notification will not be sent to the client if it is identical to the previous one.
            WlanNotifyCb,
            NULL, // context (unused)
            NULL, // reserved
            NULL);
        if (err != ERROR_SUCCESS) {
            LOG_ERROR("%s failed: %d", "WlanRegisterNotification", VPLError_XlatWinErrno(err));
            goto skip_wlan;
        }
    }
skip_wlan:
    return;
}
static void disconnectWlanService(NetMan_Wlan_t& wlan)
{
    if (wlan.hWlan != INVALID_HANDLE_VALUE) {
        WlanCloseHandle(wlan.hWlan, NULL); // Also unregisters notifications
        wlan.hWlan = INVALID_HANDLE_VALUE;
    }
}

//----------------------------
// Should refactor to SleepManager and support multiple modules (streaming needs its own
// independent ability to disallow sleep).

static bool s_allowSleep = true;

static void allowOsSleep(bool allow)
{
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&s_mutex));
    if (allow != s_allowSleep) {
        bool success;
        if (allow) {
            // TODO: Call Windows API to allow.
            success = false;
        } else {
            // TODO: Call Windows API to disallow.
            success = false;
        }
        if (success) {
            s_allowSleep = allow;
        }
    }
}
//----------------------------

static void workerThreadLoop(NetMan_Wlan_t &wlan)
{
    std::map<GUID, WakeableNetAdapter, GUIDCompare> connectedIoacAdapters;
    while (1) {

        while (!wTaskQ.empty()) {
            WorkerTask task = wTaskQ.next();

            switch (task.cmd) {
            case WCMD_QUIT:
                LOG_INFO("QUIT");
                goto handle_quit;
                break;

            case WCMD_CHECK_WAKE_REASON:
                LOG_INFO("CHECK_WAKE_REASON");
                ::checkWakeReason(connectedIoacAdapters);
                break;

            case WCMD_UNPROGRAM_ALL: {
                LOG_INFO("UNPROGRAM_ALL");
                std::map<GUID, WakeableNetAdapter, GUIDCompare>::iterator it;
                for (it = connectedIoacAdapters.begin(); it != connectedIoacAdapters.end(); it++) {
                    disableSleepKeepAlives(it->second.ioacInfo);
                    disableWakeUpPattern(it->second.ioacInfo);
                }
                ioacAdapterStateDB.clear();
                ioacAdapterStateDB.setOverallStatus(ccd::IOAC_STATUS_SUMMARY_INACTIVE);
                break;
            }

            case WCMD_SCAN: {
                LOG_INFO("SCAN");
                // Attempt to connect to the WlanService if needed.
                connectWlanService(wlan);

                ioacAdapterStateDB.upgradeOverallStatus(ccd::IOAC_STATUS_SUMMARY_UPDATING);

                ccd::IoacOverallStatus newStatus;
                newStatus.set_summary(ccd::IOAC_STATUS_SUMMARY_FAIL_INTERNAL);
                scanIoacHardware(wlan.hWlan, newStatus, connectedIoacAdapters);
                if (connectedIoacAdapters.size() == 0) {
                    ioacAdapterStateDB.setOverallStatus(newStatus.summary());
                }

                ioacAdapterStateDB.reportIoacAdapters(connectedIoacAdapters);

                break;
            }

            case WCMD_REQUEST_SLEEP_SETUP: {
                LOG_INFO("REQUEST_SLEEP_SETUP");
                std::map<GUID, WakeableNetAdapter, GUIDCompare>::iterator it;
                for (it = connectedIoacAdapters.begin(); it != connectedIoacAdapters.end(); it++) {
                    u64 asyncId = ioacWorker.getNextAsyncId();
                    ioacAdapterStateDB.prepareNextSleepSetup(it->first, asyncId);
                    ccd::IoacAdapterStatus adapterStatus;
                    adapterStatus.set_summary(ccd::IOAC_STATUS_SUMMARY_FAIL_NO_HARDWARE);
                    requestSleepSetup(it->second, asyncId, adapterStatus);
                    ioacAdapterStateDB.reportAdapterStatus(asyncId, adapterStatus);
                }
                break;
            }

            case WCMD_PROGRAM: {
                u64 asyncId = task.arg.u64;
                LOG_INFO("PROGRAM (asyncId="FMTu64")", asyncId);

                ProgramChipContext pcc;
                if (!ioacAdapterStateDB.populatePCContext(asyncId, pcc)) {
                    LOG_INFO("Unknown asyncId "FMTu64, asyncId);
                    break;
                }
                pcc.newStatus.Clear();
                pcc.newStatus.set_summary(ccd::IOAC_STATUS_SUMMARY_FAIL_INTERNAL);

                GUID guid;
                if (!ioacAdapterStateDB.getGuidByAsyncId(asyncId, guid)) {
                    LOG_INFO("Unknown asyncId "FMTu64, asyncId);
                    break;
                }
                std::map<GUID, WakeableNetAdapter, GUIDCompare>::iterator it = connectedIoacAdapters.find(guid);
                if (it == connectedIoacAdapters.end()) {
                    LOG_INFO("Unknown GUID %s", WinGuidToStr(guid));
                    break;
                }
                bool progOk = programIoacHardware(it->second, pcc);
                ioacAdapterStateDB.reportChipProgramResult(asyncId, pcc, progOk);
                break;
            }

            case WCMD_CHECK_FOR_ENABLE_IOAC: {
                u64 original_device_id = task.arg.u64;
                ccd::IoacOverallStatus status;
                bool failed_to_enable_ioac = true;
                LOG_INFO("CHECK_FOR_ENABLE_IOAC (original_device_id="FMTu64")", original_device_id);

                status = ioacAdapterStateDB.getOverallStatus();
                for (int i=0; i<status.adapters_size(); i++) {
                    if (status.summary() == ccd::IOAC_STATUS_SUMMARY_TESTING || status.summary() == ccd::IOAC_STATUS_SUMMARY_GOOD) {
                        failed_to_enable_ioac = false;
                        break;
                    }
                }

                if (failed_to_enable_ioac) {
                    int rv;
                    if (original_device_id == -1) {
                        LOG_INFO("Rollback the original value of enabled_ioac_device_id. (NULL)");

                        // Delete the enable_ioac_device_id from disk cache.
                        rv = DiskCache_DeleteData(OBJECT_ID_ENABLED_IOAC_DEVICE_ID);
                        if (rv != VPL_OK) {
                            LOG_ERROR("DiskCache_DeleteData failed: %d", rv);
                        }
                    } else {
                        char str_original_device_id[64];
                        snprintf(str_original_device_id, 64, "%llu", original_device_id);
                        LOG_INFO("Rollback the original value of enabled_ioac_device_id. (%llu)", original_device_id);

                        // Set the original value to enable_ioac_device_id.
                        rv = DiskCache_SetData(OBJECT_ID_ENABLED_IOAC_DEVICE_ID, str_original_device_id);
                        if (rv != VPL_OK) {
                            LOG_ERROR("DiskCache_SetData failed: %d", rv);
                        }
                    }
                    ioacAdapterStateDB.setOverallStatus(ccd::IOAC_STATUS_SUMMARY_INACTIVE);
                }

                break;
            }

            case WCMD_DISABLE_IOAC: {
                LOG_INFO("WCMD_DISABLE_IOAC");
                std::map<GUID, WakeableNetAdapter, GUIDCompare>::iterator it;
                for (it = connectedIoacAdapters.begin(); it != connectedIoacAdapters.end(); it++) {
                    disableSleepKeepAlives(it->second.ioacInfo);
                    disableWakeUpPattern(it->second.ioacInfo);
                }
                ioacAdapterStateDB.setOverallStatus(ccd::IOAC_STATUS_SUMMARY_INACTIVE);
            }
            } // switch
        } // while (!wTaskQ.empty())

        allowOsSleep(true);
        //LOG_DEBUG("Waiting for work");
        LOG_INFO("Waiting for work");
        wTaskQ.wait();
    } // while (1)

 handle_quit:
    LOG_INFO("Worker thread exiting");
}

static VPLTHREAD_FN_DECL workerThreadFn(void* arg)
{
    NetMan_Wlan_t wlan;

    connectWlanService(wlan);

    ComInitGuard comInit;
    HRESULT hr = comInit.init(COINIT_MULTITHREADED);
    if (SUCCEEDED(hr) || (hr == RPC_E_CHANGED_MODE)) {
        CComPtr<INetworkListManager> pNLM;
        hr = CoCreateInstance(CLSID_NetworkListManager,
                NULL, CLSCTX_ALL, __uuidof(INetworkListManager), (LPVOID*)&pNLM);
        if (hr != S_OK) {
            LOG_ERROR("CoCreateInstance(CLSID_NetworkListManager) failed: "FMT_HRESULT, hr);
        } else {
            NlmEventSink* pEventSink1;
            hr = NlmEventSink::StartListeningForEvents(IID_INetworkConnectionEvents, &pEventSink1);
            if (SUCCEEDED(hr)) {
                NlmEventSink* pEventSink2;
                hr = NlmEventSink::StartListeningForEvents(IID_INetworkEvents, &pEventSink2);
                if (SUCCEEDED(hr)) {
                    NlmEventSink* pEventSink3;
                    hr = NlmEventSink::StartListeningForEvents(IID_INetworkListManagerEvents, &pEventSink3);
                    if (SUCCEEDED(hr)) {
                        LOG_INFO("Ready to listen for NLM events.");

                        workerThreadLoop(wlan);

                        LOG_INFO("Done listening for NLM events.");
                        pEventSink3->StopListeningForEvents();
                        pEventSink3->Release();
                    }
                    pEventSink2->StopListeningForEvents();
                    pEventSink2->Release();
                }
                pEventSink1->StopListeningForEvents();
                pEventSink1->Release();
            }
        }
    } else {
        LOG_ERROR("Failed to init COM: "FMT_HRESULT, hr);
    }

    disconnectWlanService(wlan);

    return VPLTHREAD_RETURN_VALUE;
}

s32 NetMan_SetGlobalAccessDataPath(const char* m_globalAccessDataPath)
{
    hasGlobalAccessDataPath = true;
    globalAccessDataPath.assign(m_globalAccessDataPath);
    return VPL_OK;
}

s32 NetMan_Start()
{
    LOG_INFO("Set the path to access disk cache to \"%s\"", globalAccessDataPath.c_str());
    if (ioacWorker.isRunning()) {
        return CCD_ERROR_ALREADY;
    }

    ioacAdapterStateDB.clear();

    return ioacWorker.spawnThread();
}

s32 NetMan_Stop()
{
    s32 rv;

    if (!ioacWorker.isRunning()) {
        return CCD_ERROR_ALREADY;
    }

    wTaskQ.add(WCMD_UNPROGRAM_ALL);
    wTaskQ.add(WCMD_QUIT);

    LOG_INFO("Waiting for worker thread");
    rv = ioacWorker.joinThread();
    if (rv != 0) {
        goto out;
    }
    ioacAdapterStateDB.clear();
    allowOsSleep(true);
out:
    return rv;
}

ccd::IoacOverallStatus NetMan_GetIoacOverallStatus()
{
    return ioacAdapterStateDB.getOverallStatus();
}

void NetMan_NetworkChange()
{
    allowOsSleep(false);
    ioacAdapterStateDB.clear();
    ioacAdapterStateDB.upgradeOverallStatus(ccd::IOAC_STATUS_SUMMARY_UPDATING);
    wTaskQ.add(WCMD_SCAN);
    wTaskQ.add(WCMD_REQUEST_SLEEP_SETUP);
}

void NetMan_CheckWakeReason()
{
    wTaskQ.add(WCMD_CHECK_WAKE_REASON);
}

s32 NetMan_EnableIOAC(u64 device_id, bool enable)
{
    int rv = VPL_OK;
    bool has_enabled_ioac_device_id = false;
    u64 enabled_ioac_device_id = -1;

    // To check the path of disk cache.
    if (!hasGlobalAccessDataPath) {
        return IOAC_UNAVAILABLE_GLOBAL_ACCESSDATA_PATH;
    }

    // To check there is enabled_ioac_device_id in disk cache.
    rv = DiskCache_HasData(OBJECT_ID_ENABLED_IOAC_DEVICE_ID, has_enabled_ioac_device_id);
    if (rv != VPL_OK) {
        LOG_ERROR("Failed to get data from disk cache, key = %s, rv = %d", OBJECT_ID_ENABLED_IOAC_DEVICE_ID, rv);
        return rv;
    }

    // Get enabled_ioac_device_id from  disk cache.
    if (has_enabled_ioac_device_id) {
        std::string str_enabled_ioac_device_id;
        rv = DiskCache_GetData(OBJECT_ID_ENABLED_IOAC_DEVICE_ID, str_enabled_ioac_device_id);
        if (rv != VPL_OK) {
            LOG_ERROR("Failed to get data from disk cache, key = %s, rv = %d", OBJECT_ID_ENABLED_IOAC_DEVICE_ID, rv);
            return rv;
        }

        sscanf(str_enabled_ioac_device_id.c_str(), "%llu", &enabled_ioac_device_id);
    }

    // To check the IOAC function is occupied by other user.
    if (has_enabled_ioac_device_id && device_id != enabled_ioac_device_id) {
        LOG_ERROR("IOAC is already in use, current device_id: %llu  enabled_ioac_device_id: %llu", device_id, enabled_ioac_device_id);
        return IOAC_FUNCTION_ALREADY_IN_USE;
    }

    // To check if there is ioac capable adapters.
    {
        ccd::IoacOverallStatus status = ioacAdapterStateDB.getOverallStatus();
        if (status.adapters_size() == 0 && status.summary() != ccd::IOAC_STATUS_SUMMARY_UPDATING) {
            LOG_ERROR("Can not find any ioac capable adapter, and it is not updating (status=%d)", status.summary());
            return IOAC_NO_HARDWARE;
        }
    }

    if (enable) {
        char str_device_id[64];
        std::vector<u64> ioacAdapterAsyncIdList;

        // Set current device_id to enabled_ioad_device_id in disk cache.
        snprintf(str_device_id, 64, "%llu", device_id);
        rv = DiskCache_SetData(OBJECT_ID_ENABLED_IOAC_DEVICE_ID, str_device_id);
        if (rv != VPL_OK) {
            LOG_ERROR("DiskCache_SetData failed: %d", rv);
            return rv;
        }
        LOG_INFO("Set enabled ioac device_id to: %llu (original value: %llu) %s", device_id, enabled_ioac_device_id, str_device_id);

        // Get all asyncIds of ioac adapters, and add WCMD_PROGRAM and WCMD_CHECK_FOR_ENABLE_IOAC to task queue.
        ioacAdapterStateDB.getAsyncIdListToProgram(ioacAdapterAsyncIdList);
        for (u32 i=0; i<ioacAdapterAsyncIdList.size(); i++) {
            wTaskQ.add(WCMD_PROGRAM, ioacAdapterAsyncIdList[i]);
        }

        wTaskQ.add(WCMD_CHECK_FOR_ENABLE_IOAC, enabled_ioac_device_id);

    } else {
        // If there is enabled_ioad_device_id in disk cache, delete it.
        if (has_enabled_ioac_device_id) {
            rv = DiskCache_DeleteData(OBJECT_ID_ENABLED_IOAC_DEVICE_ID);
            if (rv != VPL_OK) {
                LOG_ERROR("DiskCache_DeleteData failed: %d", rv);
            }
            LOG_INFO("Delete enabled ioac device_id in disk cache. (original value: %llu)", enabled_ioac_device_id);
        }
        wTaskQ.add(WCMD_DISABLE_IOAC);
    }

    return rv;
}

s32 NetMan_IsIOACAlreadyInUse(bool& in_use)
{
    int rv = VPL_OK;
    bool has_enabled_ioac_device_id = FALSE;

    if (!hasGlobalAccessDataPath) {
        return IOAC_UNAVAILABLE_GLOBAL_ACCESSDATA_PATH;
    }

    rv = DiskCache_HasData(OBJECT_ID_ENABLED_IOAC_DEVICE_ID, has_enabled_ioac_device_id);
    if (rv != VPL_OK) {
        LOG_ERROR("Failed to get data from disk cache, key = %s, rv = %d", OBJECT_ID_ENABLED_IOAC_DEVICE_ID, rv);
        return rv;
    }

    in_use = has_enabled_ioac_device_id;
    return rv;
}

s32 NetMan_IsUserEnabledIOAC(u64 device_id, bool& enable)
{
    int rv = VPL_OK;
    bool has_enabled_ioac_device_id = FALSE;

    if (!hasGlobalAccessDataPath) {
        return IOAC_UNAVAILABLE_GLOBAL_ACCESSDATA_PATH;
    }

    rv = DiskCache_HasData(OBJECT_ID_ENABLED_IOAC_DEVICE_ID, has_enabled_ioac_device_id);
    if (rv != VPL_OK) {
        LOG_ERROR("Failed to get data from disk cache, key = %s, rv = %d", OBJECT_ID_ENABLED_IOAC_DEVICE_ID, rv);
        return rv;
    }

    if (has_enabled_ioac_device_id) {
        std::string str_enabled_device_id;
        u64 enabled_device_id;
        rv = DiskCache_GetData(OBJECT_ID_ENABLED_IOAC_DEVICE_ID, str_enabled_device_id);
        if (rv != VPL_OK) {
            LOG_ERROR("Failed to get data from disk cache, key = %s, rv = %d", OBJECT_ID_ENABLED_IOAC_DEVICE_ID, rv);
            return rv;
        }

        sscanf(str_enabled_device_id.c_str(), "%llu", &enabled_device_id);
        if (device_id == enabled_device_id) {
             enable = true;
        } else {
             enable = false;
        }
        LOG_INFO("%s IOAC: (current user device_id=%llu, enabled IOAC device_id=%llu", enable? "Enabled":"Didn't enable", device_id, enabled_device_id);
    } else {
        enable = false;
    }
    return VPL_OK;
}

#else // ! #if defined(_MSC_VER) && !defined(VPL_PLAT_IS_WINRT)
s32 NetMan_SetGlobalAccessDataPath(const char* globalAccessDataPath)
{
    return VPL_OK;
}

s32 NetMan_Start()
{
    return VPL_OK;
}

s32 NetMan_Stop()
{
    return VPL_OK;
}

s32 NetMan_SetWakeOnWlanData(
        u64 asyncId,
        const void* wakeupKey,
        size_t wakeupKeyLen,
        const char* serverHostname,
        VPLNet_port_t serverPort,
        u32 packetIntervalSec,
        const void* payload,
        size_t payloadLen)
{
    return VPL_OK;
}

s32 NetMan_ClearWakeOnWlanData()
{
    return VPL_OK;
}

void NetMan_CheckAndClearCache()
{
    // nothing to do
}

ccd::IoacOverallStatus NetMan_GetIoacOverallStatus()
{
    ccd::IoacOverallStatus result;
    result.set_summary(ccd::IOAC_STATUS_SUMMARY_FAIL_NO_HARDWARE);
    return result;
}

void NetMan_NetworkChange()
{
    // nothing to do
}

void NetMan_CheckWakeReason()
{
    // nothing to do
}

s32 NetMan_EnableIOAC(u64 device_id, bool enable)
{
    return VPL_OK;
}

s32 NetMan_IsIOACAlreadyInUse(bool& in_use)
{
    return VPL_OK;
}

s32 NetMan_IsUserEnabledIOAC(u64 device_id, bool& enable)
{
    return VPL_OK;
}

#endif // #if defined(_MSC_VER) && !defined(VPL_PLAT_IS_WINRT)
