
#ifndef __CUSTOMER_SUN_H_
#define __CUSTOMER_SUN_H_

#ifdef __cplusplus
extern "C" {
#endif

//The structures for each OID is defined

typedef enum _CustomerReturnCode {
    CUSTOMER_CONTROL_OK = 0,
    CUSTOMER_CONTROL_FAILURE = -1,
    CUSTOMER_CONTROL_BAD_PARAMETER = -2,

}CUSTOMER_RETURN_CODE, *PCUSTOMER_RETURN_CODE;

#define  IEEE80211_ADDR_LEN                         6

// Keepalive buffer size is 80, but for Intel, it is 88.
#define  ATH_CUSTOMER_KEEPALIVE_MAX_PACKET_SIZE         80
#define  KEEPALIVE_EXTRA_BUFFER_FOR_INTEL                8

#define  CUSTOMER_CONTROL_CODE_BASE                     0xACE

// for keepalive control purposes
#define  IOCTL_CUSTOMER_KEEPALIVE_SET                         (CUSTOMER_CONTROL_CODE_BASE + 0x1)
#define  IOCTL_CUSTOMER_KEEPALIVE_QUERY                       (CUSTOMER_CONTROL_CODE_BASE + 0x2)

// for wakeup match control purposes
#define  IOCTL_CUSTOMER_WAKEUP_MATCH_SET                      (CUSTOMER_CONTROL_CODE_BASE + 0x3)
#define  IOCTL_CUSTOMER_WAKEUP_DISABLE                        (CUSTOMER_CONTROL_CODE_BASE + 0x4)
#define  IOCTL_CUSTOMER_WAKEUP_QUERY                          (CUSTOMER_CONTROL_CODE_BASE + 0x5)

// for NDIS API
#define IOCTL_CUSTOMER_NDIS_SET_PM_CAPABILITIES               (CUSTOMER_CONTROL_CODE_BASE + 0x6)
#define IOCTL_CUSTOMER_NDIS_SET_PM_COUNTED_STRING             (CUSTOMER_CONTROL_CODE_BASE + 0x7)
#define IOCTL_CUSTOMER_NDIS_SET_PM_PARAMETERS                 (CUSTOMER_CONTROL_CODE_BASE + 0x8)
#define IOCTL_CUSTOMER_NDIS_SET_PM_PROTOCOL_OFFLOAD           (CUSTOMER_CONTROL_CODE_BASE + 0x9)
#define IOCTL_CUSTOMER_NDIS_SET_PM_WOL_PATTERN                (CUSTOMER_CONTROL_CODE_BASE + 0xA)

#define IOCTL_CUSTOMER_NDIS_QUERY_PM_CAPABILITIES             (CUSTOMER_CONTROL_CODE_BASE + 0xB)
#define IOCTL_CUSTOMER_NDIS_QUERY_PM_COUNTED_STRING           (CUSTOMER_CONTROL_CODE_BASE + 0xC)
#define IOCTL_CUSTOMER_NDIS_QUERY_PM_PARAMETERS               (CUSTOMER_CONTROL_CODE_BASE + 0xD)
#define IOCTL_CUSTOMER_NDIS_QUERY_PM_PROTOCOL_OFFLOAD         (CUSTOMER_CONTROL_CODE_BASE + 0xE)
#define IOCTL_CUSTOMER_NDIS_QUERY_PM_WOL_PATTERN              (CUSTOMER_CONTROL_CODE_BASE + 0xF)
#define IOCTL_CUSTOMER_NDIS_QUERY_PM_WAKE_REASON              (CUSTOMER_CONTROL_CODE_BASE + 0x10)
#define IOCTL_CUSTOMER_FIXEDPATTERN_SET                       (CUSTOMER_CONTROL_CODE_BASE + 0x11)
#define IOCTL_CUSTOMER_FIXEDPATTERN_QUERY                     (CUSTOMER_CONTROL_CODE_BASE + 0x12)
#define IOCTL_CUSTOMER_TIMERWAKEUP_SET                        (CUSTOMER_CONTROL_CODE_BASE + 0x13)
#define IOCTL_CUSTOMER_WAKEPATTERNSUPPORT_QUERY               (CUSTOMER_CONTROL_CODE_BASE + 0x14)
#define IOCTL_CUSTOMER_IOACWAKEUP_QUERY                       (CUSTOMER_CONTROL_CODE_BASE + 0x20)
//#define IOCTL_CUSTOMER_OPEN_CTL_ADAPTER                       (CUSTOMER_CONTROL_CODE_BASE + 0x20)


//for NDIS wake reason type
//#define NdisWakeReasonAcerPacket  0x08
//#define NdisWakeReasonPacket      0x01

/*
   For simplicity purpose, all the export functionalities are in the same form.
   input buffer and output buffer use different data structure accordingly.When input buffer
   or output buffer is not necessary, just set it to NULL and size to be zero.
   QCA will open and close miniport driver and send corresponding ioctl.
*/
//CUSTOMER_RETURN_CODE  CustomerControl(
//                              DWORD dwIoControlCode,
//                              LPVOID lpInBuffer,
//                              DWORD nInBufferSize,
//                              LPVOID lpOutBuffer,
//                              DWORD nOutBufferSize,
//                              LPDWORD lpBytesReturned
//                              );

//case IOCTL_CUSTOMER_KEEPALIVE_SET
// lpInBuffer: caller will supply the fields.
typedef struct _Customer_Keepalive_Set{
    UINT32      nId; // = id
    UINT32      nMSec; // = msec
    UINT32      nSize; // =size
    UCHAR       bPacket[ATH_CUSTOMER_KEEPALIVE_MAX_PACKET_SIZE]; // put packet here
    UCHAR       bPacketExtraIntel[KEEPALIVE_EXTRA_BUFFER_FOR_INTEL];  // extra buffer space for Intel
} CUSTOMER_KEEPALIVE_SET, *PCUSTOMER_KEEPALIVE_SET;
// lpOutBuffer:
// none

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully set
// 2. CUSTOMER_CONTROL_BAD_PARAMETER if
//                  Id is not 0 or 1, or
//                  Msec is > 3600000, or
//                  Packet == NULL, or
//                  Size > 80
// 3. CUSTOMER_CONTROL_FAILURE  if failed for other reasons, say, conflict with OS

//case IOCTL_CUSTOMER_KEEPALIVE_QUERY
// lpInBuffer
typedef struct _Customer_Keepalive_Query{
    UINT32      nId; // = id
    UINT32      nRet;
}CUSTOMER_KEEPALIVE_QUERY,*PCUSTOMER_KEEPALIVE_QUERY;
// lpOutBuffer
// Same as lpInBuffer
// nRet TRUE or FALSE

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_BAD_PARAMETER if
//                  Id is not 0 or 1
// 3. CUSTOMER_CONTROL_FAILURE  if failed for other reasons

// case IOCTL_CUSTOMER_WAKEUP_MATCH_SET
// lpInBuffer:
typedef struct {
    UCHAR       bPattern[IEEE80211_ADDR_LEN]; // = patten[6]
} CUSTOMER_WAKEUP_MATCH_SET,*PCUSTOMER_WAKEUP_MATCH_SET;

// lpOutBuffer
// none

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed to set

// case IOCTL_CUSTOMER_WAKEUP_DISABLE
// lpInBuffer
// None
// lpOutBuffer
// None

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed to set

// case IOCTL_CUSTOMER_WAKEUP_QUERY
// lpInBuffer:
// none
// lpOutBuffer
typedef struct _Customer_Wakeup_Query{
    UINT32      nRet;
} CUSTOMER_WAKEUP_QUERY, *PCUSTOMER_WAKEUP_QUERY;

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed

// case IOCTL_CUSTOMER_NDIS_SET_PM_CAPABILITIES
// lpInBuffer:
// NDIS_PM_CAPABILITIES defined in WDK
// lpOutBuffer
// none

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed

// case IOCTL_CUSTOMER_NDIS_SET_PM_COUNTED_STRING
// Pending

// case IOCTL_CUSTOMER_NDIS_SET_PM_PARAMETERS
// lpInBuffer:
// NDIS_PM_PARAMETERS defined in WDK
// lpOutBuffer
// none

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed

// case IOCTL_CUSTOMER_NDIS_SET_PM_PROTOCOL_OFFLOAD
// lpInBuffer:
// NDIS_PM_PROTOCOL_OFFLOAD defined in WDK
// lpOutBuffer
// none

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed

// case IOCTL_CUSTOMER_NDIS_SET_PM_WOL_PATTERN
// lpInBuffer:
// NDIS_PM_WOL_PATTERN defined in WDK
// lpOutBuffer
// none

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed

// case IOCTL_CUSTOMER_NDIS_QUERY_PM_CAPABILITIES
// lpInBuffer:
// none
// lpOutBuffer
// NDIS_PM_CAPABILITIES defined in WDK

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed

// case IOCTL_CUSTOMER_NDIS_QUERY_PM_COUNTED_STRING
// Pending

// case IOCTL_CUSTOMER_NDIS_QUERY_PM_PARAMETERS
// lpInBuffer:
// none
// lpOutBuffer
// NDIS_PM_PARAMETERS defined in WDK

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed

// case IOCTL_CUSTOMER_NDIS_QUERY_PM_PROTOCOL_OFFLOAD
// lpInBuffer:
// none
// lpOutBuffer
// NDIS_PM_PROTOCOL_OFFLOAD defined in WDK

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed

// case IOCTL_CUSTOMER_NDIS_QUERY_PM_WOL_PATTERN
// lpInBuffer:
// none
// lpOutBuffer
// NDIS_PM_WOL_PATTERN defined in WDK

// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed

// case IOCTL_CUSTOMER_FIXEDPATTERN_SET/IOCTL_CUSTOMER_FIXEDPATTERN_QUERY
// lpInBuffer:
// none
// lpOutBuffer
// NDIS_PM_WOL_PATTERN defined in WDK
typedef struct {
    UCHAR       bPattern[6]; // = patten[6]
} CUSTOMER_FIXEDPATTERN_SET,*PCUSTOMER_FIXEDPATTERN_SET;
// Return values:
// 1. CUSTOMER_CONTROL_OK if parameters valid and successfully
// 2. CUSTOMER_CONTROL_FAILURE  if failed

//CustomerControlSet
typedef CUSTOMER_RETURN_CODE (*pfbeginProgram)(void);
typedef CUSTOMER_RETURN_CODE (*pfbeginSession)(void);
typedef CUSTOMER_RETURN_CODE (*pfCustomerControl)(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned);
typedef CUSTOMER_RETURN_CODE (*pfendSession)(void);
typedef CUSTOMER_RETURN_CODE (*pfendProgram)(void);

//pfCustomerControl CustomerControl;

// For IOACNIC connection type
typedef enum _ConnectionType {
    CONNECTION_UNKNOWN  = 0,
    CONNECTION_WIRED    = 1,
    CONNECTION_WIRELESS = 2,

} CONNECTION_TYPE, *PCONNECTION_TYPE;

#define MAGIC_PACKET_TYPE_EXTENDED      0x1
#define MAGIC_PACKET_TYPE_ACER_SHORT    0x2

// Customer Control Set
// beginProgram, beginSession called when program/session begins
// CustomerControl is wrapper from customer.h
// endSession, endProgram called when session/program ends
// For some vendor, begin/end function is required to initialize. All vendor will provide this function but some will be dummy function
typedef struct _CustomerControlSet {
    GUID nId;
    CONNECTION_TYPE nType;
    BYTE nPacketFmt;                        // bit 0 : standard extended magic packet (MAGIC_PACKET_TYPE_EXTENDED)
                                            //     1 : short magic packet (MAGIC_PACKET_TYPE_ACER_SHORT)
    pfbeginProgram beginProgram;
    pfbeginSession beginSession;
    pfCustomerControl CustomerControl;
    pfendSession closeSession;
    pfendProgram closeProgram;
} CUSTOMER_CONTROL_SET, *PCUSTOMER_CONTROL_SET;

// EnumIOACNIC
// enumerate NIC supports IOAC at this device
//
// nicCount: return length of support IOAC NIC's GUID array
//
// Return value: GUID*, array of IOAC NIC's GUID array. 0 if there is no NIC support IOAC
//
GUID* EnumIOACNIC(int *nicCount);

int EnumIoacNetAdapter(char * szGuidArray[]);
// LoadCustCtlSet
// load function from vendor library of NIC supports IOAC with given NIC GUID
//
// nicGUID: one valid NIC GUID from EnumIOACNIC
//
// Return value: PCUSTOMER_CONTROL_SET, pointer of CUSTOMER_CONTROL_SET to access the IOAC NIC customer function. 0 if there is no IOAC NIC of given nicGUID
//
PCUSTOMER_CONTROL_SET LoadCustCtlSet(const char * nicGUID);

#ifdef __cplusplus
};
#endif

#endif //__CUSTOMER_SUN_H_
