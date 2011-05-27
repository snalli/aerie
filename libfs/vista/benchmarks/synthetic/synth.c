/*
 * synth.c
 *
 *   Main program and routines for synthetic benchmark. In the synthetic
 *   benchmark, we fix a database size, and then perform a series of 
 *   transactions of varying sizes in order to ascertain the overhead of
 *   vista.
 */

#include <sys/time.h>
#include "synth.h"

long get_time(void)
{
	struct timeval t;
	long s, u;

	if (gettimeofday(&t, NULL) < 0)
		perror("gettimeofday");
	s = t.tv_sec;
	u = t.tv_usec;
	return((s * 1000000L) + u);
}

/*
 * do_bench() -  modify a randomly chosen string of 'tsize' ints, 'times' times.
 */
int do_bench(int* d, int tsize, int times)
{
	int	i, c, j, size, modulus, base, g=0;

	size = DB_SIZE / sizeof(int);
	modulus = size - tsize;

	BENCH_LOOP {
		BEGIN_TRANS(tid);
		base = random() % modulus;
		SET_RANGE(tid, &d[base], tsize*sizeof(int));
		for (j = base; j < (base + tsize); j++) {
			d[j] = 5;
		}
		END_TRANS(tid);
		GROUP_INC();
		GROUP_FLUSH();
	}

	return i;
}

#ifdef USE_VISTA
int* init_vista(char* f)
{
	int*	data;

	vista_init();
	s = vista_map(f);
	if (s == NULL) {
		vista_perror("vista_map failed");
		exit(1);
	}

	data = (int*) vista_malloc(s, DB_SIZE, VISTA_NO_TRANS);
	if (data == NULL) {
		fprintf(stderr, "vista_malloc() failed.\n");
		exit(1);
	}

	return data;
}
#endif

#ifdef RVM_TRANS
int* init_rvm(char* f, int raw)
{
	int		code;
	rvm_options_t	*opt;
	rvm_region_t	*region;

	opt = rvm_malloc_options();
	opt->flags = RVM_ALL_OPTIMIZATIONS;
	opt->log_dev = LOGFILE;

	if (RVM_INIT(opt) != RVM_SUCCESS) {
		fprintf(stderr, "RVM initialization failed.\n");
		exit(1);
	}

	if (!raw) {
		char	command[512];
		int	i, count;
		fprintf(stderr, "Creating datafile... ");
		count = DB_SIZE / 1048576;
		sprintf(command, 
		      "dd if=/dev/zero of=%s bs=1024k count=%d >/dev/null 2>&1",
		      f, count);
		system(command);
		fprintf(stderr, "done.\n");
	}

	region = rvm_malloc_region();
	region->data_dev = DATAFILE;
	region->no_copy = TRUE;
	if (raw) {
		region->dev_length = RVM_MK_OFFSET(0, 1000000000);
		region->length = DB_SIZE;
	}
	if ((code = rvm_map(region, NULL)) != RVM_SUCCESS) {
		fprintf(stderr, "rvm_map failed: %s\n", rvm_return(code));
		exit(1);
	}

	tid = rvm_malloc_tid(); /* get a tid for later use */
	return ((int*) region->vmaddr);
	exit(0);
}
#endif

main(int argc, char** argv)
{
	int		i, tsize, stepsize, runs, j, loops;
	long		start, finish, secs, millis;
	double		elapsed_time, mb, tps, us_per_trans;
	int		*data;

	/*
	 * Get transaction size
	 */
#ifndef GROUP_COMMIT
	if (argc != 4) {
		fprintf(stderr, "usage: synth start step num_runs\n");
#else
	if (argc != 5) {
		fprintf(stderr, "usage: synth start step num_runs groupsize\n");
#endif
		exit(1);
	}
	else {
		tsize = atoi(argv[1]);
		if (tsize < 1)
			tsize = 1;
		stepsize = atoi(argv[2]);
		runs = atoi(argv[3]);
#ifdef GROUP_COMMIT
		group_size = atoi(argv[4]);
#endif
	}

	printf("# Version: %s\n", version);
#ifdef GROUP_COMMIT
	printf("# group size: %d\n", group_size);
#endif

	data = INIT(DATAFILE);

	fprintf(stderr, "Initializing db... ");
	for (i = 0; i < (DB_SIZE / sizeof(int)); i++) 
		data[i] = 0;
	fprintf(stderr, "done.\n");

	srandom((unsigned)time(NULL));

	fprintf(stderr, "Doing warmup... ");
	do_bench(data, tsize, WARMUP);
	fprintf(stderr, "done.\n");

	TRUNCATE_LOG();

	fprintf(stderr, "Starting runs.\n");
	for (j = 0; j < runs; j++) {

		/* Run 1 */
		start = get_time();
		loops = do_bench(data, tsize, LOOPS);
		finish = get_time();

		secs = (finish - start) / 1000000;
		millis = ((finish - start) % 1000000) / 1000;
		elapsed_time = ((float)secs) + (((float)millis) * 0.001);
		tps = loops / elapsed_time;
		us_per_trans = (1.0 / tps) * 1000000.0;

		printf("%d bytes; %.2f micros/trans; %d trans\n", 
		       tsize * sizeof(int), us_per_trans, loops);

		/* Run 2 */
		start = get_time();
		loops = do_bench(data, tsize, LOOPS);
		finish = get_time();

		secs = (finish - start) / 1000000;
		millis = ((finish - start) % 1000000) / 1000;
		elapsed_time = ((float)secs) + (((float)millis) * 0.001);
		tps = loops / elapsed_time;
		us_per_trans = (1.0 / tps) * 1000000.0;

		printf("%d bytes; %.2f micros/trans; %d trans\n", 
		       tsize * sizeof(int), us_per_trans, loops);

		/* Run 3 */
		start = get_time();
		loops = do_bench(data, tsize, LOOPS);
		finish = get_time();

		secs = (finish - start) / 1000000;
		millis = ((finish - start) % 1000000) / 1000;
		elapsed_time = ((float)secs) + (((float)millis) * 0.001);
		tps = loops / elapsed_time;
		us_per_trans = (1.0 / tps) * 1000000.0;

		printf("%d bytes; %.2f micros/trans; %d trans\n", 
		       tsize * sizeof(int), us_per_trans, loops);

		/* Run 4 */
		start = get_time();
		loops = do_bench(data, tsize, LOOPS);
		finish = get_time();

		secs = (finish - start) / 1000000;
		millis = ((finish - start) % 1000000) / 1000;
		elapsed_time = ((float)secs) + (((float)millis) * 0.001);
		tps = loops / elapsed_time;
		us_per_trans = (1.0 / tps) * 1000000.0;

		printf("%d bytes; %.2f micros/trans; %d trans\n", 
		       tsize * sizeof(int), us_per_trans, loops);

		/* Run 5 */
		start = get_time();
		loops = do_bench(data, tsize, LOOPS);
		finish = get_time();

		secs = (finish - start) / 1000000;
		millis = ((finish - start) % 1000000) / 1000;
		elapsed_time = ((float)secs) + (((float)millis) * 0.001);
		tps = loops / elapsed_time;
		us_per_trans = (1.0 / tps) * 1000000.0;

		printf("%d bytes; %.2f micros/trans; %d trans\n", 
		       tsize * sizeof(int), us_per_trans, loops);

		tsize *= stepsize;

	}
}


