#ifndef _TIME_STREAM_DOWNLOAD_HPP_
#define _TIME_STREAM_DOWNLOAD_HPP_

#include <vplu_types.h>
#include <vpl_time.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#include <vector>
#include <string>
#include <queue>

class TimeStreamDownload;
struct TimeStreamParams
{
    TimeStreamDownload* timestreamdownload;
    int instance;
    int repeat;
    int maxbytes;
    VPLTime_t maxbytes_delay;
    VPLTime_t delay;
    unsigned int width;
    unsigned int height;
    std::string format;
    std::string outputDir;
};

typedef struct FileInfo
{
    std::string itemIdx;
    std::string oid;
    std::string filefullpath;
    std::string filename;
    std::string content_url;
    std::string thumbnail_url;
} FileInfo;

class TimeStreamDownload {
public:
    TimeStreamDownload();
    ~TimeStreamDownload();
    int loadDumpfile(std::vector<std::string> &dumpfiles, std::vector<std::vector<std::pair<std::string, std::string> > > &vfiles);
    int loadDumpfile(std::vector<std::string> &dumpfiles, std::vector<std::vector<FileInfo> > &vvfileInfo);
    int downloadAllFiles(std::vector<std::string> &vdumpfiles, int repeat, std::vector<int> &maxbytes,
        std::vector<VPLTime_t> &maxbytes_delay, std::vector<VPLTime_t> &delay, int nthreads);
    void setTranscodingResolution(int width, int height);
    void setTranscodingFormat(std::string format);
    void setTranscodingOutputDir(std::string outputDir);
private:
    std::vector<std::pair<std::string, std::string> > files;
    std::vector<std::vector<std::pair<std::string, std::string> > > vfiles;
    std::string dumpfile;
    int repeat;
    int nthreads;
    int downloader(int instance, int repeat, int maxbytes, VPLTime_t maxbytes_delay, VPLTime_t delay, unsigned int width, unsigned int height, std::string format, std::string outputDir);
    static VPLThread_return_t downloader(VPLThread_arg_t arg);
    VPLSem_t q_sem;
    VPLMutex_t q_mutex;
    std::queue<std::vector<std::pair<std::string, std::string> >::const_iterator> urlQ;
    unsigned int width;
    unsigned int height;
    std::string format;
    std::string outputDir;
};

#endif // _TIME_STREAM_DOWNLOAD_HPP_
