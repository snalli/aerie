/* internal.h: ramfs internal definitions
 *
 * Copyright (C) 2005 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */


extern const struct address_space_operations ramfs_aops;
extern const struct inode_operations ramfs_file_inode_operations;

#define SCM_LATENCY 150 
int scm_simple_write_end(struct file *, struct address_space *, 
		loff_t, unsigned len, unsigned, struct page *, void *);
void emulate_latency_ns(int);
