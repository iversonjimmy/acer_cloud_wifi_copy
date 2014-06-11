#include "HttpReqHeader.hpp"

#include <gvm_errors.h>

HttpReqHeader::HttpReqHeader()
{
}

HttpReqHeader::~HttpReqHeader()
{
}

ssize_t HttpReqHeader::FeedData(const char *data, size_t datasize)
{
    if (IsComplete())
        return 0;

    size_t used = req.receive(data, datasize);
    if (IsComplete()) {
        // decrement used by size of body received
        return (ssize_t)used - req.content_available();
    }
    else {
        return (ssize_t)used;
    }
}

bool HttpReqHeader::IsComplete() const
{
    return req.headers_complete();
}

const std::string &HttpReqHeader::GetMethod() const
{
    return req.http_method;
}

int HttpReqHeader::GetMethod(std::string &method) const
{
    method.assign(req.http_method);
    return 0;
}

int HttpReqHeader::SetMethod(const std::string &method)
{
    req.http_method.assign(method);
    return 0;
}

const std::string &HttpReqHeader::GetUri() const
{
    return req.uri;
}

int HttpReqHeader::GetUri(std::string &uri) const
{
    uri.assign(req.uri);
    return 0;
}

int HttpReqHeader::SetUri(const std::string &uri)
{
    req.uri.assign(uri);
    return 0;
}

int HttpReqHeader::GetVersion(std::string &version) const
{
    version.assign(req.version);
    return 0;
}

int HttpReqHeader::SetVersion(const std::string &version)
{
    req.version.assign(version);
    return 0;
}

int HttpReqHeader::GetQuery(const std::string &name, std::string &value) const
{
    const std::string *_value = req.find_query(name);
    if (!_value)
        return CCD_ERROR_NOT_FOUND;
    value.assign(*_value);
    return 0;
}

int HttpReqHeader::SetQuery(const std::string &name, const std::string &value)
{
    req.query[name] = value;
    return 0;
}

int HttpReqHeader::RemoveQuery(const std::string &name)
{
    http_request::query_list::iterator it = req.query.find(name);
    if (it == req.query.end()) {
        return -CCD_ERROR_NOT_FOUND;  // return as positive value to indicate warning
    }
    req.query.erase(it);
    return 0;
}

int HttpReqHeader::GetHeader(const std::string &name, std::string &value) const
{
    const std::string *_value = req.find_header(name);
    if (!_value)
        return CCD_ERROR_NOT_FOUND;
    value.assign(*_value);
    return 0;
}

int HttpReqHeader::SetHeader(const std::string &name, const std::string &value)
{
    req.headers[name] = value;
    return 0;
}

int HttpReqHeader::RemoveHeader(const std::string &name)
{
    http_request::header_list::iterator it = req.headers.find(name);
    if (it == req.headers.end()) {
        return -CCD_ERROR_NOT_FOUND;  // return as positive value to indicate warning
    }
    req.headers.erase(it);
    return 0;
}

int HttpReqHeader::Serialize(std::string &out)
{
    req.dump(out);
    // http_request::dump(std::string&) includes any body it saw, so strip it out.
    if (req.content_available() > 0) {
        out.resize(out.size() - req.content_available());
    }
    return 0;
}
