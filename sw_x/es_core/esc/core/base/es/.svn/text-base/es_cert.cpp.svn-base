/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */


#include "es_utils.h"

#include <es.h>
#include <iosc.h>
#include <iosccert.h>

USING_ES_NAMESPACE
USING_ISTORAGE_NAMESPACE

#include <esi.h>

ES_NAMESPACE_START


Certificate::Certificate()
{
    IOSC_Initialize();
}


Certificate::~Certificate()
{
    // Nothing to do here
}


ESError
Certificate::GetNumCertsInList(const void *certList, u32 certListSize,
                               u32 *outNumCerts)
{
    return ESI_ParseCertList(certList, certListSize, NULL, NULL, outNumCerts);
}


ESError
Certificate::ParseCertList(const void *certList, u32 certListSize,
                           void *outCerts[], u32 *outCertSizes,
                           u32 *outNumCerts)
{
    ESError rv = ES_ERR_OK;

    // This case is not checked in ESI_ParseCertList
    if (outCerts == NULL || outCertSizes == NULL) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    rv = ESI_ParseCertList(certList, certListSize, outCerts, outCertSizes, outNumCerts);
    if (rv != ES_ERR_OK) {
        goto end;
    }

end:
    return rv;
}


ESError
Certificate::MergeCerts(const void *certs1[], u32 nCerts1,
                        const void *certs2[], u32 nCerts2,
                        void *outCerts[], u32 *outCertSizes,
                        u32 *outNumCerts)
{
    return ESI_MergeCerts(certs1, nCerts1, certs2, nCerts2, outCerts, outCertSizes, outNumCerts);
}

ES_NAMESPACE_END
