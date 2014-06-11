#include "SampleRequest.hpp"

#include <cassert>

void SampleRequest::SetRequest(const std::string &request)
{
    this->request = request;
}

const std::string &SampleRequest::GetRequest() const
{
    return request;
}

void SampleRequest::SetAnsMethod(const std::string &method)
{
    ans_method = method;
}

const std::string &SampleRequest::GetAnsMethod() const
{
    return ans_method;
}

void SampleRequest::SetAnsUri(const std::string &uri)
{
    ans_uri = uri;
}
const std::string &SampleRequest::GetAnsUri() const
{
    return ans_uri;
}

void SampleRequest::SetAnsVersion(const std::string &version)
{
    ans_version = version;
}

const std::string &SampleRequest::GetAnsVersion() const
{
    return ans_version;
}

void SampleRequest::AddAnsQuery(const std::string &name, const std::string &value)
{
    ans_query[name] = value;
}

void SampleRequest::AddAnsHeader(const std::string &name, const std::string &value)
{
    ans_header[name] = value;
}

void SampleRequest::SetAnsHeaderSize(size_t headersize)
{
    ans_headersize = headersize;
}

size_t SampleRequest::GetAnsHeaderSize() const
{
    return ans_headersize;
}

void SampleRequest::Check(const HttpReqHeader &o)
{
    int err = 0;

    {
        std::string method;
        err = o.GetMethod(method);
        assert(!err);
        assert(method == ans_method);
    }

    {
        std::string uri;
        err = o.GetUri(uri);
        assert(!err);
        assert(uri == ans_uri);
    }

    {
        std::map<std::string, std::string>::const_iterator it;
        for (it = ans_query.begin(); it != ans_query.end(); it++) {
            std::string value;
            err = o.GetQuery(it->first, value);
            assert(!err);
            assert(value == it->second);
        }
    }

    {
        std::map<std::string, std::string>::const_iterator it;
        for (it = ans_header.begin(); it != ans_header.end(); it++) {
            std::string value;
            err = o.GetHeader(it->first, value);
            assert(!err);
            assert(value == it->second);
        }
    }
}
