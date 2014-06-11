#include <vplu_types.h>
#include <vplu_format.h>
#include <vplex_file.h>
#include <gvm_file_utils.h>
#include <gvm_errors.h>
#include <log.h>

#include "SyncUp_private.hpp"

#include <iostream>
#include <cassert>

struct TestJob {
    std::string localpath;
    bool make_copy;
    u64 ctime, mtime;
    u64 datasetid;
    std::string comppath;
    u64 syncfeature;
};

#define ADDJOB(tj,jobid) AddJob(tj.localpath, tj.make_copy, tj.ctime, tj.mtime, tj.datasetid, tj.comppath, tj.syncfeature, jobid)

class TestSyncUpJobs {
public:
    TestSyncUpJobs(const std::string &workdir);
    void RunTests();
private:
    std::string workdir;
    void testAddJobs(const std::string &workdir);
    void testRemoveJobs(const std::string &workdir);
    void testRemoveJobsByOpathPrefix(const std::string &workdir);
    void testTimestamps(const std::string &workdir);
    void testUploadFailedCount(const std::string &workdir);
    void testGetNextJobs(const std::string &workdir);

    void createTestFile(const std::string &path);
    bool checkFileExists(const std::string &path);
    bool isSameJob(const SyncUpJob &job, const TestJob &tj);
};

TestSyncUpJobs::TestSyncUpJobs(const std::string &workdir)
    : workdir(workdir)
{
}

void TestSyncUpJobs::testAddJobs(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "tmp/";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncUpJobs suj(workdir);

    TestJob tj1 = { workdir + "file1",
                    false,
                    100, 1000,
                    10000, "comp1/path1/file1", 40 };
    TestJob tj2a = { tmpdir + "file2a",
                     false,
                     200, 2000,
                     20000, "comp2/path2/file2", 40 };
    TestJob tj2b = { tmpdir + "file2b",
                     false,
                     0, 2001,
                     20000, "comp2/path2/file2", 40 };

    u64 jobid;
    SyncUpJob job;
    int err;

    err = suj.ADDJOB(tj1, jobid);
    assert(!err);
    err = suj.findJob(tj1.datasetid, tj1.comppath, job);
    assert(!err);
    assert(job.id == jobid);
    assert(isSameJob(job, tj1));

    err = suj.ADDJOB(tj2a, jobid);
    assert(!err);
    err = suj.findJob(tj2a.datasetid, tj2a.comppath, job);
    assert(!err);
    assert(job.id == jobid);
    assert(isSameJob(job, tj2a));

    // this should replace tj2a
    err = suj.ADDJOB(tj2b, jobid);
    assert(!err);
    err = suj.findJob(tj2b.datasetid, tj2b.comppath, job);
    assert(!err);
    assert(job.id == jobid);
    assert(isSameJob(job, tj2b));
}

void TestSyncUpJobs::testRemoveJobs(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "tmp/";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncUpJobs suj(workdir);

    TestJob tj1 = { workdir + "file1",
                    false,
                    100, 1000,
                    10000, "comp1/path1/file1", 40 };
    TestJob tj2 = { tmpdir + "file2",
                    false,
                    200, 2000,
                    20000, "comp2/path2/file2", 40 };

    u64 jobid1, jobid2;
    int err;

    err = suj.ADDJOB(tj1, jobid1);
    assert(!err);

    err = suj.ADDJOB(tj2, jobid2);
    assert(!err);

    SyncUpJob job;

    err = suj.RemoveJob(jobid1);
    assert(!err);
    err = suj.findJob(tj1.datasetid, tj1.comppath, job);
    assert(err == CCD_ERROR_NOT_FOUND);

    err = suj.RemoveJob(jobid2);
    assert(!err);
    err = suj.findJob(tj2.datasetid, tj2.comppath, job);
    assert(err == CCD_ERROR_NOT_FOUND);
}

void TestSyncUpJobs::testRemoveJobsByOpathPrefix(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "tmp/";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncUpJobs suj(workdir);

    TestJob tj1 = { workdir + "file1",
                    false,
                    100, 1000,
                    10000, "comp1/path1/file1", 40 };
    TestJob tj2 = { tmpdir + "file2",
                    false,
                    200, 2000,
                    20000, "comp2/path2/file2", 40 };

    u64 jobid1, jobid2;
    int err;

    err = suj.ADDJOB(tj1, jobid1);
    assert(!err);

    err = suj.ADDJOB(tj2, jobid2);
    assert(!err);

    SyncUpJob job;

    err = suj.RemoveJobsByDatasetIdLocalPathPrefix(tj2.datasetid, tmpdir);
    assert(!err);
    err = suj.findJob(tj1.datasetid, tj1.comppath, job);
    assert(!err);
    err = suj.findJob(tj2.datasetid, tj2.comppath, job);
    assert(err == CCD_ERROR_NOT_FOUND);
}

void TestSyncUpJobs::testTimestamps(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "tmp/";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncUpJobs suj(workdir);

    TestJob tj1 = { workdir + "file1",
                    false,
                    100, 1000,
                    10000, "comp1/path1/file1", 40 };

    u64 jobid;
    int err;

    err = suj.ADDJOB(tj1, jobid);
    assert(!err);

    u64 timestamp = 12345;
    SyncUpJobEx jobex;
    err = suj.setJobDispatchTime(jobid, timestamp);
    assert(!err);
    err = suj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.disp_ts == timestamp);

    timestamp++;
    err = suj.setJobTryUploadTime(jobid, timestamp);
    assert(!err);
    err = suj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.ul_try_ts == timestamp);

    timestamp++;
    err = suj.setJobDoneUploadTime(jobid, timestamp);
    assert(!err);
    err = suj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.ul_done_ts == timestamp);
}

void TestSyncUpJobs::testUploadFailedCount(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "tmp/";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncUpJobs suj(workdir);

    TestJob tj1 = { workdir + "file1",
                    false,
                    100, 1000,
                    10000, "comp1/path1/file1", 40 };

    u64 jobid;
    int err;

    err = suj.ADDJOB(tj1, jobid);
    assert(!err);

    u64 failed_count = 0;
    SyncUpJobEx jobex;

    err = suj.incJobUploadFailedCount(jobid);
    assert(!err);
    failed_count++;
    err = suj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.ul_failed == failed_count);

    err = suj.incJobUploadFailedCount(jobid);
    assert(!err);
    failed_count++;
    err = suj.findJobEx(jobid, jobex);
    assert(!err);
    assert(jobex.ul_failed == failed_count);
}

void TestSyncUpJobs::testGetNextJobs(const std::string &workdir)
{
    // make sure directories exist
    Util_CreatePath(workdir.c_str(), VPL_TRUE);
    std::string tmpdir = workdir + "tmp/";
    Util_CreatePath(tmpdir.c_str(), VPL_TRUE);

    SyncUpJobs suj(workdir);

    TestJob tj1 = { workdir + "file1",
                    false,
                    100, 1000,
                    10000, "comp1/path1/file1", 40 };

    int err;

    u64 jobid;
    err = suj.ADDJOB(tj1, jobid);
    assert(!err);

    VPLThread_Sleep(VPLTime_FromSec(1));

    u64 threshold = VPLTime_ToSec(VPLTime_GetTime());
    SyncUpJob job;
    err = suj.GetNextJob(threshold, job);
    assert(!err);
    assert(job.id == jobid);

    // mark the jobs as dispatched
    u64 currVplTimeSec = VPLTime_ToSec(VPLTime_GetTime());
    err = suj.TimestampJobDispatch(jobid, currVplTimeSec);
    assert(!err);

    // make sure it is not found, because it is already dispatched wrt current threshold
    err = suj.GetNextJob(threshold, job);
    assert(err == CCD_ERROR_NOT_FOUND);

    VPLThread_Sleep(VPLTime_FromSec(1));

    threshold = VPLTime_ToSec(VPLTime_GetTime());
    // make sure it is found, because the threshold is now later
    err = suj.GetNextJob(threshold, job);
    assert(!err);
    assert(job.id == jobid);

    // mark the job done
    err = suj.TimestampJobDoneUpload(jobid);
    assert(!err);

    // make sure it is not found, because the job is done
    err = suj.GetNextJob(threshold, job);
    assert(err == CCD_ERROR_NOT_FOUND);
}

void TestSyncUpJobs::createTestFile(const std::string &path)
{
    VPLFile_handle_t h = VPLFile_Open(path.c_str(),
                                      VPLFILE_OPENFLAG_CREATE|VPLFILE_OPENFLAG_READWRITE, 
                                      VPLFILE_MODE_IRUSR|VPLFILE_MODE_IWUSR);
    assert(VPLFile_IsValidHandle(h));
    ssize_t bytes = VPLFile_Write(h, "a", 1);
    assert(bytes == 1);
    int err = VPLFile_Close(h);
    assert(err == VPL_OK);
}

bool TestSyncUpJobs::checkFileExists(const std::string &path)
{
    return VPLFile_CheckAccess(path.c_str(), VPLFILE_CHECKACCESS_EXISTS) == VPL_OK;
}

bool TestSyncUpJobs::isSameJob(const SyncUpJob &job, const TestJob &tj)
{
    return job.opath == tj.localpath &&
        job.ctime == tj.ctime &&
        job.mtime == tj.mtime &&
        job.did == tj.datasetid &&
        job.cpath == tj.comppath;
}

void TestSyncUpJobs::RunTests()
{
#define RUN_TEST(name)                          \
    test##name(workdir + #name + "/");          \
    std::cout << "test" #name << ": OK" << std::endl

    RUN_TEST(AddJobs);
    RUN_TEST(RemoveJobs);
    RUN_TEST(RemoveJobsByOpathPrefix);
    RUN_TEST(Timestamps);
    RUN_TEST(UploadFailedCount);
    RUN_TEST(GetNextJobs);
#undef RUN_TEST
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " workdir" << std::endl;
        exit(0);
    }
    std::string workdir = argv[1];
    if (workdir[workdir.length() - 1] != '/') {
        workdir.append("/");
    }

    TestSyncUpJobs tsuj(workdir);
    tsuj.RunTests();

    exit(0);
}
