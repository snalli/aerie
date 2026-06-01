/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

#include <util/completion.h>

using namespace util;

namespace util
{
	//============================================================================
	// Completion
	//============================================================================

	Completion::Completion(bool autoReset):
		completed(false),
		autoReset(autoReset)
	{
	}

	void Completion::signal()
	{
		boost::mutex::scoped_lock lock(this->completionMutex);
		this->completed = true;
		this->completionCondition.notify_all();
	}

	void Completion::reset()
	{
		this->completed = false;
	}

	void Completion::wait()
	{
		boost::mutex::scoped_lock lock(this->completionMutex);

		while (!this->completed)
		{
			this->completionCondition.wait(lock);
		}

		if (this->autoReset)
		{
			reset();
		}
	}

	void Completion::signalCompletion()
	{
		signal();
	}

	void Completion::waitForCompletion()
	{
		wait();
	}
}
