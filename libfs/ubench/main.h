#ifndef __STAMNOS_UBENCH_MAIN_H
#define __STAMNOS_UBENCH_MAIN_H

#include <vector>
#include <string>

struct UbenchDescriptor {
	UbenchDescriptor(const char* name, int (*foo)(int, char* []))
		: ubench_name(name),
		  ubench_function(foo)
	{ }

	std::string ubench_name;
	int (*ubench_function)(int, char* []);
};

extern std::vector<UbenchDescriptor> ubench_table;

#endif // __STAMNOS_UBENCH_MAIN_H
