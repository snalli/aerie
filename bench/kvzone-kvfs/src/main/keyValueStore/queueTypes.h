/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#ifndef		_QUEUE_TYPES
#define		_QUEUE_TYPES

class QueueId
{
	public:
		QueueId(uint16 queueId) : queueId(queueId) {} ;

		uint16 getQueueId() ;

	private:
		uint16 queueId ;
} ;


class Holder
{
	public:
		Holder() ;

	private:
} ;


/// abs base class
class QueueKey
{
	public:
		virtual bool operator<(QueueKey & other) ;
} ;

/// abs base class
class QueueValue
{
	public:
		QueueValue() ;

	private:
		// void * value ;
} ;

#endif



