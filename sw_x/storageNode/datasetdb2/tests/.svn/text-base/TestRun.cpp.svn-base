#include <iostream>

#include "TestRun.hpp"

#include "TestOpenClose.hpp"
#include "TestCreateComponent.hpp"
#include "TestComponentSize.hpp"
#include "TestComponentPath.hpp"
#include "TestComponentPermission.hpp"
#include "TestComponentVersion.hpp"
#include "TestComponentMTime.hpp"
#include "TestListComponents.hpp"
#include "TestBigDB.hpp"
#include "TestTransaction.hpp"
#include "TestMoveComponent.hpp"
#include "TestDatasetVersion.hpp"
//#include "TestDBConstraint.hpp"

#define KNOWN_TESTS(F)                                  \
F(OpenClose, "test open close access to db")            \
F(CreateComponent, "test creating a component")         \
F(ComponentSize, "test component size")                 \
F(ComponentPath, "test component path")                 \
F(ComponentPermission, "test component permission")     \
F(ComponentVersion, "test component change version")    \
F(ComponentMTime, "test component mtime")               \
F(ListComponents, "test listing components")            \
F(BigDB, "test with big DB")                            \
F(Transaction, "test db transaction")                   \
F(MoveComponent, "test moving a component")             \
F(DatasetVersion, "test dataset version")

#if 0
F(DBConstraint, "test db constraints")                                  \

#endif

TestRun::TestRun()
{
}

TestRun::~TestRun()
{
}

void TestRun::AddTest(const std::string &name)
{
#define TESTANDADD(t, d)							\
    if (name == #t) { testVec.push_back(new Test ## t(DATASETDB_OPTION_CASE_INSENSITIVE));} else

    KNOWN_TESTS(TESTANDADD)
	std::cerr << "ERROR: unknown test " << name << " ignored" << std::endl;

#undef TESTANDADD
}

void TestRun::AddAllTests()
{
#define ADD(t, d)				\
    AddTest(#t);

    KNOWN_TESTS(ADD)

#undef ADD
}

void TestRun::ListKnownTests()
{
#define SHOW(t, d)				\
    std::cout << #t << ", " << d << std::endl;

    KNOWN_TESTS(SHOW)

#undef SHOW
}

void TestRun::RunTests()
{
    std::vector<Test*>::iterator it;
    for (it = testVec.begin(); it != testVec.end(); it++) {
	(*it)->RunTests();
        (*it)->PrintTestResult();
    }
}

void TestRun::DestroyTests()
{
    std::vector<Test*>::iterator it;
    for (it = testVec.begin(); it != testVec.end(); it++) {
	delete *it;
        *it = NULL;
    }
}
