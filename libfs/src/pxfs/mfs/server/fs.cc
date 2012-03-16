#include "mfs/server/fs.h"

namespace mfs {
namespace server {

FileSystem::FileSystem(ssa::common::ObjectId super)
	: superblock_(super)
{ }


} // namespace server
} // namespace mfs
