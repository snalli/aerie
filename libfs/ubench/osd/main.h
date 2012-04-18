#ifndef __STAMNOS_UBENCH_OSD_MAIN_H
#define __STAMNOS_UBENCH_OSD_MAIN_H

#include "osd/main/client/osd.h"
#include "osd/main/client/omgr.h"
#include "osd/main/client/obj.h"
#include "osd/main/client/lckmgr.h"
#include "osd/main/client/hlckmgr.h"
#include "osd/main/client/salloc.h"
#include "osd/main/client/session.h"
#include "osd/main/client/stsystem.h"
#include "osd/containers/containers.h"
#include "osd/containers/name/container.h"
#include "osd/containers/byte/container.h"
#include "common/util.h"


using namespace client;

extern osd::client::StorageSystem* global_storage_system;
extern Ipc*                        global_ipc_layer;

struct UbenchDescriptor {
	const char* ubench_name;
	int (*ubench_function)(int, char* []);
};



#endif // __STAMNOS_UBENCH_OSD_MAIN_H
