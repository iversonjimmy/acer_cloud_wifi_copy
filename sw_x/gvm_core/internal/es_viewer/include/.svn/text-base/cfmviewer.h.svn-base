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
#ifndef __CFMVIEWER_H__
#define __CFMVIEWER_H__

#include "cfm.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Error codes. */
enum {
    CFMV_ERR_OK = 0,
    CFMV_ERR_BAD_ARG,
    CFMV_ERR_BAD_CFM_SIZE,
    CFMV_ERR_INVALID_SECTION_TYPE,
    CFMV_ERR_UNSUPPORTED_CFM_VERSION,
};

/** Return the records of the specific type.
 * On return, outHeader contains the type-specific section header.
 * On call, outNumRecords contains the size of the array specific to the type passed in via outRecords.
 * On return, outNumRecords contains the number of entries in the array.
 * If offset > 0, that many records will be skipped in the response.
 */
int cfmv_getSection(const void *_cfm, u32 cfmSize, CfmSections type, u32 offset,
                    void *outHeader, void *outRecords, u32 *outNumRecords);

#ifdef __cplusplus
}
#endif

#endif // include guard
