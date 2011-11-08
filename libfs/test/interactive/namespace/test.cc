#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/lock_protocol.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "client/hlckmgr.h"

using namespace client;

static lock_protocol::LockId root = 1;
static lock_protocol::LockId a = 2;
static lock_protocol::LockId b = 3;
static lock_protocol::LockId c = 4;
static lock_protocol::LockId d = 5;
static lock_protocol::LockId e = 6;
static lock_protocol::LockId f = 7;
static lock_protocol::LockId g = 8;


void test11(const char* tag)
{
	printf("%s\n", __FUNCTION__);
	libfs_mkfs("/superblock/A", "mfs", 0);
	libfs_mount("/superblock/A", "/home/hvolos", "mfs", 0);
	libfs_mkdir("/home/hvolos/dir", 0);
	libfs_mkdir("/home/hvolos/dir/test", 0);
	
	
	//libfs_open("/etc/hvolos/test", 0);
	//libfs_open("/home/hvolos/test", 0);
	//libfs_open("/home/hvolos/dir/test", O_CREATE);
	//libfs_open("/home/hvolos/file", O_CREATE);

	printf("%s: DONE\n", __FUNCTION__);
	sleep(1000);
}

void test12(const char* tag)
{
	printf("%s\n", __FUNCTION__);
	sleep(3);
	libfs_mount("/superblock/A", "/home/hvolos", "mfs", 0);
	assert(libfs_rmdir("/home/hvolos/dir") == 0);
	printf("%s: DONE\n", __FUNCTION__);
}


void test(const char* tag) {
	if (strcmp(tag, "C1") == 0) {
		test11(tag);
	} else {
		test12(tag);
	}
}
