/**
 * \brief Opaque declarations of the SSA classes for use in header files 
 */ 

#ifndef __STAMNOS_SSA_CLIENT_SSA_OPAQUE_H
#define __STAMNOS_SSA_CLIENT_SSA_OPAQUE_H


namespace ssa {

namespace client {
class Dpo;
class Registry;
class StorageAllocator;
class ObjectManager;
} // namespace client


namespace cc {
namespace client {
class LockManager;    // forward declaration
class HLockManager;   // forward declaration
} // namespace client
} // namespace cc

} // namespace ssa


#endif // __STAMNOS_SSA_CLIENT_SSA_OPAQUE_H
