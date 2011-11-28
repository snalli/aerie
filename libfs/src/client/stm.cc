#include "client/stm.h"

namespace client {

namespace stm {

__thread Transaction* transaction_; // per thread transaction descriptor


int 
Transaction::Start(JmpBuf* jmpbuf, uint32_t abort_flags)
{


}


int
Transaction::OpenRO(Object* obj)
{

}


void 
Transaction::Abort(int flags)
{

}


} // namespace stm

} // namespace client
