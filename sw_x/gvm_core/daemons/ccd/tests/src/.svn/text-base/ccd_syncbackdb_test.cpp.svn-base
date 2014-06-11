#include <vpl_types.h>
#include <vplex_file.h>
#include <syncback.hpp>
#include <log.h>

#include <iostream>
#include <sstream>
#include <cassert>

static int onTaskDeleteCb(const SyncBackTask &task)
{
    if (!task.downloadpath.empty()) {
        LOG_INFO("rm -f %s", task.downloadpath.c_str());
    }
    return 0;
}

class Component {
public:
    Component(u64 id, const std::string &name, const std::string &lpath) : id(id), name(name), lpath(lpath) {}
    u64 id;
    std::string name;
    std::string lpath;
};

static void run_test(const char *syncbackdir)
{
    VPLDir_Create(syncbackdir, 0777);

    SyncBackDB db(syncbackdir);

    Component c1(100341, "12345678/a/b/c/d.docx", "/a/b/c/d.docx");
    Component c2(100342, "12345678/x/y/z.docx", "/x/y/z.docx");

    u64 uid = 10001;
    u64 did1 = 10000001;
    u64 did2 = 10000002;
    u64 ts = 0;
    u64 ts_threshold = 0;
    int err;

    // test cb_versions table
    u64 version;
    err = db.GetLastCheckedVersion(uid, did1, version);
    assert(err == 1);  // in a new db, not found, and 1 is returne

    err = db.SetLastCheckedVersion(uid, did1, 32);
    assert(!err);

    err = db.GetLastCheckedVersion(uid, did1, version);
    assert(!err);
    assert(version == 32);

    err = db.SetLastCheckedVersion(uid, did2, 43);
    assert(!err);

    err = db.GetLastCheckedVersion(uid, did2, version);
    assert(!err);
    assert(version == 43);

    // test replacement
    err = db.SetLastCheckedVersion(uid, did1, 33);
    assert(!err);

    err = db.GetLastCheckedVersion(uid, did1, version);
    assert(!err);
    assert(version == 33);


    // set task-delete-callback
    db.SetOnTaskDeleteCallback(onTaskDeleteCb);

    // add tasks
    u64 taskid;
    err = db.AddTask(uid, did1, c1.name, c1.lpath, c1.id, ++ts, taskid);
    assert(!err);
    err = db.AddTask(uid, did1, c2.name, c2.lpath, c2.id, ++ts, taskid);
    assert(!err);
    err = db.AddTask(uid, did1, c1.name, c1.lpath, c1.id, ++ts, taskid);  // test case: replaces existing entry
    assert(!err);

    // process tasks
    SyncBackTask task;

    // advance clock to 100
    ts = ts_threshold = 100;

    // look for a task ready for copyback
    err = db.GetTaskCopybackReadyNoRecentTry(uid, ts_threshold, task);
    assert(err == 1);  // there should be none

    // look for a task ready for download
    err = db.GetTaskDownloadReadyNoRecentTry(uid, ts_threshold, task);
    assert(!err);  // there should be 2
    assert(task.userid == uid);
    assert(task.datasetid == did1);
    assert(task.downloadpath.empty());  // downloadpath should not be set
    {
        // set download path
        std::ostringstream oss;
        oss << "tmp/sb_" << task.id;
        err = db.SetTaskDownloadPath(task.userid, task.datasetid, task.compname, oss.str());
        assert(!err);
    }
    // try download
    err = db.SetTaskDownloadTryTimestamp(task.userid, task.datasetid, task.compname, ++ts);
    assert(!err);
    // download succeeded
    err = db.SetTaskDownloadDoneTimestamp(task.userid, task.datasetid, task.compname, ++ts);
    assert(!err);
    // try copyback
    err = db.SetTaskCopybackTryTimestamp(task.userid, task.datasetid, task.compname, ++ts);
    assert(!err);
    // copy-back succeeded
    err = db.SetTaskCopybackDoneTimestamp(task.userid, task.datasetid, task.compname, ++ts);
    assert(!err);

    // look for another task ready for download
    err = db.GetTaskDownloadReadyNoRecentTry(uid, ts_threshold, task);
    assert(!err);  // there should be one more
    assert(task.userid == uid);
    assert(task.datasetid == did1);
    assert(task.downloadpath.empty());  // downloadpath should not be set
    {
        // set download path
        std::ostringstream oss;
        oss << "tmp/sb_" << task.id;
        err = db.SetTaskDownloadPath(task.userid, task.datasetid, task.compname, oss.str());
        assert(!err);
    }
    // try download
    err = db.SetTaskDownloadTryTimestamp(task.userid, task.datasetid, task.compname, ++ts);
    assert(!err);
    // download failed
    err = db.BumpDownloadFailedCount(task.userid, task.datasetid, task.compname);
    assert(!err);

    // look for another task ready for download
    err = db.GetTaskDownloadReadyNoRecentTry(uid, ts_threshold, task);
    assert(err == 1);  // there should be none

    err = db.PurgeTasksDone();
    assert(!err);

    // advance clock to 200
    ts = ts_threshold = 200;

    // look for a task ready for copyback
    err = db.GetTaskCopybackReadyNoRecentTry(uid, ts_threshold, task);
    assert(err == 1);  // there should be none

    // look for a task ready for download
    err = db.GetTaskDownloadReadyNoRecentTry(uid, ts_threshold, task);
    assert(!err);  // there should be 1
    assert(task.userid == uid);
    assert(task.datasetid == did1);
    assert(!task.downloadpath.empty());  // downloadpath should be set last time
    // try download
    err = db.SetTaskDownloadTryTimestamp(task.userid, task.datasetid, task.compname, ++ts);
    assert(!err);
    // download succeeded
    err = db.SetTaskDownloadDoneTimestamp(task.userid, task.datasetid, task.compname, ++ts);
    assert(!err);
    // try copyback
    err = db.SetTaskCopybackTryTimestamp(task.userid, task.datasetid, task.compname, ++ts);
    assert(!err);
    // copyback failed
    err = db.BumpCopybackFailedCount(task.userid, task.datasetid, task.compname);
    assert(!err);

    // look for another task ready for download
    err = db.GetTaskDownloadReadyNoRecentTry(uid, ts_threshold, task);
    assert(err == 1);  // there should be none

    err = db.PurgeTasksDone();
    assert(!err);


    // advance clock to 300
    ts = ts_threshold = 300;

    // look for a task ready for copyback
    err = db.GetTaskCopybackReadyNoRecentTry(uid, ts_threshold, task);
    assert(!err);  // there should be 1
    assert(task.userid == uid);
    assert(task.datasetid == did1);
    // try copyback
    err = db.SetTaskCopybackTryTimestamp(task.userid, task.datasetid, task.compname, ++ts);
    assert(!err);
    // copy-back ok
    err = db.SetTaskCopybackDoneTimestamp(task.userid, task.datasetid, task.compname, ++ts);
    assert(!err);

    // look for another task tasks ready for copyback
    err = db.GetTaskCopybackReadyNoRecentTry(uid, ts_threshold, task);
    assert(err == 1);  // there should be none

    // look for a task ready for download
    err = db.GetTaskDownloadReadyNoRecentTry(uid, ts_threshold, task);
    assert(err = 1);  // there should be none

    err = db.PurgeTasksDone();
    assert(!err);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << "syncbackdir" << std::endl;
        exit(0);
    }

    run_test(argv[1]);
}
