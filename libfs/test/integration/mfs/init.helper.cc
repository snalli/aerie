#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <unistd.h>
#include "../osd/lock.fixture.h"
#include "tool/testfw/argvmap.h"
#include "tool/testfw/testfw.h"


int InitializeTest(testfw::TestFramework& test_fw)
{
	LockRegionFixture::InitRegion((void*) NULL);
	return 0;
}
