/*
 * Copyright (c) 1997, 1998, 1999, 2000, David E. Lowell
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Vista library, version 0.6.1, September 2000
 */

#include <stdio.h>
#include <sys/param.h>
#include "vista.h"

#define SIZE 8 * 1024 * 1024
#define TRANS1 8192
#define TRANS2 1024
#define CHUNK (SIZE / TRANS1)
#define CHUNK2 (SIZE / TRANS2)
#define PIECES_PER_CHUNK 16
#define PIECESIZE (CHUNK / PIECES_PER_CHUNK)
#define ALLOCS 100000
#define GRAMS 5000

/*
*/
#define TRANSACTION_TESTS
#define HEAP_TESTS
#ifndef NO_VISTAGRAMS
#define VISTAGRAM_TESTS
#endif

int 	prev_len = 0;

void status(char* m)
{
	fprintf(stderr, m);
}

void reset_count(void)
{
	prev_len = 0;
}

void print_count(int count)
{
	char	buf[256];
	int	i;

	for (i=0; i < prev_len; i++)
		status("\b");
	sprintf(buf, "%d", count);
	prev_len = strlen(buf);
	status(buf);
}

#ifndef NO_VISTAGRAMS

void server_process(vista_segment *s, char *h)
{
	int		port, res, tid, req, len, id, size, n1, n2, i;

	status("running.\n");

	port = 1;

	for (;;) {
		res = vista_select(s, &port, 1, 5000); /* 5 sec timeout */
		if (res < 0) {
			if (vista_errno == VISTA_ESELECTINT) 
				continue;
			vista_perror("serv: vista_select failed");
			exit(1);
		}
		else if (res == 0) {
			status("a timeout occurred.\n");
			continue;
		}

		tid = vista_begin_transaction(s, VISTA_EXPLICIT);

		res = vista_receive_request(s, port, &req, &len, &id, tid);
		if (!res) {
			vista_perror("serv: vista_receive_request failed");
			exit(1);
		}

		res = vista_send_response(s, &req, len, id, tid);
		if (!res) {
			vista_perror("serv: vista_send_response failed");
			exit(1);
		}

		vista_end_transaction(s, tid);

		if (req == -1)
			break;
	}

	/* Make a bunch of requests to client process */
	size = sizeof(int);
	for (i=0; i < GRAMS; i++) {
		tid = vista_begin_transaction(s, VISTA_EXPLICIT);
			res = vista_send_request(s, h, 2, &i, size, i, tid);
			if (!res) {
				vista_perror("vista_send_request");
				exit(1);
			}
		vista_end_transaction(s, tid);
		tid = vista_begin_transaction(s, VISTA_EXPLICIT);
			res = vista_receive_response(s, i, &n1, &n2, tid);
			if (n1 != i || n2 != size) {
				status("vistagram garbled.\n");
				exit(1);
			}
			if (!res) {
				vista_perror("vista_receive_response");
				exit(1);
			}
		vista_end_transaction(s, tid);
		print_count(i+1);
	}
	status("\n");

	exit(0);  /* Server is done */
}

void client_process(vista_segment *s, char *h)
{
	int	i, tid, size, n1, n2, res, id;

	sleep(1);
	size = sizeof(int);

	reset_count();
	status("Making 5000 requests to server and waiting for each response: ");
	for (i=0; i < GRAMS; i++) {
		tid = vista_begin_transaction(s, VISTA_EXPLICIT);
			res = vista_send_request(s, h, 1, &i, size, i, tid);
			if (!res) {
				vista_perror("vista_send_request");
				exit(1);
			}
		vista_end_transaction(s, tid);
		tid = vista_begin_transaction(s, VISTA_EXPLICIT);
			res = vista_receive_response(s, i, &n1, &n2, tid);
			if (!res) {
				vista_perror("vista_receive_response");
				exit(1);
			}
			if (n1 != i || n2 != size) {
				status("vistagram garbled.\n");
				exit(1);
			}
		vista_end_transaction(s, tid);
		print_count(i+1);
	}
	status("\n");

	status("Inducing a timeout at server... ");
	sleep(6);

	n1 = -1;
	/* Have server change roles to send requests to client */
	tid = vista_begin_transaction(s, VISTA_EXPLICIT);
		res = vista_send_request(s, h, 1, &n1, size, 1, tid);
		if (!res) {
			vista_perror("vista_send_request");
			exit(1);
		}
	vista_end_transaction(s, tid);

	tid = vista_begin_transaction(s, VISTA_EXPLICIT);
		res = vista_receive_response(s, 1, &n1, &n2, tid);
		if (!res) {
			vista_perror("vista_receive_response");
			exit(1);
		}
	vista_end_transaction(s, tid);

	status("Server making 5000 requests to client and waiting for each response: ");
	for (i=0; i < GRAMS; i++) {
		tid = vista_begin_transaction(s, VISTA_EXPLICIT);

		res = vista_receive_request(s, 2, &n1, &n2, &id, tid);
		if (!res) {
			vista_perror("test1: vista_receive_request failed");
			exit(1);
		}

		res = vista_send_response(s, &n1, n2, id, tid);
		if (!res) {
			vista_perror("test1: vista_send_response failed");
			exit(1);
		}

		vista_end_transaction(s, tid);
	}
}

#endif

main(int argc, char **argv)
{
	vista_segment	*s1, *s2, *segs[VISTA_MAX_SEGS];
	char		*c1, *c2, buf[256], *p[ALLOCS];
	char		*leak, h[512], *dir, olddir[MAXPATHLEN];
	int		i, j, tid, n1, n2, size, res, id;

	if (argc != 2) {
		status("usage: vistatest {local dir for 30MB of temp files}\n");
		exit(1);
	}

	dir = argv[1];
	getwd(olddir);
	if (chdir(dir) < 0) {
		perror("Cannot chdir to working directory");
		exit(1);
	}

	status("\nValidation program for vista library.\n\n");

	status("Initializing vista... ");
	vista_init();
	status("done\n");

	status("Mapping max number of vista segments:\n");
	for (i=0; i < VISTA_MAX_SEGS; i++) {
		sprintf(buf, "map%d", i);
		segs[i] = vista_map(buf, NULL);
		status("\t");
		vista_perror(buf);
	}

	status("Mapping one more vista segment (should fail):\n");
	s1 = vista_map("failme", NULL);
	vista_perror("\tfailme");

	vista_errno = VISTA_NOERROR;

	status("Unmapping vista segments:\n");
	for (i=0; i < VISTA_MAX_SEGS; i++) {
		vista_unmap(segs[i]);
		vista_perror("\tvista_unmap");
	}

	status("Setting up two vista segments for tests:\n");
	s1 = vista_map("mapfile1", NULL);
	vista_perror("\tmapfile1");
	s2 = vista_map("mapfile2", NULL);
	vista_perror("\tmapfile2");
	if (s1 == NULL || s2 == NULL) 
		exit(1);

	status("Allocating 8 MB in both segments... ");
	c1 = vista_malloc(s1, SIZE, VISTA_NO_TRANS);
	c2 = vista_malloc(s2, SIZE, VISTA_NO_TRANS);
	if (c1 == NULL || c2 == NULL) {
		vista_perror("vista_malloc");
		exit(1);
	}
	status("done.\n");

	status("Initializing memory... ");
	for (i=0; i < SIZE; i++) {
		c1[i] = '.';
		c2[i] = '.';
	}
	status("done.\n");

#ifdef TRANSACTION_TESTS
	status("\n\tTRANSACTION TESTS\n\t-----------------\n\n");

	status("Performing 8192 explicit transactions in each segment:\n");
	status("\tSegment 1: ");
	for (i=0; i < TRANS1; i++) {
		int	start;

		start = CHUNK * i;
		tid = vista_begin_transaction(s1, VISTA_EXPLICIT);

		/*
		 * Each transaction modifies one chunk of the character
		 * array database. Each chunk is modified PIECES_PER_CHUNK
		 * bytes at a time, resulting in that many 
		 * vista_set_range() calls per transaction.
		 */
		for (j=0; j < PIECES_PER_CHUNK; j++) {
			vista_set_range(s1, c1+start+(j*PIECESIZE),
					PIECESIZE, tid);
			memset(c1+start+(j*PIECESIZE), 'x', PIECESIZE);
		}

		vista_end_transaction(s1, tid);
		print_count(i+1);
	}
	reset_count();
	status("\n\tSegment 2: ");
	for (i=0; i < TRANS1; i++) {
		int	start;

		start = CHUNK * i;
		tid = vista_begin_transaction(s2, VISTA_EXPLICIT);

		for (j=0; j < PIECES_PER_CHUNK; j++) {
			vista_set_range(s2, c2+start+(j*PIECESIZE),
					PIECESIZE, tid);
			memset(c2+start+(j*PIECESIZE), 'x', PIECESIZE);
		}

		vista_end_transaction(s2, tid);
		print_count(i+1);
	}
	status("\n");
	reset_count();

	status("Validating memory... ");
	for (i=0;  i < SIZE; i++) {
		if (c1[i] != 'x' || c2[i] != 'x') 
			break;
	}
	if (i != SIZE) {
		status("failed.\n");
		exit(1);
	}
	else 
		status("done.\n");

	status("Performing 1024 implicit transactions in each segment:\n");
	status("\tSegment 1: ");
	for (i=0; i < TRANS2; i++) {
		tid = vista_begin_transaction(s1, VISTA_IMPLICIT);
			memset(c1+(CHUNK2*i), '.', CHUNK2);
		vista_end_transaction(s1, tid);
		print_count(i+1);
	}
	reset_count();
	status("\n\tSegment 2: ");
	for (i=0; i < TRANS2; i++) {
		tid = vista_begin_transaction(s2, VISTA_IMPLICIT);
			memset(c2+(CHUNK2*i), '.', CHUNK2);
		vista_end_transaction(s2, tid);
		print_count(i+1);
	}
	status("\n");

	status("Validating memory... ");
	for (i=0;  i < SIZE; i++) {
		if (c1[i] != '.' || c2[i] != '.') 
			break;
	}
	if (i != SIZE) {
		status("failed.\n");
		exit(1);
	}
	else 
		status("done.\n");

	reset_count();
	status("Aborting 8192 explicit transactions in each segment:\n");
	status("\tSegment 1: ");
	for (i=0; i < TRANS1; i++) {
		int	start;

		start = CHUNK * i;
		tid = vista_begin_transaction(s1, VISTA_EXPLICIT);

		for (j=0; j < PIECES_PER_CHUNK; j++) {
			vista_set_range(s1, c1+start+(j*PIECESIZE),
					PIECESIZE, tid);
			memset(c1+start+(j*PIECESIZE), 'x', PIECESIZE);
		}

		vista_abort_transaction(s1, tid);
		print_count(i+1);
	}
	reset_count();
	status("\n\tSegment 2: ");
	for (i=0; i < TRANS1; i++) {
		int	start;

		start = CHUNK * i;
		tid = vista_begin_transaction(s2, VISTA_EXPLICIT);

		for (j=0; j < PIECES_PER_CHUNK; j++) {
			vista_set_range(s2, c2+start+(j*PIECESIZE),
					PIECESIZE, tid);
			memset(c2+start+(j*PIECESIZE), 'x', PIECESIZE);
		}

		vista_abort_transaction(s2, tid);
		print_count(i+1);
	}
	status("\n");
	reset_count();

	status("Validating memory... ");
	for (i=0;  i < SIZE; i++) {
		if (c1[i] != '.' || c2[i] != '.') 
			break;
	}
	if (i != SIZE) {
		status("failed.\n");
		exit(1);
	}
	else 
		status("done.\n");

	status("Aborting 1024 implicit transactions in each segment:\n");
	status("\tSegment 1: ");
	for (i=0; i < TRANS2; i++) {
		tid = vista_begin_transaction(s1, VISTA_IMPLICIT);
			memset(c1+(CHUNK2*i), '.', CHUNK2);
		vista_abort_transaction(s1, tid);
		print_count(i+1);
	}
	reset_count();
	status("\n\tSegment 2: ");
	for (i=0; i < TRANS2; i++) {
		tid = vista_begin_transaction(s2, VISTA_IMPLICIT);
			memset(c2+(CHUNK2*i), '.', CHUNK2);
		vista_abort_transaction(s2, tid);
		print_count(i+1);
	}
	status("\n");

	status("Validating memory... ");
	for (i=0;  i < SIZE; i++) {
		if (c1[i] != '.' || c2[i] != '.') 
			break;
	}
	if (i != SIZE) {
		status("failed.\n");
		exit(1);
	}
	else 
		status("done.\n");
	reset_count();
#endif

#ifdef HEAP_TESTS
	status("\n\tHEAP TESTS\n\t----------\n\n");

	status("Performing 100,000 8-byte mallocs... ");
	tid = vista_begin_transaction(s1, VISTA_EXPLICIT);
		for (i=0; i < ALLOCS; i++) {
			p[i] = vista_malloc(s1, 8, tid);
			if (p[i] == NULL) {
				vista_perror("vista_malloc");
				exit(1);
			}
		}
	vista_end_transaction(s1, tid);
	status("done.\n");

	status("Performing 100,000 frees... ");
	tid = vista_begin_transaction(s1, VISTA_EXPLICIT);
		for (i=0; i < ALLOCS; i++) {
			vista_free(s1, p[i], tid);
		}
	vista_end_transaction(s1, tid);
	status("done.\n");

	status("Leaking 100,000 8-byte mallocs, then aborting... ");
	tid = vista_begin_transaction(s1, VISTA_EXPLICIT);
		for (i=0; i < ALLOCS; i++) {
			leak = vista_malloc(s1, 8, tid);
			if (leak == NULL) {
				vista_perror("vista_malloc");
				exit(1);
			}
		}
	vista_abort_transaction(s1, tid);
	status("done.\n");

#endif

#ifdef VISTAGRAM_TESTS

	status("\n\tVISTAGRAM TESTS\n\t---------------\n\n");

	if (gethostname(h, 512) < 0) {
		status("Cannot determine hostname.\n");
		exit(1);
	}

	status("Forking server process... ");
	res = fork();
	if (res == 0) {
		server_process(s2, h);
	}
	else if (res > 0) {
		client_process(s1, h);
	}
	else {
		status("Cannot fork server process.\n");
		exit(1);
	}


#endif

	sleep(1);
	status("\nUnmapping vista segments... ");
	vista_unmap(s1);
	vista_unmap(s2);
	status("done.\n");

	status("Removing vista files... ");
	system("rm map*");
	status("done.\n");
	status("\nAll vista library tests completed.\n\n");
	chdir(olddir);
	exit(0);
}


