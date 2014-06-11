#include <vplu_types.h>
#include <vplu_missing.h>

#include <map>
#include <string>

#include "HttpReqHeader.hpp"

class SampleRequest {
public:
    void SetRequest(const std::string &request);
    const std::string &GetRequest() const;

    // Set/Get known answers.
    void SetAnsMethod(const std::string &method);
    const std::string &GetAnsMethod() const;
    void SetAnsUri(const std::string &uri);
    const std::string &GetAnsUri() const;
    void SetAnsVersion(const std::string &version);
    const std::string &GetAnsVersion() const;
    void AddAnsQuery(const std::string &name, const std::string &value);
    void AddAnsHeader(const std::string &name, const std::string &value);
    void SetAnsHeaderSize(size_t headersize);
    size_t GetAnsHeaderSize() const;

    // Check against known answers.
    void Check(const HttpReqHeader &hrh);

private:
    std::string request;

    // Known answers.
    std::string ans_method;
    std::string ans_uri;
    std::string ans_version;
    std::map<std::string, std::string> ans_query;
    std::map<std::string, std::string> ans_header;
    size_t ans_headersize;
};
