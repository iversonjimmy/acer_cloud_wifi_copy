#include "Test.hpp"

class TestDatasetVersion : public Test {
public:
    TestDatasetVersion(u32 options);
    ~TestDatasetVersion();
    void RunTests();
    void PrintTestResult();
private:
    static const std::string dbpath;
    bool testPassed;
};
