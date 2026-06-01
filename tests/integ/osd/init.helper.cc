#include "lock.fixture.h"
// TODO: port to modern test framework (was testfw/unittest++)
// TODO: port to modern test framework (was testfw/unittest++)
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int InitializeTest(testfw::TestFramework& test_fw)
{
    LockRegionFixture::InitRegion((void*) NULL);
    return 0;
}
