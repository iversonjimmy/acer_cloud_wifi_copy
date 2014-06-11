//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __EVENT_MANAGER_PB_HPP__
#define __EVENT_MANAGER_PB_HPP__

//============================================================================
/// @file
/// Protobuf-based CCDI Events.
//============================================================================

#include "base.h"

#include <ccdi_rpc.pb.h>

/// Perform one-time init.
int EventManagerPb_Init();

/// This is provided only to allow clean shutdown of the process.
/// @note The module does not support being started again after calling this.
void EventManagerPb_Quit();

/// Caller should allocate the event via new.  The EventManager will call delete on it.
void EventManagerPb_AddEvent(const ccd::CcdiEvent* event);

int EventManagerPb_CreateQueue(u64* newHandle_out);

int EventManagerPb_DestroyQueue(u64 handle);

int EventManagerPb_GetEvents(u64 queueHandle, u32 maxToGet, int timeoutMs,
        google::protobuf::RepeatedPtrField<ccd::CcdiEvent>* events_out);

#endif // include guard
