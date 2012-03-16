/**
 * \brief Opaque declarations of the SSA classes for use as forward 
 * declarations in header files 
 */ 

#ifndef __STAMNOS_SSA_SERVER_SSA_OPAQUE_H
#define __STAMNOS_SSA_SERVER_SSA_OPAQUE_H


namespace ssa {
namespace server {
class StorageSystem;
class Registry;
class ObjectManager;
class StorageAllocator;
class SsaSession;
} // namespace server
} // namespace ssa


namespace ssa {

namespace cc {
namespace server {
class LockManager;    
class HLockManager;   
} // namespace server
} // namespace cc
} // namespace ssa


#endif // __STAMNOS_SSA_SERVER_SSA_OPAQUE_H
