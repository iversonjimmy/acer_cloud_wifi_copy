#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "InStringStream.hpp"

#include <log.h>

#include <cassert>
#include <iostream>

class InStringStreamTester {
public:
    void RunTests();
private:
    void testReadInOnePart();
    void testReadInTwoParts();
    void testReadPastEof();
    void testStopIoRead();
};

void InStringStreamTester::testReadInOnePart()
{
    const std::string sample("1234567890");
    InStringStream iss(sample);

    char buf[sample.size()];
    ssize_t bytes;

    bytes = iss.Read(buf, sizeof(buf));
    assert(bytes == sample.size());
    assert(sample.compare(0, sample.size(), buf, bytes) == 0);
}

void InStringStreamTester::testReadInTwoParts()
{
    const std::string sample("1234567890");
    InStringStream iss(sample);
    const size_t firstPartSize = 3;

    char buf[sample.size()];
    ssize_t bytes;

    // read first part
    bytes = iss.Read(buf, firstPartSize);
    assert(bytes == firstPartSize);
    assert(sample.compare(0, firstPartSize, buf, bytes) == 0);

    // read second part
    // note: this also tests read-upto-eof case
    bytes = iss.Read(buf, sizeof(buf));
    assert(bytes == sample.size() - firstPartSize);
    assert(sample.compare(firstPartSize, sample.size() - firstPartSize, buf, bytes) == 0);
}

void InStringStreamTester::testReadPastEof()
{
    const std::string sample("1234567890");
    InStringStream iss(sample);

    char buf[sample.size()];
    ssize_t bytes;

    bytes = iss.Read(buf, sizeof(buf));
    assert(bytes == sample.size());
    assert(sample.compare(0, sample.size(), buf, bytes) == 0);

    bytes = iss.Read(buf, sizeof(buf));
    assert(bytes == 0);  // 0 means EOF
}

void InStringStreamTester::testStopIoRead()
{
    const std::string sample("1234567890");
    InStringStream iss(sample);
    const size_t firstPartSize = 3;

    char buf[sample.size()];
    ssize_t bytes;

    assert(!iss.IsIoStopped());

    // read first part
    bytes = iss.Read(buf, firstPartSize);
    assert(bytes == firstPartSize);
    assert(sample.compare(0, firstPartSize, buf, bytes) == 0);

    iss.StopIo();

    // try to read second part
    bytes = iss.Read(buf, sizeof(buf));
    assert(bytes == VPL_ERR_CANCELED);
}

void InStringStreamTester::RunTests()
{
#define RUN_TEST(name)                                  \
    test##name();                                       \
    std::cout << "test" #name << ": OK" << std::endl

    RUN_TEST(ReadInOnePart);
    RUN_TEST(ReadInTwoParts);
    RUN_TEST(ReadPastEof);
    RUN_TEST(StopIoRead);
#undef RUN_TEST
}

int main(int argc, char *argv[])
{
    LOGInit("InStringStreamTester", NULL);
    LOGSetMax(0);

    VPL_Init();

    InStringStreamTester t;
    t.RunTests();

    exit(0);
}
