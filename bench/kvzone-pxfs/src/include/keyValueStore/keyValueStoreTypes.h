/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 *
 * Key-Value Stores types supported on the command line.
 */
#ifndef			_KV_TYPES_H
#define			_KV_TYPES_H

#include <boost/function.hpp>

#include "util/types.h"
#include "util/exceptions.h"

namespace keyValueStore
{
    enum KeyValueStoreType
    {
        BERKELEYDB = 1,
        TOKYOCABINET = 2,
        SQLITE = 3,
        MEMMAP = 4,
        NOOP = 5,
        BERKELEYDB2 = 8,
    };

    /**
     * Some stores could support asynchronous inserts, i.e. data just
     * cached in memory
     */
    enum Durability
    {
        DURABLE,
        VOLATILE
    };

    using util::uint64;
    using util::uint32;
    using util::Exception;

    static uint32 const BUFFER_SIZE = (256 + 16) * 1024;
    
    enum LookupResult
	{
		FOUND=31,
		NOT_FOUND,
	};

    class Serializable
    {
    public:
        virtual uint32 getSerializedSize() const = 0;
        virtual void serialize(char* buffer) const = 0;
        virtual void deserialize(char const* buffer) = 0;;
    };

    class ComparableSerializable : public Serializable
    {
    public:
        virtual bool operator<(ComparableSerializable const&) const = 0;
    };

    typedef boost::function<bool(ComparableSerializable const &, ComparableSerializable const &) > KeyCompare;

    class MyCompare
    {
    public:
        bool operator()(ComparableSerializable const& lh, ComparableSerializable const& rh)
        {
            return (lh < rh);
        }
    };


    //==============================================================================================
    //===================== Exceptions =============================================================
    //==============================================================================================
  
    class ExceptionDeviceNotFound : public Exception
    {
    } ;

    class ExceptionDeviceFailure : public Exception
    {
    } ;

    class ExceptionDeviceNotFree : public Exception
    {
    } ;

    class ExceptionDeviceNonEmpty : public Exception
    {
    } ;

    class ExceptionOutOfSpace : public Exception
    {
    } ;

    class ExceptionInvalidArgument : public Exception
    {
    } ;

    class ExceptionValueNonExistent : public Exception
    {
    } ;

    class ExceptionValueLocked : public Exception
    {
    } ;

    class ExceptionTxnDeadlock : public Exception
    {
    } ;

	class ExceptionMemoryUndersized : public Exception
	{
	} ;

	class ExceptionKVStoreVersionMismatch : public Exception
	{
	} ;

    class MiscException : public Exception
    {
    };

} // namespace keyValueStore
#endif

