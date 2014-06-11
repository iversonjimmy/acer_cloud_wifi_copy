/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#include "ccd_features.h"

#include <CloudDocMgr.hpp>

#include "stream_transaction.hpp"

#include <string>

struct StorageProxy
{
    int plugin_id;
    int (*init)();
    int (*uninit)();

    int (*readFolder)(stream_transaction *transaction);
    int (*createFolder)();
    int (*deleteFolder)();
    int (*copyFolder)();
    int (*moveFolder)();

    int (*readFile)(stream_transaction *transaction);
    int (*uploadFile)(DocSNGTicket* ticket, std::string);
    int (*downloadFile)(stream_transaction* transaction, std::string folder, std::string filename);
    int (*downloadPreview)(stream_transaction* transaction);
    int (*deleteFile)(stream_transaction* transaction);
    int (*copyFile)();
    int (*moveFile)(stream_transaction* transaction);
};
