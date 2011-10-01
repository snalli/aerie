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

	static int Successor(int mode) {
		return successor_table_[mode];
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

	static int Successor(int mode) {
		return successor_table_[mode];
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



/*
class lock_protocol::BitmapMode {
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


    operator lock_protocol::Mode() {
        //return static_cast<lock_protocol::mode>(value_);
		//TODO
    }

private:
	mode value_;
}
*/


#endif /* _LOCK_PROTOCOL_H_AKL156 */
