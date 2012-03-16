#ifndef __STAMNOS_PXFS_TOOL_MAIN_H
#define __STAMNOS_PXFS_TOOL_MAIN_H

#include "ssa/main/server/ssa.h"
#include "pxfs/server/fs.h"

extern const char*                    prog_name;
extern ::server::FileSystem*          fs;
extern ::ssa::server::StorageSystem*  storage_system;

extern int main_mkfs(int argc, char* argv[]);

#endif // __STAMNOS_PXFS_TOOL_MAIN_H
