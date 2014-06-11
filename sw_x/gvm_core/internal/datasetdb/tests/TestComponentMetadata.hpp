#include "Test.hpp"

class TestComponentMetadata : public Test {
public:
    TestComponentMetadata(u32 options);
    ~TestComponentMetadata();
    void RunTests();
    void PrintTestResult();
private:
    static const std::string dbpath;
    bool testPassed;
};
