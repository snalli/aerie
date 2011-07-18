#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include "test/unittest.h"
#include "mfs/hashtable.h"
#include "common/errno.h"

SUITE(SuiteHashTableEntry)
{
	const int buffer_size = 128;

	TEST(TestMakeEntry1)
	{
		char   b[buffer_size];
		Entry* entry; 

		entry = Entry::MakeEntry(b);
		CHECK(entry->Init(true, buffer_size-TAG_SIZE) == 0);
		CHECK(entry->IsFree() == true);
		CHECK(entry->get_size() == (buffer_size));
		CHECK(entry->get_payload_size() == (buffer_size-TAG_SIZE));

		entry = Entry::MakeEntry(b);
		CHECK(entry->IsFree() == true);
		CHECK(entry->get_size() == (buffer_size));
		CHECK(entry->get_payload_size() == (buffer_size-TAG_SIZE));

		entry = Entry::MakeEntry(b);
		CHECK(entry->Init(false, buffer_size-TAG_SIZE) == 0);
		CHECK(entry->IsFree() == false);
		CHECK(entry->get_size() == (buffer_size));
		CHECK(entry->get_payload_size() == (buffer_size-TAG_SIZE));

		entry = Entry::MakeEntry(b);
		CHECK(entry->IsFree() == false);
		CHECK(entry->get_size() == (buffer_size));
		CHECK(entry->get_payload_size() == (buffer_size-TAG_SIZE));
	
	}

	TEST(TestMakeEntry2)
	{
		char   b[buffer_size];
		Entry* entry; 
		Entry* entry2; 

		entry = Entry::MakeEntry(b);
		CHECK(entry->Init(true, buffer_size-TAG_SIZE) == 0);
		CHECK(entry->IsFree() == true);
		CHECK(entry->get_size() == (buffer_size));
		CHECK(entry->get_payload_size() == (buffer_size-TAG_SIZE));

		entry2 = Entry::MakeEntry(entry, buffer_size-TAG_SIZE+1);
		CHECK(entry2==NULL);
		entry2 = Entry::MakeEntry(entry, 18);
		CHECK(entry2!=NULL);
		CHECK(entry2->IsFree() == true);
		CHECK(entry2->get_size() == (entry->get_size() - 18));
	}

}


SUITE(SuiteHashTablePage)
{
	TEST(TestInsert1)
	{
		char*    key1 = (char*) "key1";
		uint64_t val;
		Page   page;

		CHECK(page.Insert(key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
	}

	TEST(TestInsert2)
	{
		char*    key1 = (char*) "key1";
		char*    key2 = (char*) "key2";
		char*    key3 = (char*) "key3";
		uint64_t val;
		Page   page;

		CHECK(page.Insert(key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(key3, strlen(key3)+1, 0xFB1) == 0);
		CHECK(page.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
	}

	TEST(TestInsert3)
	{
		char*    key1 = (char*) "key1";
		char*    key2 = (char*) "key2";
		char     longkey3[PAGE_SIZE];
		int      longkey3_size = PAGE_SIZE - 18;
		uint64_t val;
		Page   page;

		CHECK(page.Insert(longkey3, longkey3_size, 0xFB1) == 0);
		CHECK(page.Search(longkey3, longkey3_size, &val) == 0);
		CHECK(val == 0xFB1);
	}


	TEST(TestInsert4)
	{
		char*    key1 = (char*) "key1";
		char*    key2 = (char*) "key2";
		char     longkey3[PAGE_SIZE];
		int      longkey3_size = PAGE_SIZE - 18;
		uint64_t val;
		Page   page;

		CHECK(page.Insert(key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(longkey3, longkey3_size, 0xFB1) < 0);
		CHECK(page.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(longkey3, longkey3_size, &val) < 0);
	}


	TEST(TestInsert5)
	{
		char*    key1 = (char*) "key1";
		char*    key2 = (char*) "key2";
		char     longkey3[PAGE_SIZE];
		int      longkey3_size = PAGE_SIZE - 18;
		uint64_t val;
		Page   page;

		CHECK(page.Insert(key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(longkey3, longkey3_size, 0xFB1) < 0);
		CHECK(page.Search(longkey3, longkey3_size, &val) < 0);
		CHECK(page.Delete(key1, strlen(key1)+1) == 0);
		CHECK(page.Delete(key2, strlen(key2)+1) == 0);
		CHECK(page.Insert(longkey3, longkey3_size, 0xFB1) == 0);
		CHECK(page.Search(longkey3, longkey3_size, &val) == 0);
	}


	TEST(TestInsert6)
	{
		char*    key1 = (char*) "key1";
		char*    key2 = (char*) "key2";
		char*    key3 = (char*) "key3";
		char*    key4 = (char*) "key3";
		int      key1_size = (PAGE_SIZE-5*sizeof(uint64_t))/3;
		int      key2_size = (PAGE_SIZE-5*sizeof(uint64_t))/3;
		int      key3_size = (PAGE_SIZE-5*sizeof(uint64_t))/3;
		int      key4_size = 2*key1_size-10;
		uint64_t val;
		Page   page;

		CHECK(page.Insert(key1, key1_size, 0xCAFE) == 0);
		CHECK(page.Insert(key2, key2_size, 0xC1A) == 0);
		CHECK(page.Insert(key3, key3_size, 0xFB1) == 0);
		CHECK(page.Insert(key4, key4_size, 0xBEEF) < 0);
		CHECK(page.Search(key1, key1_size, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(key2, key2_size, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(key3, key3_size, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page.Delete(key2, key2_size) == 0);
		CHECK(page.Insert(key4, key4_size, 0xBEEF) < 0);
		CHECK(page.Insert(key2, key2_size, 0xB00) == 0);
		CHECK(page.Search(key2, key2_size, &val) == 0);
		CHECK(val == 0xB00);
		CHECK(page.Delete(key2, key2_size) == 0);
		CHECK(page.Insert(key4, key4_size, 0xBEEF) < 0);
		CHECK(page.Delete(key1, key2_size) == 0);
		CHECK(page.Insert(key4, key4_size, 0xBEEF) == 0);
	}



	TEST(TestDelete1)
	{
		char*    key1 = (char*) "key1";
		char*    key2 = (char*) "key2";
		char*    key3 = (char*) "key3";
		char*    key4 = (char*) "key4";
		uint64_t val;
		Page   page;

		CHECK(page.Insert(key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(key3, strlen(key3)+1, 0xFB1) == 0);

		CHECK(page.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);

		CHECK(page.Delete(key2, strlen(key2)+1) == 0);
		CHECK(page.Search(key2, strlen(key2)+1, &val) < 0);

		CHECK(page.Insert(key4, strlen(key4)+1, 0x18ACF9) == 0);

		CHECK(page.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(key2, strlen(key2)+1, &val) < 0);
		CHECK(page.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page.Search(key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0x18ACF9);
	}


	TEST(TestSplitHalf1)
	{
		char*    key1 = (char*) "key1_xxxx_";
		char*    key2 = (char*) "key2_xxxx_";
		char*    key3 = (char*) "key3_xxxx_";
		char*    key4 = (char*) "key4_xxxx_";
		uint64_t val;
		Page   page;
		Page   newpage;

		CHECK(page.Insert(key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(key3, strlen(key3)+1, 0xFB1) == 0);
		CHECK(page.Insert(key4, strlen(key4)+1, 0xFAFA) == 0);

		CHECK(page.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page.Search(key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);

		CHECK(page.SplitHalf(&newpage) == 0);

		CHECK(page.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(key3, strlen(key3)+1, &val) != 0);
		CHECK(page.Search(key4, strlen(key4)+1, &val) != 0);

		CHECK(newpage.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(newpage.Search(key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
	}


	TEST(TestSplitHalf2)
	{
		char*      key1 = (char*) "key1_xxxx_";
		char*      key2 = (char*) "key2_xxxx_";
		char*      key3 = (char*) "key3_xxxx_";
		char*      key4 = (char*) "key4_xxxx_";
		char*      key5 = (char*) "key5_xxxx_";
		uint64_t   val;
		Page page;
		Page newpage;

		CHECK(page.Insert(key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(key3, strlen(key3)+1, 0xFB1) == 0);
		CHECK(page.Insert(key4, strlen(key4)+1, 0xFAFA) == 0);
		CHECK(page.Insert(key5, strlen(key5)+1, 0xBEEF) == 0);

		CHECK(page.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page.Search(key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(page.Search(key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);

		CHECK(page.Delete(key2, strlen(key2)+1) == 0);

		CHECK(page.SplitHalf(&newpage) == 0);

		CHECK(page.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(key2, strlen(key2)+1, &val) != 0);
		CHECK(page.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(page.Search(key4, strlen(key4)+1, &val) != 0);
		CHECK(page.Search(key5, strlen(key5)+1, &val) != 0);

		CHECK(newpage.Search(key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(newpage.Search(key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);
	}


	TEST(TestMerge1)
	{
		char*    key1 = (char*) "key1_xxxx_";
		char*    key2 = (char*) "key2_xxxx_";
		char*    key3 = (char*) "key3_xxxx_";
		char*    key4 = (char*) "key4_xxxx_";
		char*    key5 = (char*) "key5_xxxx_";
		uint64_t val;
		Page     page1;
		Page     page2;

		CHECK(page1.Insert(key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page1.Insert(key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page1.Insert(key3, strlen(key3)+1, 0xFB1) == 0);
		CHECK(page2.Insert(key4, strlen(key4)+1, 0xFAFA) == 0);
		CHECK(page2.Insert(key5, strlen(key5)+1, 0xBEEF) == 0);

		CHECK(page1.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page1.Search(key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page1.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page2.Search(key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(page2.Search(key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);

		CHECK(page1.Merge(&page2) == 0);

		CHECK(page1.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page1.Search(key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page1.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page1.Search(key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(page1.Search(key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);
	}


	TEST(TestMergeOverflow)
	{
		char*    key1 = (char*) "key1_0000_1111_2222";
		char*    key2 = (char*) "key2_0000_1111_2222";
		char*    key3 = (char*) "key3_0000_1111_2222";
		char*    key4 = (char*) "key4_0000_1111_2222";
		char*    key5 = (char*) "key5_0000_1111_2222";
		uint64_t val;
		Page     page1;
		Page     page2;

		CHECK(page1.Insert(key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page1.Insert(key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page1.Insert(key3, strlen(key3)+1, 0xFB1) == 0);
		CHECK(page2.Insert(key4, strlen(key4)+1, 0xFAFA) == 0);
		CHECK(page2.Insert(key5, strlen(key5)+1, 0xBEEF) == 0);

		CHECK(page1.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page1.Search(key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page1.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page2.Search(key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(page2.Search(key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);

		CHECK(page1.Merge(&page2) == -ENOMEM);

		CHECK(page1.Search(key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page1.Search(key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page1.Search(key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page1.Search(key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(page1.Search(key5, strlen(key5)+1, &val) != 0);
		CHECK(page2.Search(key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);
	}


}

std::string gen_rand_str(const int minlen, const int maxlen) {
    static const char alphanum[] =
	   "0123456789"
	   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	   "abcdefghijklmnopqrstuvwxyz";
	int               len;
	char              s[128];

	assert(maxlen < 128);

	len = minlen+rand() % (maxlen - minlen+1);

	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

    s[len] = 0;

	return std::string(s);
}


SUITE(SuiteHashTable)
{
	TEST(TestInsert)
	{
		int                                                        i;
		HashTable                                                  ht;
		std::map<std::string, uint64_t>                            kvmap;
		std::map<std::string, uint64_t>::iterator                  it;
		std::pair<std::map<std::string, uint64_t>::iterator, bool> ret;
		std::string                                                key;
		uint64_t                                                   val;
		int                                                        rv;

		srand(0);

		for (i=0; i<128; i++) {
			key=gen_rand_str(16, 16);
			kvmap.insert(std::pair<std::string, uint64_t>(key, i));
		}	

		for (it=kvmap.begin(); it != kvmap.end(); it++) {
			key = (*it).first;
			val = (*it).second;
			CHECK(ht.Insert(key.c_str(), strlen(key.c_str())+1, val) == 0);
		}

		for (it=kvmap.begin(); it != kvmap.end(); it++) {
			key = (*it).first;
			CHECK(ht.Search(key.c_str(), strlen(key.c_str())+1, &val) == 0);
			CHECK(val == (*it).second);
		}

	}
}
