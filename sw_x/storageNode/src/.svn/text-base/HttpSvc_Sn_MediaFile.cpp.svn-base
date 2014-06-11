#include "HttpSvc_Sn_MediaFile.hpp"

#ifdef CLOUDNODE
#include "HttpSvc_Sn_MediaDsFile.hpp"
#endif // CLOUDNODE
#include "HttpSvc_Sn_MediaFsFile.hpp"

#include "vss_server.hpp"

#include <gvm_errors.h>
#include <log.h>

#include <vpl_conv.h>
#include <vplex_http_util.hpp>

#include <cassert>
#include <vector>

#ifdef CLOUDNODE
//#define TEST_HTTPSVC_SN_MEDIAFILE_SPLIT_CLOUDNODE_LOCATION 1
#endif // CLOUDNODE

HttpSvc::Sn::MediaFile::MediaFile()
#ifdef ENABLE_PHOTO_TRANSCODE
    : transcodeHandle(INVALID_IMAGE_TRANSCODE_HANDLE)
#endif // ENABLE_PHOTO_TRANSCODE
{
}

HttpSvc::Sn::MediaFile::~MediaFile()
{
#ifdef ENABLE_PHOTO_TRANSCODE
    if (transcodeHandle != INVALID_IMAGE_TRANSCODE_HANDLE) {
        ImageTranscode_DestroyHandle(transcodeHandle);
    }
#endif // ENABLE_PHOTO_TRANSCODE
}

bool HttpSvc::Sn::MediaFile::isTranscoded() const
{
#ifdef ENABLE_PHOTO_TRANSCODE
    return transcodeHandle != INVALID_IMAGE_TRANSCODE_HANDLE;
#else
    return false;
#endif // ENABLE_PHOTO_TRANSCODE
}

#ifdef ENABLE_PHOTO_TRANSCODE
s64 HttpSvc::Sn::MediaFile::getTranscodedSize() const
{
    assert(isTranscoded());

    size_t size;
    int err = ImageTranscode_GetContentLength(transcodeHandle, size);
    if (err) {
        LOG_ERROR("Failed to get transcoded image size: err %d", err);
        return err;
    }

    return size;
}
#endif // ENABLE_PHOTO_TRANSCODE

#ifdef ENABLE_PHOTO_TRANSCODE
ssize_t HttpSvc::Sn::MediaFile::readTranscodedAt(u64 offset, char *buf, size_t bufsize)
{
    assert(isTranscoded());

    int nbytes = ImageTranscode_Read(transcodeHandle, buf, bufsize, (size_t)offset);
    if (nbytes < 0) {
        LOG_ERROR("Failed to read transcoded image: err %d", nbytes);
    }

    return nbytes;
}
#endif // ENABLE_PHOTO_TRANSCODE

#ifdef CLOUDNODE
static int splitCloudnodeLocation(const std::string &location,
                                  std::string &loctype, u64 &userId, u64 &datasetId, std::string &path)
{
    /* "location" has two forms:
       /dataset/<userId>/<datasetId>/<path>
       /native/<path> - in this case, path must begin with /
     */
    std::vector<std::string> loc_tokens;
    VPLHttp_SplitUri(location, loc_tokens);
    if (loc_tokens.size() < 1 || (loc_tokens[0] != "dataset" && loc_tokens[0] != "native")) {
        LOG_ERROR("Malformed location %s", location.c_str());
        return CCD_ERROR_PARAMETER;
    }
    loctype.assign(loc_tokens[0]);

    if (loctype == "native") {
        if (loc_tokens.size() < 2 || loc_tokens[1].empty()) {
            LOG_ERROR("Malformed location %s", location.c_str());
            return CCD_ERROR_PARAMETER;
        }
        size_t pos = location.find('/', 1);
        assert(pos != location.npos);  // this is because we already confirmed loc_tokens.size() >= 2
        path.assign(location, pos, location.npos);
        return 0;
    }

    assert(loctype == "dataset");
    if (loc_tokens.size() < 4 || loc_tokens[3].empty()) {
        LOG_ERROR("Malformed location %s", location.c_str());
        return CCD_ERROR_PARAMETER;
    }

    userId = VPLConv_strToU64(loc_tokens[1].c_str(), NULL, 10);
    datasetId = VPLConv_strToU64(loc_tokens[2].c_str(), NULL, 10);

    size_t pos = 0;
    for (int i = 0; i < 3; i++) {
        pos = location.find('/', pos + 1);
        assert(pos != location.npos);  // this is because we already confirmed loc_tokens.size() >= 4.
    }
    path.assign(location, pos + 1, location.npos);

    return 0;
}
#endif // CLOUDNODE

HttpSvc::Sn::MediaFile *HttpSvc::Sn::MediaFile::Create(vss_server *vssServer, const std::string &location)
{
#ifdef CLOUDNODE
    int err = 0;

    std::string loctype;
    u64 userId = 0;
    u64 datasetId = 0;
    std::string path;
    err = splitCloudnodeLocation(location, loctype, userId, datasetId, path);
    if (err) {
        // error msg logged by split_cloudnode_location()
        return NULL;
    }
    assert(loctype == "dataset" || loctype == "native");

    if (loctype == "native") {
        return new (std::nothrow) HttpSvc::Sn::MediaFsFile(path);
    }

    assert(loctype == "dataset");

    dataset *ds = NULL;
    err = vssServer->getDataset(userId, datasetId, ds);
    if (err) {
        LOG_ERROR("Failed to find dataset "FMTu64, datasetId);
        return NULL;
    }
    if (!ds) {
        LOG_ERROR("Failed to find dataset obj for did "FMTu64, datasetId);
        return NULL;
    }

    return new (std::nothrow) HttpSvc::Sn::MediaDsFile(ds, path);
#else
    // "location" is always a path in the device filesystem.
    return new (std::nothrow) HttpSvc::Sn::MediaFsFile(location);
#endif // CLOUDNODE
}


#ifdef TEST_HTTPSVC_SN_MEDIAFILE_SPLIT_CLOUDNODE_LOCATION
static void testSnMediaFile_splitCloudnodeLocation(const std::string &location, bool expectOk,
                                                   const std::string &expectedLoctype = "",
                                                   u64 expectedUserId = 0,
                                                   u64 expectedDatasetId = 0,
                                                   const std::string &expectedPath = "")
{
    std::string loctype;
    u64 userId = 0;
    u64 datasetId = 0;
    std::string path;
    int err = splitCloudnodeLocation(location, loctype, userId, datasetId, path);
    if (err) {
        if (!expectOk) {
            LOG_INFO("\"%s\" PASS (func expectedly failed)", location.c_str());
        }
        else {
            LOG_INFO("\"%s\" FAIL (func unexpectedly failed)", location.c_str());
        }
    }
    else {
        int nerr = 0;
        if (loctype != expectedLoctype) {
            LOG_INFO("\"%s\" FAIL (unexpected loctype, got %s, expected %s)", location.c_str(), loctype.c_str(), expectedLoctype.c_str());
            nerr++;
        }
        if (userId != expectedUserId) {
            LOG_INFO("\"%s\" FAIL (unexpected userId, got "FMTu64", expected "FMTu64")", location.c_str(), userId, expectedUserId);
            nerr++;
        }
        if (datasetId != expectedDatasetId) {
            LOG_INFO("\"%s\" FAIL (unexpected userId, got "FMTu64", expected "FMTu64")", location.c_str(), datasetId, expectedDatasetId);
            nerr++;
        }
        if (path != expectedPath) {
            LOG_INFO("\"%s\" FAIL (unexpected path, got %s, expected %s)", location.c_str(), path.c_str(), expectedPath.c_str());
            nerr++;
        }
        if (nerr > 0) {
            LOG_INFO("\"%s\" FAIL", location.c_str());
        }
        else {
            LOG_INFO("\"%s\" PASS", location.c_str());
        }
    }
}

void testSnMediaFile_splitCloudnodeLocation();
void testSnMediaFile_splitCloudnodeLocation()
{
    testSnMediaFile_splitCloudnodeLocation("",                     /*expectOk*/false);  // does not begin with "/"
    testSnMediaFile_splitCloudnodeLocation("/abc/def",             /*expectOk*/false);  // first token must be "dataset" or "native"
    testSnMediaFile_splitCloudnodeLocation("/native",              /*expectOk*/false);  // no path
    testSnMediaFile_splitCloudnodeLocation("/native/",             /*expectOk*/false);  // no path
    testSnMediaFile_splitCloudnodeLocation("/native/abc",          /*expectOk*/true, "native", 0, 0, "/abc");
    testSnMediaFile_splitCloudnodeLocation("native/abc",           /*expectOk*/false);  // does not begin with "/"
    testSnMediaFile_splitCloudnodeLocation("/dataset",             /*expectOk*/false);  // no uid/did/path
    testSnMediaFile_splitCloudnodeLocation("/dataset/",            /*expectOk*/false);  // no uid/did/path
    testSnMediaFile_splitCloudnodeLocation("/dataset/123",         /*expectOk*/false);  // no did/path
    testSnMediaFile_splitCloudnodeLocation("/dataset/123/",        /*expectOk*/false);  // no did/path
    testSnMediaFile_splitCloudnodeLocation("/dataset/123/456",     /*expectOk*/false);  // no path
    testSnMediaFile_splitCloudnodeLocation("/dataset/123/456/",    /*expectOk*/false);  // no path
    testSnMediaFile_splitCloudnodeLocation("/dataset/123/456/abc", /*expectOk*/true, "dataset", 123, 456, "abc");
    testSnMediaFile_splitCloudnodeLocation("dataset/123/456/abc",  /*expectOk*/false);  // does not begin with "/"
}
#endif // TEST_HTTPSVC_SN_MEDIAFILE_SPLIT_CLOUDNODE_LOCATION
