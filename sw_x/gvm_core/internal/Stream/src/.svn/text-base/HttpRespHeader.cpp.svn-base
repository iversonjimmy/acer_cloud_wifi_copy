#include "HttpRespHeader.hpp"

#include <gvm_errors.h>

HttpRespHeader::HttpRespHeader()
{
}

HttpRespHeader::~HttpRespHeader()
{
}

ssize_t HttpRespHeader::FeedData(const char *data, size_t datasize)
{
    if (IsComplete())
        return 0;

    ssize_t used = resp.add_input(data, datasize);
    if (IsComplete()) {
        // decrease used by the amount of body consumed
        return (ssize_t)used - resp.content.size();
    }
    else {
        return (ssize_t)used;
    }
}

bool HttpRespHeader::IsComplete() const
{
    return resp.headers_done;
}

int HttpRespHeader::GetStatusCode() const
{
#ifdef FUMI_LATER
    if (!resp.headers_done)
        resp.headers_done = true;
#endif

    return resp.response;
}

int HttpRespHeader::GetStatusCode(int &code) const
{
#ifdef FUMI_LATER
    if (!resp.headers_done)
        resp.headers_done = true;
#endif

    code = resp.response;
    return 0;
}

int HttpRespHeader::SetStatusCode(int code)
{
    if (!resp.headers_done)
        resp.headers_done = true;

    resp.response = code;
    return 0;
}

int HttpRespHeader::GetHeader(const std::string &name, std::string &value) const
{
#ifdef FUMI_LATER
    if (!resp.headers_done)
        resp.headers_done = true;
#endif

    const std::string *_value = resp.find_header(name);
    if (!_value)
        return CCD_ERROR_NOT_FOUND;
    value.assign(*_value);
    return 0;
}

int HttpRespHeader::SetHeader(const std::string &name, const std::string &value)
{
    if (!resp.headers_done)
        resp.headers_done = true;

    resp.headers[name] = value;
    return 0;
}

int HttpRespHeader::RemoveHeader(const std::string &name)
{
    if (!resp.headers_done)
        resp.headers_done = true;

    http_response::header_list::iterator it = resp.headers.find(name);
    if (it == resp.headers.end()) {
        return -CCD_ERROR_NOT_FOUND;  // return as positive value to indicate warning
    }
    resp.headers.erase(it);
    return 0;
}

int HttpRespHeader::Serialize(std::string &out)
{
    int err = 0;
    resp.dump_headers(out);
    return err;
}
