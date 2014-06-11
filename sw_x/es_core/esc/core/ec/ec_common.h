
#ifndef __EC_COMMON_H__
#define __EC_COMMON_H__

#ifdef _MSC_VER
#ifndef bool
#define bool int
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif

// hypervisor includes
#include "km_types.h"
#include "defs.h"
#include "core_glue.h"
#include "build_features.h"
#include "ec_log.h"

#if 0
#include "assert.h"
#include "bits.h"
#include "x86.h"
#include "kvtophys.h"
#include "valloc.h"
#include "string.h"
#include "printf.h"
#include "vm.h"
#include "gpt.h"
#include "malloc.h"
#include "dev_cred.h"
#include "es_dso.h"
#endif // 0


// Note: *this* call NEEDS to zero its memory..
#define ARRAY(t, n) (t *)calloc(n, sizeof(t))
#define ALLOC(t)        ARRAY(t, 1)

#define ARRAY_LEN(x)		(sizeof(x)/sizeof(x[0]))

#endif // __EC_COMMON_H__
