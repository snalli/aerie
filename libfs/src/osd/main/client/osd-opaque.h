/**
 * \brief Opaque declarations of the OSD classes for use in header files 
 */ 

#ifndef __STAMNOS_OSD_CLIENT_OSD_OPAQUE_H
#define __STAMNOS_OSD_CLIENT_OSD_OPAQUE_H


namespace osd {

namespace client {
class StorageSystem;
class Registry;
class StorageAllocator;
class ObjectManager;
class OsdSharedBuffer;
class Journal;
class OsdSession;
} // namespace client


namespace cc {
namespace client {
class LockManager;    // forward declaration
class HLockManager;   // forward declaration
} // namespace client
} // namespace cc

} // namespace osd


#endif // __STAMNOS_OSD_CLIENT_OSD_OPAQUE_H
