//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifndef __CCDI_QUERY_H__
#define __CCDI_QUERY_H__

#include "base.h"
#include "cache.h"

#if CCD_USE_IPC_SOCKET

typedef enum {
    CCDI_CONNECTION_GAME,
    CCDI_CONNECTION_MENU,
    CCDI_CONNECTION_SYSTEM,
} CCDIConnectionType;

typedef struct {
    VPLNamedSocket_t connectedSock;
    CCDIConnectionType requestFrom;
} CCDIQueryHandler2Args;

/// Success or fail, this will close the connectedSock and free(args).
int CCDIQuery_SyncHandler2(CCDIQueryHandler2Args* args);

#endif

#endif  // include guard
