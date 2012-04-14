#ifndef __STAMNOS_OSD_CLIENT_TRANSACTION_H
#define __STAMNOS_OSD_CLIENT_TRANSACTION_H

#include <setjmp.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "osd/main/common/obj.h"
#include "common/types.h"

namespace osd {
namespace stm {
namespace client {

#define STM_BEGIN()                                                        \
  ::osd::stm::client::Transaction* __tx = ::osd::stm::client::Self();      \
  ::osd::stm::client::JmpBuf*      __jmpbuf = __tx->jmpbuf();              \
  uint32_t __abort_flags = sigsetjmp(*__jmpbuf, 1);                        \
  if (__tx->Start(__jmpbuf, __abort_flags) == 0) {   

#define STM_END()                                                          \
  } 

#define STM_ABORT()                                                        \
  do {                                                                     \
     ::osd::stm::client::Transaction* __tx = ::osd::stm::client::Self();   \
     __tx->Abort();                                       \
  } while(0);

#define STM_ABORT_IF_INVALID()                                             \
  do {                                                                     \
     ::osd::stm::client::Transaction* __tx = ::osd::stm::client::Self();   \
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
	int OpenRO(::osd::cc::common::Object* obj);

	JmpBuf* jmpbuf() {
		return &jmpbuf_;
	}

private:
	struct ReadSetEntry {
		Version version_;
	};
	typedef google::dense_hash_map< ::osd::cc::common::Object*, ReadSetEntry> ReadSet;
	
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
} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_TRANSACTION_H
