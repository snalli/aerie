#include "lock.fixture.h"
#include "tool/testfw/argvmap.h"
#include "tool/testfw/testfw.h"
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int InitializeTest(testfw::TestFramework& test_fw)
{
    LockRegionFixture::InitRegion((void*) NULL);
    return 0;
}
