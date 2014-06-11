/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */


#ifndef __ESITYPES_H__
#define __ESITYPES_H__

#include "estypes.h"
#include "iosccert.h"

ES_NAMESPACE_START

/*
 * Title types are usually defined in estypes.h, but GVM-specific type
 * needs to be hidden in an internal header file
 */
#define ES_TITLE_TYPE_GVM_TITLE 0x80    /* GVM title */


#define ES_APP_CERT_PREFIX      "AP"
#define ES_CP_PREFIX            "CP"
#define ES_XS_PREFIX            "XS"
#define ES_MS_PREFIX            "MS"
#define ES_SP_PREFIX            "SP"


typedef u32 ESContainerType;

#define ES_CONTAINER_TMD        0
#define ES_CONTAINER_TKT        1
#define ES_CONTAINER_DEV        2


// Common ticket definition
#define ES_COMMON_TICKET_DEVICE_ID  0

// Property mask flags
#define ES_TICKET_PRE_INSTALL_FLAG  0x1

// V1 ticket header flag
#define ES_ETS_COMPRESSED           0x01


// There are a maximum of 64 CMD groups, each with a maximum of 1K CMDs
#define ES_MAX_CMDS_IN_GROUP        1024
#define ES_MAX_CMD_GROUPS           64


typedef u8 ESVersion;
typedef u8 ESTicketCustomData[20];
typedef u8 ESTicketReserved[25];
typedef u16 ESPropertyMask;
typedef u8 ESCidxMask[64];
typedef u8 ESReferenceId[ES_REFERENCE_ID_LEN];
typedef u8 ESV1ContentRecordAccessMask[ES_CONTENT_ITEM_ACCESS_MASK_LEN];
typedef u8 ESTmdCustomData[32];
typedef u8 ESTmdReserved[30];


#pragma pack(push, 4)


typedef struct {
    IOSCSigRsa2048      sig;            // RSA 2048-bit sign of the ticket
    IOSCEccPublicKey    serverPubKey;   // Ticketing server public key
    ESVersion           version;        // Ticket data structure version number
    ESVersion           caCrlVersion;   // CA CRL version number
    ESVersion           signerCrlVersion;   // Signer CRL version number
    IOSCAesKey          titleKey;       // Published title key
    ESTicketId          ticketId;       // Unique 64bit ticket ID
    ESDeviceId          deviceId;       // Unique 32bit device ID
    ESTitleId           titleId;        // Unique 64bit title ID
    ESSysAccessMask     sysAccessMask;  // 16-bit cidx mask to indicate which
                                        // of the first 16 pieces of contents
                                        // can be accessed by the system app
    ESTicketVersion     ticketVersion;  // 16-bit ticket version
    u32                 accessTitleId;  // 32-bit title ID for access control
    u32                 accessTitleMask;    // 32-bit title ID mask
    ESLicenseType       licenseType;
    u8                  keyId;          // Common key ID
    ESPropertyMask      propertyMask;   // 16-bit property mask
    ESTicketCustomData  customData;     // 20-byte custom data
    ESTicketReserved    reserved;       // 25-byte reserved info
    u8                  audit;
    ESCidxMask          cidxMask;       // Bit-mask of the content indices
    ESLpEntry           limits[ES_MAX_LIMIT_TYPE];  // Limited play entries
} ESTicket;

typedef struct {
    u16     hdrVersion;         // Version of the ticket header
    u16     hdrSize;            // Size of ticket header
    u32     ticketSize;         // Size of the v1 portion of the ticket
    u32     sectHdrOfst;        // Offset of the section header table
    u16     nSectHdrs;          // Number of section headers
    u16     sectHdrEntrySize;   // Size of each section header
    u32     flags;              // Miscellaneous attributes
} ESV1TicketHeader;

typedef struct {
    u32     sectOfst;       // Offset of this section
    u32     nRecords;       // Number of records in this section
    u32     recordSize;     // Size of each record
    u32     sectionSize;    // Total size of this section
    u16     sectionType;    // Type code of this section
    u16     flags;          // Miscellaneous attributes
} ESV1SectionHeader;

typedef struct {
    u32             limit;              // Expiration time
    ESReferenceId   referenceId;        // Reference ID
    u32             referenceIdAttr;    // Reference ID attributes
} ESV1SubscriptionRecord;

typedef struct {
    ESReferenceId   referenceId;        // Reference ID
    u32             referenceIdAttr;    // Reference ID attributes
} ESV1PermanentRecord;

typedef struct {
    u32                             offset;     // Offset content index
    ESV1ContentRecordAccessMask     accessMask; // Access mask
} ESV1ContentRecord;

typedef struct {
    ESContentIndex  index;              // Content index
    u16             code;               // Limit code
    u32             limit;              // Limit value
} ESV1ContentConsumptionRecord;

typedef struct {
    u64             accessTitleId;      // Access title ID
    u64             accessTitleMask;    // Access title mask
} ESV1AccessTitleRecord;


typedef struct {
    ESContentId     cid;    // 32-bit content ID
    ESContentIndex  index;  // Content index, unique per title
    ESContentType   type;   // Content type
    u64             size;   // Unencrypted content size in bytes
    IOSCHash256     hash;   // Hash of the content
} ESV1ContentMeta;

typedef struct {
    ESVersion       version;            // TMD version number
    ESVersion       caCrlVersion;       // CA CRL version number
    ESVersion       signerCrlVersion;   // Signer CRL version number
    ESSysVersion    sysVersion;         // System software version number
    ESTitleId       titleId;            // 64-bit title id
    ESTitleType     type;               // 32-bit title type
    u16             groupId;
    ESTmdCustomData customData;         // 32-byte custom data
    ESTmdReserved   reserved;           // 30-byte reserved info
    u32             accessRights;       // Rights to system resources
    ESTitleVersion  titleVersion;       // 16-bit title version
    u16             numContents;        // Number of contents
    ESContentIndex  bootIndex;          // Boot content index
    ESMinorTitleVersion minorTitleVersion;  // 16-bit minor title version
} ESTitleMetaHeader;

typedef struct {
    ESContentIndex  offset;             // Offset content index
    u16             nCmds;              // Number of CMDs in this group
    IOSCHash256     groupHash;          // Hash for this group of CMDs
} ESV1ContentMetaGroup;

typedef struct {
    IOSCHash256          hash;          // Hash for the CMD groups
    ESV1ContentMetaGroup cmdGroups[ES_MAX_CMD_GROUPS];
} ESV1TitleMetaHeader;

typedef struct {
    IOSCSigRsa2048      sig;            // RSA 2048-bit sign of the TMD header
    ESTitleMetaHeader   head;
    ESV1TitleMetaHeader v1Head;         // Extension to the v0 TMD header
    //ESV1ContentMeta   contents[0];    // CMD array sorted by content index
} ESV1TitleMeta;


#pragma pack(pop)

ES_NAMESPACE_END

#endif  // include guard
