#pragma once

#include <windows.h>
#include <wlanapi.h>
#include <tchar.h>
#include <strsafe.h>
#include "customer.h"

#define DEBUG_NDIS          0
#define DEBUG_NICENUM       0

#define MAX_ADAPTER_NUM     10
#define MAX_LEN             256

#define OFF_VID             8
#define OFF_DID             17
#define OFF_SVID            33
#define OFF_SSID            29

#define REALTEK             0
#define ATHEROS_WIFI        1
#define INTEL               2
#define BROADCOM_WIFI       3
#define ATHEROS_LAN         4
#define BROADCOM_LAN        5
#define OTHER_IOAC_ADAPTER  6

#define UNKNOWN             0
#define LAN                 1
#define WLAN                2


typedef struct {
    WCHAR vid[4];
    WCHAR tmp1[1];
    WCHAR did[4];
    WCHAR tmp2[1];
    WCHAR ssid[4];
    WCHAR tmp3[1];
    WCHAR svid[4];
    WCHAR tmp4[1];
} tPciID, *ptPciID;

typedef struct {
    WCHAR *vid;
    WCHAR *did;
    WCHAR *svid;
    WCHAR *ssid;
} tOpt, ptOpt;

typedef struct {
    CUSTOMER_CONTROL_SET    scCcs;
    WCHAR szDriverPath[MAX_LEN];
    WCHAR szDeviceInstanceID[MAX_LEN];
    WCHAR szNetCfgInstanceId[MAX_LEN];
    WCHAR szComponentId[MAX_LEN];
    char  szGuid[MAX_LEN];          /* for App use */
    char  szBioLAN;                 /* BIOS support Always connect on this interface */
    char  szBioWLAN;                /* BIOS support Always connect on this interface */
    char  szAc;                     /* system is connected to AC power */
    char  szOemType:4;              /* 0:rtk,1:ath,2:intel,3:bcom*/
    char  szNetType:4;              /* 0: Lan   , 1 : WLan      */
    char  szAsp;                    /* API test                 */
    char  szWde;                    /* Device Wake Up           */
    char  szWmp;                    /* Magic Packet Wake Up     */
    char  szCsu;                    /* Conection Status         */
    char  szLnf;                    /* Legacy Ndis Filter Drvier*/
    char  szWse[MAX_LEN/4];         /* WLAN Security Type       */
    char  szSsu;                    /* Server status            */
    DWORD dwPnpCap;
    DWORD dwGSmBios;

}tNetAdpaterInfo, *ptNetAdapterInfo;

/* should we need to support 6.30   ?*/
#define NDIS_SUPPORT_NDIS630                                    1

#define NDIS_HEADER_TYPE NDIS_OBJECT_TYPE_DEFAULT

#define NDIS_PM_WAKE_PACKET_INDICATION_SUPPORTED                0x00000001
#define NDIS_PM_SELECTIVE_SUSPEND_SUPPORTED                     0x00000002
#define NDIS_PM_WOL_PACKET_WILDCARD_SUPPORTED                   0x00100000
#define NDIS_WLAN_WAKE_ON_NLO_DISCOVERY_SUPPORTED               0x00000001
#define NDIS_WLAN_WAKE_ON_AP_ASSOCIATION_LOST_SUPPORTED         0x00000002
#define NDIS_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR_SUPPORTED         0x00000004
#define NDIS_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST_SUPPORTED      0x00000008
#define NDIS_PM_WAKE_ON_LINK_CHANGE_ENABLED                     0x00000001
#define NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_ENABLED                0x00000002
#define NDIS_PM_SELECTIVE_SUSPEND_ENABLED                       0x00000010
#define NDIS_WLAN_WAKE_ON_NLO_DISCOVERY_ENABLED                 0x00000001
#define NDIS_WLAN_WAKE_ON_AP_ASSOCIATION_LOST_ENABLED           0x00000002
#define NDIS_WLAN_WAKE_ON_GTK_HANDSHAKE_ERROR_ENABLED           0x00000004
#define NDIS_WLAN_WAKE_ON_4WAY_HANDSHAKE_REQUEST_ENABLED        0x00000008

#define NDIS_PM_CAPABILITIES_REVISION                           2
#define NDIS_PM_PARAMETERS_REVISION                             2

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

typedef enum _ExtendMagicPacketModule{
    QCAWB222 = 0,
    QCAMD222 = 1,
    BCM43228 = 2,
    RTL8111FA = 3
} ExtendMagicPacketModule;
