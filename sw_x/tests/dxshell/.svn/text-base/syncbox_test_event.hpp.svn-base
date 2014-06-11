//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef __SYNCBOX_TEST_EVENT_H__
#define __SYNCBOX_TEST_EVENT_H__

enum Expected_SyncEvnetSequence {
    SYNC_EVENT_SEQ_SERVER_UP_FILE_CLIENT_LISTEN, // create/modify files in server (Archive Storage)
    SYNC_EVENT_SEQ_CLIENT_UP_FILE_CLIENT_LISTEN, // create/modify files in client
    SYNC_EVENT_SEQ_SERVER_UP_FILE_SERVER_LISTEN, // create/modify files in server (Archive Storage)
    SYNC_EVENT_SEQ_CLIENT_UP_FILE_SERVER_LISTEN, // create/modify files in client

    SYNC_EVENT_SEQ_SERVER_UP_DIR_CLIENT_LISTEN, // create/modify directory in server (Archive Storage)
    SYNC_EVENT_SEQ_CLIENT_UP_DIR_CLIENT_LISTEN, // create/modify directory in client
    SYNC_EVENT_SEQ_SERVER_UP_DIR_SERVER_LISTEN, // create/modify directory in server (Archive Storage)
    SYNC_EVENT_SEQ_CLIENT_UP_DIR_SERVER_LISTEN, // create/modify directory in client

    SYNC_EVENT_SEQ_SERVER_DEL_CLIENT_LISTEN, // delete files/directories in server (Archive Storage)
    SYNC_EVENT_SEQ_CLIENT_DEL_CLIENT_LISTEN, // delete files/directories in client
    SYNC_EVENT_SEQ_SERVER_DEL_SERVER_LISTEN, // delete files/directories in server (Archive Storage)
    SYNC_EVENT_SEQ_CLIENT_DEL_SERVER_LISTEN, // delete files/directories in client

    SYNC_EVENT_SEQ_CLIENT_CONFLICT_CLIENT_LISTEN, // conflict files/directories in client
};

struct syncbox_expected_sync_events {
    // for EventSyncHistory
    std::vector<ccd::SyncEventType_t> sync_event_type;

    // for EventSyncFeatureStatusChange
    std::vector<int> status;
    std::vector<int> uploads_remaining;
    std::vector<int> downloads_remaining;
    std::vector<bool> remote_scan_pending;
    std::vector<bool> scan_in_progress;
};

struct syncbox_event_visitor_ctx {
    VPLSem_t sem;

    // mutex to protect all following members
    VPLMutex_t ctx_mutext;
    int result; // 0: not done yet; 1: done and pass; -1: done but fail
    bool check_extra_event;
    bool consume_all;
    std::vector<Target> *targets;
    ccd::CcdiEvent event;
    u32 count;
    u32 expected_count;
    int instance_num;
    std::string alias;
    EventQueue eq;
    int timeout_sec;
    bool is_run_from_remote;
    struct syncbox_expected_sync_events expected_sync_event_list;

    syncbox_event_visitor_ctx() :
        result(0),
        check_extra_event(false),
        consume_all(false),
        count(0),
        timeout_sec(MAX_TIME_FOR_SYNC),
        is_run_from_remote(false)
    {
        VPLSem_Init(&sem, 1, 0);
        VPLMutex_Init(&ctx_mutext);
    }

    ~syncbox_event_visitor_ctx()
    {
        VPLSem_Destroy(&sem);
        VPL_SET_UNINITIALIZED(&ctx_mutext);
    }
};
#endif // __SYNCBOX_TEST_EVENT_H__
