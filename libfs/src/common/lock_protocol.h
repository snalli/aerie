// lock protocol

#ifndef _LOCK_PROTOCOL_H_AKL156
#define _LOCK_PROTOCOL_H_AKL156

#include <stdint.h>
#include "rpc/rpc.h"
#include "common/bitmap.h"


class lock_protocol {
public:
	class Mode;
	class MODE;
	enum xxstatus { OK, RETRY, RPCERR, NOENT, IOERR };

	enum mode {
		NONE = -1,
		NL,   // not locked
		SL,   // shared local
		SR,   // shared recursive
		IS,   // intent shared
		IX,   // intent exclusive
		XL,   // exclusive local
		XR,   // exclusive recursive
		IXSL, // intent exclusive and shared local
	};

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
	enum mode {
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

	static std::string mode2str(int mode) {
		return mode2str_table_[mode];
	}

	static int Successor(int mode) {
		return successor_table_[mode];
	}


	/// \brief Returns the partial order of two modes mode1 and mode2
	//  \param mode1 mode
	//  \param mode2 mode
	/// \return 1, if mode1 less-than        mode2\n
	///         -1, if mode1 greater-than     mode2\n
	///         0, if mode1 not-ordered-with mode2
	static int PartialOrder(int mode1, int mode2) {
		if (mode1 == lock_protocol::Mode::IX && mode2 == lock_protocol::Mode::IXSL) {
			return -1;
		} else if (mode1 == lock_protocol::Mode::IXSL && mode2 == lock_protocol::Mode::IX) {
			return 1;
		} else {
			int s1 = severity_table_[mode1];
			int s2 = severity_table_[mode2];
			if (s1 > s2) {
				return 1;
			} else if (s1 < s2) {
				return -1;
			} 
		}
		return 0; // no ordering
	}

	/// \brief Returns the supremum mode of two modes mode1 and mode2
	static int Supremum(int mode1, int mode2) {
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

	static bool Compatible(int mode1, int mode2)
	{
		return compatibility_table_[mode1][mode2];
	}

	static bool AbidesRecursiveRule(int mode, int ancestor_recursive_mode)
	{
		uint32_t bm = recursive_rule_bitmaps_[mode];

		if (Bitmap<uint32_t>::IsSet(bm, ancestor_recursive_mode)) {
			return true;
		}
		return false;
	}

	static bool AbidesHierarchyRule(int mode, int ancestor_mode)
	{
		uint32_t bm = hierarchy_rule_bitmaps_[mode];

		if (Bitmap<uint32_t>::IsSet(bm, ancestor_mode)) {
			return true;
		}
		return false;
	}

private:
	static bool         compatibility_table_[][lock_protocol::Mode::CARDINALITY];
	static uint32_t     recursive_rule_bitmaps_[lock_protocol::Mode::CARDINALITY];
	static uint32_t     hierarchy_rule_bitmaps_[lock_protocol::Mode::CARDINALITY];
	static std::string  mode2str_table_[lock_protocol::Mode::CARDINALITY];
	static int          severity_table_[lock_protocol::Mode::CARDINALITY];
	static int          successor_table_[lock_protocol::Mode::CARDINALITY];

	int value_;
};


class lock_protocol::MODE {
public:
	class Bitmap;

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

	MODE(lock_protocol::MODE::Enum val)
		: value_(val)
	{ }
	
	lock_protocol::MODE Successor() {
		return MODE(successor_table_[value_]);
	}
	
	static lock_protocol::MODE Successor(lock_protocol::MODE mode) {
		return MODE(successor_table_[mode.value_]);
	}

	static std::string String(int mode) {
		return mode2str_table_[mode];
	}

	operator std::string() {
		return mode2str_table_[value_];
	}

	/// \brief Returns the partial order of two modes mode1 and mode2
	/// \param mode1 mode
	/// \param mode2 mode
	/// \return 1, if mode1 less-than        mode2\n
	///         -1, if mode1 greater-than     mode2\n
	///         0, if mode1 not-ordered-with mode2
	static int PartialOrder(lock_protocol::MODE mode1, lock_protocol::MODE mode2) {
		if (mode1 == MODE(lock_protocol::MODE::IX) && 
		    mode2 == MODE(lock_protocol::MODE::IXSL)) 
		{
			return -1;
		} else if (mode1 == MODE(lock_protocol::MODE::IXSL) && 
		           mode2 == MODE(lock_protocol::MODE::IX)) 
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
	static lock_protocol::MODE Supremum(lock_protocol::MODE mode1, 
	                                    lock_protocol::MODE mode2) 
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

	static bool Compatible(lock_protocol::MODE mode1, lock_protocol::MODE mode2)
	{
		return compatibility_table_[mode1.value_][mode2.value_];
	}

	static bool AbidesRecursiveRule(lock_protocol::MODE mode, 
	                                lock_protocol::MODE ancestor_recursive_mode)
	{
		uint32_t bm = recursive_rule_bitmaps_[mode.value_];

		if (BITMAP_ISSET(bm, ancestor_recursive_mode.value_)) {
			return true;
		}
		return false;
	}

	static bool AbidesHierarchyRule(lock_protocol::MODE mode, 
	                                lock_protocol::MODE ancestor_mode)
	{
		uint32_t bm = hierarchy_rule_bitmaps_[mode.value_];

		if (BITMAP_ISSET(bm, ancestor_mode.value_)) {
			return true;
		}
		return false;
	}

	bool operator==(const lock_protocol::MODE& other) {
		return (value_ == other.value_) ? true: false;
	}

	bool operator!=(const lock_protocol::MODE& other) {
		return !(*this == other);
	}

	bool operator<(const lock_protocol::MODE& other) {
		return (PartialOrder(*this, other) < 0) ? true: false;
	}

	bool operator>(const lock_protocol::MODE& other) {
		return (PartialOrder(*this, other) > 0) ? true: false;
	}

private:
	static bool         compatibility_table_[][lock_protocol::MODE::CARDINALITY];
	static uint32_t     recursive_rule_bitmaps_[lock_protocol::MODE::CARDINALITY];
	static uint32_t     hierarchy_rule_bitmaps_[lock_protocol::MODE::CARDINALITY];
	static std::string  mode2str_table_[lock_protocol::MODE::CARDINALITY];
	static int          severity_table_[lock_protocol::MODE::CARDINALITY];
	static lock_protocol::MODE::Enum          successor_table_[lock_protocol::MODE::CARDINALITY];

	Enum value_;
};


class lock_protocol::MODE::Bitmap {
public:
	enum Enum {
		NONE = -1,
		NL = BITMAP_SET(lock_protocol::MODE::NL),   // not locked
		SL = BITMAP_SET(lock_protocol::MODE::SL),   // shared local
		SR = BITMAP_SET(lock_protocol::MODE::SR),   // shared recursive
		IS = BITMAP_SET(lock_protocol::MODE::IS),   // intent shared
		IX = BITMAP_SET(lock_protocol::MODE::IX),   // intent exclusive
		XL = BITMAP_SET(lock_protocol::MODE::XL),   // exclusive local
		XR = BITMAP_SET(lock_protocol::MODE::XR),   // exclusive recursive
		IXSL = BITMAP_SET(lock_protocol::MODE::IXSL), // intent exclusive and shared local
		CARDINALITY 
	};

	Bitmap(lock_protocol::MODE::Enum val) 
		: value_(static_cast<int>(BITMAP_SET(val)))
	{ }

	Bitmap(lock_protocol::MODE mode) 
		: value_(static_cast<int>(BITMAP_SET(mode.value_)))
	{ }

	Bitmap(const lock_protocol::MODE::Bitmap& bitmap_mode)
		: value_(bitmap_mode.value_)
	{ }

	Bitmap(int val)
		: value_(val)
	{ }


	lock_protocol::MODE::Bitmap& operator|=(const lock_protocol::MODE::Bitmap& other) 
	{
		value_ |= other.value_;
		return *this;
	}

	const lock_protocol::MODE::Bitmap operator|(const lock_protocol::MODE::Bitmap& other)
	{
		Bitmap result = *this;
		result |= other;
		return result;
	}
	
	int value() const { return value_; }

private:
	int value_;
};

inline lock_protocol::MODE::Bitmap::Enum operator|(lock_protocol::MODE::Enum a, lock_protocol::MODE::Enum b)
{
	return static_cast<lock_protocol::MODE::Bitmap::Enum>((1 << a) | (1 <<b ));
}
#endif /* _LOCK_PROTOCOL_H_AKL156 */
