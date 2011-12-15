// lock protocol

#ifndef __STAMNOS_DPO_LOCK_PROTOCOL_H
#define __STAMNOS_DPO_LOCK_PROTOCOL_H

#include <stdio.h>
#include <stdint.h>
#include <string>
#include "rpc/rpc.h"
#include "common/bitmap.h"


class lock_protocol {
public:
	class Mode;
	enum xxstatus { 
		OK,      // okay. no error
		RETRY,   // expect a retry call
		RPCERR,  // RPC frustrate error
		NOENT,   // generic error
		IOERR,   // IO error 
		HRERR,   // Hierarchy violation
		CANCEL,  // Request is cancelled. don't expect a retry.
		DEADLK   // Deadlock detected
	};

	enum flag {
		FLG_NOQUE      = 0x1,  // don't queue client if can't grant request
		FLG_CAPABILITY = 0x2
	};

	enum revoke {
		RVK_NO = 0,      // no revoke
		RVK_NL,      
		RVK_XL2SL,
		RVK_SR2SL,
		RVK_XR2IX,
		RVK_XR2XL,
		RVK_XR2IXSL,
		RVK_IXSL2IX
	};

	typedef int status;
	typedef unsigned long long LockId;

	enum rpc_numbers {
		acquire = 0x7001,
		acquirev,
		release,
		convert,
		subscribe,
		stat
	};
};


class rlock_protocol {
public:
	enum xxstatus { OK, RPCERR };
	typedef int status;
	enum rpc_numbers {
		revoke = 0x8001,
		retry = 0x8002
	};
};


class lock_protocol::Mode {
public:
	class Set;

	enum Enum {
		NONE = -1,
		NL,   // not locked
		SL,   // shared local
		SR,   // shared recursive
		IS,   // intent shared
		IX,   // intent exclusive
		XL,   // exclusive local
		XR,   // exclusive recursive
		IXSL, // intent exclusive and shared local
		CARDINALITY 
	};

	Mode()
		: value_(lock_protocol::Mode::NL)
	{ }
	
	Mode(lock_protocol::Mode::Enum val)
		: value_(val)
	{ }
	
	lock_protocol::Mode Successor() {
		return Mode(successor_table_[value_]);
	}
	
	static lock_protocol::Mode Successor(lock_protocol::Mode mode) {
		return Mode(successor_table_[mode.value_]);
	}

	static std::string String(lock_protocol::Mode mode) {
		return mode2str_table_[mode.value_];
	}

	std::string String() { return mode2str_table_[value_]; }

	/// \brief Returns the partial order of two modes mode1 and mode2
	/// \param mode1 mode
	/// \param mode2 mode
	/// \return 1, if mode1 less-than        mode2\n
	///         -1, if mode1 greater-than     mode2\n
	///         0, if mode1 not-ordered-with mode2
	static int PartialOrder(lock_protocol::Mode mode1, lock_protocol::Mode mode2) 
	{
		int s1 = severity_table_[mode1.value_];
		int s2 = severity_table_[mode2.value_];
		
		if (s1 > s2) {
			return 1;
		} else if (s1 < s2) {
			return -1;
		} else { /* s1 == s2 */
			if (mode1 == Mode(lock_protocol::Mode::XL) && 
				mode2 == Mode(lock_protocol::Mode::IXSL)) 
			{
				return 1;
			} else if (mode1 == Mode(lock_protocol::Mode::IXSL) && 
					   mode2 == Mode(lock_protocol::Mode::XL)) 
			{
				return -1;
			}
			return 0; // no ordering
		}
	}


	/// \brief Returns the supremum mode of two modes mode1 and mode2
	static lock_protocol::Mode Supremum(lock_protocol::Mode mode1, 
	                                    lock_protocol::Mode mode2) 
	{
		int po;

		while (mode1 != mode2) {
			po = PartialOrder(mode1, mode2);
			if (po < 0) {
				return mode2;
			} else if (po > 0) {
				return mode1;
			}
			mode1 = Successor(mode1);
			mode2 = Successor(mode2);
		}

		return mode1;
	}

	lock_protocol::Mode LeastRecursiveMode()
	{
		return lock_protocol::Mode(least_recursive_mode_table_[value_]);
	}
	
	bool Compatible(lock_protocol::Mode other)
	{
		return compatibility_table_[value_][other.value_];
	}

	static bool Compatible(lock_protocol::Mode mode1, lock_protocol::Mode mode2)
	{
		return compatibility_table_[mode1.value_][mode2.value_];
	}

	static bool AbidesRecursiveRule(lock_protocol::Mode mode, 
	                                lock_protocol::Mode recursive_mode)
	{
		uint32_t bm = recursive_rule_bitmaps_[mode.value_];

		if (BITMAP_ISSET(bm, recursive_mode.value_)) {
			return true;
		}
		return false;
	}

	static bool AbidesHierarchyRule(lock_protocol::Mode mode, 
	                                lock_protocol::Mode ancestor_mode)
	{
		uint32_t bm = hierarchy_rule_bitmaps_[mode.value_];

		if (BITMAP_ISSET(bm, ancestor_mode.value_)) {
			return true;
		}
		return false;
	}

	bool operator==(const lock_protocol::Mode& other) {
		return (value_ == other.value_) ? true: false;
	}

	bool operator!=(const lock_protocol::Mode& other) {
		return !(*this == other);
	}

	bool operator<(const lock_protocol::Mode& other) {
		return (PartialOrder(*this, other) < 0) ? true: false;
	}

	bool operator>(const lock_protocol::Mode& other) {
		return (PartialOrder(*this, other) > 0) ? true: false;
	}

	int value() const { return static_cast<int>(value_); }

private:
	static bool                      compatibility_table_[][lock_protocol::Mode::CARDINALITY];
	static uint32_t                  recursive_rule_bitmaps_[lock_protocol::Mode::CARDINALITY];
	static uint32_t                  hierarchy_rule_bitmaps_[lock_protocol::Mode::CARDINALITY];
	static std::string               mode2str_table_[lock_protocol::Mode::CARDINALITY];
	static int                       severity_table_[lock_protocol::Mode::CARDINALITY];
	static lock_protocol::Mode::Enum successor_table_[lock_protocol::Mode::CARDINALITY];
	static lock_protocol::Mode::Enum least_recursive_mode_table_[lock_protocol::Mode::CARDINALITY];

	Enum value_;
};


class lock_protocol::Mode::Set {
public:
	class Iterator;

	enum Enum {
		NONE = 0,
		NL = BITMAP_SET(lock_protocol::Mode::NL),   // not locked
		SL = BITMAP_SET(lock_protocol::Mode::SL),   // shared local
		SR = BITMAP_SET(lock_protocol::Mode::SR),   // shared recursive
		IS = BITMAP_SET(lock_protocol::Mode::IS),   // intent shared
		IX = BITMAP_SET(lock_protocol::Mode::IX),   // intent exclusive
		XL = BITMAP_SET(lock_protocol::Mode::XL),   // exclusive local
		XR = BITMAP_SET(lock_protocol::Mode::XR),   // exclusive recursive
		IXSL = BITMAP_SET(lock_protocol::Mode::IXSL), // intent exclusive and shared local
		CARDINALITY 
	};

	Set() 
		: value_(0)
	{ }

	Set(lock_protocol::Mode::Enum val) 
		: value_(static_cast<int>(BITMAP_SET(val)))
	{ }

	Set(lock_protocol::Mode mode) 
		: value_(static_cast<int>(BITMAP_SET(mode.value_)))
	{ }

	Set(const lock_protocol::Mode::Set& mode_set)
		: value_(mode_set.value_)
	{ }

	Set(int val)
		: value_(val)
	{ }

	bool Exists(lock_protocol::Mode mode)
	{
		return ((1 << mode.value_) & value_) ? true : false;
	}
	
	void Insert(lock_protocol::Mode mode)
	{
		value_ |= (1 << mode.value_);
	}

	void Remove(lock_protocol::Mode mode)
	{
		value_ = (~(1 << mode.value_)) & value_;
	}
	
	bool Empty()
	{
		return value_ == 0;
	}

	void Clear()
	{
		value_ = 0;
	}

	static bool Compatible(lock_protocol::Mode mode, lock_protocol::Mode::Set mode_set);
	static int PartialOrder(lock_protocol::Mode mode, lock_protocol::Mode::Set mode_set);
	lock_protocol::Mode MostSevere(lock_protocol::Mode compatible_mode);
	lock_protocol::Mode LeastSevere();

	lock_protocol::Mode::Set& operator|=(const lock_protocol::Mode::Set& other) 
	{
		value_ |= other.value_;
		return *this;
	}

	const lock_protocol::Mode::Set operator|(const lock_protocol::Mode::Set& other)
	{
		Set result = *this;
		result |= other;
		return result;
	}
	
	int value() const { return value_; }
	inline std::string String();

	Iterator begin();
	Iterator end();

private:
	int value_;
};


class lock_protocol::Mode::Set::Iterator {
public:
	Iterator()
		: value_(0)
	{ }

	Iterator(lock_protocol::Mode::Set set)
		: value_(set.value_)
	{ }

	void operator++(int) {
		int m;

		m = __builtin_ctz(value_); 
		value_ &= ~(1 << m);
	}

	lock_protocol::Mode operator*() {
		int                       m;
		lock_protocol::Mode::Enum enum_m;
		m = __builtin_ctz(value_); 
		enum_m = static_cast<lock_protocol::Mode::Enum>(m);
		return lock_protocol::Mode(enum_m);
	}

	bool operator==(const lock_protocol::Mode::Set::Iterator& other)
	{
		return (value_ == other.value_) ? true : false;
	}

	bool operator!=(const lock_protocol::Mode::Set::Iterator& other)
	{
		return !(*this == other);
	}

private:
	int value_;
};

#include "dpo/common/lock_protocol-inl.h"

#endif // __STAMNOS_DPO_LOCK_PROTOCOL_H
