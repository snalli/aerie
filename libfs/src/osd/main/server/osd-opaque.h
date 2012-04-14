/**
 * \brief Opaque declarations of the OSD classes for use as forward 
 * declarations in header files 
 */ 

#ifndef __STAMNOS_OSD_SERVER_OSD_OPAQUE_H
#define __STAMNOS_OSD_SERVER_OSD_OPAQUE_H


namespace osd {
namespace server {
class StorageSystem;
class Registry;
class ObjectManager;
class StorageAllocator;
class OsdSession;
} // namespace server
} // namespace osd


namespace osd {

namespace cc {
namespace server {
class LockManager;    
class HLockManager;   
} // namespace server
} // namespace cc
} // namespace osd


#endif // __STAMNOS_OSD_SERVER_OSD_OPAQUE_H
