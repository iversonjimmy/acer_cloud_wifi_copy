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


#ifndef __ES_TICKET_H__
#define __ES_TICKET_H__


#include "es_utils.h"

ES_NAMESPACE_START

ESError esImportTicket(IInputStream &ticket,
                       const void *certs[], u32 nCerts,
                       IOutputStream &outTicket);

ESError esExportTicket(IInputStream &ticket,
                       const void *certs[], u32 nCerts,
                       IOutputStream &outTicket);

ES_NAMESPACE_END

#endif  // __ES_TICKET_H__
