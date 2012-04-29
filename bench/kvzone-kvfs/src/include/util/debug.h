/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */
#ifndef _UTIL_DEBUG_H
#define _UTIL_DEBUG_H
/**
 * Debugging macros and classes.
 */

#include <iostream>
#include <assert.h>

namespace util
{
    std::string backtrace();

	#define DLOG(level, msg) 
	#define LOG(level, msg) 

	#define _DLOG(level, ...) 
	#define _LOG(level, ...) 

	#define __DLOG(level, msg) 
	#define __LOG(level, msg) 
	
	#define DLOGT(level, msg, tag) 
	
    #define _DLOGT(level, tag, ...) 

	//============================================================================

	/**
	 * Assert macro. Always, regardless of UTIL_ASSERT.
	 * This is to be used for "production" or "permanent" asserts.
         *
         * \c cond SHOULD NOT have side effects --- if \c cond has side
         * effects, then execution may differ between debug and release
         * compilations if PASSERTs are ever switched to DASSERTs later
         * on.
         *
         * A \c PASSERT comment is a `<<' separated ostream-ready code.
	 */
	#define PASSERT(cond, comment) assert(cond)

	#define _PASSERT(cond, ...) 

	/**
	 * Causes program to abort with message, file/line and backtrace
	 * information printed to both the log and error output. Has 
	 * guarantee that no subsequent messages will be printed.
	 */
	#define FAILURE(comment) do{ cerr << comment << endl; ::abort(); }while(0)

	#define _FAILURE(...) 

	#define DASSERT(cond, comment) 

	#define _DASSERT(cond, ...) 

	#define BEGIN_DASSERT  
	
	#define END_DASSERT 
	
    #define BEGIN_DEBUG
	
    #define END_DEBUG

	/**
	 * FT equivalent of PASSERT macro
	 */
	#define FT_ASSERT(ftlevel, cond, comment) 

	/**
     * FT equivalent of FAILURE macro
     */
	#define FT_FAILURE(ftlevel, comment) 

#define CLASS_WRAPPER(className) private util::Counter< className >

}

#endif
