/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 

/**************************************************************
  blockingMultiQueue.h
  - has multiple queues
  - searchable multi queue
  - each queue blockable 
  - dequeue functionality exists
  - is not threadsafe

  ************************************************************/


#include "queueTypes.h"

//#include "blockingQueue.h"

namespace util
{
namespace queue
{

	class BlockingMultiQueue
	{
		public:

			BlockingMultiQueue(uint16 numQueues ) ;

			blockQueue( QueueId queueId ) ;
			unblockQueue( QueueId queueId ) ;

			pair< QueueKey, QueueValue> pop() ;

			pair<QueueKey, QueueValue> searchQueue( QueueId queueId, QueueKey & key ) ;

			bool insert( QueueId queueId, QueueKey & key, QueueValue & value) ;

			bool erase( QueueId queueId, QueueKey & key ) ;

		private:

			QueueId nextQueueInSequence ;
			QueueId * ptrToLastInsertedQueueId ;
	} ; // BlockingMultiQueue

}

}



