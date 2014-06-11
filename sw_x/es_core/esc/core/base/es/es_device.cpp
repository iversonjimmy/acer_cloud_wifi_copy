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


#include <estypes.h>
#include <iosc.h>
#include <istorage.h>
#ifdef _KERNEL_MODULE
#include "core_glue.h"
#endif

USING_ES_NAMESPACE
USING_ISTORAGE_NAMESPACE

#include <esi.h>
#include "es_device.h"

ES_NAMESPACE_START



ESError
esGetDeviceCert(void *outDeviceCert, u32 outDeviceCertSize)
{
    ESError rv = ES_ERR_OK;

    if (outDeviceCert == NULL || outDeviceCertSize != ES_DEVICE_CERT_SIZE || sizeof(IOSCEccSignedCert) != ES_DEVICE_CERT_SIZE) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    if ((rv = IOSC_GetDeviceCertificate((IOSCEccSignedCert *) outDeviceCert)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to get device certificate, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

end:
    return rv;
}


ESError
esGetDeviceId(ESDeviceId *outDeviceId)
{
    ESError rv = ES_ERR_OK;

    if (outDeviceId == NULL) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    if ((rv = IOSC_GetData(IOSC_DEV_ID_HANDLE, outDeviceId)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to get device ID, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

end:
    return rv;
}

ES_NAMESPACE_END
