#ifndef __STAMNOS_SSA_CLIENT_TRANSACTION_H
#define __STAMNOS_SSA_CLIENT_TRANSACTION_H

#include <setjmp.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "ssa/main/common/obj.h"
#include "common/types.h"

namespace ssa {
namespace stm {
namespace client {

#define STM_BEGIN()                                                        \
  ::ssa::stm::client::Transaction* __tx = ::ssa::stm::client::Self();      \
  ::ssa::stm::client::JmpBuf*      __jmpbuf = __tx->jmpbuf();              \
  uint32_t __abort_flags = sigsetjmp(*__jmpbuf, 1);                        \
  if (__tx->Start(__jmpbuf, __abort_flags) == 0) {   

#define STM_END()                                                          \
  } 

#define STM_ABORT()                                                        \
  do {                                                                     \
     ::ssa::stm::client::Transaction* __tx = ::ssa::stm::client::Self();   \
     __tx->Abort();                                       \
  } while(0);

#define STM_ABORT_IF_INVALID()                                             \
  do {                                                                     \
     ::ssa::stm::client::Transaction* __tx = ::ssa::stm::client::Self();   \
     __tx->AbortIfInvalid();                                               \
  } while(0);


enum {
	ABORT_EXPLICIT = 1,
	ABORT_VALIDATE = 2,
};

typedef sigjmp_buf  JmpBuf;
typedef uint64_t    Version;

class Transaction {
public:
	int Init();
	int Start(JmpBuf* jmpbuf, uint32_t abort_flags);
	int	Commit();
	void AbortIfInvalid(); 
	int Validate();
	void Abort();
	void Rollback(int flags);
	int OpenRO(::ssa::cc::common::Object* obj);

	JmpBuf* jmpbuf() {
		return &jmpbuf_;
	}

private:
	struct ReadSetEntry {
		Version version_;
	};
	typedef google::dense_hash_map< ::ssa::cc::common::Object*, ReadSetEntry> ReadSet;
	
	ReadSet rset_;
	JmpBuf  jmpbuf_;
	int     nesting_;
};


extern __thread Transaction* transaction_; // per thread transaction descriptor

inline Transaction* 
Self()
{
	if (transaction_) {
		return transaction_;
	}
	transaction_ = new Transaction();
	transaction_->Init();
	return transaction_;
}


} // namespace client
} // namespace stm
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_TRANSACTION_H
