#include "DatasetDB.hpp"

#include "TestReplacePrefix.hpp"
#include "utils.hpp"

const std::string TestReplacePrefix::dbpath = "testReplacePrefix.db";

TestReplacePrefix::TestReplacePrefix(u32 options) : Test(dbpath, options), testPassed(false)
{
}

TestReplacePrefix::~TestReplacePrefix()
{
}

static void testReplacePrefix(const std::string &name,
			      const std::string &prefix,
			      const std::string &replstr,
			      const std::string &expected_newname)
{
    std::string newname;
    replaceprefix(name, prefix, replstr, newname);
    if (newname != expected_newname) {
        fatal("expected %s got %s", newname.c_str(), expected_newname.c_str());
    }
}

void TestReplacePrefix::RunTests()
{
    try {
        // trivial cases
        testReplacePrefix("", "", "", "");
        testReplacePrefix("a", "", "", "a");
        testReplacePrefix("a", "a", "z", "z");
        testReplacePrefix("a", "b", "z", "a");

        // special handling of empty pattern
        testReplacePrefix("", "", "z", "z");
        testReplacePrefix("a", "", "z", "z/a");
        testReplacePrefix("a/b", "", "z", "z/a/b");

        // special handling of empty replacement string
        testReplacePrefix("a", "a", "", "");
        testReplacePrefix("a/b", "a", "", "b");

        // pattern must be anchored at left-end
        testReplacePrefix("abc", "bc", "zz", "abc");

        // prefix pattern must match upto end or '/'
        testReplacePrefix("abc", "ab", "zz", "abc");
        testReplacePrefix("abc", "abc", "zzz", "zzz");
        testReplacePrefix("abc/def", "ab", "zz", "abc/def");
        testReplacePrefix("abc/def", "abc", "zzz", "zzz/def");

        testPassed = true;
    }
    catch (...) {
        testPassed = false;
    }
}

void TestReplacePrefix::PrintTestResult()
{
    printTestResult("ReplacePrefix", testPassed);
}
