/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

#ifndef _UTIL_GETTID_H
#define _UTIL_GETTID_H

#include <syscall.h>

/**
	Returns thread id (Linux kernel thread identifier).
*/
inline pid_t gettid()
{               
    return (pid_t)syscall(SYS_gettid);                                                           
}                                                                                                

#endif // _UTIL_THREADS_H
