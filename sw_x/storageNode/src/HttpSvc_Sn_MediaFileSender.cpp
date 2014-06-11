#include "HttpSvc_Sn_MediaFileSender.hpp"

#include "HttpSvc_Sn_MediaFile.hpp"

#include <cslsha.h>
#include <gvm_errors.h>
#include <HttpStream.hpp>
#include <HttpStream_Helper.hpp>
#include <log.h>
#include <util_mime.hpp>

#include <scopeguard.hpp>
#include <vplex_http_util.hpp>

#include <cassert>
#include <iomanip>
#include <iterator>
#include <sstream>

#if defined(WIN32) || defined(CLOUDNODE) || defined(LINUX)
static const size_t mediaFile_ReadBufSize = 1024 * 1024;  // 1MB
#else
static const size_t mediaFile_ReadBufSize = 64 * 1024;  // 64KB
#endif

HttpSvc::Sn::MediaFileSender::MediaFileSender(MediaFile *mf, const std::string &mediaType, HttpStream *hs)
    : mf(mf), mediaType(mediaType), hs(hs)
{
}

HttpSvc::Sn::MediaFileSender::~MediaFileSender()
{
}

int HttpSvc::Sn::MediaFileSender::Send()
{
    int err = processRangeSpec();
    if (err) {
        // processRangeSpec() already sent response
        return 0;
    }

    if (hs->GetMethod() == "HEAD") {
        return sendHttpHeaders();
    }

    switch (ranges.size()) {
    case 0:
        return sendWholeFile();
    case 1:
        return sendSingleRange();
    default: // more than 1 range
        return sendMultipleRanges();
    }
}

int HttpSvc::Sn::MediaFileSender::processRangeSpec()
{
    int err = 0;

    std::string rangesSpec;
    err = hs->GetReqHeader("Range", rangesSpec);
    if (err) {
        return 0;
    }

    VPLHttp_ParseRangesSpec(rangesSpec, ranges);

    /*
      http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.35

       "If a syntactically valid byte-range-set includes at least one
       byte-range-spec whose first-byte-pos is less than the current
       length of the entity-body, or at least one
       suffix-byte-range-spec with a non-zero suffix-length, then the
       byte-range-set is satisfiable. Otherwise, the byte-range-set is
       unsatisfiable. If the byte-range-set is unsatisfiable, the
       server SHOULD return a response with a status of 416 (Requested
       range not satisfiable)."

       ---

       http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.16

       "A server sending a response with status code 416 (Requested
       range not satisfiable) SHOULD include a Content-Range field
       with a byte-range-resp-spec of "*". The instance-length
       specifies the current length of the selected resource."

     */
    if (ranges.size() == 0) {
        HttpStream_Helper::AddStdRespHeaders(hs);
        hs->SetStatusCode(416);
        std::ostringstream oss;
        oss << "bytes */" << mf->GetSize();
        hs->SetRespHeader("Content-Range", oss.str());
        hs->Flush();
        return -1;  // FIXME
    }

    return err;
}

int HttpSvc::Sn::MediaFileSender::sendHttpHeaders()
{
    int err = 0;

    {
        HttpStream_Helper::AddStdRespHeaders(hs);
        hs->SetStatusCode(200);

        if (!mediaType.empty()) {
            hs->SetRespHeader("Content-Type", mediaType);
        }

        s64 filesize = mf->GetSize();
        if (filesize < 0) {
            err = (int)filesize;
            LOG_ERROR("Handle_mm[%p]: Failed to get media file size: loc %s, err %d", this, location.c_str(), err);
            goto end;
        }
        {
            std::ostringstream oss;
            oss << filesize;
            hs->SetRespHeader("Content-Length", oss.str());
        }

        if ((strncmp(mediaType.data(), "video", 5) == 0) ||
            (strncmp(mediaType.data(), "audio", 5) == 0)) {
            hs->SetRespHeader("transferMode.dlna.org", "Streaming");
        } else {
            hs->SetRespHeader("transferMode.dlna.org", "Interactive");
        }

        hs->SetRespHeader("Connection", "close");
        hs->SetRespHeader("contentFeatures.dlna.org", "DLNA.ORG_OP=01;DLNA.ORG_FLAGS=00100000000000000000000000000000");

        hs->Flush();
    }

 end:
    mf->Close();
    return err;
}

int HttpSvc::Sn::MediaFileSender::sendWholeFile()
{
    int err = 0;

    {
        HttpStream_Helper::AddStdRespHeaders(hs);
        hs->SetStatusCode(200);

        if (!mediaType.empty()) {
            hs->SetRespHeader("Content-Type", mediaType);
        }

        s64 filesize = mf->GetSize();
        if (filesize < 0) {
            err = (int)filesize;
            LOG_ERROR("Handle_mm[%p]: Failed to get media file size: loc %s, err %d", this, location.c_str(), err);
            goto end;
        }
        {
            std::ostringstream oss;
            oss << filesize;
            hs->SetRespHeader("Content-Length", oss.str());
        }

        if ((strncmp(mediaType.data(), "video", 5) == 0) ||
            (strncmp(mediaType.data(), "audio", 5) == 0)) {
            hs->SetRespHeader("transferMode.dlna.org", "Streaming");
        } else {
            hs->SetRespHeader("transferMode.dlna.org", "Interactive");
        }

        hs->SetRespHeader("Connection", "close");
        hs->SetRespHeader("contentFeatures.dlna.org", "DLNA.ORG_OP=01;DLNA.ORG_FLAGS=00100000000000000000000000000000");

        hs->Flush();

        err = mf->Open();
        if (err < 0) {
            LOG_ERROR("Handle_mm[%p]: Failed to open media file: loc %s, err %d", this, location.c_str(), err);
            goto end;
        }

        char *buf = new (std::nothrow) char[mediaFile_ReadBufSize];
        if (!buf) {
            err = CCD_ERROR_NOMEM;
            LOG_ERROR("MediaFileSender[%p]: Insufficient memory for buffer", this);
            goto end;
        }
        ON_BLOCK_EXIT(deleteArray<char>, buf);

        u64 offset = 0;
        while (1) {
            size_t reqsize = (size_t)MIN(filesize - offset, mediaFile_ReadBufSize);
            ssize_t nbytes = mf->ReadAt(offset, buf, reqsize);
            if (nbytes < 0) {
                err = nbytes;
                LOG_ERROR("MediaFileSender[%p]: Failed to read from media file: loc %s, err %d", this, location.c_str(), err);
                goto end;
            }
            if (nbytes == 0)  // EOF
                break;

            nbytes = hs->Write(buf, nbytes);
            if (nbytes < 0) {
                err = nbytes;
                LOG_ERROR("MediaFileSender[%p]: Failed to write to stream: err %d", this, err);
                goto end;
            }

            offset += nbytes;
        }
    }

 end:
    mf->Close();
    return err;
}

int HttpSvc::Sn::MediaFileSender::sendSingleRange()
{
    assert(ranges.size() == 1);

    int err = 0;
    u64 absFirstBytePos = 0, absLastBytePos = 0;

    {
        HttpStream_Helper::AddStdRespHeaders(hs);
        hs->SetStatusCode(206);

        if (!mediaType.empty()) {
            hs->SetRespHeader("Content-Type", mediaType);
        }

        s64 filesize = mf->GetSize();
        if (filesize < 0) {
            err = (int)filesize;
            LOG_ERROR("Handle_mm[%p]: Failed to get media file size: loc %s, err %d", this, location.c_str(), err);
            goto end;
        }
        err = VPLHttp_ConvToAbsRange(ranges[0], filesize, absFirstBytePos, absLastBytePos);
        if (err) {
            LOG_ERROR("Handle_mm[%p]: Bad range: err %d", this, err);
            goto end;
        }
        {
            std::ostringstream oss;
            oss << absLastBytePos - absFirstBytePos + 1;
            hs->SetRespHeader("Content-Length", oss.str());
        }
        {
            std::ostringstream oss;
            oss << "bytes " << absFirstBytePos << "-" << absLastBytePos << "/" << filesize;
            hs->SetRespHeader("Content-Range", oss.str());
        }

        if ((strncmp(mediaType.data(), "video", 5) == 0) ||
            (strncmp(mediaType.data(), "audio", 5) == 0)) {
            hs->SetRespHeader("transferMode.dlna.org", "Streaming");
        } else {
            hs->SetRespHeader("transferMode.dlna.org", "Interactive");
        }

        hs->SetRespHeader("Connection", "close");
        hs->SetRespHeader("contentFeatures.dlna.org", "DLNA.ORG_OP=01;DLNA.ORG_FLAGS=00100000000000000000000000000000");

        hs->Flush();

        err = mf->Open();
        if (err < 0) {
            LOG_ERROR("Handle_mm[%p]: Failed to open media file: loc %s, err %d", this, location.c_str(), err);
            goto end;
        }

        char *buf = new (std::nothrow) char[mediaFile_ReadBufSize];
        if (!buf) {
            err = CCD_ERROR_NOMEM;
            LOG_ERROR("MediaFileSender[%p]: Insufficient memory for buffer", this);
            goto end;
        }
        ON_BLOCK_EXIT(deleteArray<char>, buf);

        u64 offset = absFirstBytePos;
        while (1) {
            size_t reqsize = (size_t)MIN(absLastBytePos - offset + 1, mediaFile_ReadBufSize);
            if (reqsize == 0) {
                break;
            }
            ssize_t nbytes = mf->ReadAt(offset, buf, reqsize);
            if (nbytes < 0) {
                err = nbytes;
                LOG_ERROR("MediaFileSender[%p]: Failed to read from media file: loc %s, err %d", this, location.c_str(), err);
                goto end;
            }
            if (nbytes == 0) { // EOF
                break;
            }

            nbytes = hs->Write(buf, nbytes);
            if (nbytes < 0) {
                err = nbytes;
                LOG_ERROR("MediaFileSender[%p]: Failed to write to stream: err %d", this, err);
                goto end;
            }

            offset += nbytes;
        }
    }

 end:
    mf->Close();
    return err;
}

int HttpSvc::Sn::MediaFileSender::sendMultipleRanges()
{
    assert(ranges.size() > 1);

    int err = 0;

    {
        HttpStream_Helper::AddStdRespHeaders(hs);
        hs->SetStatusCode(206);

        std::string boundaryString;
        generateBoundaryString(boundaryString);

        s64 filesize = mf->GetSize();
        if (filesize < 0) {
            err = (int)filesize;
            LOG_ERROR("Handle_mm[%p]: Failed to get media file size: loc %s, err %d", this, location.c_str(), err);
            goto end;
        }

        u64 totalSize = 0;

        std::vector<std::string> headers(ranges.size());
        std::vector<std::pair<u64, u64> > absRanges(ranges.size());
        for (size_t i = 0; i < ranges.size(); i++) {
            u64 absFirstBytePos = 0, absLastBytePos = 0;
            err = VPLHttp_ConvToAbsRange(ranges[i], filesize, absFirstBytePos, absLastBytePos);
            if (err) {
                LOG_ERROR("Handle_mm[%p]: Bad range: err %d", this, err);
                goto end;
            }
            absRanges[i] = std::make_pair(absFirstBytePos, absLastBytePos);

            std::ostringstream oss;
            oss << "\r\n"
                << "--" << boundaryString << "\r\n";
            if (!mediaType.empty()) {
                oss << "Content-Type: " << mediaType << "\r\n";
            }
            oss << "Content-Range: bytes " << absFirstBytePos << "-" << absLastBytePos << "/" << filesize << "\r\n"
                << "\r\n";
            headers[i].assign(oss.str());

            totalSize +=  headers[i].size();
            totalSize += absLastBytePos - absFirstBytePos + 1;
        }

        std::string footer;
        {
            std::ostringstream oss;
            oss << "\r\n"
                << "--" << boundaryString << "--\r\n";
            footer.assign(oss.str());
        }
        totalSize += footer.size();

        {
            std::ostringstream oss;
            oss << "multipart/byteranges; boundary=" << boundaryString;
            hs->SetRespHeader("Content-Type", oss.str());
        }

        {
            std::ostringstream oss;
            oss << totalSize;
            hs->SetRespHeader("Content-Length", oss.str());
        }

        hs->Flush();

        err = mf->Open();
        if (err < 0) {
            LOG_ERROR("Handle_mm[%p]: Failed to open media file: loc %s, err %d", this, location.c_str(), err);
            goto end;
        }

        char *buf = new (std::nothrow) char[mediaFile_ReadBufSize];
        if (!buf) {
            err = CCD_ERROR_NOMEM;
            LOG_ERROR("MediaFileSender[%p]: Insufficient memory for buffer", this);
            goto end;
        }
        ON_BLOCK_EXIT(deleteArray<char>, buf);

        ssize_t nbytes = 0;
        for (size_t i = 0; i < absRanges.size(); i++) {
            nbytes = hs->Write(headers[i].data(), headers[i].size());
            if (nbytes < 0) {
                err = nbytes;
                LOG_ERROR("Handle_mm[%p]: Failed to write to stream: err %d", this, err);
                goto end;
            }

            u64 offset = absRanges[i].first;
            while (1) {
                size_t reqsize = (size_t)MIN(absRanges[i].second - offset + 1, mediaFile_ReadBufSize);
                if (reqsize == 0) {
                    break;
                }
                nbytes = mf->ReadAt(offset, buf, reqsize);
                if (nbytes < 0) {
                    err = nbytes;
                    LOG_ERROR("MediaFileSender[%p]: Failed to read from media file: loc %s, err %d", this, location.c_str(), err);
                    goto end;
                }
                if (nbytes == 0) { // EOF
                    break;
                }

                nbytes = hs->Write(buf, nbytes);
                if (nbytes < 0) {
                    err = nbytes;
                    LOG_ERROR("MediaFileSender[%p]: Failed to write to stream: err %d", this, err);
                    goto end;
                }

                offset += nbytes;
            }  // while (1)
        }  // for (i)

        nbytes = hs->Write(footer.data(), footer.size());
        if (nbytes < 0) {
            err = nbytes;
            LOG_ERROR("MediaFileSender[%p]: Failed to write to stream: err %d", this, err);
            goto end;
        }
    }

 end:
    mf->Close();
    return err;
}

void HttpSvc::Sn::MediaFileSender::generateBoundaryString(std::string &boundaryString)
{
    CSL_ShaContext context;
    std::string salt("strm_http_salt--for http multirange separator boundary");
    CSL_ResetSha(&context);
    CSL_InputSha(&context, location.data(), location.size());
    CSL_InputSha(&context, salt.c_str(), salt.size());
    u8 result[CSL_SHA1_DIGESTSIZE];

    CSL_ResultSha(&context, (u8*)result);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(2);
    std::ostream_iterator<u32> osit(oss);
    std::copy(result, result + CSL_SHA1_DIGESTSIZE, osit);
    boundaryString.assign(oss.str());
}

