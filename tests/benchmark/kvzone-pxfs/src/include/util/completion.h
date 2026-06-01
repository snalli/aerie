/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */
#ifndef _UTIL_COMPLETION_H
#define _UTIL_COMPLETION_H

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

/**
	Returns thread id (Linux kernel thread identifier).
*/
namespace util
{
	//============================================================================

	/**
	 * Class providing thread-safe waiting for completion of
	 * one operation.
	 */
	class Completion
	{
	public:
		Completion(bool autoReset = true);

		/**
		 * Signal that operation has been completed.
		 */
		void signal();

		/**
		 * Wait until operation is completed.
		 * If operation is completed before entering this method,
		 * caller is not suspended.
		 */
		void wait();

		/**
		 * Resets the completion.
		 */
		void reset();

		/**
		 * @deprecated
		 */
		void signalCompletion();

		/**
		 * @deprecated
		 */
		void waitForCompletion();

	protected:
		bool completed;
		bool const autoReset;

		boost::mutex completionMutex;
		boost::condition completionCondition;
	};

}
#endif // _UTIL_COMPLETION_H
