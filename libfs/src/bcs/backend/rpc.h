#ifndef __STAMNOS_BCS_BACKEND_RPC_H
#define __STAMNOS_BCS_BACKEND_RPC_H

#ifdef _RPCSOCKET
#include "bcs/backend/rpc-socket/rpc.h"
#endif

#ifdef _RPCFAST
#include "bcs/backend/rpc-fast/rpc.h"
#endif

#endif // __STAMNOS_BCS_BACKEND_RPC_H
