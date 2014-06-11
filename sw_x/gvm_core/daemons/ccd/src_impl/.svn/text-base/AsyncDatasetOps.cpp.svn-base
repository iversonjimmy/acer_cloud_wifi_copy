//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "AsyncDatasetOps.hpp"
#include "ccd_features.h"

#if CCD_ENABLE_DOC_SAVE_N_GO
#include "config.h"
#include "CloudDocMgr.hpp"
#include "ccd_storage.hpp"
#include "EventManagerPb.hpp"
#include "cache.h"

static int DocSaveAndGo_CompletionCb(int change_type,
                                     const char *name,
                                     const char *newname,
                                     u64 modify_time,
                                     int result,
                                     const char *docname,
                                     u64 comp_id,
                                     u64 revision)
{

    if (change_type == 0 || name == NULL) {
        LOG_ERROR("Invalid parameters, change_type = %d, name = %s", change_type, name);
        return -1;
    }

    ccd::CcdiEvent *ep = new ccd::CcdiEvent();
    ccd::EventDocSaveAndGoCompletion *cp = ep->mutable_doc_save_and_go_completion();
    cp->set_change_type((ccd::DocSaveAndGoChangeType)change_type);
    cp->set_file_path_and_name(name);
    if (newname) {
        cp->set_new_file_path_and_name(newname);
    }
    if (modify_time) {
        cp->set_modify_time(modify_time);
    }
    cp->set_result(result);
    if (docname != NULL) {
        cp->set_docname(docname);
    }
    if (comp_id != 0) {
        cp->set_comp_id(comp_id);
    }
    if (revision != 0) {
        cp->set_revision(revision);
    }
    EventManagerPb_AddEvent(ep);
    return 0;
}

static int DocSaveAndGo_EngineStateChangeCb(bool engine_started)
{
    ccd::CcdiEvent *ep = new ccd::CcdiEvent();
    ccd::EventDocSaveAndGoEngineStateChange *cp = ep->mutable_doc_save_and_go_engine_state_change();
    cp->set_engine_started(engine_started);
    EventManagerPb_AddEvent(ep);
    return 0;
}

int ADO_Enable_DocSaveNGo()
{
    int rv = 0;

    {
        // Lock cache to determine current logged-in user.
        CacheAutoLock autoLock;
        rv = autoLock.LockForRead();
        if (rv < 0) {
            LOG_ERROR("Failed to obtain lock");
            goto out;
        }
        CachePlayer* currUser = cache_getUserByPlayerIndex(0);
        if (currUser == NULL) {
            LOG_WARN("No logged-in user; not enabling Cloud Doc.");
            goto out;
        }

        // Enable queue processing.
        rv = DocSNGQueue_Enable(currUser->user_id());
        if (rv != 0) {
            LOG_ERROR("Failed to enable Cloud Doc: %d", rv);
        }
    }

 out:
    return rv;
}

int ADO_Disable_DocSaveNGo(bool userLogout)
{
    int rv = DocSNGQueue_Disable(userLogout);
    if (rv != 0) {
        LOG_ERROR("Failed to disable Cloud Doc: %d", rv);
    }
    return 0;
}

int ADO_Init_DocSaveNGo()
{
    int rv = 0;

    // set path and callback functions
    char path[CCD_PATH_MAX_LENGTH];
    DiskCache::getPathForDocSNG(sizeof(path), path);
    rv = DocSNGQueue_Init(path, DocSaveAndGo_CompletionCb, DocSaveAndGo_EngineStateChangeCb);
    if (rv != 0) {
        LOG_ERROR("Failed to initialize Cloud Doc: %d", rv);
    }

    return rv;
}

int ADO_Release_DocSaveNGo()
{
    int rv = 0;

    rv = DocSNGQueue_Release();
    if (rv != 0) {
        LOG_ERROR("Failed to release Cloud Doc resources: %d", rv);
    }

    return rv;
}

#endif
