/**
 * \brief Opaque declarations of the DPO classes for use in header files 
 */ 

#ifndef __STAMNOS_DPO_CLIENT_DPO_OPAQUE_H
#define __STAMNOS_DPO_CLIENT_DPO_OPAQUE_H


namespace dpo {

namespace client {
class Dpo;
class Registry;
class StorageManager;
class ObjectManager;
} // namespace client


namespace cc {
namespace client {
class LockManager;    // forward declaration
class HLockManager;   // forward declaration
} // namespace client
} // namespace cc

} // namespace dpo


#endif // __STAMNOS_DPO_CLIENT_DPO_OPAQUE_H
