#ifndef _TRANSACTION_CLIENT_STAMNOS_H_BMA567
#define _TRANSACTION_CLIENT_STAMNOS_H_BMA567

#include <setjmp.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/types.h"
#include "dpo/common/cow.h"
#include "dcc/common/stm.h"

namespace client {

#define STM_BEGIN()                                       \
  stm::Transaction* __tx = stm::Self();                   \
  stm::JmpBuf*       __jmpbuf = __tx->jmpbuf();           \
  uint32_t __abort_flags = sigsetjmp(*__jmpbuf, 1);       \
  if (__tx->Start(__jmpbuf, __abort_flags) == 0) {   

#define STM_END()                                         \
  } 

#define STM_ABORT()                                       \
  do {                                                    \
     stm::Transaction* __tx = stm::Self();                \
     __tx->Abort();                                       \
  } while(0);

#define STM_ABORT_IF_INVALID()                            \
  do {                                                    \
     stm::Transaction* __tx = stm::Self();                \
     __tx->AbortIfInvalid();                              \
  } while(0);


namespace stm {

enum {
	ABORT_EXPLICIT = 1,
	ABORT_VALIDATE = 2,
};

typedef sigjmp_buf        JmpBuf;
typedef dcc::stm::Object  Object;
typedef dcc::stm::Version Version;

class Transaction {
public:
	int Init();
	int Start(JmpBuf* jmpbuf, uint32_t abort_flags);
	int	Commit();
	void AbortIfInvalid(); 
	int Validate();
	void Abort();
	void Rollback(int flags);
	int OpenRO(Object* obj);
	JmpBuf* jmpbuf() {
		return &jmpbuf_;
	}

private:
	struct ReadSetEntry {
		Version version_;
	};
	typedef google::dense_hash_map<Object*, ReadSetEntry> ReadSet;
	
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



template<class Proxy, class Subject>
class ObjectProxy: public cow::ObjectProxy<Proxy, Subject> {
public:
	Proxy* xOpenRO(Transaction* tx);
	Proxy* xOpenRO();
private:
	Transaction* tx_;
};


template<class Proxy, class Subject>
Proxy* ObjectProxy<Proxy, Subject>::xOpenRO(Transaction* tx)
{
	Proxy* proxy = static_cast<Proxy*>(this);
	Object* subj = proxy->subject();
	tx->OpenRO(subj);
	return proxy;
}

template<class Proxy, class Subject>
Proxy* ObjectProxy<Proxy, Subject>::xOpenRO()
{
	Transaction* tx = Self();
	return xOpenRO(tx);
}



} // namespace stm

} // namespace client

#endif // TRANSACTION_CLIENT_STAMNOS_H_BMA567
