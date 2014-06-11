#ifndef __HTTP_STREAM_HPP__
#define __HTTP_STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <string>

#include "Stream.hpp"
#include "HttpReqHeader.hpp"
#include "HttpRespHeader.hpp"

class HttpStream : public Stream {
public:
    HttpStream();
    virtual ~HttpStream();

    // interface inherited from Stream
    ssize_t Read(char *buf, size_t bufsize);
    ssize_t Write(const char *data, size_t datasize);
    int Flush();
    virtual void StopIo() = 0;
    virtual bool IsIoStopped() const = 0;

    // new interface
    const std::string &GetMethod();
    int GetMethod(std::string &method);
    int SetMethod(const std::string &method);
    const std::string &GetUri();
    int GetUri(std::string &uri);
    int SetUri(const std::string &uri);
    int GetVersion(std::string &version);
    int SetVersion(const std::string &version);
    int GetQuery(const std::string &name, std::string &value);
    int SetQuery(const std::string &name, const std::string &value);
    int RemoveQuery(const std::string &name);
    int GetReqHeader(const std::string &name, std::string &value);
    int SetReqHeader(const std::string &name, const std::string &value);
    int RemoveReqHeader(const std::string &name);

    int GetStatusCode();
    int SetStatusCode(int code);
    int GetStatusCode(int &code);
    int SetRespHeader(const std::string &name, const std::string &value);
    int GetRespHeader(const std::string &name, std::string &value);
    int RemoveRespHeader(const std::string &name);

    u64 GetRespBodyBytesWritten() const;

    // Auxiliary data

    /// This will always be set before #Handler::_Run() is called.
    u64 GetUserId() const;
    void SetUserId(u64 userId);

    /// This must be set to the destination deviceId when forwarding the
    /// #HttpStream via #HttpSvc::Ccd::Handler_Helper::ForwardToServerCcd() or
    /// #HttpSvc::HsToTsAdapter::Run().
    /// It will not be available before then.
    u64 GetDeviceId() const;
    void SetDeviceId(u64 deviceId);

protected:
    HttpReqHeader reqHdr;
    HttpRespHeader respHdr;

    int loadReqHeader();
    virtual ssize_t read_header(char *buf, size_t bufsize) = 0;
    virtual ssize_t read_body(char *buf, size_t bufsize) = 0;
    bool reqHdrAllRead;
    size_t reqHdrReadBytes;
    u64 reqBodyReadBytes;

    virtual ssize_t write_header(const char *data, size_t datasize) = 0;
    virtual ssize_t write_body(const char *data, size_t datasize) = 0;
    bool respHdrPushed;
    u64 respBodyBytesWritten;

    // auxiliary data
    // HttpStream does not depend on these data.
    u64 userId;
    u64 deviceId;
};

#endif // incl guard
