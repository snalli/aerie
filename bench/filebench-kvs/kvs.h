#ifndef _KVS_H
#define _KVS_H

#include <kclangc.h>

void kvs_init();
ssize_t kvs_get(const char* key, void* buf);
ssize_t kvs_del(const char* key);
ssize_t kvs_put (const char* key, const void *buf, size_t count);
void kvs_shutdown();

#endif
