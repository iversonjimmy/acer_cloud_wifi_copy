#include <vplu_types.h>
#include <vplu_missing.h>

#include <map>
#include <string>

#include "HttpRespHeader.hpp"

class SampleResponse {
public:
    void SetResponse(const std::string &request);
    const std::string &GetResponse() const;

    // Set/Get known answers.
    void SetAnsStatusCode(int code);
    int GetAnsStatusCode() const;
    void AddAnsHeader(const std::string &name, const std::string &value);
    void SetAnsHeaderSize(size_t headersize);
    size_t GetAnsHeaderSize() const;

    // Check against known answers.
    void Check(const HttpRespHeader &hrh);

private:
    std::string response;

    // Known answers.
    int ans_statusCode;
    std::map<std::string, std::string> ans_header;
    size_t ans_headersize;
};
