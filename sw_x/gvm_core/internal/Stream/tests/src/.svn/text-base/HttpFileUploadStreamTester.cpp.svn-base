#include "HttpFileUploadStream.hpp"
#include "InStringStream.hpp"
#include "OutStringStream.hpp"
#include "SampleRequest.hpp"

#include <log.h>

#include <cassert>
#include <fstream>
#include <iostream>

class HttpFileUploadStreamTester {
public:
    void RunTests();
private:
    SampleRequest samplePutReq;

    void testReadAllAtOnce();
};

void HttpFileUploadStreamTester::testReadAllAtOnce()
{
    const std::string &request = samplePutReq.GetRequest();
    const std::string reqhdr = request.substr(0, samplePutReq.GetAnsHeaderSize());
    InStringStream iss_reqhdr(reqhdr);
    VPLFile_handle_t file_reqbody = VPLFile_Open("_reqbody", VPLFILE_OPENFLAG_READONLY, 0);
    assert(VPLFile_IsValidHandle(file_reqbody));
    OutStringStream oss_resp;
    HttpFileUploadStream hss(&iss_reqhdr, file_reqbody, &oss_resp);

    char buf[4096];  // large enough to read in a single call
    ssize_t bytes = hss.Read(buf, sizeof(buf));
    assert(bytes == request.size());
    assert(request.compare(0, request.npos, buf, bytes) == 0);

    bytes = hss.Read(buf, sizeof(buf));
    assert(bytes == 0);  // 0 means EOF

    VPLFile_Close(file_reqbody);
}

void HttpFileUploadStreamTester::RunTests()
{
    {
        std::string header =
            "PUT /service/objtype/path?name=value HTTP/1.1\r\n"
            "Content-Length: 10\r\n"
            "\r\n";
        std::string body =
            "1234567890";
        samplePutReq.SetRequest(header + body);
        samplePutReq.SetAnsMethod("PUT");
        samplePutReq.SetAnsUri("/service/objtype/path?name=value");
        samplePutReq.SetAnsVersion("HTTP/1.1");
        samplePutReq.AddAnsQuery("name", "value");
        samplePutReq.AddAnsHeader("Content-Length", "10");
        samplePutReq.SetAnsHeaderSize(header.size());

        std::ofstream ofs("_reqbody", std::ofstream::out);
        ofs << body;
        ofs.close();
    }

#define RUN_TEST(name)                                  \
    test##name();                                       \
    std::cout << "test" #name << ": OK" << std::endl

    RUN_TEST(ReadAllAtOnce);
#undef RUN_TEST
}

int main(int argc, char *argv[])
{
    LOGInit("HttpFileUploadStreamTester", NULL);
    LOGSetMax(0);

    VPL_Init();

    HttpFileUploadStreamTester t;
    t.RunTests();

    exit(0);
}
