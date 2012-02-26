/**
 * \brief Opaque declarations of the DPO classes for use as forward 
 * declarations in header files 
 */ 

#ifndef __STAMNOS_DPO_SERVER_DPO_OPAQUE_H
#define __STAMNOS_DPO_SERVER_DPO_OPAQUE_H


namespace dpo {
namespace server {
class Dpo;               
class Registry;          
class ObjectManager;     
class StorageAllocator;    
} // namespace server
} // namespace dpo


namespace dpo {

namespace cc {
namespace server {
class LockManager;    
class HLockManager;   
} // namespace server
} // namespace cc
} // namespace dpo


#endif // __STAMNOS_DPO_SERVER_DPO_OPAQUE_H
