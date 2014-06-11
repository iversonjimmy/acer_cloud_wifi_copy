#include "Test.hpp"

class TestTransaction : public Test {
public:
    TestTransaction(u32 options);
    ~TestTransaction();
    void RunTests();
    void PrintTestResult();
private:
    static const std::string dbpath;
    bool testPassed;
};
