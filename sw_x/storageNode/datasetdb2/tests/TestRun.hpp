#ifndef __TESTRUN_HPP__
#define __TESTRUN_HPP__

#include "Test.hpp"

class TestRun {
public:
    TestRun();
    ~TestRun();
    void AddTest(const std::string &name);
    void AddAllTests();
    void ListKnownTests();
    void RunTests();
    void DestroyTests();
private:
    std::vector<Test*> testVec;
};

#endif
