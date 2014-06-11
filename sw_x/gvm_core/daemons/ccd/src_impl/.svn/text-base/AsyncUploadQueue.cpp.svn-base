/* 
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */
#include "AsyncUploadQueue.hpp"

#include "ccd_storage.hpp"
#include "ccd_util.hpp"
#include "ccdi.hpp"
#include "EventManagerPb.hpp"
#include "stream_transaction.hpp"
#include "util_open_db_handle.hpp"
#include <vssi_error.h>
#include <vplex_file.h>
#include <vplex_http_util.hpp>
#include <sstream>

#define CHECK_PREPARE(rv, rc, db, lbl)					\
if (isSqliteError(rv)) {						\
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to prepare SQL stmt: %d, %s", rc, sqlite3_errmsg(db)); \
    goto lbl;								\
 }

#define CHECK_BIND(rv, rc, db, lbl)					\
if (isSqliteError(rv)) {						\
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to bind value in prepared stmt: %d, %s", rc, sqlite3_errmsg(db)); \
    goto lbl;								\
}

#define CHECK_STEP(rv, rc, db, lbl)					\
if (isSqliteError(rv)) {						\
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to execute prepared stmt: %d, %s", rc, sqlite3_errmsg(db)); \
    goto lbl;								\
}

#define CHECK_FINALIZE(rv, rc, db, lbl)					\
if (isSqliteError(rv)) {						\
    rc = mapSqliteErrCode(rv);                                          \
    LOG_ERROR("Failed to finalize prepared stmt: %d, %s", rc, sqlite3_errmsg(db)); \
    goto lbl;								\
}

#define CHECK_RV(rv, rc, lbl)			\
if (rv != 0) {                                  \
    rc = rv;					\
    goto lbl;					\
}

static bool isSqliteError(int ec)
{
    return ec != SQLITE_OK && ec != SQLITE_ROW && ec != SQLITE_DONE;
}

#define SQLITE_ERROR_BASE 0
static int mapSqliteErrCode(int ec)
{
    return SQLITE_ERROR_BASE - ec;
}

static int AsyncUpload_Completion(u64 id,
                                  u64 uid,
                                  u64 handle,
                                  UploadStatusType upload_status)
{
    ccd::CcdiEvent *ep = new ccd::CcdiEvent();
    ccd::EventAsyncUploadCompletion *cp = ep->mutable_async_upload_completion();
    cp->set_transaction_id(id);
    cp->set_user_id(uid);
    cp->set_handle(handle);
    cp->set_upload_status(upload_status); // DEPRECATED
    ccd::AsyncUploadState_t state;
    int errorcode = 0;
    switch (upload_status) {
    case pending:
        state = ccd::ASYNC_UPLOAD_STATE_WAIT;
        break;
    case querying:
    case processing:
    case cancelling:
        state = ccd::ASYNC_UPLOAD_STATE_ACTIVE;
        break;
    case complete:
        state = ccd::ASYNC_UPLOAD_STATE_DONE;
        break;
    case cancelled:
        state = ccd::ASYNC_UPLOAD_STATE_ERROR;
        errorcode = VSSI_ABORTED;
        break;
    case failed:
    default:
        state = ccd::ASYNC_UPLOAD_STATE_ERROR;
        errorcode = -1;  // FIXME - do we have the exact error code saved someplace?
        break;
    }
    cp->set_state(state);
    cp->set_error_code(errorcode);
    EventManagerPb_AddEvent(ep);
    // ep will be freed by EventManagerPb.
    return 0;
}

static const char test_and_create_db_sql[] = 
"CREATE TABLE IF NOT EXISTS async_upload ("
    "id INTEGER PRIMARY KEY, "
    "uid INTEGER NOT NULL, "
    "deviceid INTEGER NOT NULL, "
    "sent INTEGER NOT NULL, "
    "size INTEGER NOT NULL, "
    "request_content TEXT NOT NULL, "
    "status INTEGER NOT NULL, "
    "handle INTEGER NOT NULL)";

#define DEFSQL(tag, def) static const char sql_ ## tag [] = def
#define SQLNUM(tag) DSNG_STMT_ ## tag
#define GETSQL(tag) sql_ ## tag

enum {
    DSNG_STMT_BEGIN,
    DSNG_STMT_COMMIT,
    DSNG_STMT_ROLLBACK,
    DSNG_STMT_INSERT_TRANS,
    DSNG_STMT_UPDATE_STATUS,
    DSNG_STMT_UPDATE_UPLOAD_STATUS,
    DSNG_STMT_UPDATE_SENT_SO_FAR,
    DSNG_STMT_DELETE_TRANS,
    DSNG_STMT_SELECT_OLDEST_TRANS,
    DSNG_STMT_SELECT_TRANS_IN_ORDER,
    DSNG_STMT_SELECT_TRANS_COUNT,
    DSNG_STMT_DELETE_ALL_TRANS,
    DSNG_STMT_MAX, // this must be last
};

DEFSQL(BEGIN,                                   \
       "BEGIN IMMEDIATE");
DEFSQL(COMMIT,                                  \
       "COMMIT");
DEFSQL(ROLLBACK,                                \
       "ROLLBACK");
DEFSQL(INSERT_TRANS,                                                  \
       "INSERT INTO async_upload (uid,deviceid, sent, size, request_content, status, handle)"  \
       "VALUES (:uid, :deviceid, :sent, :size, :request_content, :status, :handle)");
DEFSQL(UPDATE_STATUS,                           \
       "UPDATE async_upload "                   \
       "SET status=:status, sent=:sent "        \
       "WHERE id=:id");
DEFSQL(UPDATE_UPLOAD_STATUS,                    \
       "UPDATE async_upload "                   \
       "SET status=:status "                    \
       "WHERE id=:id");
DEFSQL(UPDATE_SENT_SO_FAR,                      \
       "UPDATE async_upload "                   \
       "SET sent=:sent "                        \
       "WHERE id=:id");
DEFSQL(DELETE_TRANS,                          \
       "DELETE FROM async_upload "                   \
       "WHERE id=:id");
DEFSQL(SELECT_OLDEST_TRANS,                                           \
       "SELECT id, uid, deviceid, sent, size, request_content, status "      \
       "FROM async_upload "                                                  \
       "WHERE uid=:uid AND (status=:status OR status=:status1 OR status=:status2)"    \
       "ORDER BY id LIMIT 1");
DEFSQL(SELECT_TRANS_IN_ORDER,                \
       "SELECT id, uid, deviceid, sent, size, request_content, status "  \
       "FROM async_upload "                          \
       "WHERE uid=:uid "                        \
       "ORDER BY id");
DEFSQL(SELECT_TRANS_COUNT,                                      \
       "SELECT count(*) " \
       "FROM async_upload "                                                  \
       "WHERE uid=:uid AND (status=:status OR status=:status1 OR status=:status2)");
DEFSQL(DELETE_ALL_TRANS,                        \
       "DELETE FROM async_upload");

// async_upload_queue
async_upload_queue::async_upload_queue()
    : isRunning(false),
    isActive(false),
    db(NULL),
    activeTransaction(false),
    currentUserId(0),
    isReady(false),
    trans_num(0)
{
}

async_upload_queue::~async_upload_queue()
{
    release();
}

int async_upload_queue::init(const std::string &rootdir, u64 uid) 
{
    int rv = 0;

    if(isReady) {
        LOG_ERROR("Already initialized");
        return rv;
    }

    VPLMutex_Init(&m_mutex);

    this->rootdir = rootdir;
    dbpath = rootdir + "db";

    VPLDir_Create(rootdir.c_str(), 0777);

    LOG_INFO("rootdir=%s", rootdir.c_str());
    LOG_INFO("dbpath=%s", dbpath.c_str());

    currentUserId = uid;

    isReady = true;

    return rv;
}

int async_upload_queue::release()
{
    if (isReady) {
        if (db)
            closeDB();
        VPLMutex_Destroy(&m_mutex);
        isReady = false;
    }
    return 0;
}

int async_upload_queue::openDB()
{
    int rv = 0;
    char *errmsg = NULL;

    rv = Util_OpenDbHandle(dbpath.c_str(), true, true, &db);
    if (rv != 0) {
        LOG_ERROR("Util_OpenDbHandle(%s):%d",
                  dbpath.c_str(), rv);
        goto end;
    }

    rv = sqlite3_exec(db, test_and_create_db_sql, NULL, NULL, &errmsg);
    if (rv != SQLITE_OK) {
        LOG_ERROR("Failed to create tables: %d, %s", rv, errmsg);
        sqlite3_free(errmsg);
        goto end;
    }

    LOG_INFO("Opened DB");

    dbstmts.resize(DSNG_STMT_MAX, NULL);

 end:
    return rv;
}

int async_upload_queue::closeDB()
{
    int rv = 0;

    std::vector<sqlite3_stmt*>::iterator it;
    for (it = dbstmts.begin(); it != dbstmts.end(); it++) {
        if (*it != NULL) {
            sqlite3_finalize(*it);
        }
        *it = NULL;
    }

    sqlite3_close(db);
    db = NULL;

    LOG_INFO("Closed DB");

    dbstmts.clear();

    return rv;
}

int async_upload_queue::beginTransaction()
{
    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(BEGIN)];
    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(BEGIN), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return rc;
}

int async_upload_queue::commitTransaction()
{
    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(COMMIT)];
    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(COMMIT), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return rc;
}

int async_upload_queue::rollbackTransaction()
{
    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(ROLLBACK)];
    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(ROLLBACK), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }
    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);
 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    return rc;
}

int async_upload_queue::enqueue(u64 uid,
                         u64 deviceid,
                         u64 sent,
                         u64 size,
                         const char *request_content,
                         int status,
                         u64 &handle)
{
    if (!isReady)
        return -1;

    int rc = 0;
    int rv;
    u64 id = -1;

    VPLMutex_Lock(&m_mutex);

    if (!db) {
        openDB();
    }
    sqlite3_stmt *&stmt_ins = dbstmts[SQLNUM(INSERT_TRANS)];

    rv = beginTransaction();
    CHECK_RV(rv, rc, end);

    if (!stmt_ins) {
        rv = sqlite3_prepare_v2(db, GETSQL(INSERT_TRANS), -1, &stmt_ins, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }
    // bind :uid
    rv = sqlite3_bind_int64(stmt_ins, 1, uid);
    CHECK_BIND(rv, rc, db, end);
    // bind :deviceid
    rv = sqlite3_bind_int64(stmt_ins, 2, deviceid);
    CHECK_BIND(rv, rc, db, end);
    // bind :sent_so_far
    rv = sqlite3_bind_int64(stmt_ins, 3, sent);
    CHECK_BIND(rv, rc, db, end);
    // bind :size
    rv = sqlite3_bind_int64(stmt_ins, 4, size);
    CHECK_BIND(rv, rc, db, end);
    // bind :request_content
    rv = sqlite3_bind_text(stmt_ins, 5, request_content, -1, NULL);
    CHECK_BIND(rv, rc, db, end);
    // bind :status
    rv = sqlite3_bind_int(stmt_ins, 6, status);
    CHECK_BIND(rv, rc, db, end);
    // bind :handle
    rv = sqlite3_bind_int(stmt_ins, 7, status);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt_ins);
    CHECK_STEP(rv, rc, db, end);

    id = sqlite3_last_insert_rowid(db);

    rv = commitTransaction();
    CHECK_RV(rv, rc, end);

    UTIL_LOG_SENSITIVE_STRING_INFO("Added async upload:\n", request_content, "");

 end:
    if (rc) {
        rollbackTransaction();
    }
    else {
        handle = id;
    }

    if (stmt_ins) {
        sqlite3_reset(stmt_ins);
    }

    VPLMutex_Unlock(&m_mutex);

    return rc;
}

int async_upload_queue::dequeue(stream_transaction& st)
{
    if (!isReady)
        return -1;

    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_TRANS)];

    VPLMutex_Lock(&m_mutex);

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_TRANS), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :id
    rv = sqlite3_bind_int64(stmt, 1, st.id);
    CHECK_BIND(rv, rc, db, dbcleanup);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, dbcleanup);

    if (sqlite3_changes(db) == 0) {
        // no rows deleted; this is unexpected.  emit warning
        LOG_WARN("Ticket ID "FMTu64" not found in queue", st.id);
    }
    else {
        LOG_INFO("Removed transaction ID "FMTu64, st.id);
    }

 dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    activeTransaction = NULL;

    VPLMutex_Unlock(&m_mutex);

    return rc;
}

// this function is called if the transaction could not be processed
int async_upload_queue::resetTransaction(stream_transaction *st)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rv = 0;

    //default:
        rv = dequeue(*st);
    //}

    delete st;

    return rv;
}

// this function is called if the transaction was processed
int async_upload_queue::completeTransaction(stream_transaction *st)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    int rv = dequeue(*st);

    delete st;

    activateNextTransaction(*st);

    return rv;
}

int async_upload_queue::activateNextTransaction(stream_transaction& st)
{
    VPLMutex_Lock(&m_mutex);

    if (!db) {
        openDB();
    }

    int rc = 0;
    if (isRunning && !activeTransaction) {
        int rv;
        sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_OLDEST_TRANS)];
        if (!stmt) {
            rv = sqlite3_prepare_v2(db, GETSQL(SELECT_OLDEST_TRANS), -1, &stmt, NULL);
            CHECK_PREPARE(rv, rc, db, dbcleanup);
        }

        // activate transaction that's still pending, processing or querying
        // if transaction is in "proccessing" or "pending", it means that the transaction is
        // actually resumed (StopCCD/StartCCD)

        // bind :uid
        rv = sqlite3_bind_int64(stmt, 1, currentUserId);
        CHECK_BIND(rv, rc, db, dbcleanup);

        // bind :status pending
        rv = sqlite3_bind_int(stmt, 2, (int)pending);
        CHECK_BIND(rv, rc, db, dbcleanup);

        // bind :status processing
        rv = sqlite3_bind_int(stmt, 3, (int)processing);
        CHECK_BIND(rv, rc, db, dbcleanup);

        // bind :status querying
        rv = sqlite3_bind_int(stmt, 4, (int)querying);
        CHECK_BIND(rv, rc, db, dbcleanup);

        rv = sqlite3_step(stmt);
        CHECK_STEP(rv, rc, db, dbcleanup);

        if (rv == SQLITE_ROW) {
            st.id = sqlite3_column_int64(stmt, 0);  // id
            st.uid = sqlite3_column_int64(stmt, 1);  // uid
            st.deviceid = sqlite3_column_int64(stmt, 2);  // deviceid
            st.sent_so_far = sqlite3_column_int64(stmt, 3); // sent
            st.size = sqlite3_column_int64(stmt, 4); // size
            st.request_content = (const char*)sqlite3_column_text(stmt, 5);  // request_content
            st.upload_status = (UploadStatusType)sqlite3_column_int(stmt, 6);  // upload_status
        }

    dbcleanup:
        if (stmt) {
            sqlite3_reset(stmt);
        }
    }

    if (!activeTransaction && !(&st)) {  // idle - close DB
        closeDB();

        if (isActive) {
            // Notify EventManagerPb that queue processing has gone to sleep.
            //(*engineStateChangeCb)(false);
        }
        isActive = false;
    }

    if (&st) {
        LOG_INFO("Change request "FMTu64" status to querying.", st.id);
        // Reset sent_so_far for it might be a resumed transaction
        // XXX this asummption is incorrect if we are able to resume the transaction
        // at the offset we stopped. But this is not true for now
        st.sent_so_far = 0;
        st.upload_status = querying;
        if (!isActive) {
            // Notify EventManagerPb that queue processing has resumed.
            //(*engineStateChangeCb)(true);
        }
        isActive = true;
    }

    VPLMutex_Unlock(&m_mutex);

    return rc;
}

static void parse_filepath(const std::string content, std::string& path, std::string& filepath) {
    size_t start = 0;
    size_t end = 0;

    if ((start = content.find_first_of(" ")) != std::string::npos &&
        (end = content.find(" ", start+1)) != std::string::npos) {
        // search for the path
        int count = 0;
        size_t uri_start = start+1; // start at first "/"
        size_t uri_end = content.find_first_of("/? ", uri_start);
        while (uri_end != std::string::npos && uri_end <= end && count < 4 && content[end] != '?') {
            uri_start = uri_end + 1;
            uri_end = content.find_first_of("/? ", uri_start);
            count++;
        }
        int err = VPLHttp_DecodeUri(content.substr(uri_start,uri_end-uri_start), path);
        if (err) {
            LOG_ERROR("failed to decode");
            path = "NA";
        }
    } else {
        // XXX shouldn't happen
        LOG_ERROR("dst filepath not found!");
        path = "NA";
    }
    if ((start = content.find("x-ac-srcfile:")) != std::string::npos &&
        (start = content.find_first_of(" ", start+1)) != std::string::npos &&
        (end = content.find_first_of("\r\n", start+1)) != std::string::npos) {
        filepath = content.substr(start+1, end-start-1);
    } else {
        // XXX shouldn't happen
        LOG_ERROR("src filepath not found!");
        filepath = "NA";
    }
}

int async_upload_queue::getJsonTaskStatus(std::string& out)
{
    int rc = 0;
    int rv = 0;
    int count = 0;
    bool addComma = false;
    std::string path, filepath;
    std::ostringstream oss;

    VPLMutex_Lock(&m_mutex);

    if (!db) {
        openDB();
    }
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_TRANS_IN_ORDER)];

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_TRANS_IN_ORDER), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :uid
    rv = sqlite3_bind_int64(stmt, 1, currentUserId);
    CHECK_BIND(rv, rc, db, dbcleanup);

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (addComma) {
            oss << ",";
        }
        oss << "{\"id\":"<< sqlite3_column_int64(stmt, 0);
        oss << ",\"datasetId\":"<< sqlite3_column_int64(stmt, 1);
        oss << ",\"op\":"<< "\"upload\""; // FIXME or download
        parse_filepath((const char*)sqlite3_column_text(stmt, 5), path, filepath);
        oss << ",\"path\":\""<< path << "\"";
        oss << ",\"filepath\":\""<< filepath << "\"";
        oss << "}";
        count++;
        addComma = true;
    }
    CHECK_STEP(rv, rc, db, dbcleanup);

dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }
    VPLMutex_Unlock(&m_mutex);

    oss << "],\"numOfRequests\":" << count << "}";
    out.assign("{\"requestList\":[" + oss.str());

    return rc;
}

int async_upload_queue::queryTransaction(stream_transaction& st, u64 id, u64 userId, bool& found)
{
    int rc = 0;
    int rv = 0;
    found = false;

    VPLMutex_Lock(&m_mutex);

    if (!db) {
        openDB();
    }
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_TRANS_IN_ORDER)];

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_TRANS_IN_ORDER), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :uid
    rv = sqlite3_bind_int64(stmt, 1, userId);
    CHECK_BIND(rv, rc, db, dbcleanup);

    while ((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        if(sqlite3_column_int64(stmt, 0) != id) {
            continue;
        }
        st.id = sqlite3_column_int64(stmt, 0);  // id
        st.uid = sqlite3_column_int64(stmt, 1);  // uid
        st.deviceid = sqlite3_column_int64(stmt, 2); // did
        st.sent_so_far = sqlite3_column_int64(stmt, 3);  // sent_so_far
        st.size = sqlite3_column_int64(stmt, 4);  // size
        st.request_content = (const char*)sqlite3_column_text(stmt, 5);  // size
        st.upload_status = (UploadStatusType)sqlite3_column_int(stmt, 6);  // size
        found = true;
        break;
    }
    CHECK_STEP(rv, rc, db, dbcleanup);

dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&m_mutex);
    return rc;
}

bool async_upload_queue::isEnabled()
{
    if (!isReady)
        return false;

    int rc = 0;
    int rv;
    bool result = false;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_TRANS_IN_ORDER)];

    VPLMutex_Lock(&m_mutex);

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_TRANS_IN_ORDER), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    // bind :uid
    rv = sqlite3_bind_int64(stmt, 1, currentUserId);
    CHECK_BIND(rv, rc, db, dbcleanup);

    if((rv = sqlite3_step(stmt)) == SQLITE_ROW)
        result = true;

 dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&m_mutex);

    return result;
}

bool async_upload_queue::isReadyToUse()
{
    return (isReady&&isRunning);
}

int async_upload_queue::startProcessing(u64 userId)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    VPLMutex_Lock(&m_mutex);
    if (!isRunning) {
        isRunning = true;
        currentUserId = userId;
    }
    VPLMutex_Unlock(&m_mutex);

    return 0;
}

int async_upload_queue::refreshTransNum()
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    if (!db) {
        openDB();
    }

    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(SELECT_TRANS_COUNT)];

    VPLMutex_Lock(&m_mutex);

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(SELECT_TRANS_COUNT), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }


    // find every transactions that's still pending, processing or querying
    // and set the first match ID as trans_num.
    // if transaction is in "proccessing" or "pending", it means that the transaction is
    // actually resumed (StopCCD/StartCCD)

    // bind :uid
    rv = sqlite3_bind_int64(stmt, 1, currentUserId);
    CHECK_BIND(rv, rc, db, dbcleanup);

    // bind :status pending
    rv = sqlite3_bind_int(stmt, 2, (int)pending);
    CHECK_BIND(rv, rc, db, dbcleanup);

    // bind :status processing
    rv = sqlite3_bind_int(stmt, 3, (int)processing);
    CHECK_BIND(rv, rc, db, dbcleanup);

    // bind :status querying
    rv = sqlite3_bind_int(stmt, 4, (int)querying);
    CHECK_BIND(rv, rc, db, dbcleanup);


    if((rv = sqlite3_step(stmt)) == SQLITE_ROW) {
        trans_num = sqlite3_column_int(stmt, 0);
    }

 dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&m_mutex);

    return rc;
}

int async_upload_queue::updateStatus(stream_transaction& st)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    if (!db) {
        openDB();
    }

    int rc = 0;
    int rv;
    u64 handle = 0;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(UPDATE_STATUS)];

    VPLMutex_Lock(&m_mutex);

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(UPDATE_STATUS), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind status
    rv = sqlite3_bind_int(stmt, 1, (int)st.upload_status);
    CHECK_BIND(rv, rc, db, end);
    // bind status
    rv = sqlite3_bind_int64(stmt, 2, st.sent_so_far);
    CHECK_BIND(rv, rc, db, end);
    // bind :uid
    rv = sqlite3_bind_int64(stmt, 3, st.id);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&m_mutex);

    if(st.upload_status == complete || st.upload_status == failed) {
        // Use id as handle for now
        handle = st.id;
        // Notify EventManagerPb of completion or failure.
        AsyncUpload_Completion(st.id, st.uid, handle, st.upload_status);
    }

    return rc;
}

int async_upload_queue::updateUploadStatus(u64 requestId, UploadStatusType upload_status, u64 userId)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    if (!db) {
        openDB();
    }

    int rc = 0;
    int rv;
    u64 handle = 0;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(UPDATE_UPLOAD_STATUS)];

    VPLMutex_Lock(&m_mutex);

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(UPDATE_UPLOAD_STATUS), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :status
    rv = sqlite3_bind_int(stmt, 1, (int)upload_status);
    CHECK_BIND(rv, rc, db, end);
    // bind :id
    rv = sqlite3_bind_int64(stmt, 2, requestId);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&m_mutex);

    if (upload_status == complete || upload_status == failed) {
        // Use requestsId as handle for now
        handle = requestId;
        // Notify EventManagerPb of completion or failure.
        AsyncUpload_Completion(requestId, userId, handle, upload_status);
    }

    return rc;
}

int async_upload_queue::updateSentSoFar(u64 requestId, u64 sent_so_far)
{
    if (!isReady)
        return CCD_ERROR_NOT_INIT;

    if (!db) {
        openDB();
    }

    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(UPDATE_SENT_SO_FAR)];

    VPLMutex_Lock(&m_mutex);

    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(UPDATE_SENT_SO_FAR), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, end);
    }

    // bind :sent
    rv = sqlite3_bind_int64(stmt, 1, sent_so_far);
    CHECK_BIND(rv, rc, db, end);
    // bind :id
    rv = sqlite3_bind_int64(stmt, 2, requestId);
    CHECK_BIND(rv, rc, db, end);

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, end);

 end:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&m_mutex);

    return rc;
}

int async_upload_queue::deleteAllTrans()
{
    int rc = 0;
    int rv;
    sqlite3_stmt *&stmt = dbstmts[SQLNUM(DELETE_ALL_TRANS)];

    VPLMutex_Lock(&m_mutex);

    if (!db) {
        openDB();
    }
    if (!stmt) {
        rv = sqlite3_prepare_v2(db, GETSQL(DELETE_ALL_TRANS), -1, &stmt, NULL);
        CHECK_PREPARE(rv, rc, db, dbcleanup);
    }

    rv = sqlite3_step(stmt);
    CHECK_STEP(rv, rc, db, dbcleanup);

 dbcleanup:
    if (stmt) {
        sqlite3_reset(stmt);
    }

    VPLMutex_Unlock(&m_mutex);

    return rc;
}
