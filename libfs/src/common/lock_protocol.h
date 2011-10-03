// lock protocol

#ifndef _LOCK_PROTOCOL_H_AKL156
#define _LOCK_PROTOCOL_H_AKL156

#include <stdio.h>
#include <stdint.h>
#include "rpc/rpc.h"
#include "common/bitmap.h"


class lock_protocol {
public:
	class Mode;
	enum xxstatus { OK, RETRY, RPCERR, NOENT, IOERR };

	enum flag {
		FLG_NOQUE = 0x1,  // don't queue client if can't grant request
	};

	enum revoke {
		RVK_NO = 0,      // no revoke
		RVK_NL,      
		RVK_XL2SL,
		RVK_SR2SL,
		RVK_XR2XL,
		RVK_IXSL2IX
	};

	typedef int status;
	typedef unsigned long long LockId;
	enum rpc_numbers {
		acquire = 0x7001,
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

	operator std::string() {
		return mode2str_table_[value_];
	}

	/// \brief Returns the partial order of two modes mode1 and mode2
	/// \param mode1 mode
	/// \param mode2 mode
	/// \return 1, if mode1 less-than        mode2\n
	///         -1, if mode1 greater-than     mode2\n
	///         0, if mode1 not-ordered-with mode2
	static int PartialOrder(lock_protocol::Mode mode1, lock_protocol::Mode mode2) 
	{
		if (mode1 == Mode(lock_protocol::Mode::IX) && 
		    mode2 == Mode(lock_protocol::Mode::IXSL)) 
		{
			return -1;
		} else if (mode1 == Mode(lock_protocol::Mode::IXSL) && 
		           mode2 == Mode(lock_protocol::Mode::IX)) 
		{
			return 1;
		} else {
			int s1 = severity_table_[mode1.value_];
			int s2 = severity_table_[mode2.value_];
			if (s1 > s2) {
				return 1;
			} else if (s1 < s2) {
				return -1;
			} 
		}
		return 0; // no ordering
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

	static bool Compatible(lock_protocol::Mode mode1, lock_protocol::Mode mode2)
	{
		return compatibility_table_[mode1.value_][mode2.value_];
	}

	static bool AbidesRecursiveRule(lock_protocol::Mode mode, 
	                                lock_protocol::Mode ancestor_recursive_mode)
	{
		uint32_t bm = recursive_rule_bitmaps_[mode.value_];

		if (BITMAP_ISSET(bm, ancestor_recursive_mode.value_)) {
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
	static bool         compatibility_table_[][lock_protocol::Mode::CARDINALITY];
	static uint32_t     recursive_rule_bitmaps_[lock_protocol::Mode::CARDINALITY];
	static uint32_t     hierarchy_rule_bitmaps_[lock_protocol::Mode::CARDINALITY];
	static std::string  mode2str_table_[lock_protocol::Mode::CARDINALITY];
	static int          severity_table_[lock_protocol::Mode::CARDINALITY];
	static lock_protocol::Mode::Enum          successor_table_[lock_protocol::Mode::CARDINALITY];

	Enum value_;
};


class lock_protocol::Mode::Set {
public:
	class Iterator;

	enum Enum {
		NONE = -1,
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


	static bool Compatible(lock_protocol::Mode mode, lock_protocol::Mode::Set mode_set)
	{
		int                       val = mode_set.value_;
		int                       m;
		lock_protocol::Mode::Enum enum_m;

		while (val) {
			m = __builtin_ctz(val); 
			enum_m = static_cast<lock_protocol::Mode::Enum>(m);
			val &= ~(1 << m);
			if (!lock_protocol::Mode::Compatible(lock_protocol::Mode(enum_m), mode)) { 
				return false;
			}
		}
		return true;
	}


	static int PartialOrder(lock_protocol::Mode mode, 
	                        lock_protocol::Mode::Set mode_set) 
	{
		int                       val = mode_set.value_;
		int                       m;
		lock_protocol::Mode::Enum enum_m;
		int                       r;
		bool                      init_po = false;
		int                       po;

		while (val) {
			m = __builtin_ctz(val); 
			enum_m = static_cast<lock_protocol::Mode::Enum>(m);
			val &= ~(1 << m);
			r = lock_protocol::Mode::PartialOrder(mode, lock_protocol::Mode(enum_m));
			if (init_po == false) {
				init_po = true;
				po = r;
			} else {
				if (r != po) {
					po = 0;
					return po;
				}
			} 
		}
		return po;
	}

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

	std::string String() {  
		//TODO
	}

private:
	int value_;
};

inline lock_protocol::Mode::Set::Enum operator|(lock_protocol::Mode::Enum a, lock_protocol::Mode::Enum b)
{
	return static_cast<lock_protocol::Mode::Set::Enum>((1 << a) | (1 <<b ));
}

/*
class lock_protocol::Mode::Set::Iterator {
public:
	Iterator() { }

	Iterator(lock_protocol::Mode::Set mode_set )
	{
	}

    Iterator(const PInode::Iterator& val)
    //  	start_(val.start_), current_(val.current_) {}
	{}
}
*/

#endif /* _LOCK_PROTOCOL_H_AKL156 */
