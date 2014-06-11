#ifndef __HTTP_RESP_HEADER_HPP__
#define __HTTP_RESP_HEADER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

// for the time being, depend on http_response class and make the most of it
#include <http_response.hpp>

class HttpRespHeader {
public:
    HttpRespHeader();
    ~HttpRespHeader();

    ssize_t FeedData(const char *data, size_t datasize);
    bool IsComplete() const;

    int GetStatusCode() const;
    int GetStatusCode(int &code) const;
    int SetStatusCode(int code);
    int GetHeader(const std::string &name, std::string &value) const;
    int SetHeader(const std::string &name, const std::string &value);
    int RemoveHeader(const std::string &name);

    int Serialize(std::string &out);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(HttpRespHeader);

    http_response resp;
};

#endif // incl guard
