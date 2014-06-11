#ifndef __HTTP_REQ_HEADER_HPP__
#define __HTTP_REQ_HEADER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <string>

// for the time being, depend on http_request class and make the most of it
#include <http_request.hpp>

class HttpReqHeader {
public:
    HttpReqHeader();
    ~HttpReqHeader();

    ssize_t FeedData(const char *data, size_t datasize);
    bool IsComplete() const;

    const std::string &GetMethod() const;
    int GetMethod(std::string &method) const;
    int SetMethod(const std::string &method);
    const std::string &GetUri() const;
    int GetUri(std::string &uri) const;
    int SetUri(const std::string &uri);
    int GetVersion(std::string &version) const;
    int SetVersion(const std::string &version);
    int GetQuery(const std::string &name, std::string &value) const;
    int SetQuery(const std::string &name, const std::string &value);
    int RemoveQuery(const std::string &name);
    int GetHeader(const std::string &name, std::string &value) const;
    int SetHeader(const std::string &name, const std::string &value);
    int RemoveHeader(const std::string &name);

    int Serialize(std::string &out);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(HttpReqHeader);

    http_request req;
};

#endif // incl guard
