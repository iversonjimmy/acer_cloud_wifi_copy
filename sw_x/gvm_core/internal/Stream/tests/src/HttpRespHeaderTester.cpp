#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "HttpRespHeader.hpp"
#include "SampleResponse.hpp"

#include <log.h>

#include <cassert>
#include <iostream>

class HttpRespHeaderTester {
public:
    void RunTests();
private:
    SampleResponse sampleGet;

    void testFeedDataInOnePart();
    void testFeedDataInTwoParts();
    void testSerialize();
};

void HttpRespHeaderTester::testFeedDataInOnePart()
{
    {
        const std::string &response = sampleGet.GetResponse();
        HttpRespHeader o;
        ssize_t used = o.FeedData(response.data(), response.size());
        assert(used == sampleGet.GetAnsHeaderSize());
        assert(o.IsComplete());
        sampleGet.Check(o);
    }
}

void HttpRespHeaderTester::testFeedDataInTwoParts()
{
    const size_t firstPartSize = 10;
    {
        const std::string &response = sampleGet.GetResponse();
        HttpRespHeader o;
        ssize_t used;
        used = o.FeedData(response.data(), firstPartSize);
        assert(used == firstPartSize);
        assert(!o.IsComplete());
        used += o.FeedData(response.data() + firstPartSize, response.size() - firstPartSize);
        assert(used == sampleGet.GetAnsHeaderSize());
        assert(o.IsComplete());
        sampleGet.Check(o);
    }
}

void HttpRespHeaderTester::testSerialize()
{
    {
        const std::string &response = sampleGet.GetResponse();
        HttpRespHeader o;
        ssize_t used = o.FeedData(response.data(), response.size());
        assert(used == sampleGet.GetAnsHeaderSize());
        assert(o.IsComplete());
        sampleGet.Check(o);
        std::string out;
        int err = o.Serialize(out);
        assert(!err);
        assert(response.compare(0, sampleGet.GetAnsHeaderSize(), out) == 0);
    }
}

void HttpRespHeaderTester::RunTests()
{
    // create samples
    {
        std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 10\r\n"
            "\r\n";
        std::string body =
            "1234567890";
        sampleGet.SetResponse(header + body);
        sampleGet.SetAnsStatusCode(200);
        sampleGet.AddAnsHeader("Content-Length", "10");
        sampleGet.SetAnsHeaderSize(header.size());
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
    LOGInit("HttpRespHeaderTester", NULL);
    LOGSetMax(0);

    VPL_Init();

    HttpRespHeaderTester t;
    t.RunTests();

    exit(0);
}
