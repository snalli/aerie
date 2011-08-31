
#include "bench.h"
#include "rvm.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

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
 * If count is positive, it specifies the number of transactions to run. If
 * it's negative the benchmark will run until one truncation has been 
 * performed. In either case the number of transactions performed is returned.
 */
int do_bench(int sc, account* a, branch* b, teller* t, history* h, int hstart,
	     int count) 
{
	int	i, a_id, b_id, t_id, delta, j, timestamp, truncations;
	rvm_tid_t *tid;
	rvm_return_t code;

	timestamp = 0;
	truncations = rvm_num_truncations;

	tid = rvm_malloc_tid();
	if (tid == NULL) {
		fprintf(stderr, "rvm_malloc_tid failed.\n");
		exit(1);
	}
	for (i = 0;; i++) {

		if (count > 0 && i >= count)
			return i;

		t_id = random() % (NTELLERS * sc);
		b_id = t_id / (NTELLERS * sc);
		a_id = random() % (NACCOUNTS * sc);
		delta = (random() % 100000) - 50000;

		if ((code = rvm_begin_transaction(tid, restore))!=RVM_SUCCESS) {
			fprintf(stderr, "rvm_begin_transaction failed: %s\n",
				rvm_return(code));
			exit(1);
		}

		if (rvm_set_range(tid, &(a[a_id].balance), sizeof(int)) 
		    != RVM_SUCCESS)
			fprintf(stderr, "rvm_set_range failed.\n");
		a[a_id].balance += delta;
		if (rvm_set_range(tid, &(t[t_id].teller_balance), sizeof(int)) 
		    != RVM_SUCCESS)
			fprintf(stderr, "rvm_set_range failed.\n");
		t[t_id].teller_balance += delta;
		if (rvm_set_range(tid, &(b[b_id].branch_balance), sizeof(int))
		    != RVM_SUCCESS)
			fprintf(stderr, "rvm_set_range failed.\n");
		t[t_id].teller_balance += delta;
		b[b_id].branch_balance += delta;

		hstart = (hstart + 1) % NHISTRECS;
		j = hstart;
		if (rvm_set_range(tid, &h[j], sizeof(history)) != RVM_SUCCESS)
			fprintf(stderr, "rvm_set_range failed.\n");
		h[j].account_id = a_id;
		h[j].teller_id = t_id;
		h[j].branch_id = b_id;
		h[j].amount = delta;
		h[j].timestamp = timestamp++;

		if (rvm_end_transaction(tid, flush) != RVM_SUCCESS)
			fprintf(stderr, "rvm_end_transaction failed.\n");
		
		if (count < 0 && truncations != rvm_num_truncations)
			return i;
	}
}

void make_datafile(char* file, int size)
{
	char	command[256];
	int	blocks;
	
	blocks = (size / 1048576) + 1;

	sprintf(command, "dd if=/dev/zero of=%s bs=1024k count=%d>/dev/null 2>&1", 
		file, blocks);
	system(command);
}


main(int argc, char** argv)
{
	int		i, tps, size, sc, count;
	char		*base;
	long		start, finish, secs, millis;
	double		elapsed_time, mb;
	account*	accounts;
	teller*		tellers;
	branch*		branches;
	history*	hist;
	rvm_options_t   *opt;
	rvm_offset_t    offset;
	rvm_region_t    *region;
	rvm_return_t    code;

	/*
	 * Get scaling factor
	 */
	if (argc != 2) {
		sc = TPS;
	}
	else {
		sc = atoi(argv[1]);
	}
	
	/*
	 * Set up RVM
	 */
        opt = rvm_malloc_options();
        opt->flags = RVM_ALL_OPTIMIZATIONS;
        opt->truncate = 50;
	/*
        opt->log_dev = "/dev/rrz11e";
	*/
        opt->log_dev = "logfile";

        if (RVM_INIT(opt) != RVM_SUCCESS) {
                fprintf(stderr, "initialization failed.\n");
                exit(1);
        }

	size = sc * (NACCOUNTS + NTELLERS + NBRANCHES) * 100;
	size += NHISTRECS * sizeof(history);
	size = (size / 8192) + 1;
	/*
	fprintf(stderr, "Creating datafile of size %d... ", size * 8192);
	make_datafile(DATAFILE, size * 8192);
	fprintf(stderr, "done.\n");
	*/
	fprintf(stderr, "\n");

	fprintf(stderr, "Mapping region... ");
        region = rvm_malloc_region();
        region->data_dev = DATAFILE;
        region->dev_length = RVM_MK_OFFSET(0, 1000000000); 
        region->length = (rvm_length_t) (size * 8192);
        region->no_copy = TRUE;
        if ((code = rvm_map(region, NULL)) != RVM_SUCCESS) {
                fprintf(stderr, "rvm_map failed: %s\n", rvm_return(code));
                exit(1);
        }
	fprintf(stderr, "done.\n");

	if (region->length != (size * 8192)) {
                fprintf(stderr, "mapped wrong size.\n");
                exit(1);
	}

	/*
	 * Create our database
	 */
	base = region->vmaddr;
	accounts = (account*) base;
	base += NACCOUNTS * sc * sizeof(account);
	tellers = (teller*) base;
	base += NTELLERS * sc * sizeof(teller);
	branches = (branch*) base;
	base += NBRANCHES * sc * sizeof(branch);
	hist = (history*) base;

	fprintf(stderr, "Initializing... ");
	for (i = 0; i < NACCOUNTS * sc; i++) {
		accounts[i].account_id = i;
		accounts[i].branch_id = i / NACCOUNTS;
		accounts[i].balance = 0;
	}
	for (i = 0; i < NTELLERS * sc; i++) {
		tellers[i].teller_id = i;
		tellers[i].branch_id = i / NTELLERS;
		tellers[i].teller_balance = 0;
	}
	for (i = 0; i < NBRANCHES * sc; i++) {
		branches[i].branch_id = i;
		branches[i].branch_balance = 0;
	}
	for (i = 0; i < NHISTRECS; i++) {
		hist[i].account_id = 0;
		hist[i].teller_id = 0;
		hist[i].branch_id = 0;
		hist[i].amount = 0;
		hist[i].timestamp = 0;
	}
	fprintf(stderr, "done\n");

	fprintf(stderr, "Starting warmup... ");
	count = do_bench(sc, accounts, branches, tellers, hist, 0, WARMUP);
	fprintf(stderr, "done\n");
	fprintf(stderr, "Did %d transactions during warmup.\n", count);

	fprintf(stderr, "Truncating log... ");
	if (rvm_truncate() != RVM_SUCCESS) {
		fprintf(stderr, "log truncation failed.\n");
		exit(1);
	}
	fprintf(stderr, "done\n");

	fprintf(stderr, "Starting run... ");
	start = get_time();
	count = do_bench(sc, accounts, branches, tellers, hist, WARMUP, -1);
	finish = get_time();
	fprintf(stderr, "done\n");
	fprintf(stderr, "Did %d transactions during run.\n", count);

	secs = (finish - start) / 1000000;
	millis = ((finish - start) % 1000000) / 1000;
	elapsed_time = ((float)secs) + (((float)millis) * 0.001);
	tps = (int)(count / elapsed_time);
	mb = ((sc*NACCOUNTS*100) + (sc*NTELLERS*100) + (sc*NBRANCHES*100)
	     + (NHISTRECS * 52)) / 1048576.0;
	
	printf("%d: %d tps; %1.2f secs; %1.2f mb\n", sc, tps, elapsed_time,mb);
	exit(0);
}


