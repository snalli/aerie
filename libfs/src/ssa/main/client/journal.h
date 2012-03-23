#ifndef __STAMNOS_SSA_CLIENT_JOURNAL_H
#define __STAMNOS_SSA_CLIENT_JOURNAL_H

#include "ssa/main/client/session.h"

namespace ssa {
namespace client {

class Journal {
public:
	Journal(SsaSession* session);

	int BeginLogicalOperation(int id);
	int EndLogicalOperation();
	
private:
	SsaSession* session_;
};


} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_JOURNAL_H
