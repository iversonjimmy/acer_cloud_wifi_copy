/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */


#ifndef __ES_DEVICE_H__
#define __ES_DEVICE_H__


#include "es_utils.h"

ES_NAMESPACE_START

ESError esGetDeviceCert(void *outDeviceCert, u32 outDeviceCertSize);

ESError esGetDeviceId(ESDeviceId *outDeviceId);

ES_NAMESPACE_END

#endif  // __ES_DEVICE_H__
