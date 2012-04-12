#include "kvfs/client/c_api.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rpc/rpc.h"
#include "kvfs/client/client.h"
#include "bcs/main/common/cdebug.h"

using namespace client;


int
FRONTAPI(init) (int argc, char* argv[])
{
	return Client::Init(argc, argv);
}


int
FRONTAPI(init2) (const char* xdst)
{
	return Client::Init(xdst);
}


int
FRONTAPI(shutdown) ()
{
	return Client::Shutdown();
}


int 
FRONTAPI(mount) (const char* source, uint32_t flags)
{
	return Client::Mount(source, flags);
}


int 
FRONTAPI(umount) (const char* target)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");	
}
 

ssize_t 
FRONTAPI(put) (const char* key, const void *buf, size_t count)
{
	const char* src = reinterpret_cast<const char*>(buf);

	return Client::Put(key, src, count);
}


ssize_t 
FRONTAPI(get) (const char* key, void *buf, size_t count)
{
	char* dst = reinterpret_cast<char*>(buf);

	return Client::Get(key, dst, count);
}


int 
FRONTAPI(del) (const char* key)
{
	return Client::Delete(key);
}


int
FRONTAPI(sync) ()
{
	return Client::Sync();
}
