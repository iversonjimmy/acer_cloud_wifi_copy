//
//  Copyright (C) 2011, iGware Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of iGware Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of iGware Communications Corp.
//

#include "vpl_plat.h"
#include "vplTest.h"

// Unreliable on Ubuntu; we don't currently need it anyway.
#if 0
static void testGetOSUserName(void)
{
    char* username = NULL;
    int rc = VPL_GetOSUserName(&username);
    VPLTEST_CHK_OK(rc, "VPL_GetOSUserName");
    if (username == NULL) {
        VPLTEST_NONFATAL_ERROR("The retrieved string was NULL");
    } else {
        VPLTEST_LOG("OS User Name = \"%s\"", username);
    }
    VPL_ReleaseOSUserName(username);
}
#endif

#ifndef VPL_PLAT_IS_WINRT
static void testGetOSUserId(void)
{
    char* userId = NULL;
    int rc = VPL_GetOSUserId(&userId);
    VPLTEST_CHK_OK(rc, "VPL_GetOSUserId");
    if (userId == NULL) {
        VPLTEST_NONFATAL_ERROR("The retrieved string was NULL");
    } else {
        VPLTEST_LOG("OS User ID = \"%s\"", userId);
    }
    VPL_ReleaseOSUserId(userId);
}
#endif

static void testGetHwUuid(void)
{
    char* hwUuid = NULL;
    int rc = VPL_GetHwUuid(&hwUuid);
    VPLTEST_CHK_OK(rc, "VPL_GetHwUuid");
    if (hwUuid == NULL) {
        VPLTEST_NONFATAL_ERROR("The retrieved string was NULL");
    } else {
        VPLTEST_LOG("hardware uuid = \"%s\"", hwUuid);
    }
    VPL_ReleaseHwUuid(hwUuid);
}

void testVPLPlat(void)
{
    // Unreliable on Ubuntu; we don't currently need it anyway.
#if 0
    VPLTEST_LOG("testGetOSUserName");
    testGetOSUserName();
#endif
#ifndef VPL_PLAT_IS_WINRT
    VPLTEST_LOG("testGetOSUserId");
    testGetOSUserId();
#endif
    VPLTEST_LOG("testGetHwUuid");
    testGetHwUuid();

    VPLTEST_LOG("Test invalid params:");
    VPLTEST_CALL_AND_CHK_RV(VPL_GetHwUuid(NULL), VPL_ERR_INVALID);
    VPLTEST_CALL_AND_CHK_RV(VPL_GetOSUserName(NULL), VPL_ERR_INVALID);
#ifndef VPL_PLAT_IS_WINRT
    VPLTEST_CALL_AND_CHK_RV(VPL_GetOSUserId(NULL), VPL_ERR_INVALID);
#endif
    VPLTEST_LOG("Test releasing NULL strings:");
    VPL_ReleaseHwUuid(NULL);
    VPL_ReleaseOSUserName(NULL);
#ifndef VPL_PLAT_IS_WINRT
    VPL_ReleaseOSUserId(NULL);
#endif
}
