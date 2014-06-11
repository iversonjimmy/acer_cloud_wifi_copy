#include "SampleResponse.hpp"

#include <cassert>

void SampleResponse::SetResponse(const std::string &response)
{
    this->response = response;
}

const std::string &SampleResponse::GetResponse() const
{
    return response;
}

void SampleResponse::SetAnsStatusCode(int statusCode)
{
    ans_statusCode = statusCode;
}

int SampleResponse::GetAnsStatusCode() const
{
    return ans_statusCode;
}

void SampleResponse::AddAnsHeader(const std::string &name, const std::string &value)
{
    ans_header[name] = value;
}

void SampleResponse::SetAnsHeaderSize(size_t headersize)
{
    ans_headersize = headersize;
}

size_t SampleResponse::GetAnsHeaderSize() const
{
    return ans_headersize;
}

void SampleResponse::Check(const HttpRespHeader &o)
{
    int err = 0;

    {
        int statusCode = 0;
        err = o.GetStatusCode(statusCode);
        assert(!err);
        assert(statusCode == ans_statusCode);
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
