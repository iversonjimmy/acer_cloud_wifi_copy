#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "HttpReqHeader.hpp"
#include "SampleRequest.hpp"

#include <log.h>

#include <cassert>
#include <iostream>

class HttpReqHeaderTester {
public:
    void RunTests();
private:
    SampleRequest sampleGet;
    SampleRequest samplePut;

    void testFeedDataInOnePart();
    void testFeedDataInTwoParts();
    void testSerialize();
};

void HttpReqHeaderTester::testFeedDataInOnePart()
{
    {
        const std::string &request = sampleGet.GetRequest();
        HttpReqHeader o;
        ssize_t used = o.FeedData(request.data(), request.size());
        assert(used == sampleGet.GetAnsHeaderSize());
        assert(o.IsComplete());
        sampleGet.Check(o);
    }
    {
        // This also tests the case of feeding request body.
        const std::string &request = samplePut.GetRequest();
        HttpReqHeader o;
        ssize_t used = o.FeedData(request.data(), request.size());
        assert(used == samplePut.GetAnsHeaderSize());
        assert(o.IsComplete());
        samplePut.Check(o);
    }
}

void HttpReqHeaderTester::testFeedDataInTwoParts()
{
    const size_t firstPartSize = 10;
    {
        const std::string &request = sampleGet.GetRequest();
        HttpReqHeader o;
        ssize_t used;
        used = o.FeedData(request.data(), firstPartSize);
        assert(used == firstPartSize);
        assert(!o.IsComplete());
        used += o.FeedData(request.data() + firstPartSize, request.size() - firstPartSize);
        assert(used == sampleGet.GetAnsHeaderSize());
        assert(o.IsComplete());
        sampleGet.Check(o);
    }
    {
        // This also tests the case of feeding request body.
        const std::string &request = samplePut.GetRequest();
        HttpReqHeader o;
        ssize_t used;
        used = o.FeedData(request.data(), firstPartSize);
        assert(used == firstPartSize);
        assert(!o.IsComplete());
        used += o.FeedData(request.data() + firstPartSize, request.size() - firstPartSize);
        assert(used == samplePut.GetAnsHeaderSize());
        assert(o.IsComplete());
        samplePut.Check(o);
    }
}

void HttpReqHeaderTester::testSerialize()
{
    {
        const std::string &request = sampleGet.GetRequest();
        HttpReqHeader o;
        ssize_t used = o.FeedData(request.data(), request.size());
        assert(used == sampleGet.GetAnsHeaderSize());
        assert(o.IsComplete());
        sampleGet.Check(o);
        std::string out;
        int err = o.Serialize(out);
        assert(!err);
        assert(request.compare(0, sampleGet.GetAnsHeaderSize(), out) == 0);
    }
    {
        // This also tests the case of feeding request body.
        const std::string &request = samplePut.GetRequest();
        HttpReqHeader o;
        ssize_t used = o.FeedData(request.data(), request.size());
        assert(used == samplePut.GetAnsHeaderSize());
        assert(o.IsComplete());
        samplePut.Check(o);
        std::string out;
        int err = o.Serialize(out);
        assert(!err);
        assert(request.compare(0, samplePut.GetAnsHeaderSize(), out) == 0);
    }
}

void HttpReqHeaderTester::RunTests()
{
    // create samples
    {
        std::string header =
            "GET /service/objtype/path?name=value HTTP/1.1\r\n"
            "Header: Value\r\n"
            "\r\n";
        sampleGet.SetRequest(header);
        sampleGet.SetAnsMethod("GET");
        sampleGet.SetAnsUri("/service/objtype/path?name=value");
        sampleGet.SetAnsVersion("HTTP/1.1");
        sampleGet.AddAnsQuery("name", "value");
        sampleGet.AddAnsHeader("Header", "Value");
        sampleGet.SetAnsHeaderSize(header.size());
    }
    {
        std::string header =
            "PUT /service/objtype/path?name=value HTTP/1.1\r\n"
            "Content-Length: 10\r\n"
            "\r\n";
        std::string body =
            "1234567890";
        samplePut.SetRequest(header + body);
        samplePut.SetAnsMethod("PUT");
        samplePut.SetAnsUri("/service/objtype/path?name=value");
        samplePut.SetAnsVersion("HTTP/1.1");
        samplePut.AddAnsQuery("name", "value");
        samplePut.AddAnsHeader("Content-Length", "10");
        samplePut.SetAnsHeaderSize(header.size());
    }

#define RUN_TEST(name)                                  \
    test##name();                                       \
    std::cout << "test" #name << ": OK" << std::endl

    RUN_TEST(FeedDataInOnePart);
    RUN_TEST(FeedDataInTwoParts);
    RUN_TEST(Serialize);
#undef RUN_TEST
}

int main(int argc, char *argv[])
{
    LOGInit("HttpReqHeaderTester", NULL);
    LOGSetMax(0);

    VPL_Init();

    HttpReqHeaderTester t;
    t.RunTests();

    exit(0);
}
