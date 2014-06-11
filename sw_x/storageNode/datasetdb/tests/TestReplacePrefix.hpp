#include "Test.hpp"

class TestReplacePrefix : public Test {
public:
    TestReplacePrefix(u32 options);
    ~TestReplacePrefix();
    void RunTests();
    void PrintTestResult();
private:
    static const std::string dbpath;
    bool testPassed;
};
