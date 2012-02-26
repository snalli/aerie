#include "mfs/server/fs.h"

namespace mfs {
namespace server {

FileSystem::FileSystem(dpo::common::ObjectId super)
	: superblock_(super)
{ }


} // namespace server
} // namespace mfs
