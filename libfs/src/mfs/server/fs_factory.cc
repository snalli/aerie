#include "mfs/server/fs_factory.h"
#include "common/errno.h"
#include "server/session.h"
#include "ssa/containers/super/container.h"
#include "ssa/containers/name/container.h"
#include "ssa/containers/set/container.h"
#include "ssa/main/common/obj.h"
#include "ssa/main/server/salloc.h"
#include "spa/pool/pool.h"
#include "mfs/server/fs.h"
#include "spa/const.h"



namespace mfs {
namespace server {


FileSystemFactory::FileSystemFactory()
{
	
}


int
FileSystemFactory::Make(ssa::server::Dpo* ssa, size_t nblocks, size_t block_size, int flags)
{
}


int
FileSystemFactory::Load(ssa::server::Dpo* ssa, int flags, ::server::FileSystem** filesystem)
{
	if ((*filesystem = new FileSystem(ssa->super_obj()->oid())) == NULL) {
		return -E_NOMEM;
	}
	return E_SUCCESS;
}


} // namespace server
} // namespace mfs
