#include "DatasetDB.hpp"

#include "TestOpenClose.hpp"
#include "utils.hpp"

const std::string TestOpenClose::dbpath = "testOpenClose.db";

TestOpenClose::TestOpenClose(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestOpenClose::~TestOpenClose()
{
}

void TestOpenClose::RunTests()
{
    try {
        // nothing to do

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestOpenClose::PrintTestResult()
{
    printTestResult("OpenClose", testPassed);
}
