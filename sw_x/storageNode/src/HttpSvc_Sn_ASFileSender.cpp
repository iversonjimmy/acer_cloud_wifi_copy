#include "HttpSvc_Sn_ASFileSender.hpp"
#include "HttpSvc_Sn_MediaFile.hpp"
#include <md5.h>
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
static const size_t ASFile_ReadBufSize = 1024 * 1024;  // 1MB
#else
static const size_t ASFile_ReadBufSize = 64 * 1024;  // 64KB
#endif

static void genHashString(const unsigned char* hash, std::string& hashString, int digestSize)
{
    std::ostringstream oss;
    // Write recorded hash
    hashString.clear();
    oss << std::hex;
    for(int hashIndex = 0; hashIndex<digestSize; hashIndex++) {
        oss << std::setfill('0')
            << std::setw(2)
            << static_cast<unsigned>(hash[hashIndex]);
    }
    hashString.assign(oss.str());
}

HttpSvc::Sn::ASFileSender::ASFileSender(MediaFile *mf, u64 mtime, const std::string& hash, HttpStream *hs)
    : mf(mf), originalModTime(mtime), originalHash(hash), hs(hs)
{
}

HttpSvc::Sn::ASFileSender::~ASFileSender()
{
}

int HttpSvc::Sn::ASFileSender::Send()
{
    int err; 
    u64 absFirstBytePos = 0, absLastBytePos = 0;

    err = processRangeSpec();
    if (err) {
        return 0; // processRangeSpec() already sent response 416
    }


    // There is no point for partial and multi-range transfer. If client request
    // that, it is a bug and terminate the transfer. Remove following check if 
    // we support that in the future.
    s64 filesize = mf->GetSize(); 
    if (filesize < 0) {
        err = (int)filesize;
        return err;
    }

    if (ranges.size() > 1) {
        LOG_ERROR("Multi-range not supported");
        return -1;
    } else if (ranges.size() == 1) {
        err = VPLHttp_ConvToAbsRange(ranges[0], filesize, absFirstBytePos, absLastBytePos);
        if (err) {
            LOG_ERROR("Handle_mm[%p]: Bad range: err %d", this, err);
            return -1;
        }
        if ((absFirstBytePos != 0) || (absLastBytePos != (filesize - 1))) {
            LOG_ERROR("Partial file transfer not supported");
            return -1;
        }
    }

    if (hs->GetMethod() == "HEAD") {
        return sendHttpHeaders();
    }

    return sendWholeFile();
}

int HttpSvc::Sn::ASFileSender::processRangeSpec()
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

int HttpSvc::Sn::ASFileSender::sendHttpHeaders()
{
    int err = 0;

    HttpStream_Helper::AddStdRespHeaders(hs);
    hs->SetStatusCode(200);

    s64 filesize = mf->GetSize();
    if (filesize < 0) {
        err = (int)filesize;
        LOG_ERROR("Handle_mm[%p]: Failed to get media file size: err %d", this, err);
        goto end;
    }

    {
        std::ostringstream oss;
        oss << filesize;
        hs->SetRespHeader("Content-Length", oss.str());
    }

    hs->SetRespHeader("Connection", "close");
    hs->Flush();

end:
    return err;
}

int HttpSvc::Sn::ASFileSender::sendWholeFile()
{
    int err = 0;
    unsigned char hashValue[MD5_DIGESTSIZE];
    MD5Context hashContext;

    {
        HttpStream_Helper::AddStdRespHeaders(hs);
        hs->SetStatusCode(200);
        hs->SetRespHeader("Content-Type", "text");

        s64 filesize = mf->GetSize();
        if (filesize < 0) {
            err = (int)filesize;
            LOG_ERROR("Handle_mm[%p]: Failed to get media file size: err %d", this, err);
            goto end;
        }

        {
            std::ostringstream oss;
            oss << filesize;
            hs->SetRespHeader("Content-Length", oss.str());
        }

        hs->SetRespHeader("Connection", "close");
        hs->Flush();

        if (!originalHash.empty()) {
            err = MD5Reset(&hashContext);
            if (err < 0) {
                LOG_ERROR("MD5Reset fail err %d", err);
                goto end;
            }
        }

        err = mf->Open();
        if (err < 0) {
            LOG_ERROR("Handle_mm[%p]: Failed to open media file: err %d", this, err);
            goto end;
        }

        char *buf = new (std::nothrow) char[ASFile_ReadBufSize];
        if (!buf) {
            err = CCD_ERROR_NOMEM;
            LOG_ERROR("MediaFileSender[%p]: Insufficient memory for buffer", this);
            goto end;
        }
        ON_BLOCK_EXIT(deleteArray<char>, buf);

        u64 offset = 0;

        // TODO: bug 17346 
        // Hash computation is serialized with hs->Write() in initial implementation.
        // Note that the TS_Write() actually is blocked until ACK by destination.
        // The overhead should still be small though compared to the time it takes 
        // to send through the data. Nevertheless, further optimization should be 
        // made to pipeline the operations.
        while (1) {
            size_t reqsize = (size_t)MIN(filesize - offset, ASFile_ReadBufSize);

            ssize_t nbytes = mf->ReadAt(offset, buf, reqsize);
            if (nbytes < 0) {
                err = nbytes;
                LOG_ERROR("ASFileSender[%p]: Failed to read from file: err %d", this, err);
                goto end;
            }

            if (nbytes == 0)  // EOF
                break;

            if (!originalHash.empty()) {
                err = MD5Input(&hashContext, buf, nbytes);
                if (err != 0) {
                    LOG_ERROR("MD5Input() error at %d", (int)offset);
                    goto end;
                }

                if (offset + nbytes == filesize) {  // The last block
                    std::string hashStr;
                    err = MD5Result(&hashContext, hashValue);
                    if (err != 0) {
                        LOG_ERROR("MD5Result() error");
                        goto end;
                    }
                    genHashString(hashValue, hashStr, MD5_DIGESTSIZE);
                    if (hashStr.compare(originalHash) != 0) {
                        LOG_ERROR("File changed, terminate connection");
                        LOG_ERROR("  expected:%s", originalHash.c_str());
                        LOG_ERROR("  actual  :%s", hashStr.c_str()); 
                        goto end;
                    }
                }
            }

            nbytes = hs->Write(buf, nbytes);
            if (nbytes < 0) {
                err = nbytes;
                LOG_ERROR("ASFileSender[%p]: Failed to write to stream: err %d", this, err);
                goto end;
            } 

            offset += nbytes;
        }
    }

 end:
    mf->Close();
    return err;
}

