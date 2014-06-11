#ifndef __BUILD_FEATURES_H__
#define __BUILD_FEATURES_H__

//
// This file contains feature definitions for the various GVM Build types.
// The GVM_BUILD_XXX defs should not be used in code to enable or disable
// code paths in source code. Instead, the code to be ifdef'd should be
// aligned with a particular feature and be ifdef'd with the corresponding
// FEATURE definition.
//

#ifdef GVM_BUILD_INTERN
#define GVM_FEAT_RW_ROOTFS
#define GVM_FEAT_AUTOTEST
#define GVM_FEAT_TESTD
// #define GVM_FEAT_GHV_DBG
#define GVM_FEAT_GHV_CLEAR_ES
#define GVM_FEAT_PLAT_CMN_KEY
#endif // GVM_BUILD_INTERN

#ifdef GVM_BUILD_DEVEL
// #define GVM_FEAT_GHV_DBG
#define GVM_FEAT_PLAT_CMN_KEY
#define GVM_FEAT_PCK_SYS
#endif // GVM_BUILD_DEVEL

#ifdef GVM_BUILD_TESTPROD
#define GVM_FEAT_AUTOTEST
#define GVM_FEAT_TESTD
#endif // GVM_BUILD_TESTPROD

#ifdef GVM_BUILD_PROD
#endif // GVM_BUILD_PROD

#ifndef GVM_FEAT_PLAT_CMN_KEY
#define GVM_FEAT_PLAT_CMN_KEY
#endif

#endif // __BUILD_FEATURES_H__
