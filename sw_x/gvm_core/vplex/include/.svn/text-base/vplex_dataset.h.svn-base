//
//  Copyright (C) 2009, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_DATASET_H__
#define __VPLEX_DATASET_H__

//############################################################################
/// @file
/// Generic utilities for gathering statistics for a set of data.
//############################################################################

#include "vplex_plat.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Tracks basic statistics for a set of measurements, using a constant (and small) amount of memory.
typedef struct VPLDataSet_u32 {
    u64 sum;
    u64 sum_of_squares;
    u32 count;
    u32 min;
    u32 max;
} VPLDataSet_u32_t;

/// Initializes \a dataset.
void VPLDataSet_u32_Init(VPLDataSet_u32_t* dataset);

/// Adds a sample to \a dataset.
void VPLDataSet_u32_AddSample(VPLDataSet_u32_t* dataset, u32 new_value);

/// Adds all samples from \a other into \a dataset.
void VPLDataSet_u32_MergeSets(VPLDataSet_u32_t* dataset, const VPLDataSet_u32_t* other);

u32 VPLDataSet_u32_GetMin(VPLDataSet_u32_t const* dataset);
double VPLDataSet_u32_GetMean(VPLDataSet_u32_t const* dataset);
double VPLDataSet_u32_GetVariance(VPLDataSet_u32_t const* dataset);
//double VPLDataSet_u32_GetStdDev(VPLDataSet_u32_t const* dataset); // no sqrt on IOP?

#define FMT_VPLDataSet_u32_t  "avg="FMTdouble", var="FMTdouble", min="FMTu32", max="FMTu32", count="FMTu32
#define VAL_VPLDataSet_u32_t(dataset)  VPLDataSet_u32_GetMean(dataset), VPLDataSet_u32_GetVariance(dataset), \
                                      VPLDataSet_u32_GetMin(dataset), (dataset)->max, (dataset)->count

#ifdef __cplusplus
}
#endif

#endif // include guard

