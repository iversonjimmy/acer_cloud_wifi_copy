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
#ifndef __TIKVIEWER_H__
#define __TIKVIEWER_H__

#include "estypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Error codes. */
enum {
    TIKV_ERR_OK = 0,
    TIKV_ERR_BAD_ARG,
    TIKV_ERR_BAD_TIK_SIZE,
    TIKV_ERR_INVALID_ITEMRIGHT_TYPE,
};

/** Return the ticket ID. */
int tikv_getTicketId(const void *tik, u32 tikSize, ESTicketId *outTicketId);

/** Return the ticket version. */
int tikv_getTicketVersion(const void *tik, u32 tikSize, ESTicketVersion *outTicketVersion);

/** Return the title ID. */
int tikv_getTitleId(const void *tik, u32 tikSize, ESTitleId *outTitleId);

/** Return the device ID. */
int tikv_getDeviceId(const void *tik, u32 tikSize, ESDeviceId *outDeviceId);

/** Return the state of the pre-installation flag. */
int tikv_getPreInstallationFlag(const void *tik, u32 tikSize, int *outFlag);

/** Return the license type. */
int tikv_getLicenseType(const void *tik, u32 tikSize, ESLicenseType *outLicenseType);

/** Return the system access mask. */
int tikv_getSysAccessMask(const void *tik, u32 tikSize, ESSysAccessMask outSysAccessMask);

/** Return the title limits. */
int tikv_getLimits(const void *tik, u32 tikSize, ESLpEntry *outLimits, u32 *outNumLimits);

/** Return the item rights of the specific type.
 * On call, outNumItemRights contains the size of the array specific to the type passed in via outItemRights.
 * On return, outNumItemRights contains the number of entries in the array.
 * On return, outTotal contains the number of item rights of the given type in the ticket (regardless of how many were returned).
 * If offset > 0, that many item rights will be skipped in the response.
 */
int tikv_getItemRights(const void *tik, u32 tikSize, ESItemType type, u32 offset, void *outItemRights, u32 *outNumItemRights, u32 *outTotal);

#ifdef __cplusplus
}
#endif

#endif // include guard
