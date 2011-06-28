#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include "param.h"
#include "proc.h"

struct proc _proc;
struct proc *proc = &_proc;


void
print_trace (void)
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;
 
	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);
									      
	printf ("Obtained %zd stack frames.\n", size);
																	      
	for (i = 0; i < size; i++)
		printf ("%s\n", strings[i]);
	free (strings);
}

void panic(char *str)
{
	fprintf(stderr, "PANIC: %s\n", str);
	fflush(stderr);
	print_trace();
	abort();
}


void
my_sleep(void *chan, struct spinlock *lk)
{
	// TODO 
}


void
my_wakeup(void *chan)
{
	// TODO
}
