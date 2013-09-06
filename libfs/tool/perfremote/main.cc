#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <list>
#include <iostream>
#include "error.h"
#include "monitor.h"

static void 
usage(const char* name)
{
	std::cout << "usage: -p PERF_PATH [-r PORT]" << std::endl;
	exit(1);
}


int main(int argc, char** argv)
{
	const char*      progname = "perfremote";
	Monitor::Server* monitor;
	int              serv_port = MONITOR_PORT;
	char*            perf_path = NULL;
	char             ch;

	opterr=0;
	while ((ch = getopt(argc, argv, "p:r:"))!=-1) {
		switch (ch) {
			case 'r':
				serv_port = atoi(optarg);
				break;
			case 'p':
				perf_path = optarg;
				break;
			case '?':
				usage(progname);
			default:
				break;
		}
	}
	if (!perf_path) {
		usage(progname);
	}

	monitor = Monitor::Server::create(perf_path, serv_port);
	monitor->run();
	return 0;
}
