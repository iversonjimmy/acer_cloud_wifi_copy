/*
 *  Copyright 2011 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */
#ifndef __ESTYPES_H__
#define __ESTYPES_H__

#include "vplu_types.h"

#if defined(__cplusplus) && defined(USE_ES_NAMESPACE)
#  define ES_NAMESPACE_START \
        namespace bcc { \
        namespace client { \
        namespace es {
#  define ES_NAMESPACE_END \
        } } }
#  define ES_NAMESPACE bcc::client::es
#  define USING_ES_NAMESPACE using namespace ES_NAMESPACE;
#else
#  define ES_NAMESPACE_START 
#  define ES_NAMESPACE_END
#  define ES_NAMESPACE
#  define USING_ES_NAMESPACE
#endif

// ES error codes
#define ES_ERR_OK                           0
#define ES_ERR_FAIL                         -1
#define ES_ERR_INVALID                      -2
#define ES_ERR_STORAGE                      -3
#define ES_ERR_STORAGE_SIZE                 -4
#define ES_ERR_CRYPTO                       -5
#define ES_ERR_VERIFICATION                 -6
#define ES_ERR_DEVICE_ID_MISMATCH           -7
#define ES_ERR_ISSUER_NOT_FOUND             -8
#define ES_ERR_INCORRECT_SIG_TYPE           -9
#define ES_ERR_INCORRECT_PUBKEY_TYPE        -10
#define ES_ERR_INCORRECT_TICKET_VERSION     -11
#define ES_ERR_INCORRECT_TMD_VERSION        -12
#define ES_ERR_NO_RIGHT                     -13
#define ES_ERR_ALIGNMENT                    -14


// ES signature types
#define ES_SIG_TYPE_ECC_SHA1                1
#define ES_SIG_TYPE_ECC_SHA256              2


// ES license types
#define ES_LICENSE_MASK                     0xf
#define ES_LICENSE_PERMANENT                0
#define ES_LICENSE_DEMO                     1
#define ES_LICENSE_TRIAL                    2
#define ES_LICENSE_RENTAL                   3
#define ES_LICENSE_SUBSCRIPTION             4
#define ES_LICENSE_SERVICE                  5


// ES title-level limit codes
#define ES_MAX_LIMIT_TYPE                   8

#define ES_LC_DURATION_TIME                 1
#define ES_LC_ABSOLUTE_TIME                 2
#define ES_LC_NUM_TITLES                    3
#define ES_LC_NUM_LAUNCH                    4
#define ES_LC_ELAPSED_TIME                  5


// ES item-level rights
#define ES_ITEM_RIGHT_PERMANENT             1
#define ES_ITEM_RIGHT_SUBSCRIPTION          2
#define ES_ITEM_RIGHT_CONTENT               3
#define ES_ITEM_RIGHT_CONTENT_CONSUMPTION   4
#define ES_ITEM_RIGHT_ACCESS_TITLE          5

#define ES_REFERENCE_ID_LEN                 16
#define ES_CONTENT_ITEM_ACCESS_MASK_LEN     128


// ES title type
#define ES_TITLE_TYPE_DATA                  0x8
#define ES_TITLE_TYPE_CT_TITLE              0x40


// ES content type
#define ES_CONTENT_TYPE_ENCRYPTED           0x1
#define ES_CONTENT_TYPE_DISC                0x2
#define ES_CONTENT_TYPE_CFM                 0x4
#define ES_CONTENT_TYPE_OPTIONAL            0x4000
#define ES_CONTENT_TYPE_SHARED              0x8000


// Certificate and signature sizes
#define ES_DEVICE_CERT_SIZE                 384
#define ES_SIGNING_CERT_SIZE                384
#define ES_SIGNATURE_SIZE                   60


// Size of custom data area in the TMD
#define ES_TMD_CUSTOM_DATA_LEN              32


//
// Maximum possible content index value is 64K - 2, since
// the maximum number of contents per title is 64K - 1
//
#define ES_CONTENT_INDEX_MAX                65534

ES_NAMESPACE_START

typedef s32 ESError;

typedef u32 ESDeviceId;
typedef u64 ESTicketId;
typedef u64 ESTitleId;
typedef u32 ESContentId;
typedef u16 ESContentIndex;

typedef u16 ESTicketVersion;
typedef u16 ESTitleVersion;
typedef u16 ESMinorTitleVersion;
typedef ESTitleId ESSysVersion;

typedef u32 ESSigType;
typedef u8 ESLicenseType;
typedef u32 ESTitleType;
typedef u16 ESContentType;
typedef u8 ESSysAccessMask[2];


typedef struct {
    ESContentId     id;
    ESContentIndex  index;
    ESContentType   type;
    u64             size;
} ESContentInfo;


typedef u32 ESLimitCode;

typedef struct {
    ESLimitCode     code;
    u32             limit;
} ESLpEntry;


typedef u32 ESItemType;

typedef struct {
    u8              referenceId[ES_REFERENCE_ID_LEN];
    u32             referenceIdAttr;  // Reserved
} ESPermanentItemRight;

typedef struct {
    u32             limit;
    u8              referenceId[ES_REFERENCE_ID_LEN];
    u32             referenceIdAttr;  // Reserved
} ESSubscriptionItemRight;

typedef struct {
    ESContentIndex  offset;
    u8              accessMask[ES_CONTENT_ITEM_ACCESS_MASK_LEN];
} ESContentItemRight;

typedef struct {
    ESContentIndex  index;
    u16             padding;
    ESLimitCode     code;
    u32             limit;
} ESContentConsumptionItemRight;

typedef struct {
    u64             accessTitleId;
    u64             accessTitleMask;
} ESAccessTitleItemRight;

typedef struct {
    ESItemType      type;
    u32             padding;
    union {
        ESPermanentItemRight            perm;
        ESSubscriptionItemRight         sub;
        ESContentItemRight              cnt;
        ESContentConsumptionItemRight   cntLmt;
        ESAccessTitleItemRight          accTitle;
        u8                              padding[sizeof(ESContentItemRight) + 6];
    } right;
} ESItemRight;

ES_NAMESPACE_END

#endif // include guard
