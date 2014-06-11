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

#include <estypes.h>
#include <istorage.h>

USING_ES_NAMESPACE
USING_ISTORAGE_NAMESPACE

#include "es_container.h"
#include "es_storage.h"
#include "es_ticket.h"


typedef u32 ESTicketOperation;

#define ES_TICKET_OPERATION_IMPORT  1
#define ES_TICKET_OPERATION_EXPORT  2


ES_NAMESPACE_START

static u8 __buf[SIZE_SHA_ALIGN(sizeof(ESTicket) + sizeof(ESV1TicketHeader))]
          ATTR_SHA_ALIGN;


static ESError
__importTicket(IInputStream &ticket, const void *certs[], u32 nCerts,
               IOutputStream &outTicket, ESTicketOperation op)
{
    ESError rv = ES_ERR_OK;
    ESTicketWriter ticketWriter;

    if (certs == NULL || nCerts == 0) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    ticketWriter.ticketDest = &outTicket;
    ticketWriter.isImport = (op == ES_TICKET_OPERATION_IMPORT);

    if ((rv = esVerifyTicket(ticket, certs, nCerts, true, &ticketWriter, NULL, __buf)) != ES_ERR_OK) {
        goto end;
    }

end:
    return rv;
}


ESError
esImportTicket(IInputStream &ticket, const void *certs[], u32 nCerts,
               IOutputStream &outTicket)
{
    return __importTicket(ticket, certs, nCerts, outTicket, ES_TICKET_OPERATION_IMPORT);
}


ESError
esExportTicket(IInputStream &ticket, const void *certs[], u32 nCerts,
               IOutputStream &outTicket)
{
    return __importTicket(ticket, certs, nCerts, outTicket, ES_TICKET_OPERATION_EXPORT);
}

ES_NAMESPACE_END
