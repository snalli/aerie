#include "mfs/server/fs_factory.h"
#include "common/errno.h"
#include "server/session.h"
#include "dpo/containers/super/container.h"
#include "dpo/containers/name/container.h"
#include "dpo/containers/set/container.h"
#include "dpo/main/common/obj.h"
#include "dpo/main/server/salloc.h"
#include "sal/pool/pool.h"
#include "mfs/server/fs.h"
#include "sal/const.h"



namespace mfs {
namespace server {


FileSystemFactory::FileSystemFactory()
{
	
}


int
FileSystemFactory::Make(dpo::server::Dpo* dpo, size_t nblocks, size_t block_size, int flags)
{
}


int
FileSystemFactory::Load(dpo::server::Dpo* dpo, int flags, ::server::FileSystem** filesystem)
{
	if ((*filesystem = new FileSystem(dpo->super_obj()->oid())) == NULL) {
		return -E_NOMEM;
	}
	return E_SUCCESS;
}


} // namespace server
} // namespace mfs
