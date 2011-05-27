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
 * vistagrams.h
 *
 * DESCRIPTION
 *
 *   This file contains the global declarations for the 
 *   portion of the vista library that provides vistagram
 *   service with a reliable request/response protocol.
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

#ifndef VISTAGRAMS_H
#define VISTAGRAMS_H


/**
 ** Includes
 **/

#include "heap.h"
#include "hash.h"
#include <netinet/in.h>


/**
 ** Defines
 **/

#define VISTA_NPORTS 1000
#define UDP_BASE 7000
#define VISTA_MAP_TO_UDP_PORT(vport) (htons((vport % VISTA_NPORTS) + UDP_BASE))
#define VISTA_REXMIT_PORT(vport) (htons((vport % VISTA_NPORTS) + UDP_BASE + VISTA_NPORTS))
#define VISTA_TIMEOUT 200000  /* 200 ms timeout expressed in microsecs */
#define VISTA_SENDTO_BACKOFF 1000 /* micros to wait if send buffers full */

#if defined(__FreeBSD__)
#define VISTA_SENDBUF (128 * 1024 + 100) /* set socket sendbuf to this size */
#else
#define VISTA_SENDBUF (64 * 1024 + 100) /* set socket sendbuf to this size */
#endif


/**
 ** Types
 **/


/*
 * Header for vistagram message.
 */
typedef struct vistagram_s {
	int	mesg_id;	/* id of request/response */
	int	mesg_len;	/* # bytes in message */
	int	vport;		/* vista port for message */
	char	forward_flag;	/* did this message get forwarded by server? */
	int	ack_id;		/* mesg id to ack at server */
	struct sockaddr_in cli_addr; /* addr for response */
	char	data[1];
} vistagram;

/* size of just header portion of vistagram message */
#define VISTA_VG_SIZE (sizeof(vistagram)-4) 
/*
#define VISTA_MAX_MSG (VISTA_SENDBUF-VISTA_VG_SIZE-32) 
*/
#define VISTA_MAX_MSG (VISTA_VG_SIZE + (32 * 1024)) /* max user data size */


/*
 * Node on linked list that makes up the ack_queue
 */
typedef struct q_node_s {
	int	id;		/* id of request to ack */
	char	s_key[20];	/* server process' key */
	struct q_node_s* next;	/* next element in queue */
} q_node;


/*
 * Queue of request id's that will be ack'ed at the server eventually
 */
typedef volatile struct ack_queue_s {
	q_node		*h;	/* head of linked list */
} ack_queue;


/*
 * This struct describes a vistagram sent or received during a transaction.
 * It plays different roles depending on whether it is being used by the
 * client or the server in the request-response exchange.
 */
typedef struct request_s {
	vistagram*		req; 	/* assembled request */
	vistagram*		response; /* response to request */
	int			commit; /* has response committed? */
	int			req_id; /* id of request */
	int			vport; /* vista port for request/response */
	char			key[20]; /* key for accessing per serv info */
	struct sockaddr_in	addr; /* addr of server or client */
	struct request_s*	next;	/* next deferred request */
	struct request_s*	dnext;	/* next request to delete on abort */
} request;


/*
 * The vistagram_trans struct will hold onto all the per transaction
 * vistagram information.
 */
typedef struct vistagram_trans_s {
	request		*to_send_h;  /* outgoing message log head */
	request		*to_send_t;  /* outgoing message log tail */
	request		*done_list;  /* requests to delete at commit */
	request		*del_list;   /* requests to delete at abort */
	ack_queue	ack;         /* ids to ack if transaction commits */
	ack_queue	ack_done;    /* ids to delete if transaction commits */
} vistagram_trans;


/*
 * The message_info struct encapsulates the various tables needed to support
 * our style of vistagram messaging.
 */
typedef struct message_info_s {
	hash_table	*request_hash; 	/* client side vistagrams */
	hash_table	*response_hash; /* server side vistagrams */
	hash_table	*host_table; 	/* table of hostnames/IPs */
	hash_table	*port_table; 	/* maps vista port->socket */
	hash_table	*server_acks;	/* table of messages/server to ack */
} message_info;


typedef struct vistagram_stats_s {
	int	messages_sent;
	int	retransmits;
	int	dropped_requests;
} vistagram_stats;

#endif

