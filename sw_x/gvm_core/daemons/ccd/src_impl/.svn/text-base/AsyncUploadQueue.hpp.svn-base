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

#ifndef __ASYNC_UPLOAD_QUEUE_HPP__
#define __ASYNC_UPLOAD_QUEUE_HPP__

#include <vplu_types.h>
#include <string>
#include <vector>
#include <sqlite3.h>
#include <vpl_th.h>

enum UploadStatusType
{
    // These values are used in the DB, so they cannot be changed without migration.
    pending    = 0,
    processing = 1,
    complete   = 2,
    failed     = 3,
    cancelling = 4,
    cancelled  = 5,
    querying   = 6,
};

class stream_transaction;

class async_upload_queue
{
public:
    async_upload_queue();
    ~async_upload_queue();

    int init(const std::string &rootdir, u64 uid);
    int release();

    /// Enqueue new request for processing.
    /// If running, queue task to actively process tickets.
    int enqueue(u64 uid,
                u64 deviceid,
                u64 sent_so_far,
                u64 size,
                const char *request_content,
                int status,
                u64 &handle);

    /// Dequeue completed ticket (called by ticket destructor)
    int dequeue(stream_transaction& st);

    /// Reset failed transaction. This causes the transaction to be retained in the queue.
    int resetTransaction(stream_transaction* st);

    /// Complete the transaction. This causes the transaction to be removed from the queue.
    int completeTransaction(stream_transaction* st);

    /// Return head-most not-in-progress transaction and mark it in-progress.
    /// Return NULL if no not-in-progress transactions in queue.
    int activateNextTransaction(stream_transaction& st);

    /// Query a transaction in the queue.
    int queryTransaction(stream_transaction& st, u64 id, u64 userId, bool& found);

    /// Enable transaction processing. Queue task to actively process transactions.
    int startProcessing(u64 userId);

    /// Check to see if tickets are actively being processed.
    bool isProcessing();

    /// Check to see if processing is enabled.
    bool isEnabled();

    int checkTransactionAfterEnqueue(); //check the transactions after enqueue

    /// Get transaction count
    int getTransNum() {return trans_num;}
    int refreshTransNum();

    /// Update the upload status
    int updateStatus(stream_transaction& st);
    /// Update only the status column.
    int updateUploadStatus(u64 requestId, UploadStatusType upload_status, u64 userId);
    /// Update only the sent column.
    int updateSentSoFar(u64 requestId, u64 sent_so_far);

    /// Check to see if it is running
    bool isReadyToUse();

    /// Get the status of all async requests in JSON format
    int getJsonTaskStatus(std::string&);

    // Delete all transactions from the queue.
    int deleteAllTrans();

private:
    VPLMutex_t m_mutex;
    std::vector<sqlite3_stmt*> dbstmts;
    bool isRunning; /// Able to process queued tickets
    bool isActive;  /// Actively processing one or more queued tickets
    std::string rootdir;  // root dir for DocSNG
    std::string tmpdir;   // dir to hold temp files
    std::string dbpath;  // path to sqlite file
    sqlite3 *db;  // handle to db
    stream_transaction *activeTransaction;
    u64 currentUserId;
    bool isReady;
    int trans_num;

    int openDB();
    int closeDB();

    int beginTransaction();
    int commitTransaction();
    int rollbackTransaction();
};

#endif // incl guard
