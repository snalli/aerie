/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#if 0

#include "blockingMultiQueue.h"

using namespace util::queue ;

BlockingMultiQueue::BlockingMultiQueue( uint16 numQueues ) 
{
	/// create 'numQueues' number of queuees 
}

bool BlockingMultiQueue::blockQueue( QueueId queueId )
{
	/// block queueId queue
}

bool BlockingMultiQueue::unblockQueue( QueueId queueId )
{
	/// unblock queueId queue
}

BlockingMultiQueue::pair< QueueKey, QueueValue> pop()
{
	/// delete by insert sequence --

	/// use the nextQueueInSequence to pop

	/// update nextQueueInSequence
}

pair<QueueKey, QueueValue> BlockingMultiQueue::searchQueue( QueueId queueId, QueueKey & key )
{
	/// pass the search request to queue with id queueId
}

bool BlockingMultiQueue::insert( QueueId queueId, QueueKey & key, QueueValue & value)
{
	/// insert into a paticular queue; if queue is blocked, return false
}

bool BlockingMultiQueue::erase( QueueId queueId, QueueKey & key )
{
	/// delete by key -- 
}

#endif
