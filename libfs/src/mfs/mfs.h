#ifndef _MFS_MFS_H_AAF781
#define _MFS_MFS_H_AAF781

#include "client/backend.h"
#include "mfs/sb.h"

namespace mfs {

client::SuperBlock* CreateSuperBlock(void* ptr);

} // namespace mfs


#endif /* _MFS_MFS_H_AAF781 */
