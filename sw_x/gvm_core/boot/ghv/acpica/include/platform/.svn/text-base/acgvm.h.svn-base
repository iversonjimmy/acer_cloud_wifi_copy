/******************************************************************************
 *
 * Name: acgvm.h - OS specific defines for GVM
 * Copyright (c) 2010 Broadon Inc, All rights reserved.
 *
 *****************************************************************************/

#ifndef __ACGVM_H__
#define __ACGVM_H__

#include <stdarg.h>

#define ACPI_MACHINE_WIDTH          64
#define COMPILER_DEPENDENT_INT64    long
#define COMPILER_DEPENDENT_UINT64   unsigned long

#undef ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_LOCAL_CACHE

#define ACPI_USE_DO_WHILE_0
#define ACPI_MUTEX_TYPE             ACPI_BINARY_SEMAPHORE

#define strtoul                     simple_strtoul

#define ACPI_CPU_FLAGS              unsigned long

#define ACPI_FLUSH_CPU_CACHE() asm volatile ("wbinvd")

#include "acgcc.h"

#endif /* __ACGVM_H__ */
