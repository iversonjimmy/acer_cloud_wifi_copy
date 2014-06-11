#include "Test.hpp"

class TestOpenClose : public Test {
public:
    TestOpenClose(u32 options);
    ~TestOpenClose();
    void RunTests();
    void PrintTestResult();
private:
    static const std::string dbpath;
    bool testPassed;
};
