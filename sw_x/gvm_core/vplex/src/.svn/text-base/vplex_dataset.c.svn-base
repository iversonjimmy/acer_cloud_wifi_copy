//
//  Copyright (C) 2009, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.

// no sqrt on IOP?
//#include <math.h>

#include "vplex_safe_float_conversion.h"

#include "vplex_dataset.h"

void VPLDataSet_u32_Init(VPLDataSet_u32_t* dataset)
{
    memset(dataset, 0, sizeof(VPLDataSet_u32_t));
    dataset->min = UINT32_MAX;
    //dataset->max = 0;
}

void VPLDataSet_u32_AddSample(VPLDataSet_u32_t* dataset, u32 new_value)
{
    dataset->count++;
    dataset->sum += new_value;
    dataset->sum_of_squares += (new_value * new_value);
    dataset->min = MIN(new_value, dataset->min);
    dataset->max = MAX(new_value, dataset->max);
}

u32 VPLDataSet_u32_GetMin(VPLDataSet_u32_t const* dataset)
{
    if (dataset->count == 0) {
        return 0;
    } else {
        return dataset->min;
    }
}

double VPLDataSet_u32_GetMean(VPLDataSet_u32_t const* dataset)
{
    if (dataset->count == 0) {
        return 0.0;
    } else {
        return U64_TO_DOUBLE(dataset->sum) / U32_TO_DOUBLE(dataset->count);
    }
}

#if 0 
// no sqrt on IOP?
double VPLDataSet_u32_GetStdDev(VPLDataSet_u32_t const* dataset)
{
    return sqrt(VPLDataSet_u32_GetVariance(dataset));
}
#endif

double VPLDataSet_u32_GetVariance(VPLDataSet_u32_t const* dataset)
{
    if (dataset->count == 0) {
        return 0.0;
    } else {
        double mean = VPLDataSet_u32_GetMean(dataset);
        /*  # (1/n) * sum((x_i-mean)^2) ==
            # (1/n) * sum(x_i^2 - 2*mean*x_i + mean^2) ==  
            # (1/n) * (sum(x_i^2) - 2*mean*sum(x_i) + mean*mean*sum(1)) ==
            # sum(x_i^2)/n - 2*mean*(mean*n)/n + mean*mean*n/n ==
            # sum(x_i^2)/n - 2*mean*mean + mean*mean ==
        */
        return (U64_TO_DOUBLE(dataset->sum) / U32_TO_DOUBLE(dataset->count)) - 
                (mean * mean);
    }
}

void VPLDataSet_u32_MergeSets(VPLDataSet_u32_t* dataset_in_out, const VPLDataSet_u32_t* other)
{
    dataset_in_out->count += other->count;
    dataset_in_out->sum += other->sum;
    dataset_in_out->sum_of_squares += other->sum_of_squares;
    dataset_in_out->min = MIN(other->min, dataset_in_out->min);
    dataset_in_out->max = MAX(other->max, dataset_in_out->max);
}
