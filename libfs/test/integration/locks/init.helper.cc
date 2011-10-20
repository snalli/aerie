#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <unistd.h>
#include "lock.fixture.h"
#include "tool/testfw/argvmap.h"
#include "tool/testfw/testfw.h"


int InitializeTest(testfw::TestFramework& test_fw)
{
	LockRegionFixture::InitRegion((void*) test_fw.numclients());
	return 0;
}
