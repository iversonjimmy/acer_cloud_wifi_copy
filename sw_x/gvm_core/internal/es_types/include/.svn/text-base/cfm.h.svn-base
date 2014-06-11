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
#ifndef __CFM_H__
#define __CFM_H__

#include "vplu_types.h"

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

#endif  // include guard
