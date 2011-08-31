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

/*
 * vistagrams.c
 *
 * DESCRIPTION
 *
 *   This file contains the implementation of the vistagram routines.
 *   These routines provide a reliable request/response service similar
 *   to RPC.
 *
 * PUBLIC FUNCTIONS
 *
 *   int vista_select(vista_segment* s, int* port_vector, int num_ports,
 *		      int ms_timeout);
 *   int vista_send_request(vista_segment* s, char* host, int vg_port, 
 *		            void* req, int req_len, int req_id, int tid);
 *   int vista_receive_request(vista_segment* s, int vg_port, void* req,
 *			       int* req_len, int* req_id, int tid);
 *   int vista_send_response(vista_segment* s, void* response, 
 *                           int resp_len, int req_id, int tid);
 *   int vista_receive_response(vista_segment* s, int req_id, 
 *				void* response, int *resp_len, int tid);
 *   int vista_sigio_enable(vista_segment* s, int vg_port);
 *
 * AUTHOR
 *   Dave Lowell 
 */

/**
 ** Includes
 **/

#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#define _SOCKADDR_LEN 1  /* make alpha use bsd 4.4 msghdr struct */
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include "vistagrams.h"
#include "vista.h"

/**
 ** Functions 
 **/

#ifdef VISTAGRAMS

extern void vista_sigother(int sig);

/*
 * Invoked by the main vista module when vista is initialized.
 */
void vistagrams_init(void)
{
	int	i, size, len;
	struct sigaction  sa;

	/*
	 * Open a socket for client side requests
	 */
	cli_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (cli_sock < 0) {
		fprintf(stderr, "Unable to open client socket.\n");
	}

	/*
	 * Set size of send and receive buffers for 
	 * client socket
	 */
	size = VISTA_SENDBUF;
	len = sizeof(size);
	if (setsockopt(cli_sock, SOL_SOCKET, SO_SNDBUF, (char*)&size,len) < 0) {
		perror("vistagrams_init: setsockopt failed");
		exit(1);
	}
	if (setsockopt(cli_sock, SOL_SOCKET, SO_RCVBUF, (char*)&size,len) < 0) {
		perror("vistagrams_init: setsockopt failed");
		exit(1);
	}


	/* 
	 * Set up a handler for SIGIO signals
	 */
	sa.sa_handler = (VISTA_HANDLER_T) vista_sigother;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGIO, &sa, NULL) < 0) {
		perror("failed to start sigio handler");
		exit(1);
	}

	vista_localhost = inet_addr("127.0.0.1");
	nsocks = 0;
}

/*
 * Invoked by the main vista module when new vista seg is created/mapped
 */
void vistagrams_map_init(vista_segment *s)
{
	hash_table*	h;

	/*
	 * Set up the hash tables we'll need for keeping track
	 * of requests, responses, hosts->IP mappings, and 
	 * port->socket mappings. The host and port tables really 
	 * shouldn't be persistent, so we'll reset them if this 
	 * segment is later remapped.
	 */
	h = (hash_table*) heap_malloc(s->mh, sizeof(hash_table));
	vista_hash_init(h, HASH_ONE_WORD_KEYS, s->mh);
	s->minfo.request_hash = h;

	h = (hash_table*) heap_malloc(s->mh, sizeof(hash_table));
	vista_hash_init(h, HASH_ONE_WORD_KEYS, s->mh);
	s->minfo.response_hash = h;

	h = (hash_table*) heap_malloc(s->mh, sizeof(hash_table));
	vista_hash_init(h, HASH_STRING_KEYS, s->mh);
	s->minfo.host_table = h;

	h = (hash_table*) heap_malloc(s->mh, sizeof(hash_table));
	vista_hash_init(h, HASH_ONE_WORD_KEYS, s->mh);
	s->minfo.port_table = h;

	/* Init table of per server process ack queues */
	h = (hash_table*) heap_malloc(s->mh, sizeof(hash_table));
	vista_hash_init(h, HASH_STRING_KEYS, s->mh);
	s->minfo.server_acks = h;
}

/*
 * Invoked by the main vista module when old vista seg is mapped
 */
void vistagrams_remap_init(vista_segment *s)
{
	/*
	 * It turns out that we don't really want the port table
	 * to be persistent because the sockets it keeps in it's
	 * table do not themselves persist past program exit.
	 * We've used persistent hash tables though to store the
	 * port->socket mapping, because that's the only kind of
	 * hash table we have handy. To make sure that we don't
	 * accidentally use some socket that we haven't actually
	 * opened, we'll free all the old table entries and start
	 * fresh.
	 */
	vista_hash_delete_table(s->minfo.port_table);
	vista_hash_init(s->minfo.port_table, HASH_ONE_WORD_KEYS, s->mh);

	/* 
	 * We also would like to start fresh on the host->IP mapping
	 * table. IPs could have changed since the last time this 
	 * segment was mapped.
	 */
	vista_hash_delete_table(s->minfo.host_table);
	vista_hash_init(s->minfo.host_table, HASH_ONE_WORD_KEYS, s->mh);
}

/*
 * Invoked by the main vista module every time a transaction is begun.
 */
void vistagrams_begin_transaction(vista_segment *s, int tid)
{
	transaction_info	*t;

	t = &s->t[tid];
	t->vg.to_send_h = NULL;
	t->vg.to_send_t = NULL;
	t->vg.done_list = NULL;
	t->vg.del_list = NULL;
	t->vg.ack.h = NULL;
	t->vg.ack_done.h = NULL;
}

/*
 * Atomically add ack to head of queue
 */
void prepend_ack(ack_queue* q, q_node* n)
{
	n->next = q->h;
	q->h = n;
}


/*
 * Move all the pending id's to the official per server ack queues.
 */
void move_acks(vista_segment *s, ack_queue *a)
{
	q_node		*c, *n;
	ack_queue	*q;

	for (c = a->h; c != NULL; c = a->h) {
		a->h = c->next;
		q = vista_hash_find(s->minfo.server_acks, c->s_key);
		if (q != NULL) {
			prepend_ack(q, c);
		}
		else {
			q = (ack_queue*) heap_malloc(s->mh, sizeof(ack_queue));
			if (q == NULL) {
				fprintf(stderr, "heap_malloc failed.\n");
				return;
			}
			q->h = NULL;
			prepend_ack(q, c);
			vista_hash_insert(s->minfo.server_acks, c->s_key, q);
		}
	}
}


/*
 * Free the contents of the ack queue.
 */
void delete_acks(vista_segment *s, ack_queue *q)
{
	q_node	*c, *n;

	c = q->h;
	q->h = NULL;
	for (; c != NULL; c = n) {
		n = c->next;
		heap_free(s->mh, c, sizeof(q_node));
	}
}


/*
 * Delete the contents of this transaction's ack queue and reinstate those
 * acks that we're deleted during this aborted transaction.
 */
void abort_acks(vista_segment *s, transaction_info *t)
{
	/* 
	 * Delete all the pending acks 
	 */
	delete_acks(s, &t->vg.ack);

	/* 
	 * Put acks back on the per server lists 
	 */
	move_acks(s, &t->vg.ack_done);
}


/*
 * Invoked by the main vista module every time a transaction aborts.
 */
void vistagrams_abort(vista_segment* sd, transaction_info* t, int verbose)
{
	int 	i;

	/*
	 * We back out of any outgoing vistagrams simply by removing
	 * them all from the request table, and freeing them.
	 */
	i = 0;
	if (t->vg.del_list != NULL) {
		request		*c, *n;
		hash_table	*table;

		/*
		 * Go through the list once removing each 'request' 
		 * from the appropriate table. The request structs
		 * themselves were already freed off the heap 
		 * operation log.
		 */
		for (c = t->vg.del_list; c != NULL; c = c->dnext, i++) {
			if (c->req != NULL)
				table = sd->minfo.request_hash;
			else
				table = sd->minfo.response_hash;
			vista_hash_delete_entry(table, (char*) c->req_id);
		}
		t->vg.del_list = NULL;
	}

	/*
	 * Reset the message log. Its contents was freed as needed
	 * via the heap operation log.
	 */
	t->vg.to_send_h = NULL;
	t->vg.to_send_t = NULL;

	if (verbose) {
		fprintf(stderr, "\tKilled %d vistagrams\n", i);
	}
	
	/*
	 * We also need to delete the entries on this transaction's
	 * ack queue.
	 */
	abort_acks(sd, t);
}


/*
 * Get appropriate socket for this vista port, or make one as needed.
 * Returns the sock file descriptor, or FALSE on error.
 */
static int get_server_sock(vista_segment* s, int port)
{
	int	sock, re_sock, size, len;
	struct sockaddr_in addr;

        sock = (int) vista_hash_find(s->minfo.port_table, (char*)port);
        if (sock != 0) 
		return sock;

	/*
	 * We've never used the requested port before,
	 * so open up a socket.
	 */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		vista_errno = VISTA_ESOCK;
		return FALSE;
	}

	/*
	 * Set size of send and receive buffers for new socket
	 */
	size = VISTA_SENDBUF;
	len = sizeof(size);
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*) &size, len) < 0) {
		perror("vistagrams_init: setsockopt failed");
		exit(1);
	}
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*) &size, len) < 0) {
		perror("vistagrams_init: setsockopt failed");
		exit(1);
	}

	/* 
	 * For server sockets, we bind the socket to the UDP port
	 * that corresponds to the provided vista port. We also want 
	 * to set up SIGIO on the corresponding retransmit port. 
	 */
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = VISTA_MAP_TO_UDP_PORT(port);
	if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		vista_errno = VISTA_EPORTINUSE;
		return FALSE;
	}

	/*
	 * Open a socket that the server will use to receive
	 * retransmitted requests.
	 */
	re_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (re_sock < 0) {
		vista_errno = VISTA_ESOCK;
		return FALSE;
	}

	/*
	 * Set size of send and receive buffers for retrans socket
	 */
	size = VISTA_SENDBUF;
	len = sizeof(size);
	if (setsockopt(re_sock, SOL_SOCKET, SO_SNDBUF, (char*)&size, len) < 0) {
		perror("vistagrams_init: setsockopt failed");
		exit(1);
	}
	if (setsockopt(re_sock, SOL_SOCKET, SO_RCVBUF, (char*)&size, len) < 0) {
		perror("vistagrams_init: setsockopt failed");
		exit(1);
	}

	/*
	 * Bind the retransmit socket to the known UDP port for
	 * those retransmits.
	 */
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = VISTA_REXMIT_PORT(port);
	if (bind(re_sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		vista_errno = VISTA_EPORTINUSE;
		return FALSE;
	}

	/*
	 * Add this retrans socket to the list that the sigio handler
	 * will watch for incoming retransmissions.
	 */
	retrans_sockets[nsocks++] = re_sock;

	/*
	 * Configure this retrans socket to do sigio...
	 */
	if (fcntl(re_sock, F_SETOWN, getpid()) < 0) {
		vista_errno = VISTA_ESIGIO;
		return FALSE;
	}
	if (fcntl(re_sock, F_SETFL, FASYNC) < 0) {
		vista_errno = VISTA_ESIGIO;
		return FALSE;
	}

	/*
	 * Remember the appropriate socket for this port
	 */
	vista_hash_insert(s->minfo.port_table, (char*)port, (char*)sock);

	return sock;
}


/*
 * Move all the pending id's to the official per server ack queues.
 * Also delete all ack nodes on the done queue.
 */
void commit_acks(vista_segment *s, transaction_info *t)
{
	/*
	 * Put the pending acks onto the official queues 
	 * of acks ready to go to servers.
	 */
	move_acks(s, &t->vg.ack);

	/*
	 * Delete all the acks on our list of finished acks
	 */
	delete_acks(s, &t->vg.ack_done);
}


/*
 * If ack_id corresponds to a response that some segment is still buffering,
 * delete it.
 */
void handle_ack(int ack_id)
{
	int		i;
	vista_segment 	*s;
	request		*r;

	if (ack_id < 0)
		return;
	
	for (i=0, r=NULL; i < num_segs; i++) {
		/*
		 * Find if any segment knows about this request.
		 */
		s = segments[i];
		r = vista_hash_find(s->minfo.response_hash, (char*)ack_id);
		if (r != NULL)
			break;
	}

	if (r != NULL) {
		int	size;

		/*
		 * A segment is holding a request with id 'ack_id', so it's
		 * time to delete that request.
		 */
		if (r->response != NULL) {
			size = VISTA_VG_SIZE + r->response->mesg_len;
			heap_free(s->mh, r->response, size);
		}
		else {
			fprintf(stderr, "vistagrams: weird internal err -7\n");
		}
		vista_hash_delete_entry(s->minfo.response_hash, (char*)ack_id);
		heap_free(s->mh, r, sizeof(request));
	}
}


/*
 * Invoked by the main vista module every time a transaction commits.
 */
void vistagrams_commit(vista_segment* s, transaction_info *t)
{
	request	 *c, *n; /* current and next pointers for list traversal */

	/*
	 * Were there any vistagrams sent during the transaction?
	 * If so, we'll do the actual send here.
	 */
	if (t->vg.to_send_h != NULL) {
		int		err, msize, sock;
		vistagram	*m;

		/*
		 * Send all outgoing messages 
		 */
		for (c = t->vg.to_send_h; c != NULL; ) {
			c->commit = TRUE;			
			if (c->req != NULL) {
				m = c->req;
				sock = cli_sock;
			}
			else {
				m = c->response;
				sock = get_server_sock(s, c->vport);
			}
			msize = m->mesg_len + VISTA_VG_SIZE;
			vista_unlock();
			err = sendto(sock, m, msize, 0, (struct sockaddr*)
				     &c->addr, sizeof(c->addr));
			vista_lock();
			if (err < 0) {
				if (errno != ENOBUFS) {
					vista_unlock();
					fprintf(stderr, 
					  "vistagrams_commit: sendto() failed");
					perror(" ");
					vista_lock();
				}
			}

			if (err < 0 && errno == ENOBUFS) {
				struct timeval 	t = {0, VISTA_SENDTO_BACKOFF};
				sigset_t	s;

				/*
				 * The system is out of buffers, so lets
				 * back off a bit and let the thing drain.
				 */
				sigemptyset(&s);
				sigaddset(&s, SIGIO);
				if (sigprocmask(SIG_BLOCK, &s, NULL) < 0)
					perror("sigprocmask failed");
				if (select(1, NULL, NULL, NULL, &t) != 0) {
					if (errno == EINTR)
						fprintf(stderr, "EINTR\n");
					else
						perror("select error");

				}
				if (sigprocmask(SIG_UNBLOCK, &s, NULL) < 0)
					perror("sigprocmask failed");

			}
			else {
				c = c->next;
			}
		}

		/*
		 * Reset the list. We don't need to free the elements of
		 * the list because they're also held by the request table.
		 * They will be freed off the done list when the 
		 * corresponding response has been safely handled and these 
		 * messages are no longer needed for retransmits.
		 */
		t->vg.to_send_h = NULL;
		t->vg.to_send_t = NULL;
	}

	/*
	 * Delete all the request structs with which
	 * we have finished. The vistagrams pointed to by each
	 * struct were scheduled for freeing at commit using
	 * the heap operation log, as was the request struct
	 * itself.
	 */
	for (c = t->vg.done_list; c != NULL; c = c->dnext) {
		vista_hash_delete_entry(s->minfo.request_hash,
					(char*) c->req_id);
	}
	t->vg.done_list = NULL;

	/*
	 * Move each message on the transaction's ack_queue to the
	 * committed queue for the appropriate server. Delete
	 * acks on the done queue.
	 */
	commit_acks(s, t);

}


/*
 * Called by main vista module to check if the vistagram log is empty
 */
int vistagrams_log_empty(transaction_info *t)
{
	return (t->vg.to_send_h == NULL && t->vg.done_list == NULL &&
		t->vg.ack.h == NULL && t->vg.ack_done.h == NULL && 
		t->vg.del_list == NULL);
}


/*
 * Called to process one incoming message that has arrived asynchronously.
 * Returns TRUE on success or FALSE if there was no message to process.
 * This function assumes that it is being invoked at a time at which it is
 * safe for it to manipulate internal vista data structures. 
 */
int vistagrams_sigio_do_one_message(void)
{
	int		i, res;
	fd_set		socks;
	vista_segment	*s;
	struct timeval	timeout = {0, 0};

	vista_lock();

	/*
	 * Set up a descriptor set to contain all currently used 
	 * retransmit sockets. 
	 */
	FD_ZERO(&socks);
	for (i = 0; i < nsocks; i++) {
		FD_SET(retrans_sockets[i], &socks);
	}

	/*
	 * Check to see which socket has a message waiting on it.
	 */
	res = select(4096, &socks, NULL, NULL, &timeout);
	if (res == 0) {
		/*
		 * There are no messages waiting to be handled.
		 */
		vista_unlock();
		return FALSE;
	}
	else if (res < 0) {
		perror("select error");
		vista_unlock();
		return FALSE;
	}
	else if (res > 0) {
		int		cli_addr_len, sock, msize, err, i;
		char		buf[100000];
		vistagram	*m;
		vista_segment	*s;
		request		*r;
		struct sockaddr_in cli_addr, my_addr;

		/*
		 * There are one or more messages waiting on one or
		 * more sockets. We'll process the first ready message
		 * on the first ready socket we find.
		 */
		for (i=0; i < nsocks; i++) {
			if (FD_ISSET(retrans_sockets[i], &socks)) {
				sock = retrans_sockets[i];
				break;
			}
		}
		if (i == nsocks) {
			fprintf(stderr, "_sigio_do_one... internal error.\n");
			vista_unlock();
			return TRUE;
		}

		cli_addr_len = sizeof(cli_addr);
		err = recvfrom(sock, buf, 100000, 0,
			       (struct sockaddr*)&cli_addr, &cli_addr_len);
		if (err < 0) {
			fprintf(stderr, "recvfrom failed in sigio handler.\n");
			vista_unlock();
			return TRUE;
		}
		m = (vistagram*)buf;
		m->cli_addr = cli_addr;
		m->forward_flag = TRUE;
		msize = VISTA_VG_SIZE + m->mesg_len;

		/*
		 * If the client dropped or aborted responses, we might
		 * only get ACKs for them here with the retransmission
		 * of the requests. So we handle the acks here. Assuming
		 * the client is using a large enough window for request
		 * ids, it's not a problem for both the sigio handler
		 * and vista_receive_request to handle the ack. 
		 */
		handle_ack(m->ack_id);

		/*
		 * Figure out what to do with this message. If this is
		 * a request for which we've already sent a response, 
		 * resend the response. If we've already seen this request
		 * but we haven't sent a response, just drop it. If this
		 * appears to be a new request, forward it on to the 
		 * proper server port for it. 
		 */
		for (i=0, r=NULL; i < num_segs; i++) {
			/*
			 * Find if any segment already knows about this
			 * request.
			 */
			s = segments[i];
			r = vista_hash_find(s->minfo.response_hash,
					    (char*)m->mesg_id);
			if (r != NULL)
				break;
		}

		if (r != NULL) {
			if (r->response != NULL) {
				int	sz;

				/*
				 * Retransmit response as long as it has
				 * committed.
				 */
				if (!r->commit) {
					vista_unlock();
					return TRUE;
				}

				/*
				 * Reset return address in case client has
				 * crashed and has a new return port.
				 */
				r->addr = cli_addr;
				sz = VISTA_VG_SIZE + r->response->mesg_len;
				vista_unlock();
				err = sendto(sock, r->response, sz, 0,
					     (struct sockaddr*) &r->addr, 
					     sizeof(r->addr));
				vista_lock();
				if (err < 0) {
					perror("retransmit sendto");
				}
			}
			else {
				/*
				 * We've seen this request before, but
				 * we have yet to get around to sending
				 * the response. We'll drop this 
				 * retransmission of the request.
				 */
			}
			vista_unlock();
			return TRUE;
		}

		/*
		 * Forward message to normal receive port on this server
		 */
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = VISTA_MAP_TO_UDP_PORT(m->vport);
		my_addr.sin_addr.s_addr = vista_localhost;
		err = sendto(sock, m, msize, 0, (struct sockaddr*)
			     &my_addr, sizeof(my_addr));
		if (err < 0) {
			fprintf(stderr, "sendto failed in sigio handler.\n");
		}

		vista_unlock();
		return TRUE;
	}
}


/*
 * void enqueue_ack(vista_segment *s, int ack_id, char* s_key, int tid)
 *
 *   Enqueue a message id to be ack'ed at the server given by s_key on a 
 *   subsequent request if this transaction commits.
 */
void enqueue_ack(vista_segment *s, int ack_id, char* s_key, int tid)
{
	q_node	*n;
	transaction_info* t;

	/*
	 * We'll add this message id to the queue of id's associated
	 * with this transaction. If it commits, these id's will be moved
	 * over to the queue of id's officially waiting to be ack'ed at
	 * the server specified by s_key.
	 */

	t = &s->t[tid];

	n = (q_node*) heap_malloc(s->mh, sizeof(q_node));
	if (n == NULL) {
		fprintf(stderr, "heap_malloc failed in enqueue_ack()\n");
		return;
	}

	n->id = ack_id;
	strcpy(n->s_key, s_key);
	prepend_ack(&t->vg.ack, n);
}


/*
 * int get_next_ack(vista_segment *s, int tid)
 *
 *   Return the next mesg id to acknowledge at the server specified by
 *   key. If there is no message waiting to be acknowledged, return -1.
 */
int get_next_ack(vista_segment *s, char *key, int tid)
{
	q_node		*n;
	ack_queue 	*q;
	transaction_info *t;

	t = &s->t[tid];

	q = vista_hash_find(s->minfo.server_acks, key);
	if (q == NULL) 
		return -1;
	n = q->h;
	if (n == NULL)
		return -1;
	q->h = n->next;
	prepend_ack(&t->vg.ack_done, n);

	return n->id;
}


/*
 * int vista_select(vista_segment* s, int* port_vector, int* num_ports, 
 *		    int ms_timeout)
 *
 *   s -		relevant vista segment
 *   port_vector -	array of ports to watch
 *   num_ports -	num of ports to watch
 *   ms_timeout -	max time in ms to wait for messages
 *
 *   This function checks every vistagram port in the port_vector array for
 *   incoming messages, and returns once one of two situations has arisen.
 *   If one or more ports has messages ready to be read, vista_select()
 *   returns. It sets 'port_vector' to reflect which ports are ready to
 *   be read, and returns the number of ready ports. This function 
 *   will also return if 'ms_timeout' milliseconds have passed since
 *   this function was called without any of the ports in 'port_vector' 
 *   receiving messages. This function returns the number of ports with 
 *   messages waiting, or 0 on a timeout. It returns -1 on error and sets
 *   vista_errno to indicate the nature of the error. This function can
 *   return an error if a signal is received while it is waiting for 
 *   messages.
 */
int vista_select(vista_segment* s, int* port_vector, int num_ports, 
		 int ms_timeout)
{
	int		res, i, sock, count, new_vector[4096];
	fd_set		socks;
	struct timeval 	timeout;

	vista_lock();

	timeout.tv_sec = ms_timeout / 1000;
	timeout.tv_usec = (ms_timeout % 1000) * 1000;

	FD_ZERO(&socks);

	for (i=0; i < num_ports; i++) {
		sock = get_server_sock(s, port_vector[i]);
		if (!sock) {
			vista_unlock();
			return -1;
		}

		FD_SET(sock, &socks);
	}

	prot_unlock();
	res = select(4096, &socks, NULL, NULL, &timeout);
	prot_lock();

	if (res == 0) {
		/* timeout */
		vista_unlock();
		return 0;
	}
	else if (res < 0) {
		/* select error */
		if (errno == EINTR)
			vista_errno = VISTA_ESELECTINT;
		else
			vista_errno = VISTA_ESELECT;
		vista_unlock();
		return -1;
	}
	else {
		/* some sockets are ready for reading */
		count = 0;
		for (i=0; i < num_ports; i++) {
			sock = get_server_sock(s, port_vector[i]);
			if (FD_ISSET(sock, &socks)) {
				new_vector[count++] = port_vector[i];
			}
		}

		/*
		 * Unlock before writing port_vector, because port_vector might
		 * be in Vista's space and cause a trap back to Vista's
		 * sighandler.
		 */
		prot_unlock();
		for (i=0; i < count; i++) {
			port_vector[i] = new_vector[i];
		}
		prot_lock();

		if (count != 0) {
			vista_unlock();
			return count;
		}
		else {
			vista_errno = VISTA_ESELECTRES;
			vista_unlock();
			return -1;
		}
	}
}

/*
 * get_address() converts its host argument string (either a hostname or an
 * IP address in dotted decimal format), into an internet address and returns
 * it, or FALSE on failure.
 */
static unsigned long get_address(vista_segment* s, char* host)
{
	struct in_addr	*addr_ptr;
	unsigned long	inaddr; 

	/*
	 * First check if host string we've been passed is in
	 * dotted decimal format. If not, we'll check if we're
	 * caching the IP for it already. Failing that, we'll
	 * get the IP from the system.
	 */
	if ((int)(inaddr = inet_addr(host)) == INADDR_NONE) {
		addr_ptr = vista_hash_find(s->minfo.host_table, host);
		if (addr_ptr == NULL) {
			/* Haven't seen this hostname before */
			struct hostent	*hp;

			/*
			 * Unlock around gethostbyname because it calls
			 * malloc, which Free Checking turns back into
			 * vista_malloc.
			 */
			prot_unlock();
			hp = gethostbyname(host);
			prot_lock();

			if (hp == NULL) {
				vista_errno = VISTA_EBADHOST;
				return FALSE;
			}
			addr_ptr = heap_malloc(s->mh, sizeof(struct in_addr));
			if (addr_ptr == NULL) {
				vista_errno = VISTA_ELOGALLOC;
				return FALSE;
			}
			bcopy(hp->h_addr, (char*) addr_ptr, hp->h_length);
			/* Remember this IP in case it's used again */
			vista_hash_insert(s->minfo.host_table, host, addr_ptr);
		}

		return addr_ptr->s_addr;
	}
	else {
		return inaddr;
	}
}


static void done_with_request(vista_segment* s, request* r, int tid)
{
	int		size;
	vistagram	*m;
	transaction_info *t;

	t = &s->t[tid];

	/*
	 * Set things up to automatically free the req/response 
	 * messages if the transaction commits.
	 */
	if (r->req != NULL) {
		m = r->req;
		size = m->mesg_len + VISTA_VG_SIZE;
		log_heap_op(s, s, VISTA_FREE, m, size, tid, FALSE); 
	}
	if (r->response != NULL) {
		m = r->response;
		size = m->mesg_len + VISTA_VG_SIZE;
		log_heap_op(s, s, VISTA_FREE, m, size, tid, FALSE); 
	}

	/* 
	 * Now that the vistagram messages are scheduled for being freed
	 * at commit, put this request on the done list so it will be
	 * removed from the request table and freed at commit as well.
	 */
	r->dnext = t->vg.done_list;
	t->vg.done_list = r;
	log_heap_op(s, s, VISTA_FREE, r, sizeof(request), tid, FALSE); 
}

static void delete_at_abort(vista_segment* s, request* r, int tid)
{
	int		size;
	vistagram	*m;
	transaction_info *t;

	t = &s->t[tid];

	if (r->req != NULL) {
		/*
		 * Arrange to have request message freed at abort
		 */
		m = r->req;
		size = VISTA_VG_SIZE + m->mesg_len;
		log_heap_op(s, s, VISTA_MALLOC, m, size, tid, FALSE); 
	}

	/*
	 * Arrange to have request removed from the request or response
	 * table at abort. If the 'req' field of the request is NULL, this
	 * request will be removed from the response table. It'll come out
	 * of the request table otherwise. Have it freed at abort in 
	 * either case.
	 */
	r->dnext = t->vg.del_list;
	t->vg.del_list = r;
	log_heap_op(s, s, VISTA_MALLOC, r, sizeof(request), tid, FALSE); 
}


/*
 * int recv_vec(int sock, char* b1, int b1_len, char* b2, int b2_len, 
 *              struct sockaddr_in* addr)
 *   
 *    Receive an incoming message into two buffers without extra copying.
 *
 *    Note: for prefaulting purposes, we assume that the first buffer
 *    is for the mesg header and wont be more than one page in length, 
 *    and the second is for the user data.
 */
int recv_vec(int sock, char* b1, int b1_len, char* b2, int b2_len, 
	     struct sockaddr_in* addr)
{
	struct msghdr	h;
	struct iovec	sg[2];
	struct sockaddr_in a;
	vistagram	peek;
	int		i, len, bytes, val;

	/* 
	 * Do some prefaulting of buffers in case they live in
	 * protected vista memory. We'll touch one byte per page 
	 * of buffer.
	 *
	 * In order to know how much space to prefault in the
	 * user's buffers we have no choice but to peek at the
	 * message header in the message queue, and use the mesg_len
	 * field to tell us how much to prefault.
	 *
	 * Make sure to retry this recvfrom in case a signal
	 * comes in while it's doing it's thing and causes
	 * it to return early.
	 */
	len = sizeof(a);
	while ((val = recvfrom(sock, &peek, VISTA_VG_SIZE, MSG_PEEK, 
			       (struct sockaddr*) &a, &len)) < 0 
			       && errno == EINTR);
	if (val < 0) {
		fprintf(stderr, "recv_vec: cannot peek msg header.\n");
		return -1;
	}

	bytes = peek.mesg_len;
	b1[0] = b1[0]; /* prefault byte 0 of 1st buffer */
	/* prefault at least one byte per page of 2nd buffer */
	for (i=0; i < bytes; i += VISTA_PAGE)
		b2[i] = b2[i];
	b2[bytes-1] = b2[bytes-1];

	sg[0].iov_base = b1;
	sg[0].iov_len = b1_len;
	sg[1].iov_base = b2;
	sg[1].iov_len = b2_len;

	h.msg_name = (caddr_t) addr;
	h.msg_namelen = sizeof(struct sockaddr_in);
	h.msg_iov = sg;
	h.msg_iovlen = 2;
	h.msg_control = NULL;
	h.msg_controllen = 0;
	h.msg_flags = 0;

	/*
	 * Retry this receive if necessary, as above.
	 */
	while ((val = recvmsg(sock, &h, 0)) < 0 && errno == EINTR);
	return val;
}


/*
 * int vista_send_request(vista_segment* s, char* host, int vg_port, void* req, 
 *		          int req_len, int req_id, int tid)
 *
 *   s -		relevant vista segment
 *   host -		address of host that will handle request
 *   vg_port -		vista port number 
 *   req -		request message bytes
 *   req_len -		request message length
 *   req_id -		unique id number for this request
 *   tid -		transaction identifier for enclosing transaction
 *
 *   This function initiates a request with a particular host. It builds 
 *   an outgoing message containing the 'req_len' bytes pointed to by
 *   'req'. The message is queued so that it will be sent only once
 *   transaction 'tid' commits. When the transaction commits, the message
 *   is sent to the host identified by 'host', a string that is either
 *   a hostname such as "rio.eecs.umich.edu" or an IP address in
 *   dotted decimal format such as "141.212.99.38". The message will be
 *   delivered to any process on that host that performs a 
 *   vista_receive_request() on vista port 'vg_port'. 'vg_port' can have
 *   any value from 0 to 999. The request message can be at most 65,528 
 *   bytes long.
 *
 *   This function returns TRUE if the message is safely enqueued, and 
 *   FALSE otherwise. If TRUE is returned and the enclosing transaction
 *   'tid' commits, the request message is guaranteed to be delivered
 *   to the receiving process, even if the sender or receiver process
 *   or machine crashes.
 */
int vista_send_request(vista_segment* s, char* host, int vg_port, void* req, 
		       int req_len, int req_id, int tid)
{
	vistagram	*new_mesg;
	request		*new_req;
	int		sock;
	char		key[20];
	unsigned long	ip;
	transaction_info *t;

	vista_lock();

	t = &s->t[tid];

	/*
	 * Check to make sure that req_id is valid for us
	 * to use on this message
	 */
	new_req = vista_hash_find(s->minfo.request_hash, (char*)req_id);
	if (new_req != NULL) {
		vista_errno = VISTA_EREQINUSE;
		vista_unlock();
		return FALSE;
	}

	/*
	 * Check to make sure that we have a socket for our request
	 */
	if (cli_sock <= 0) {
		vista_errno = VISTA_ESOCK;
		vista_unlock();
		return FALSE;
	}

	/*
	 * Check that outgoing message is not too big to handle
	 */
	if (req_len > VISTA_MAX_MSG) {
		vista_errno = VISTA_EMSGSIZE;
		vista_unlock();
		return FALSE;
	}

	/*
	 * Get host address
	 */
	ip = get_address(s, host); 
	if (ip == FALSE) {
		vista_unlock();
		return FALSE;
	}

	/* set up server key for later use in directing ack's to server */
	sprintf(key, "%lu:%d", ip, vg_port);

	/*
	 * Build the new outgoing message
	 */
	new_mesg = (vistagram*) heap_malloc(s->mh, VISTA_VG_SIZE + req_len);
	if (new_mesg == NULL) {
		vista_errno = VISTA_ELOGALLOC;
		vista_unlock();
		return FALSE;
	}
	new_mesg->mesg_id = req_id;
	new_mesg->mesg_len = req_len;
	new_mesg->vport = vg_port;
	new_mesg->forward_flag = FALSE;
	new_mesg->ack_id = get_next_ack(s, key, tid);
	memcpy(new_mesg->data, req, req_len);

	/*
	 * Remember this request, and where it is to be sent
	 * upon commit. 
	 */
	new_req = (request*) heap_malloc(s->mh, sizeof(request));
	if (new_req == NULL) {
		vista_errno = VISTA_ELOGALLOC;
		vista_unlock();
		return FALSE;
	}
	new_req->req = new_mesg;
	new_req->response = NULL; /* used if we have to buffer response */
	new_req->req_id = req_id;
	new_req->vport = vg_port; /* vistaport for this request */
	new_req->addr.sin_family = AF_INET;
	new_req->addr.sin_port = VISTA_MAP_TO_UDP_PORT(vg_port);
	new_req->addr.sin_addr.s_addr = ip;
	strcpy(new_req->key, key);

	/*
	 * Add this request to the table of requests, as well
	 * as to the list of outgoing messages associated
	 * with this transaction. Arrange to have things cleaned
	 * up if we should abort.
	 */
	new_req->next = NULL;
	if (t->vg.to_send_h == NULL) {
		t->vg.to_send_h = new_req;
		t->vg.to_send_t = new_req;
	}
	else {
		t->vg.to_send_t->next = new_req;
		t->vg.to_send_t = new_req;
	}
	delete_at_abort(s, new_req, tid);
	vista_hash_insert(s->minfo.request_hash, (char*)req_id, new_req);

	vista_unlock();
	return TRUE;
}


/*
 * int vista_receive_request(vista_segment* s, int vg_port, void* req, 
 *			     int* req_len, int* req_id, int tid)
 *
 *   s -		relevant vista segment
 *   vg_port -		vista port from which to receive request 
 *   req -		place to store request message bytes
 *   req_len -		place to store request message length
 *   req_id -		place to store unique id number for this request
 *   tid -		transaction identifier for enclosing transaction
 *
 *   vista_receive_request() receives the first request to arrive at vista
 *   port 'vg_port' and stores the bytes of that request at the location
 *   pointed to by 'req'. It also stores the number of bytes in the request
 *   at the location pointed to by 'req_len', and it stores the id given 
 *   the request by the sender at the location pointed to by 'req_id'. If
 *   local transaction 'tid' aborts, it will appear to the receiver as if
 *   the incoming request had never been delivered, and it will be returned
 *   by a subsequent call to vista_receive_request(). If this function
 *   succeeds it returns TRUE. It returns FALSE on error and sets 
 *   vista_errno to indicate the nature of the error. The user buffer may
 *   be trashed, even if this function returns an error.
 */
int vista_receive_request(vista_segment* s, int vg_port, void* req, 
			  int* req_len, int* req_id, int tid)
{
	vistagram	*mesg;
	request		*new_req;
	char		buf[256];
	int		sock;
	struct sockaddr_in cli_addr, serv_addr;

	vista_lock();

        /*
         * Get the socket for this port
         */
        sock = get_server_sock(s, vg_port);
	if (!sock) {
		vista_unlock();
		return FALSE;
	}

	/*
	 * Repeatedly get messages until we get one that isn't 
	 * one that we've seen before (ie. a retransmission of a 
	 * prior request).
	 */
	for (;;) {

		/*
		 * We don't want to block holding vista's lock, so
		 * we'll release it and grab it when we wake up. Furthermore,
		 * the user's buffer which recv_vec writes to might be
		 * in vista's space which could cause a trap to our 
		 * page fault handler if the user is doing implicit 
		 * transactions. So we unlock...
		 */
		vista_unlock();

		if (recv_vec(sock, buf, VISTA_VG_SIZE, req, VISTA_MAX_MSG,
		             &cli_addr) < 0) {
			vista_errno = VISTA_ERECV;
			return FALSE;
		}
		mesg = (vistagram*)buf;

		vista_lock();

		/* Delete the ack'ed request */
		handle_ack(mesg->ack_id);

		/*
		 * Check to see if this incoming message corresponds to a 
		 * request that we already know about. If not, we'll add
		 * it to the response table along with info about where to 
		 * send the response.
		 */
		new_req = vista_hash_find(s->minfo.response_hash, 
				    (char*)mesg->mesg_id);
		if (new_req == NULL) {

			/*
			 * This will be the common case: we haven't seen 
			 * this request before. Make a request struct for it.
			 */
			new_req = heap_malloc(s->mh, sizeof(request));
			if (new_req == NULL) {
				vista_errno = VISTA_ELOGALLOC;
				vista_unlock();
				return FALSE;
			}
			new_req->req = NULL;
			new_req->response = NULL;
			new_req->req_id = mesg->mesg_id;
			new_req->next = NULL;
			new_req->vport = vg_port;  /* vistaport for response */
			if (mesg->forward_flag) {
				/*
				 * This request is a forwarded 
				 * retransmission. We need to handle the 
				 * return address specially.
				 */
				new_req->addr = mesg->cli_addr; 
			}
			else {
				new_req->addr = cli_addr; 
			}
			delete_at_abort(s, new_req, tid);
			vista_hash_insert(s->minfo.response_hash, 
				    (char*)mesg->mesg_id, new_req);

			/*
			 * Pass message info up to the user. The message
			 * data itself was copied into the user buffer 
			 * by recv_vec(). 
			 */
			vista_unlock();
			*req_len = mesg->mesg_len;
			*req_id = mesg->mesg_id;

			break;
		}
		else {
			/*
			 * We've seen this request before. Let's hope it's
			 * just a retransmission by the client. We wont do
			 * anything special with the message in this case.
			 * We'll just loop around for another request.
			 */
		}

	}
	return TRUE;
}


/*
 * int vista_send_response(vista_segment* s, void* response, int resp_len, 
 *			   int req_id, int tid)
 *
 *   s -		relevant vista segment
 *   response -		response message bytes
 *   resp_len -		number of bytes in the response message
 *   req_id -		id of corresponding request
 *   tid -		transaction identifier for enclosing transaction
 *
 *   vista_send_response() sends a response message to a prior request,
 *   which implicitly acknowledges receipt of the request by the server.
 *   The 'resp_len' bytes pointed to by 'response' are queued up to be
 *   sent at commit to the client that sent the request identified by 
 *   'req_id'. If transaction 'tid' aborts, the response will not be 
 *   sent. This function returns TRUE, on success. It returns FALSE on
 *   failure and sets vista_errno to indicate the nature of the error.
 */
int vista_send_response(vista_segment* s, void* response, int resp_len, 
			int req_id, int tid)
{
	int		size;
	request		*new_req;
	vistagram	*mesg;
	transaction_info *t;

	vista_lock();

	t = &s->t[tid];

	/*
	 * Look in our request table to find where to send
	 * the response.
	 */
	new_req = vista_hash_find(s->minfo.response_hash, (char*)req_id);
	if (new_req == NULL) {
		vista_errno = VISTA_EBADREQ;
		vista_unlock();
		return FALSE;
	}
	
	/*
	 * Build outgoing message and queue it up to be sent
	 * when the transaction commits. We'll also arrange to have
	 * this message freed if the transaction aborts.
	 */
	size = VISTA_VG_SIZE + resp_len;
	mesg = (vistagram*) heap_malloc(s->mh, size);
	if (mesg == NULL) {
		vista_errno = VISTA_ELOGALLOC;
		vista_unlock();
		return FALSE;
	}
	log_heap_op(s, s, VISTA_MALLOC, mesg, size, tid, FALSE); 
	mesg->mesg_id = req_id;
	mesg->mesg_len = resp_len;
	bcopy(response, mesg->data, resp_len);

	/*
	 * The response has its own commit flag that suppresses retransmits
	 * of this response until commit. It'll be set to true by 
	 * vistagrams_commit().
	 */
	new_req->commit = FALSE;
	new_req->response = mesg;

	new_req->next = NULL;
	if (t->vg.to_send_h == NULL) {
		t->vg.to_send_h = new_req;
		t->vg.to_send_t = new_req;
	}
	else {
		t->vg.to_send_t->next = new_req;
		t->vg.to_send_t = new_req;
	}

	/*
	 * The response message and request struct will be freed by
	 * the sigio handler when this response is ack'ed by a 
	 * subsequent request.
	 */

	vista_unlock();
	return TRUE;
}


/*
 * int vista_receive_response(vista_segment* s, int req_id, void* response, 
 *                            int *resp_len, int tid)
 *
 *   s -		relevant vista segment
 *   req_id -		id number for corresponding request
 *   response -		location to store response message
 *   resp_len -		location to store length of response message
 *   tid -		transaction identifier for enclosing transaction
 *
 *   vista_receive_response() receives the response to the corresponding 
 *   request identified by 'req_id'. It stores the response message at 
 *   'response', and the number of bytes in the response at 'resp_len'. 
 *   If transaction 'tid' aborts, it will appear to the application as if 
 *   the response message had never been delivered: the response would be 
 *   returned by a later call to vista_receive_response(). This function
 *   returns TRUE on success, and FALSE on failure, setting vista_errno
 *   to indicate the nature of the error.
 */
int vista_receive_response(vista_segment* s, int req_id, void* response, 
                           int *resp_len, int tid)
{
	int		res, size, sock;
	char		buf[256];
	request		*r, *r2;
	vistagram	*m;
	transaction_info *t;
	fd_set		socks;
	struct timeval 	timeout = {VISTA_TIMEOUT/1000000,VISTA_TIMEOUT%1000000};

	vista_lock();

	t = &s->t[tid];

	/*
	 * Look in our request table to find the port from
	 * which to read the response.
	 */
	r = vista_hash_find(s->minfo.request_hash, (char*)req_id);
	if (r == NULL) {
		vista_errno = VISTA_EBADREQ;
		vista_unlock();
		return FALSE;
	}
	sock = cli_sock;
	if (sock < 0) {
		vista_errno = VISTA_ESOCK;
		vista_unlock();
		return FALSE;
	}

	/*
	 * Since the server is not required to send stuff back in any kind
	 * of order, we might have gotten the response to our request
	 * during a prior call to this function. We'll check here to see
	 * if that has happened.
	 */
	if (r->response != NULL) {
		/*
		 * We've already got the response, so return it, set
		 * it up to be ack'ed at the server, and clean up.
		 */
		/*
		 * Unlock before these things , because the data might be in
		 * Vista's space and cause a trap back to Vista's sighandler.
		 */
		m = r->response;
		prot_unlock();
		*resp_len = m->mesg_len;
		bcopy(m->data, response, m->mesg_len);
		prot_lock();

		enqueue_ack(s, req_id, r->key, tid);

		/* get rid of allocated data at commit */
		done_with_request(s, r, tid);
		vista_unlock();
		return TRUE;
	}

	for (;;) {

		FD_ZERO(&socks);
		FD_SET(sock, &socks);

		/*
		 * Unlock during blocking operation and so that select
		 * can safely set errno when running Discount Checking.
		 */
		prot_unlock();
		res = select(4096, &socks, NULL, NULL, &timeout);
		prot_lock();
		if (res < 0) {
			/* Check if simply a signal came in during select */
			if (errno == EINTR) {
				fprintf(stderr, "restarting select.\n");
				continue;
			}
			vista_errno = VISTA_ESELECT;
			vista_unlock();
			return FALSE;
		}
		else if (res == 1) {
			/*
			 * There's a message ready to read from socket.
			 * Unlock so that other stuff can be scheduled
			 * while we block, and so that writes to the 
			 * user's buffer can safely trap to vista (if
			 * they're doing IMPLICIT transactions).
			 */
			vista_unlock();
			if (recv_vec(sock, buf, VISTA_VG_SIZE, response, 
				     VISTA_MAX_MSG, NULL) < 0) {
				vista_errno = VISTA_ERECV;
				vista_unlock();
				return FALSE;
			}
			vista_lock();
			m = (vistagram*)buf;

			/*
			 * Check if this is the response we're waiting for.
			 * If not, hold on to this one in the appropriate
			 * request struct. If it is, copy the response out
			 * to the user, and schedule the free of the req
			 * message and request struct at commit.
			 */
			if (m->mesg_id == req_id) {
				/* 
				 * This is our response. It was copied
				 * into the user's buffer by recv_vec().
				 */
				/*
				 * Unlock before the store, because the data
				 * might be in Vista's space and cause a trap
				 * back to Vista's sighandler.
				 */
				prot_unlock();
				*resp_len = m->mesg_len;
				prot_lock();

				enqueue_ack(s, req_id, r->key, tid);

				/* get rid of allocated data at commit */
				done_with_request(s, r, tid);
				vista_unlock();
				return TRUE;
			}
			else {
				/* 
				 * Not it. Remember this one for later if 
				 * necessary...
				 */
				r2 = vista_hash_find(s->minfo.request_hash, 
					       (char*)m->mesg_id);
				if (r2 == NULL) {
					/*
					 * We received a response to a request
					 * we don't know about. It's probably
					 * a second response to a retransmit. 
					 * We'll just drop it.
					 */
				}
				else if (r2->response == NULL) {
					vistagram* m2;

					m2 = heap_malloc(s->mh, VISTA_VG_SIZE
							 + m->mesg_len);
					if (m2 == NULL) {
						vista_errno = VISTA_ELOGALLOC;
						vista_unlock();
						return FALSE;
					}
					m2->mesg_id = m->mesg_id;
					m2->mesg_len = m->mesg_len;
					bcopy(response, m2->data, m->mesg_len);
					r2->response = m2;
				}
				/* Wait for next message */
				continue;
			}
		}
		else if (res == 0) {
			int	err, len;

			/*
			 * We timed out, so retransmit request and wait again.
			 * Note that we send the retransmit to a different
			 * port on the server so it can handle the retransmit
			 * specially.
			 */
			r->addr.sin_port = VISTA_REXMIT_PORT(r->req->vport);
			len = r->req->mesg_len + VISTA_VG_SIZE;
			if (r->req->ack_id == -1)
				r->req->ack_id = get_next_ack(s, r->key, tid);
			err = sendto(sock, r->req, len, 0, (struct sockaddr*) 
				     &r->addr, sizeof(r->addr));
			if (err < 0) {
				vista_errno = VISTA_ESEND;
				vista_unlock();
				return FALSE;
			}

			continue;
		}
		else {
			fprintf(stderr, "receive_response: weird select err\n");
			vista_errno = VISTA_ESELECT;
			vista_unlock();
			return FALSE;
		}
	}

	/* not reached */
}


/*
 *   int vista_sigio_enable(vista_segment* s, int vg_port)
 *
 *   s -		vista segment that will use this port
 *   vg_port -		Vista port that will generate sigio
 *
 *   vista_sigio_enable() enables sigio on vg_port.  This is not a persistent
 *   call--i.e. applications will have to reinvoke vista_sigio_enable during
 *   recovery.  Returns TRUE on success; FALSE on failure.
 */
int vista_sigio_enable(vista_segment* s, int vg_port)
{
	int sock;

        sock = get_server_sock(s, vg_port);
	if (!sock)
		return FALSE;

	fcntl(sock, F_SETOWN, getpid());
	fcntl(sock, F_SETFL, FASYNC);

	return TRUE;
}

#else

void vistagrams_init(void) {}
void vistagrams_map_init(vista_segment* s) {}
void vistagrams_remap_init(vista_segment* s) {}
void vistagrams_begin_transaction(vista_segment* s, int t) {}
void vistagrams_abort(vista_segment* s, transaction_info* t, int tid) {}
void vistagrams_commit(vista_segment* s, transaction_info* t) {}
int  vistagrams_log_empty(transaction_info* t) {return TRUE;}
int  vistagrams_sigio_do_one_message(void) {return FALSE;}

#endif

