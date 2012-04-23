#ifndef __STAMNOS_OSD_CONTAINERS_CONTAINERS_H
#define __STAMNOS_OSD_CONTAINERS_CONTAINERS_H

/**
 * Each container defines two factory methods:
 *
 * 1) static Object* Make(Session* session);
 *
 * This calls into the storage allocator and allocates a pre-constructed
 * container.
 *
 * 2) static Object* Make(Session* session, volatile char* ptr);
 *
 * This constructs the container in the region of storage memory pointed
 * by ptr.
 *
 */


/**
 * TODO: The idea of relying on C++ templates to generate container code 
 * for client and server side turned out to be bad as it requires several
 * components (journal, allocator) at the server and client to support 
 * a common API. Looks like using MACRO expansions might be a better approach.
 */

namespace osd {
namespace containers {

enum {
	T_EXTENT = 0, // must agree with osd::common::T_EXTENT
	T_SUPER_CONTAINER = 1,
	T_NAME_CONTAINER = 2,
	T_BYTE_CONTAINER = 3,
	T_SET_CONTAINER = 4,
	T_MAP_CONTAINER = 5,
	T_NEEDLE_CONTAINER = 6,
	T_CONTAINER_TYPE_COUNT
};

} // namespace containers
} // namespace osd

#endif // __STAMNOS_OSD_CONTAINERS_CONTAINERS_H
