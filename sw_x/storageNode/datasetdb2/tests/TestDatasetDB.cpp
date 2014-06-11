#include <iostream>
#include <cstdlib>
#include <cstring>

#include "TestRun.hpp"

int main(int argc, char *argv[])
{
    if (argc == 1) {
	std::cout << "Usage: " << argv[0] << " [ all | list | tests ... ]" << std::endl;
	exit(0);
    }

    TestRun testRun;

    if (argc == 2 && strcmp(argv[1], "all") == 0) {
	testRun.AddAllTests();
    }
    else if (argc == 2 && strcmp(argv[1], "list") == 0) {
	testRun.ListKnownTests();
    }
    else {
	for (int i = 1; i < argc; i++)
	    testRun.AddTest(argv[i]);
    }

    testRun.RunTests();

    testRun.DestroyTests();

    exit(0);
}
