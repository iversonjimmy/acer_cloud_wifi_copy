#include <vpl_types.h>
#include <vplu_format.h>
#include <vplex_file.h>
#include <gvm_file_utils.h>
#include <gvm_errors.h>
#include <log.h>

#include "SyncDown_private.hpp"

#include <iostream>
#include <sstream>
#include <cassert>

struct TestJob {
    u64 datasetid;
    std::string comppath;
    u64 compid;
    std::string localpath;
    u64 syncfeature;
    u64 rev;
};

#define ADDJOB(tj,jobid) AddJob(tj.datasetid, tj.comppath, tj.compid, tj.localpath, tj.syncfeature, tj.rev, jobid)

class TestSyncDownJobs {
public:
    TestSyncDownJobs(const std::string &workdir);
    void RunTests();
private:
    std::string workdir;
    void testDatasetVersions(const std::string &workdir);
    void testAddJobs(const std::string &workdir);
    void testRemoveJobs(const std::string &workdir);
    void testDownloadPath(const std::string &workdir);
    void testDownloadedRevision(const std::string &workdir);
    void testTimestamps(const std::string &workdir);
    void testFailedCounts(const std::string &workdir);
    void testGetNextJobs(const std::string &workdir);
    void testVisitJobs(const std::string &workdir);

    bool isSameJob(const SyncDownJob &job, const TestJob &tj);
};

TestSyncDownJobs::TestSyncDownJobs(const std::string &workdir)
    : workdir(workdir)
{
}

void TestSyncDownJobs::testDatasetVersions(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "/tmp";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncDownJobs sdj(workdir);

    int err = 0;
    u64 datasetId = 100;
    u64 version;

    err = sdj.GetDatasetLastCheckedVersion(datasetId, version);
    assert(err = CCD_ERROR_NOT_FOUND);

    err = sdj.SetDatasetLastCheckedVersion(datasetId, 10);
    assert(!err);
    err = sdj.GetDatasetLastCheckedVersion(datasetId, version);
    assert(!err);
    assert(version == 10);

    err = sdj.SetDatasetLastCheckedVersion(datasetId, 11);
    assert(!err);
    err = sdj.GetDatasetLastCheckedVersion(datasetId, version);
    assert(!err);
    assert(version == 11);

    err = sdj.ResetDatasetLastCheckedVersion(datasetId);
    assert(!err);
    err = sdj.GetDatasetLastCheckedVersion(datasetId, version);
    assert(err == CCD_ERROR_NOT_FOUND);
}

void TestSyncDownJobs::testAddJobs(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "/tmp";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncDownJobs sdj(workdir);

    TestJob tj1 = { 100, "comp1/path1/file1", 1000,
                    "/home/build/file1", 60,1 };
    TestJob tj2a = { 100, "comp1/path1/file2a", 2000,
                    "/home/build/file2", 50,1 };
    TestJob tj2b = { 100, "comp1/path1/file2b", 2001,
                    "/home/build/file2", 50,1 };

    u64 jobid;
    SyncDownJob job;
    int err;

    err = sdj.ADDJOB(tj1, jobid);
    assert(!err);
    err = sdj.findJob(jobid, job);
    assert(!err);
    assert(job.id == jobid);
    assert(isSameJob(job, tj1));
    err = sdj.findJob(tj1.localpath, job);
    assert(!err);
    assert(job.id == jobid);
    assert(isSameJob(job, tj1));

    err = sdj.ADDJOB(tj2a, jobid);
    assert(!err);
    err = sdj.findJob(jobid, job);
    assert(!err);
    assert(job.id == jobid);
    assert(isSameJob(job, tj2a));
    err = sdj.findJob(tj2a.localpath, job);
    assert(!err);
    assert(job.id == jobid);
    assert(isSameJob(job, tj2a));

    err = sdj.ADDJOB(tj2b, jobid);
    assert(!err);
    err = sdj.findJob(jobid, job);
    assert(!err);
    assert(job.id == jobid);
    assert(isSameJob(job, tj2b));
    err = sdj.findJob(tj2b.localpath, job);
    assert(!err);
    assert(job.id == jobid);
    assert(isSameJob(job, tj2b));
}

void TestSyncDownJobs::testRemoveJobs(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "/tmp";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncDownJobs sdj(workdir);

    TestJob tj1 = { 100, "comp1/path1/file1", 1000,
                    "/home/build/file1", 60,1 };
    TestJob tj2 = { 100, "comp1/path1/file2a", 2000,
                    "/home/build/file2", 50,1 };

    u64 jobid1, jobid2;
    int err;

    err = sdj.ADDJOB(tj1, jobid1);
    assert(!err);

    err = sdj.ADDJOB(tj2, jobid2);
    assert(!err);

    SyncDownJob job;

    err = sdj.RemoveJob(jobid1);
    assert(!err);
    err = sdj.findJob(jobid1, job);
    assert(err == CCD_ERROR_NOT_FOUND);
    err = sdj.findJob(tj1.localpath, job);
    assert(err == CCD_ERROR_NOT_FOUND);

    err = sdj.RemoveJob(jobid2);
    assert(!err);
    err = sdj.findJob(jobid2, job);
    assert(err == CCD_ERROR_NOT_FOUND);
    err = sdj.findJob(tj2.localpath, job);
    assert(err == CCD_ERROR_NOT_FOUND);
}

void TestSyncDownJobs::testDownloadPath(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "/tmp";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncDownJobs sdj(workdir);

    TestJob tj1 = { 100, "comp1/path1/file1", 1000,
                    "/home/build/file1", 60,1 };

    int err;

    u64 jobid1;
    err = sdj.ADDJOB(tj1, jobid1);
    assert(!err);

    SyncDownJob job;
    err = sdj.findJob(jobid1, job);
    assert(!err);
    assert(job.dl_path.empty());

    std::string dl_path = "dl/file1";
    err = sdj.SetJobDownloadPath(jobid1, dl_path);
    assert(!err);

    err = sdj.findJob(jobid1, job);
    assert(!err);
    assert(job.dl_path == dl_path);
}

void TestSyncDownJobs::testDownloadedRevision(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "/tmp";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncDownJobs sdj(workdir);

    TestJob tj1 = { 100, "comp1/path1/file1", 1000,
                    "/home/build/file1", 60,1 };

    int err;

    u64 jobid1;
    err = sdj.ADDJOB(tj1, jobid1);
    assert(!err);

    SyncDownJob job;
    err = sdj.findJob(jobid1, job);
    assert(!err);
    assert(job.dl_rev == 0);

    u64 dl_rev = 123;
    err = sdj.SetJobDownloadedRevision(jobid1, dl_rev);
    assert(!err);

    err = sdj.findJob(jobid1, job);
    assert(!err);
    assert(job.dl_rev == dl_rev);
}

void TestSyncDownJobs::testTimestamps(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "/tmp";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncDownJobs sdj(workdir);

    TestJob tj1 = { 100, "comp1/path1/file1", 1000,
                    "/home/build/file1", 60,1 };

    int err;

    u64 jobid;
    err = sdj.ADDJOB(tj1, jobid);
    assert(!err);

    u64 timestamp = 12345;
    SyncDownJobEx jobex;
    err = sdj.setJobDispatchTime(jobid, timestamp);
    assert(!err);
    err = sdj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.disp_ts == timestamp);

    timestamp++;
    err = sdj.setJobTryDownloadTime(jobid, timestamp);
    assert(!err);
    err = sdj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.dl_try_ts == timestamp);

    timestamp++;
    err = sdj.setJobDoneDownloadTime(jobid, timestamp);
    assert(!err);
    err = sdj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.dl_done_ts == timestamp);

    timestamp++;
    err = sdj.setJobTryCopybackTime(jobid, timestamp);
    assert(!err);
    err = sdj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.cb_try_ts == timestamp);

    timestamp++;
    err = sdj.setJobDoneCopybackTime(jobid, timestamp);
    assert(!err);
    err = sdj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.cb_done_ts == timestamp);
}

void TestSyncDownJobs::testFailedCounts(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "/tmp";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncDownJobs sdj(workdir);

    TestJob tj1 = { 100, "comp1/path1/file1", 1000,
                    "/home/build/file1", 60,1 };

    int err;

    u64 jobid;
    err = sdj.ADDJOB(tj1, jobid);
    assert(!err);

    SyncDownJobEx jobex;
    err = sdj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.dl_failed == 0);
    assert(jobex.cb_failed == 0);

    u64 failed_download_count = 0;

    err = sdj.incJobDownloadFailedCount(jobid);
    assert(!err);
    failed_download_count++;
    err = sdj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.dl_failed == failed_download_count);

    err = sdj.incJobDownloadFailedCount(jobid);
    assert(!err);
    failed_download_count++;
    err = sdj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.dl_failed == failed_download_count);

    u64 failed_copyback_count = 0;

    err = sdj.incJobCopybackFailedCount(jobid);
    assert(!err);
    failed_copyback_count++;
    err = sdj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.cb_failed == failed_copyback_count);

    err = sdj.incJobCopybackFailedCount(jobid);
    assert(!err);
    failed_copyback_count++;
    err = sdj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.cb_failed == failed_copyback_count);
}

void TestSyncDownJobs::testGetNextJobs(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "/tmp";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncDownJobs sdj(workdir);

    TestJob tj1 = { 100, "comp1/path1/file1", 1000,
                    "/home/build/file1", 60,1 };

    int err;

    u64 jobid;
    err = sdj.ADDJOB(tj1, jobid);
    assert(!err);

    VPLThread_Sleep(VPLTime_FromSec(1));

    u64 threshold = VPLTime_ToSec(VPLTime_GetTime());
    SyncDownJob job;
    err = sdj.GetNextJobToDownload(threshold, job);
    assert(!err);
    assert(job.id == jobid);

    // mark the jobs as dispatched
    u64 currVplTimeSec = VPLTime_ToSec(VPLTime_GetTime());
    err = sdj.TimestampJobDispatch(jobid, currVplTimeSec);
    assert(!err);

    // make sure it is not found, because it is already dispatched wrt current threshold
    err = sdj.GetNextJobToDownload(threshold, job);
    assert(err == CCD_ERROR_NOT_FOUND);

    VPLThread_Sleep(VPLTime_FromSec(1));

    threshold = VPLTime_ToSec(VPLTime_GetTime());
    // make sure it is found, because the threshold is now later
    err = sdj.GetNextJobToDownload(threshold, job);
    assert(!err);
    assert(job.id == jobid);

    // mark the job done download
    err = sdj.TimestampJobDoneDownload(jobid);
    assert(!err);

    // make sure it is not found, because the job is done
    err = sdj.GetNextJobToDownload(threshold, job);
    assert(err == CCD_ERROR_NOT_FOUND);

    // make sure the job can be found to copyback
    err = sdj.GetNextJobToCopyback(threshold, job);
    assert(!err);
    assert(job.id == jobid);

    // mark the jobs as dispatched
    currVplTimeSec = VPLTime_ToSec(VPLTime_GetTime());
    err = sdj.TimestampJobDispatch(jobid, currVplTimeSec);
    assert(!err);

    // make sure it is not found, because it is already dispatched wrt current threshold
    err = sdj.GetNextJobToCopyback(threshold, job);
    assert(err == CCD_ERROR_NOT_FOUND);

    VPLThread_Sleep(VPLTime_FromSec(1));

    threshold = VPLTime_ToSec(VPLTime_GetTime());
    // make sure it is found, because the threshold is now later
    err = sdj.GetNextJobToCopyback(threshold, job);
    assert(!err);
    assert(job.id == jobid);

    // mark the job done copyback
    err = sdj.TimestampJobDoneCopyback(jobid);
    assert(!err);

    // make sure it is not found, because the job is done
    err = sdj.GetNextJobToCopyback(threshold, job);
    assert(err == CCD_ERROR_NOT_FOUND);
}

static int jobVisitor(SyncDownJob &job, void *param)
{
    int *counter = (int*)param;
    (*counter)++;
    return 0;
}

void TestSyncDownJobs::testVisitJobs(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "/tmp";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncDownJobs sdj(workdir);

    TestJob tj1 = { 100, "comp1/path1/file1", 1000,
                    "/home/build/file1", 60,1 };
    TestJob tj2 = { 200, "comp2/path2/file2", 2000,
                    "/home/build/file2", 50,1 };

    u64 jobid1, jobid2;
    SyncDownJob job;
    int err;

    err = sdj.ADDJOB(tj1, jobid1);
    assert(!err);
    err = sdj.ADDJOB(tj2, jobid2);
    assert(!err);

    int counter = 0;
    err = sdj.visitJobs(jobVisitor, (void*)&counter);
    assert(!err);
    assert(counter == 2);

    counter = 0;
    err = sdj.visitJobsByDatasetId(100, false, jobVisitor, (void*)&counter);
    assert(!err);
    assert(counter == 1);
}

bool TestSyncDownJobs::isSameJob(const SyncDownJob &job, const TestJob &tj)
{
    return job.did == tj.datasetid &&
        job.cpath == tj.comppath &&
        job.cid == tj.compid &&
        job.lpath == tj.localpath;
}

void TestSyncDownJobs::RunTests()
{
#define RUN_TEST(name)                          \
    test##name(workdir + "/" + #name);          \
    std::cout << "test" #name << ": OK" << std::endl

    RUN_TEST(DatasetVersions);
    RUN_TEST(AddJobs);
    RUN_TEST(RemoveJobs);
    RUN_TEST(DownloadPath);
    RUN_TEST(DownloadedRevision);
    RUN_TEST(Timestamps);
    RUN_TEST(FailedCounts);
    RUN_TEST(GetNextJobs);
    RUN_TEST(VisitJobs);
#undef RUN_TEST
}

int main(int argc, char *argv[])
{
    LOGInit("TestSyncDownJobs", NULL);
    LOGSetMax(0);
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    VPL_Init();

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " workdir" << std::endl;
        exit(0);
    }
    std::string workdir = argv[1];

    TestSyncDownJobs tsdj(workdir);
    tsdj.RunTests();

    exit(0);
}
