#include "kvs.h"

const char* dbdir = "/tmp";

static KCDB* db;

void kvs_init() {
	char dbpath[512];
	/* create the database object */
	db = kcdbnew();
	/* open the database */
	fprintf(stderr, "Initialize KVS...\n");
	uint32_t mode = KCOWRITER | KCOCREATE | KCOAUTOTRAN;
	sprintf(dbpath, "%s/kvs.kch", dbdir);
	if (!kcdbopen(db, dbpath, mode)) {
		fprintf(stderr, "open error: %s\n", kcecodename(kcdbecode(db)));
	}
}

ssize_t 
kvs_get(const char* key, void* buf)
{
	char* vbuf;
	size_t vsiz;
	/* retrieve a record */
	vbuf = kcdbget(db, key, strlen(key), &vsiz);
	if (!vbuf) {
		//return -1;
		return 0; 
	} 
	memcpy(buf, vbuf, vsiz);
	kcfree(vbuf);
	return vsiz;
}


ssize_t 
kvs_del(const char* key)
{
	int ret;
	ret = kcdbremove(db, key, strlen(key));
	if (ret) {
		return 0;
	}
	return -1;
}

ssize_t
kvs_put (const char* key, const void *buf, size_t count)
{
	if (!kcdbset(db, key, strlen(key), buf, count)) {
		return -1;
	}
	return count;
}

void kvs_shutdown()
{
  /* close the database */
  if (!kcdbclose(db)) {
    fprintf(stderr, "close error: %s\n", kcecodename(kcdbecode(db)));
  }

  /* delete the database object */
  kcdbdel(db);
}
