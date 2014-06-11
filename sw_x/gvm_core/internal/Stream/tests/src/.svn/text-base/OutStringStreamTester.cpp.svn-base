#include <vplu_types.h>

#include "OutStringStream.hpp"

#include <log.h>

#include <cassert>
#include <iostream>

class OutStringStreamTester {
public:
    void RunTests();
private:
    void testWriteInOnePart();
    void testWriteInTwoParts();
    void testStopIoWrite();
};

void OutStringStreamTester::testWriteInOnePart()
{
    OutStringStream oss;
    const std::string sample("1234567890");

    ssize_t bytes;

    bytes = oss.Write(sample.data(), sample.size());
    assert(bytes == sample.size());
    assert(oss.GetOutput() == sample);
}

void OutStringStreamTester::testWriteInTwoParts()
{
    OutStringStream oss;
    const std::string sample("1234567890");
    const size_t firstPartSize = 3;

    ssize_t bytes;

    // write first part
    bytes = oss.Write(sample.data(), firstPartSize);
    assert(bytes == firstPartSize);

    // write second part
    bytes = oss.Write(sample.data() + firstPartSize, sample.size() - firstPartSize);
    assert(bytes == sample.size() - firstPartSize);

    assert(oss.GetOutput() == sample);
}

void OutStringStreamTester::testStopIoWrite()
{
    OutStringStream oss;
    const std::string sample("1234567890");
    const size_t firstPartSize = 3;

    ssize_t bytes;

    assert(!oss.IsIoStopped());

    // write first part
    bytes = oss.Write(sample.data(), firstPartSize);
    assert(bytes == firstPartSize);

    oss.StopIo();

    // try to write second part
    bytes = oss.Write(sample.data() + firstPartSize, sample.size() - firstPartSize);
    assert(bytes == VPL_ERR_CANCELED);

    std::string s = oss.GetOutput();
    assert(s.size() == firstPartSize);
    assert(s.compare(0, firstPartSize, sample.data(), firstPartSize) == 0);
}

void OutStringStreamTester::RunTests()
{
#define RUN_TEST(name)                                  \
    test##name();                                       \
    std::cout << "test" #name << ": OK" << std::endl

    RUN_TEST(WriteInOnePart);
    RUN_TEST(WriteInTwoParts);
    RUN_TEST(StopIoWrite);
#undef RUN_TEST
}

int main(int argc, char *argv[])
{
    LOGInit("OutStringStreamTester", NULL);
    LOGSetMax(0);

    VPL_Init();

    OutStringStreamTester t;
    t.RunTests();

    exit(0);
}
