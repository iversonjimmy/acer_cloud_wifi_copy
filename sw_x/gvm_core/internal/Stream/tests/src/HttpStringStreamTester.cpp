#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "HttpStringStream.hpp"
#include "InStringStream.hpp"
#include "OutStringStream.hpp"
#include "SampleRequest.hpp"
#include "SampleResponse.hpp"

#include <log.h>

#include <cassert>
#include <iostream>

class HttpStringStreamTester {
public:
    void RunTests();
private:
    SampleRequest sampleGetReq;
    SampleRequest samplePutReq;
    SampleResponse sampleGetResp;

    void testReadAllAtOnce();
    void testReadAllAtOnce(const SampleRequest &req);
    void testReadInBits();
    void testReadInBits(const SampleRequest &req);
    void testReadAttributes();
    void testReadAfterReadAttr();
    void testRead_TwoReqsInStream();
    void testRead_TwoReqsInStream(const SampleRequest &req1, const SampleRequest &req2);
    void testWriteAllAtOnce();
    void testWriteAllAtOnce(const SampleResponse &resp);
    void testWriteOnlyHeader();
    void testWriteOnlyHeader(const SampleResponse &resp);
    void testWriteUptoFirstBodyByte();
    void testWriteUptoFirstBodyByte(const SampleResponse &resp);
    void testSetRespAttrs();
    void testSetRespAttrsThenBody();
};

void HttpStringStreamTester::testReadAllAtOnce()
{
    testReadAllAtOnce(sampleGetReq);
    testReadAllAtOnce(samplePutReq);
}

void HttpStringStreamTester::testReadAllAtOnce(const SampleRequest &req)
{
    const std::string &request = req.GetRequest();
    InStringStream iss(request);
    OutStringStream oss;
    HttpStringStream hss(&iss, &oss);

    char buf[4096];  // large enough to read in a single call
    ssize_t bytes = hss.Read(buf, sizeof(buf));
    assert(bytes == request.size());
    assert(request.compare(0, request.npos, buf, bytes) == 0);

    bytes = hss.Read(buf, sizeof(buf));
    assert(bytes == 0);  // 0 means EOF
}

void HttpStringStreamTester::testReadInBits()
{
    testReadInBits(sampleGetReq);
    testReadInBits(samplePutReq);
}

void HttpStringStreamTester::testReadInBits(const SampleRequest &req)
{
    const std::string &request = req.GetRequest();
    InStringStream _iss(request);
    OutStringStream _oss;
    HttpStringStream hss(&_iss, &_oss);

    char buf[2];  // small enough to require multiple reads
    ssize_t bytes;
    std::ostringstream oss;
    while (1) {
        bytes = hss.Read(buf, sizeof(buf));
        assert(bytes >= 0);
        if (bytes == 0) break;
        oss.write(buf, bytes);
    }
    assert(oss.str().compare(request) == 0);

    bytes = hss.Read(buf, sizeof(buf));
    assert(bytes == 0);  // 0 means EOF
}

void HttpStringStreamTester::testReadAttributes()
{
    {
        const std::string &request = sampleGetReq.GetRequest();
        InStringStream iss(request);
        OutStringStream oss;
        HttpStringStream hss(&iss, &oss);
        std::string method;
        int err = hss.GetMethod(method);
        assert(!err);
        assert(method == sampleGetReq.GetAnsMethod());
        std::string uri;
        err = hss.GetUri(uri);
        assert(!err);
        assert(uri == sampleGetReq.GetAnsUri());
        std::string version;
        err = hss.GetVersion(version);
        assert(!err);
        assert(version == sampleGetReq.GetAnsVersion());
    }
}

void HttpStringStreamTester::testReadAfterReadAttr()
{
    {
        const std::string &request = samplePutReq.GetRequest();
        InStringStream iss(request);
        OutStringStream oss;
        HttpStringStream hss(&iss, &oss);
        std::string method;
        int err = hss.GetMethod(method);
        assert(!err);
        assert(method == "PUT");
        char buf[4096];  // large enough to read in one call
        ssize_t bytes = hss.Read(buf, sizeof(buf));
        assert(bytes == request.size());
        assert(memcmp(buf, request.data(), bytes) == 0);
    }
}

void HttpStringStreamTester::testRead_TwoReqsInStream()
{
    testRead_TwoReqsInStream(sampleGetReq, sampleGetReq);
    testRead_TwoReqsInStream(samplePutReq, sampleGetReq);
}

void HttpStringStreamTester::testRead_TwoReqsInStream(const SampleRequest &req1, const SampleRequest &req2)
{
    // Case: Two requests in the underlying stream.
    //       Read() should only return the first request and then report EOF.

    const std::string &request1 = req1.GetRequest();
    const std::string &request2 = req2.GetRequest();
    InStringStream iss(request1 + request2);  // two reqs back-to-back
    OutStringStream oss;
    HttpStringStream hss(&iss, &oss);
    char buf[4096];  // large enough to read in once call
    ssize_t bytes;

    bytes = hss.Read(buf, sizeof(buf));
    assert(bytes == request1.size());
    assert (memcmp(buf, request1.data(), request1.size()) == 0);

    bytes = hss.Read(buf, sizeof(buf));
    assert(bytes == 0);  // 0 means EOF
}

void HttpStringStreamTester::testWriteAllAtOnce()
{
    testWriteAllAtOnce(sampleGetResp);
}

void HttpStringStreamTester::testWriteAllAtOnce(const SampleResponse &resp)
{
    InStringStream iss(sampleGetReq.GetRequest());
    OutStringStream oss;
    HttpStringStream hss(&iss, &oss);

    const std::string &response = resp.GetResponse();
    ssize_t bytes = hss.Write(response.data(), response.size());
    assert(bytes == response.size());
    assert(oss.GetOutput() == response);
}

void HttpStringStreamTester::testWriteOnlyHeader()
{
    testWriteOnlyHeader(sampleGetResp);
}

void HttpStringStreamTester::testWriteOnlyHeader(const SampleResponse &resp)
{
    // Response header should be buffered completed, only to be written out on Flush or first byte of body.
    // Thus, if only the response buffer is written, nothing should be written out to the underlying stream.

    InStringStream iss(sampleGetReq.GetRequest());
    OutStringStream oss;
    HttpStringStream hss(&iss, &oss);

    const std::string &response = resp.GetResponse();
    ssize_t bytes = hss.Write(response.data(), resp.GetAnsHeaderSize());
    assert(bytes == resp.GetAnsHeaderSize());
    assert(oss.GetOutput().empty());

    int err = hss.Flush();
    assert(!err);
    std::string out = oss.GetOutput();
    assert(response.compare(0, out.size(), out) == 0);
}

void HttpStringStreamTester::testWriteUptoFirstBodyByte()
{
    testWriteUptoFirstBodyByte(sampleGetResp);
}

void HttpStringStreamTester::testWriteUptoFirstBodyByte(const SampleResponse &resp)
{
    // Response header should be buffered completed, only to be written out on Flush or first byte of body.

    InStringStream iss(sampleGetReq.GetRequest());
    OutStringStream oss;
    HttpStringStream hss(&iss, &oss);

    const std::string &response = resp.GetResponse();
    ssize_t bytes = hss.Write(response.data(), resp.GetAnsHeaderSize());
    assert(bytes == sampleGetResp.GetAnsHeaderSize());
    assert(oss.GetOutput().empty());

    bytes = hss.Write(response.data() + resp.GetAnsHeaderSize(), 1);
    assert(bytes == 1);
    assert(oss.GetOutput().compare(0, std::string::npos, response, 0, resp.GetAnsHeaderSize() + 1) == 0);
}

void HttpStringStreamTester::testSetRespAttrs()
{
    InStringStream _iss(sampleGetReq.GetRequest());
    OutStringStream _oss;
    HttpStringStream hss(&_iss, &_oss);

    int err;
    err = hss.SetStatusCode(200);
    assert(!err);
    assert(_oss.GetOutput().empty());
    err = hss.SetRespHeader("Content-Length", "10");
    assert(!err);
    assert(_oss.GetOutput().empty());

    err = hss.Flush();
    assert(!err);
    assert(_oss.GetOutput().compare("HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n") == 0);
}

void HttpStringStreamTester::testSetRespAttrsThenBody()
{
    InStringStream _iss(sampleGetReq.GetRequest());
    OutStringStream _oss;
    HttpStringStream hss(&_iss, &_oss);

    int err;
    err = hss.SetStatusCode(200);
    assert(!err);
    assert(_oss.GetOutput().empty());
    err = hss.SetRespHeader("Content-Length", "10");
    assert(!err);
    assert(_oss.GetOutput().empty());

    ssize_t bytes = hss.Write("1234567890", 10);
    assert(bytes == 10);
    assert(_oss.GetOutput().compare("HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\n1234567890") == 0);
}

void HttpStringStreamTester::RunTests()
{
    // create samples
    {
        std::string header =
            "GET /service/objtype/path?name=value HTTP/1.1\r\n"
            "Header: Value\r\n"
            "\r\n";
        sampleGetReq.SetRequest(header);
        sampleGetReq.SetAnsMethod("GET");
        sampleGetReq.SetAnsUri("/service/objtype/path?name=value");
        sampleGetReq.SetAnsVersion("HTTP/1.1");
        sampleGetReq.AddAnsQuery("name", "value");
        sampleGetReq.AddAnsHeader("Header", "Value");
        sampleGetReq.SetAnsHeaderSize(header.size());
    }
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
    }
    {
        std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 10\r\n"
            "\r\n";
        std::string body =
            "1234567890";
        sampleGetResp.SetResponse(header + body);
        sampleGetResp.SetAnsStatusCode(200);
        sampleGetResp.AddAnsHeader("Content-Length", "10");
        sampleGetResp.SetAnsHeaderSize(header.size());
    }

#define RUN_TEST(name)                                  \
    test##name();                                       \
    std::cout << "test" #name << ": OK" << std::endl

    RUN_TEST(ReadAllAtOnce);
    RUN_TEST(ReadInBits);
    RUN_TEST(ReadAttributes);
    RUN_TEST(ReadAfterReadAttr);
    RUN_TEST(Read_TwoReqsInStream);
    RUN_TEST(WriteAllAtOnce);
    RUN_TEST(WriteOnlyHeader);
    RUN_TEST(WriteUptoFirstBodyByte);
    RUN_TEST(SetRespAttrs);
    RUN_TEST(SetRespAttrsThenBody);
#undef RUN_TEST
}

int main(int argc, char *argv[])
{
    LOGInit("HttpStringStreamTester", NULL);
    LOGSetMax(0);

    VPL_Init();

    HttpStringStreamTester t;
    t.RunTests();

    exit(0);
}
