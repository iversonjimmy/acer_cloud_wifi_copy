/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 */

#ifndef __CONTENT_META_H__
#define __CONTENT_META_H__

#include "km_types.h"

#define CFM_VERSION     1

/*
 * Content file metadata (CFM) (V1) describes a content file.
 * Refer to http://intwww/wiki/index.php/GVM_ES_Todo#CFM_Definition_Change
 * for latest updates. 
 */
typedef struct {
    u16 version;       /* Format version of this option header */
    u8  numSections;
    u8  flags;         
    u32 sectionOffset; 
} CfmHeader;

typedef struct {
    u8  sectionType;
    u8  reserved[3];
    u32 nextSectionOffset;
    u32 payloadHeaderOffset;
    u32 payloadDataOffset;
} CfmSectionHeader;

typedef struct {
    u32 numRecords;
    u8  log2BlockSize;
    u8  hashType;
    u8  recordSize;
    u8  reserved;
} CfmHashSectionHeader;

typedef struct {
    u32 numRecords;
    u8  log2BlockSize;
    u8  recordSize;
    u16 reserved;
} CfmZeroSectionHeader;

typedef enum {
    CFM_SHA1_HASH       = 0,
    CFM_SHA256_HASH
} CfmHashType;

typedef enum {
    CFM_HASH_SECTION    = 0,
    CFM_DISC1_SECTION,                  // discontinued exec bits
    CFM_ZERO_SECTION,
    CFM_DISC2_SECTION,                  // discontinued self-mod bits
    CFM_NUM_SECTIONS
} CfmSections;

#endif  /* CFM_H */
