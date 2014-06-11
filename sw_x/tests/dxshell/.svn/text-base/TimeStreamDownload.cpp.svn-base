#include "TimeStreamDownload.hpp"
#include "HttpAgent.hpp"

#include <vplu_format.h>
#include <vpl_th.h>
#include <vpl_fs.h>
#include <log.h>
#include <vpl_string.h>

#include <fstream>
#include <time.h>


TimeStreamDownload::TimeStreamDownload() : width(0), height(0), format(""), outputDir("")
{
}

TimeStreamDownload::~TimeStreamDownload()
{
}

static int get_token(std::string &line, std::string &token, bool &next_available)
{
  u32 i;
  bool in_quotes = false;  // inside double-quoted char sequence
  bool escaped = false;  // following char is escaped

  token.clear();
  next_available = false;

  i = 0;
  while (i < line.size()) {
    if (escaped) {
      token.append(1, line[i]);
      i++;
      escaped = false;
    }
    else {
      if (in_quotes) {
        if (line[i] == '"') {
          i++;
          in_quotes = false;
        }
        else {
          token.append(1, line[i]);
          i++;
        }
      }
      else {
        if (line[i] == '"') {
          i++;
          in_quotes = true;
        }
        else if (line[i] == ',') {
          i++;
          next_available = true;
          break;
        }
        else {
          token.append(1, line[i]);
          i++;
        }
      }
    }
  }

  line.erase(0, i);

  return 0;
}

int TimeStreamDownload::loadDumpfile(std::vector<std::string> &dumpfiles,
    std::vector<std::vector<std::pair<std::string, std::string> > > &vfiles)
{
    int rv = 0;
    std::vector<std::vector<FileInfo> > vvfileInfo;

    vfiles.clear();
    rv = loadDumpfile(dumpfiles, vvfileInfo);
    if(rv != 0){
        LOG_ERROR("loadDumpfile fail, %d", rv);
        return rv;
    }
    
    vfiles.resize(vvfileInfo.size());
    LOG_ALWAYS("vvfileInfo.size : %d", vvfileInfo.size());
    for(size_t i=0; i<vvfileInfo.size(); i++){
        std::vector<FileInfo> &vfileInfo = vvfileInfo[i];
        LOG_ALWAYS("vfileInfo.size : %d", vfileInfo.size());
        for (size_t j=0; j<vfileInfo.size(); j++){
            vfiles[i].push_back(std::make_pair(vfileInfo[j].filename, vfileInfo[j].content_url));
        }
    }

    return rv;
}

int TimeStreamDownload::loadDumpfile(std::vector<std::string> &dumpfiles,
    std::vector<std::vector<FileInfo> > &vfiles)
{
    int rv = 0;
    int rc;
    std::string line;
    int linenum;
    size_t listIndex = 0;
    VPLFS_dir_t dir_folder;
    std::deque<std::string> localDirs;
    std::vector<FileInfo> files;

    vfiles.clear();
    for(u32 i = 0; i != dumpfiles.size(); i++) {
        files.clear();
        std::ifstream f;
        localDirs.clear();
        localDirs.push_back(dumpfiles[i]);
        while(!localDirs.empty()) {
            std::string dirPath(localDirs.front());
            localDirs.pop_front();
            f.open(dirPath.c_str());
            if (!f.is_open()) {
                //LOG_ERROR("Failed to open dumpfile %s", dumpfiles[i].c_str());
                rc = VPLFS_Opendir(dirPath.c_str(), &dir_folder);
                if(rc == VPL_ERR_NOENT){
                    // no photos
                    continue;
                }else if(rc != VPL_OK) {
                    LOG_ERROR("Unable to open %s:%d", dirPath.c_str(), rc);
                    continue;
                }
                VPLFS_dirent_t folderDirent;
                while((rc = VPLFS_Readdir(&dir_folder, &folderDirent))==VPL_OK)
                {
                    std::string dirent(folderDirent.filename);
                    std::string fullPath = dirPath;
                    fullPath.append("/");
                    fullPath.append(dirent);
                    /*fullPath += ("/" + dirent);*/
                    std::string direntLowercase(dirent);
                    //transform(direntLowercase.begin(), direntLowercase.end(), direntLowercase.begin(), tolower);
                    if(folderDirent.type != VPLFS_TYPE_FILE) {
                        if(dirent=="." || dirent=="..") {
                            // We want to skip these directories
                        }else{
                            // save directory for later processing
                            localDirs.push_back(fullPath);
                        }
                    }
                    else {
                        localDirs.push_back(fullPath);
                    }
                    continue;
                }
            }
            else {
                linenum = 0;
                listIndex = dirPath.find(".list");
                if (listIndex != std::string::npos) 
                {
                    while (getline(f, line)) {
                        linenum++;
                        if (line.empty()) continue;
                        if (line[0] == '#') continue;
                        std::string itemIdx, oid, filename, content_url, thumbnail_url, filefullpath;
                        bool next_token_available;

                        get_token(line, itemIdx, next_token_available);
                        if (!next_token_available) {
                            LOG_ERROR("Parse error at line %d: first comma missing", linenum);
                            rv = -1;
                            goto end;
                        }

                        get_token(line, oid, next_token_available);
                        if (!next_token_available) {
                            LOG_ERROR("Parse error at line %d: second comma missing", linenum);
                            rv = -1;
                            goto end;
                        }

                        get_token(line, filefullpath, next_token_available);
                        if (!next_token_available) {
                            LOG_ERROR("Parse error at line %d: third comma missing", linenum);
                            rv = -1;
                            goto end;
                        }

                        char fSeparater = '/';
                        #ifdef _MSC_VER
                            fSeparater = '\\';
                        #endif
            
                        if (filefullpath.find(fSeparater) == std::string::npos)
                        {
                            filename = filefullpath;
                        }
                        else
                        {
                            filename = filefullpath.substr(filefullpath.find_last_of(fSeparater) + 1);
                        }

                        get_token(line, content_url, next_token_available);
                        if (!next_token_available) {
                            LOG_ERROR("Parse error at line %d: fourth comma missing", linenum);
                            rv = -1;
                            goto end;
                        }

                        get_token(line, thumbnail_url, next_token_available);
                        if (!next_token_available) {
                            LOG_ERROR("Parse error at line %d: fifth comma missing", linenum);
                            rv = -1;
                            goto end;
                        }

                        LOG_INFO("itemIdx=%s oid=%s filefullpath=%s filename=%s contenturl=%s thumbnail_url=%s", 
                                itemIdx.c_str(), oid.c_str(), filefullpath.c_str(), filename.c_str(), content_url.c_str(), thumbnail_url.c_str());

                        FileInfo fileInfo;
                        fileInfo.itemIdx        = itemIdx;
                        fileInfo.oid            = oid;
                        fileInfo.filefullpath   = filefullpath;
                        fileInfo.filename       = filename;
                        fileInfo.content_url    = content_url;
                        fileInfo.thumbnail_url  = thumbnail_url;
                        files.push_back(fileInfo);
                    }
                }
                else
                {
                    while (getline(f, line)) {
                        linenum++;
                        if (line.empty()) continue;
                        if (line[0] == '#') continue;
                        std::string oid, filename, content_url, thumbnail_url;
                        bool next_token_available;

                        // get oid from column #1
                        get_token(line, oid, next_token_available);
                        if (!next_token_available) {
                            LOG_ERROR("Parse error at line %d: first comma missing", linenum);
                            rv = -1;
                            goto end;
                        }

                        // get file name from column #2
                        get_token(line, filename, next_token_available);
                        if (!next_token_available) {
                            LOG_ERROR("Parse error at line %d: second comma missing", linenum);
                            rv = -1;
                            goto end;
                        }

                        // get content url from column #3
                        get_token(line, content_url, next_token_available);
                        if (!next_token_available) {
                            LOG_ERROR("Parse error at line %d: third comma missing", linenum);
                            rv = -1;
                            goto end;
                        }

                        // get thumbnail url from column #4
                        get_token(line, thumbnail_url, next_token_available);

                        LOG_INFO("oid=%s filename=%s contenturl=%s thumbnailurl=%s", oid.c_str(), filename.c_str(), content_url.c_str(), thumbnail_url.c_str());

                        FileInfo fileInfo;
                        fileInfo.oid            = oid;
                        fileInfo.filename       = filename;
                        fileInfo.content_url    = content_url;
                        fileInfo.thumbnail_url  = thumbnail_url;
                        files.push_back(fileInfo);
                    }
                }
            }
        }
    end:
        vfiles.push_back(files);
        f.close();
    }


    return rv;
}

int TimeStreamDownload::downloader(int instance, int repeat, int maxbytes, VPLTime_t maxbytes_delay, VPLTime_t delay, unsigned int width, unsigned int height, std::string format, std::string outputDir)
{
    int rv = 0;
    std::vector<std::pair<std::string, std::string> > files;

    static int num = 0;
    HttpAgent *agent = getHttpAgent();
    if(vfiles.size() == 1) {
        files = vfiles[0];
    }
    else {
        files = vfiles[instance];
    }

    if(vfiles.size() == 1) {
        while (1) {
            int rc;
            VPLSem_Wait(&q_sem);
            VPLMutex_Lock(&q_mutex);
            std::vector<std::pair<std::string, std::string> >::const_iterator it = vfiles[0].end();
            if (!urlQ.empty()) {
                it = urlQ.front();
                urlQ.pop();
            }
            VPLMutex_Unlock(&q_mutex);
            if (it == vfiles[0].end())
                break;
            LOG_INFO("[%d] Downloading %s via %s...", instance, it->first.c_str(), it->second.c_str());
            if (width > 0 && height > 0 && format.size() > 0) {
                char act_xcode_dimension[60];
                char act_xcode_fmt[60];

                const char* headers[] = {
                    act_xcode_dimension,
                    act_xcode_fmt
                };
                VPL_snprintf(act_xcode_dimension, 60, "act_xcode_dimension: %u, %u", width, height);
                VPL_snprintf(act_xcode_fmt, 60, "act_xcode_fmt: %s", format.c_str());

                if (outputDir.length() > 0) {
                    char download_path[300];
                    VPL_snprintf(download_path, 300, "%s/%d.%s", outputDir.c_str(), num++, format.c_str());
                    rc = agent->get_extended(it->second, maxbytes, maxbytes_delay, headers, 2, download_path);
                } else {
                    rc = agent->get(it->second, maxbytes, maxbytes_delay, headers, 2);
                }

            } else if (width > 0 && height > 0) {
                char act_xcode_dimension[60];
                const char* headers[] = {
                    act_xcode_dimension
                };
                VPL_snprintf(act_xcode_dimension, 60, "act_xcode_dimension: %u, %u", width, height);
                if (outputDir.length() > 0) {
                    char download_path[300];
                    VPL_snprintf(download_path, 300, "%s/%d.JPG", outputDir.c_str(), num++);
                    rc = agent->get_extended(it->second, maxbytes, maxbytes_delay, headers, 1, download_path);
                } else {
                    rc = agent->get(it->second, maxbytes, maxbytes_delay, headers, 1);
                }

            } else if (format.size() > 0) {
                char act_xcode_fmt[60];
                const char* headers[] = {
                    act_xcode_fmt
                };
                VPL_snprintf(act_xcode_fmt, 60, "act_xcode_fmt: %s", format.c_str());
                if (outputDir.length() > 0) {
                    char download_path[300];
                    VPL_snprintf(download_path, 300, "%s/%d.%s", outputDir.c_str(), num++, format.c_str());
                    rc = agent->get_extended(it->second, maxbytes, maxbytes_delay, headers, 1, download_path);
                } else {
                    rc = agent->get(it->second, maxbytes, maxbytes_delay, headers, 1);
                }
            } else {
                rc = agent->get(it->second, maxbytes, maxbytes_delay);
            }
            LOG_INFO("[%d] Downloaded "FMTu64" bytes of %s via %s in %f seconds: %d", instance, agent->getFileBytes(), it->first.c_str(), it->second.c_str(), agent->getFileTime() / 1000000.0, rc);

            if (delay) VPLThread_Sleep(delay);

            // remember the first error and eventually return it
            if (rc != 0 && rv == 0)
                rv = rc;
        }
    }
    else {
        for (int i = 0; i < repeat; i++) {
            std::vector<std::pair<std::string, std::string> >::const_iterator it;
            for (it = files.begin(); it != files.end(); it++) {
                LOG_INFO("Downloading %s via %s...", it->first.c_str(), it->second.c_str());
                int rc = agent->get(it->second, maxbytes, maxbytes_delay);
                LOG_INFO("Downloaded "FMTu64" bytes of %s via %s in %f seconds", agent->getFileBytes(), it->first.c_str(), it->second.c_str(), agent->getFileTime() / 1000000.0);

                if (delay) VPLThread_Sleep(delay);

                if (rc != 0 && rv == 0)
                    rv = rc;
            }
        }
    }

    LOG_INFO("[%d] Downloaded "FMTu32" files, "FMTu64" bytes, in %f seconds: %d, Pass: "FMTu32" files, Failed: "FMTu32" files",
             instance, agent->getTotalFiles(), agent->getTotalBytes(), agent->getTotalTime() / 1000000.0, rv, agent->getPassFiles(), agent->getFailedFiles());

    delete agent;

    return rv;
}

VPLThread_return_t TimeStreamDownload::downloader(VPLThread_arg_t arg)
{
    TimeStreamParams *args = (TimeStreamParams*)arg;
    TimeStreamDownload *timeStreamDownload = args->timestreamdownload;
    int instance = args->instance;
    int repeat = args->repeat;
    int maxbytes = args->maxbytes;
    VPLTime_t maxbytes_delay = args->maxbytes_delay;
    VPLTime_t delay = args->delay;

    return (VPLThread_return_t)timeStreamDownload->downloader(instance, repeat, maxbytes, maxbytes_delay, delay, timeStreamDownload->width, timeStreamDownload->height, timeStreamDownload->format, timeStreamDownload->outputDir);
}

int TimeStreamDownload::downloadAllFiles(std::vector<std::string> &vdumpfiles, int repeat, std::vector<int> &maxbytes,
    std::vector<VPLTime_t> &maxbytes_delay, std::vector<VPLTime_t> &delay, int nthreads)
{
    int rv = 0;

    this->nthreads = nthreads;

    VPLSem_Init(&q_sem, VPLSEM_MAX_COUNT, 0);
    VPLMutex_Init(&q_mutex);

    // spawn downloader threads    
    std::vector<VPLThread_t> threads(nthreads);
    std::vector<TimeStreamParams> args(nthreads);
    TimeStreamParams timestreamparams;
    rv = loadDumpfile(vdumpfiles, vfiles);
    if(rv != 0) {
        LOG_ERROR("Failed to load dump file: %d", rv);
        goto end;
    }
    for (int i = 0; i < nthreads; i++) {
        timestreamparams.timestreamdownload = this;
        timestreamparams.instance = i;
        timestreamparams.repeat = repeat;
        timestreamparams.maxbytes = maxbytes[i];
        timestreamparams.maxbytes_delay = maxbytes_delay[i];
        timestreamparams.delay = delay[i];
        timestreamparams.width = width;
        timestreamparams.height = height;
        timestreamparams.format = format;
        timestreamparams.outputDir = outputDir;
        args[i] = timestreamparams;
        int rc = VPLThread_Create(&threads[i], downloader, (VPLThread_arg_t)&args[i], NULL, "downloader");
        if (rc != VPL_OK) {
            LOG_ERROR("Failed to spawn downloader thread: %d", rc);
            rv = rc;
            goto end;
        }
    }

    // enqueue iterators into the queue
    if (vdumpfiles.size() == 1) {
        for (int i = 0; i < repeat; i++) {
            std::vector<std::pair<std::string, std::string> >::const_iterator it;
            for (it = vfiles[0].begin(); it != vfiles[0].end(); it++) {
                VPLMutex_Lock(&q_mutex);
                urlQ.push(it);
                VPLMutex_Unlock(&q_mutex);
                VPLSem_Post(&q_sem);
            }
        }
    }

    // extra posts to let threads know no more files to download
    for (int i = 0; i < nthreads; i++) {
        VPLSem_Post(&q_sem);
    }

    // wait for all the downloader threads to terminate
    for (int i = 0; i < nthreads; i++) {
        VPLThread_return_t retval;
        int rc = VPLThread_Join(&threads[i], &retval);
        if (rc != VPL_OK) {
            LOG_ERROR("Failed to join: %d", rc);
            rv = rc;
            goto end;
        }
        if (retval != 0) {
            LOG_ERROR("Downloader thread %d exited with %d", i, (int)retval);
            // remember the first error and eventually return it
            if (rv == 0)
                rv = (int)retval;
        }
    }

    VPLMutex_Destroy(&q_mutex);
    VPLSem_Destroy(&q_sem);

 end:
    return rv;
}

void TimeStreamDownload::setTranscodingResolution(int width, int height)
{
    this->width = width;
    this->height = height;
}

void TimeStreamDownload::setTranscodingFormat(std::string format)
{
    this->format = format;
}

void TimeStreamDownload::setTranscodingOutputDir(std::string outputDir)
{
    this->outputDir = outputDir;
}
