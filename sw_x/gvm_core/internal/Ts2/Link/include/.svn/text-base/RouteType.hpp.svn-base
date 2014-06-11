/*
 *  Copyright 2013 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#ifndef __TS2_LINK_ROUTE_TYPE_HPP__
#define __TS2_LINK_ROUTE_TYPE_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

namespace Ts2 {
namespace Link {

enum RouteType {
    ROUTE_TYPE_INVALID = 0,
    // RouteType values should be in the order of increasing cost.
    ROUTE_TYPE_DIN = 1,
    ROUTE_TYPE_DEX,
    ROUTE_TYPE_P2P,
    ROUTE_TYPE_PRX,
    ROUTE_TYPE_LOWEST_COST = ROUTE_TYPE_DIN,
    ROUTE_TYPE_HIGHEST_COST = ROUTE_TYPE_PRX,
};

} // end namespace Link
} // end namespace Ts2

#endif // incl guard
