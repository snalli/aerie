#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <unistd.h>
#include "lock.fixture.hxx"

static void InitRegions(int clients_count)
{
	LockFixture::InitRegion((void*) clients_count);
}

int InitializeTests(std::list<std::string>& args)
{
	std::list<std::string>::iterator it;
	int                              do_init_regions = 0;
	int                              clients_count=0;

	for (it=args.begin(); it!=args.end(); it++) {
		// init regions
		if (*it == "-initreg") { 
			do_init_regions = 1;
		}
		// set clients_ count
		if ((*it).find("-ccount") != std::string::npos) {  
			sscanf((*it).c_str(), "-ccount=%d", &clients_count);
		}
	}
	if (do_init_regions) {
		InitRegions(clients_count);
	}
	return 0;
}
