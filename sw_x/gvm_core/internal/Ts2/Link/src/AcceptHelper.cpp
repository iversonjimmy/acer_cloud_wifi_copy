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

#include "AcceptHelper.hpp"

#include <cassert>

Ts2::Link::AcceptHelper::AcceptHelper(AcceptedSocketHandler acceptedSocketHandler, void *acceptedSocketHandler_context,
                                      LocalInfo *localInfo)
    : acceptedSocketHandler(acceptedSocketHandler), acceptedSocketHandler_context(acceptedSocketHandler_context),
      localInfo(localInfo)
{
    assert(acceptedSocketHandler);
    assert(localInfo);
    VPLMutex_Init(&mutex);
}

Ts2::Link::AcceptHelper::~AcceptHelper()
{
    VPLMutex_Destroy(&mutex);
}
