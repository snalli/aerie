#ifndef __STAMNOS_UBENCH_OSD_H
#define __STAMNOS_UBENCH_OSD_H

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
#include "bcs/bcs.h"
#include "common/util.h"
#include "ubench/main.h"

namespace client {
	extern osd::client::StorageSystem* global_storage_system;
	extern Ipc*                        global_ipc_layer;
}

extern int ubench_create(int argc, char* argv[]);
extern int ubench_open(int argc, char* argv[]);
extern int ubench_lock(int argc, char* argv[]);
extern int ubench_hlock(int argc, char* argv[]);
extern int ubench_hlock_create(int argc, char* argv[]);
extern int ubench_rpc(int argc, char* argv[]);


#endif // __STAMNOS_UBENCH_MAIN_H
