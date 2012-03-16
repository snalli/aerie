#ifndef __STAMNOS_SSA_COMMON_CC_LOCK_PROTOCOL_H
#define __STAMNOS_SSA_COMMON_CC_LOCK_PROTOCOL_H

#include <sstream>
#include <boost/functional/hash.hpp>
#include "ssa/main/common/lock_protocol.h"


namespace ssa {
namespace cc {
namespace common {

typedef uint16_t LockType;

class LockId {
	enum {
		LOCK_NUMBER_LEN_LOG2 = 48
	};
public:
	LockId(uint64_t u64 = 0)
		: u64_(u64)
	{ }

	LockId(const LockId& lid)
		: u64_(lid.u64_)
	{ }

	LockId(LockType type_id, uint64_t num) {
		Init(type_id, num);
	}

	LockType type() const {
		return u16_[3];
	}

	uint64_t number() const {
		return u64_ & ((1LLU << LOCK_NUMBER_LEN_LOG2) - 1);
	}

	bool operator==(const LockId& other) const {
		return (u64_ == other.u64_);
	}

	bool operator!=(const LockId& other) const {
		return !(*this == other);
	}

	lock_protocol::LockId marshall() const {
		return u64_;
	}

	std::string string() const {
		std::stringstream oss;
		oss << type() << "." << number();
		oss << std::hex << " (0x" << type() << ".0x" << number() << ")";
		return oss.str();
	}

	const char* c_str() const {
		return string().c_str();
	}
private:
	void Init(LockType type_id, uint64_t num) {
		u64_ = type_id;
		u64_ = u64_ << LOCK_NUMBER_LEN_LOG2;
		u64_ = u64_ | num;
	}

	union {
		uint64_t u64_;  
		uint16_t u16_[4];
	};
};

struct LockIdHashFcn {
	std::size_t operator()(const LockId& lid) const {
		boost::hash<uint64_t> hasher;
		return hasher(lid.marshall());
	}
};


} // namespace common
} // namespace cc
} // namespace ssa


#endif // __STAMNOS_SSA_COMMON_CC_LOCK_PROTOCOL_H
