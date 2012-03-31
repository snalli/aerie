#ifndef __STAMNOS_SSA_CONTAINERS_CONTAINERS_H
#define __STAMNOS_SSA_CONTAINERS_CONTAINERS_H

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


namespace ssa {
namespace containers {

enum {
	T_SUPER_CONTAINER = 1,
	T_NAME_CONTAINER = 2,
	T_BYTE_CONTAINER = 3,
	T_SET_CONTAINER = 4
};

} // namespace containers
} // namespace ssa

#endif // __STAMNOS_SSA_CONTAINERS_CONTAINERS_H
