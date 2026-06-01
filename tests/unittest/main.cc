#include "testfw/testfw.h"
#include <iostream>
#include <string.h>

namespace testfw
{
TestFramework* __testfwp;
}

int main(int argc, char** argv)
{
    extern char* optarg;
    int c;
    char* suiteName;
    char* testName;

    testfw::TestFramework test_fw(argc, argv);
    testfw::__testfwp = &test_fw;

    return test_fw.RunTests();
}
