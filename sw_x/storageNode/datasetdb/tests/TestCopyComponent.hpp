#include "Test.hpp"

class TestCopyComponent : public Test {
public:
    TestCopyComponent(u32 options);
    ~TestCopyComponent();
    void RunTests();
    void PrintTestResult();
private:
    static const std::string dbpath;
    bool testPassed;
};
