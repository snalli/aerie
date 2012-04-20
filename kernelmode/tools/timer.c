# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <sched.h>
# include <math.h>

struct timeval start, end;

void print_time_diff()
{
        double st = 0, et = 0;

        et = end.tv_sec;
        et += (end.tv_usec * pow(10,-6));
        st = start.tv_sec;
        st += (start.tv_usec * pow(10,-6));
	printf("time diff : %lf\n", et-st);
	return;
}

void start_timer()
{
	gettimeofday(&start, NULL);
}

void end_timer()
{
	gettimeofday(&end, NULL);
}
