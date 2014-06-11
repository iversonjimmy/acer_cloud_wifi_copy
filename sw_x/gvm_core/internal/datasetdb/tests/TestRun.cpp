#include <iostream>

#include "TestRun.hpp"

#include "TestOpenClose.hpp"
#include "TestDatasetVersion.hpp"
#include "TestComponentPath.hpp"
#include "TestComponentMTime.hpp"
#include "TestComponentSize.hpp"
#include "TestComponentHash.hpp"
#include "TestComponentVersion.hpp"
#include "TestComponentMetadata.hpp"
#include "TestTrashComponent.hpp"
#include "TestMoveComponent.hpp"
#include "TestCopyComponent.hpp"
#include "TestDBConstraint.hpp"
#include "TestListComponents.hpp"
#include "TestCreateComponent.hpp"
#include "TestCreateComponentAncestryList.hpp"
#include "TestTransaction.hpp"
#include "TestReplacePrefix.hpp"

#define KNOWN_TESTS(F)							\
F(OpenClose, "test open close access to db")				\
F(DatasetVersion, "test dataset version")				\
F(ComponentPath, "test component path")			\
F(ComponentMTime, "test component mtime")				\
F(ComponentSize, "test component size")					\
F(ComponentHash, "test component hash")					\
F(ComponentVersion, "test component change version")			\
F(ComponentMetadata, "test component metadata")				\
F(TrashComponent, "test trashing a component")				\
F(MoveComponent, "test moving a component")				\
F(CopyComponent, "test copying a component")				\
F(DBConstraint, "test db constraints")					\
F(ListComponents, "test listing components")				\
F(CreateComponent, "test creating a component")				\
F(CreateComponentAncestryList, "test ancestry construction code")	\
F(Transaction, "test db transaction")                                   \
F(ReplacePrefix, "test replaceprefix")

TestRun::TestRun()
{
}

TestRun::~TestRun()
{
}

void TestRun::AddTest(const std::string &name)
{
#define TESTANDADD(t, d)							\
    if (name == #t) { testVec.push_back(new Test ## t(0)); testVec.push_back(new Test ## t(DATASETDB_OPTION_CASE_INSENSITIVE));} else

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
