#ifndef __STAMNOS_CFS_TOOL_MAIN_H
#define __STAMNOS_CFS_TOOL_MAIN_H

#include "osd/main/server/osd.h"
#include "cfs/server/fs.h"

extern const char*                    prog_name;
extern ::server::FileSystem*          fs;
extern ::osd::server::StorageSystem*  storage_system;

extern int main_mkfs(int argc, char* argv[]);

#endif // __STAMNOS_CFS_TOOL_MAIN_H
