#ifndef __STAMNOS_PXFS_TOOL_MAIN_H
#define __STAMNOS_PXFS_TOOL_MAIN_H

#include "ssa/main/server/ssa.h"
#include "server/fsmgr.h"

extern const char*                  prog_name;
extern ::server::FileSystemManager* fsmgr;
extern ::ssa::server::Dpo*          ssa_layer;

extern int main_mkfs(int argc, char* argv[]);

#endif // __STAMNOS_PXFS_TOOL_MAIN_H
