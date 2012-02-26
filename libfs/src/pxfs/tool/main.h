#ifndef __STAMNOS_PXFS_TOOL_MAIN_H
#define __STAMNOS_PXFS_TOOL_MAIN_H

#include "dpo/main/server/dpo.h"
#include "server/fsmgr.h"

extern const char*                  prog_name;
extern ::server::FileSystemManager* fsmgr;
extern ::dpo::server::Dpo*          dpo_layer;

extern int main_mkfs(int argc, char* argv[]);

#endif // __STAMNOS_PXFS_TOOL_MAIN_H
