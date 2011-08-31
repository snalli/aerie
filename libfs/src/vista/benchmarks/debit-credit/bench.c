
#include "bench.h"
#include <sys/time.h>

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


void do_bench(vista_segment* s, int sc, account* a, branch* b, teller* t, 
	      history* h, int hstart, int times)
{
	int	i, a_id, b_id, t_id, delta, j, timestamp;
	int	tid, k;

	timestamp = 0;

	for (i = 0; i < times; i++) {

		t_id = random() % (NTELLERS * sc);
		b_id = t_id / (NTELLERS * sc);
		a_id = random() % (NACCOUNTS * sc);
		delta = (random() % 100000) - 50000;

		tid = vista_begin_transaction(s, VISTA_EXPLICIT);
		if (tid < 0) 
			vista_perror("begin_transaction");

		if (!vista_set_range(s, &(a[a_id].balance), sizeof(int), tid))
			vista_perror("set_range");
		a[a_id].balance += delta;
		if (!vista_set_range(s, &(t[t_id].teller_balance), sizeof(int), 
				     tid))
			vista_perror("set_range");
		t[t_id].teller_balance += delta;
		if (!vista_set_range(s, &(b[b_id].branch_balance), sizeof(int), 
				     tid))
			vista_perror("set_range");
		b[b_id].branch_balance += delta;

		hstart = (hstart + 1) % NHISTRECS;
		j = hstart;
		if (!vista_set_range(s, &h[j], sizeof(history), tid))
			vista_perror("set_range");
		h[j].account_id = a_id;
		h[j].teller_id = t_id;
		h[j].branch_id = b_id;
		h[j].amount = delta;
		h[j].timestamp = timestamp++;

		vista_end_transaction(s, tid);
	}
}


main(int argc, char** argv)
{
	int		i, tps, sc;
	long		start, finish, secs, millis;
	double		elapsed_time, mb;
	vista_segment	*s;
	account*	accounts;
	teller*		tellers;
	branch*		branches;
	history*	hist;

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
	 * Set up our persistent heap
	 */
	vista_init();
	s = vista_map("data");
	if (s == NULL) {
		vista_perror("vista_map failed");
		exit(1);
	}

	/*
	 * Create our database
	 */
	accounts = (account*) vista_malloc(s, NACCOUNTS * sc * sizeof(account),
					   VISTA_NO_TRANS);
	tellers = (teller*) vista_malloc(s, NTELLERS * sc * sizeof(teller),
					 VISTA_NO_TRANS);
	branches = (branch*) vista_malloc(s, NBRANCHES * sc * sizeof(branch),
					  VISTA_NO_TRANS);
	hist = (history*) vista_malloc(s, NHISTRECS * sizeof(history), 
				       VISTA_NO_TRANS);
	if (accounts == NULL || tellers == NULL || branches == NULL
	    || hist == NULL) {
		vista_perror("vista_malloc failed");
		exit(1);
	}

	fprintf(stderr, "\n");
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
		/* we init the history to page it all in... */
		hist[i].account_id = 0;
		hist[i].teller_id = 0;
		hist[i].branch_id = 0;
		hist[i].amount = 0;
		hist[i].timestamp = 0;
	}
	fprintf(stderr, "done\n");

	srandom((unsigned)time(NULL));
	fprintf(stderr, "Starting warmup... ");
	do_bench(s, sc, accounts, branches, tellers, hist, 0, WARMUP);
	fprintf(stderr, "done\n");

	fprintf(stderr, "Starting run... ");
	start = get_time();
	do_bench(s, sc, accounts, branches, tellers, hist, 0, LOOPS);
	finish = get_time();
	fprintf(stderr, "done\n\n");

	secs = (finish - start) / 1000000;
	millis = ((finish - start) % 1000000) / 1000;
	elapsed_time = ((float)secs) + (((float)millis) * 0.001);
	tps = (int)(LOOPS / elapsed_time);
	mb = ((sc*NACCOUNTS*100) + (sc*NTELLERS*100) + (sc*NBRANCHES*100)
	     + (NHISTRECS * 52)) / 1048576.0;
	
	printf("%d: %d tps; %1.2f secs; %1.2f mb\n", sc, tps, elapsed_time,mb);
}


