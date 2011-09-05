#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <unistd.h>
#include "lock.fixture.hxx"
#include "tool/testfw/argvmap.h"
#include "tool/testfw/testfw.h"


static void InitRegions(int clients_count)
{
	LockFixture::InitRegion((void*) clients_count);
}

int InitializeTests(testfw::TestFramework& test_fw)
{
	std::list<std::string>::iterator it;
	int                              do_init_regions = 0;
	int                              clients_count=0;
	std::string                      strv;

	if (test_fw.ArgVal("initreg", strv)==0) {
		do_init_regions = 1;
	}
	if (test_fw.ArgVal("ccount", strv)==0) {
		clients_count = atoi(strv.c_str());
	}

	if (do_init_regions) {
		InitRegions(clients_count);
	}
	return 0;
}
