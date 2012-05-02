#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "kvfs/client/client.h"
#include "kvfs/client/libfs.h"
#include "kvfs.fixture.h"

using namespace client;

static const char* storage_pool_path = "/tmp/stamnos_pool";
static const char* test_str1 = "TEST_CREATE";

SUITE(KVFSFile)
{
	TEST_FIXTURE(KVFSFixture, TestPut)
	{
		int  fd;
		char buf[512];

		EVENT("E1");
		CHECK(kvfs_mount(storage_pool_path, 0) == 0);
		CHECK(kvfs_put("test1", test_str1, strlen(test_str1)+1) > 0);
		CHECK(kvfs_get("test1", buf) > 0);
		CHECK(strcmp(buf, test_str1) == 0);
		EVENT("E2");
		EVENT("E3");
	}
	
	TEST_FIXTURE(KVFSFixture, TestPutSync)
	{
		int  fd;
		char buf[512];

		EVENT("E1");
		CHECK(kvfs_mount(storage_pool_path, 0) == 0);
		CHECK(kvfs_put("test1", test_str1, strlen(test_str1)+1) > 0);
		CHECK(kvfs_get("test1", buf) > 0);
		CHECK(strcmp(buf, test_str1) == 0);
		CHECK(kvfs_sync() == 0);
		EVENT("E2");
		EVENT("E3");
	}
	
	TEST_FIXTURE(KVFSFixture, TestPutDelSync)
	{
		int  fd;
		char buf[512];

		EVENT("E1");
		CHECK(kvfs_mount(storage_pool_path, 0) == 0);
		CHECK(kvfs_put("test1", test_str1, strlen(test_str1)+1) > 0);
		CHECK(kvfs_put("test2", test_str1, strlen(test_str1)+1) > 0);
		CHECK(kvfs_put("test3", test_str1, strlen(test_str1)+1) > 0);
		CHECK(kvfs_put("test4", test_str1, strlen(test_str1)+1) > 0);
		CHECK(kvfs_put("test5", test_str1, strlen(test_str1)+1) > 0);
		CHECK(kvfs_del("test2") == 0);
		CHECK(kvfs_del("test3") == 0);
		CHECK(kvfs_get("test1", buf) > 0);
		CHECK(strcmp(buf, test_str1) == 0);
		CHECK(kvfs_sync() == 0);
		EVENT("E2");
		EVENT("E3");
	}

	TEST_FIXTURE(KVFSFixture, TestGet)
	{
		int  fd;
		char buf[512];

		EVENT("E1");
		CHECK(kvfs_mount(storage_pool_path, 0) == 0);
		CHECK(kvfs_get("test1", buf) > 0);
		CHECK(strcmp(buf, test_str1) == 0);
		EVENT("E2");
		EVENT("E3");
	}

	TEST_FIXTURE(KVFSFixture, TestGetAfterPutDelSync)
	{
		int  fd;
		char buf[512];

		EVENT("E1");
		CHECK(kvfs_mount(storage_pool_path, 0) == 0);
		CHECK(kvfs_get("test1", buf) > 0);
		CHECK(strcmp(buf, test_str1) == 0);
		CHECK(kvfs_get("test4", buf) > 0);
		CHECK(strcmp(buf, test_str1) == 0);
		CHECK(kvfs_get("test5", buf) > 0);
		CHECK(strcmp(buf, test_str1) == 0);
		EVENT("E2");
		EVENT("E3");
	}


	struct Entry {
		Entry(const char* _key, const char* _val) 
		{ 
			strcpy(key, _key);
			strcpy(val, _val);
		}

		char key[32];
		char val[32];
	};

	// continuously remove and insert items in round robin
	TEST_FIXTURE(KVFSFixture, TestStress)
	{
		int  r;
		int  fd;
		char buf[512];
		char key_buf[128];
		char val_buf[128];

		EVENT("E1");
		CHECK(kvfs_mount(storage_pool_path, 0) == 0);

		std::list<Entry> items_out; // items outside the table
		std::list<Entry> items_in;  // items inside the table

		srand(1);

		for (int i=0; i<1024; i++) {
			sprintf(key_buf, "%lu", i);
			sprintf(val_buf, "%lu", rand() % (1024*1024));
			items_out.push_back(Entry(key_buf, val_buf));
		}
		
		for (int i=0; i<1024; i++) {
			sprintf(key_buf, "%lu", i+1024);
			sprintf(val_buf, "%lu", rand() % (1024*1024));
			items_in.push_back(Entry(key_buf, val_buf));
			CHECK(kvfs_put(key_buf, val_buf, strlen(val_buf)+1) > 0);
		}
	
		for (int i=0; i<1024; i++) {
			Entry entry_del = items_in.front();
			Entry entry_ins = items_out.front();
			items_in.pop_front();
			items_out.pop_front();
			CHECK((r=kvfs_get(entry_del.key, val_buf)) > 0);
			CHECK(strcmp(val_buf, entry_del.val) == 0);
			items_out.push_back(entry_del);
			items_in.push_back(entry_ins);
			CHECK(kvfs_del(entry_del.key) == 0);
			CHECK(kvfs_put(entry_del.key, entry_del.val, strlen(entry_del.val)+1) > 0);
		}
		EVENT("E2");
		EVENT("E3");
	}


}
