#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <string>
#include "tool/testfw/unittest.h"
#include "osd/containers/map/hashtable.h"
#include "common/errno.h"
#include "unit/fixture/session.fixture.h"


SUITE(ContainersHashTableEntry)
{
	const int buffer_size = 128;

	TEST_FIXTURE(SessionFixture, TestMakeEntry1)
	{
		char            b[buffer_size];
		Entry<Session>* entry; 

		CHECK((entry = Entry<Session>::Make(b, true, buffer_size-TAG_SIZE)) != NULL);
		CHECK(entry->IsFree() == true);
		CHECK(entry->get_size() == (buffer_size));
		CHECK(entry->get_payload_size() == (buffer_size-TAG_SIZE));

		entry = Entry<Session>::Load(b);
		CHECK(entry->IsFree() == true);
		CHECK(entry->get_size() == (buffer_size));
		CHECK(entry->get_payload_size() == (buffer_size-TAG_SIZE));

		CHECK((entry = Entry<Session>::Make(b, false, buffer_size-TAG_SIZE)) != NULL);
		CHECK(entry->IsFree() == false);
		CHECK(entry->get_size() == (buffer_size));
		CHECK(entry->get_payload_size() == (buffer_size-TAG_SIZE));

		entry = Entry<Session>::Load(b);
		CHECK(entry->IsFree() == false);
		CHECK(entry->get_size() == (buffer_size));
		CHECK(entry->get_payload_size() == (buffer_size-TAG_SIZE));
	
	}

	TEST_FIXTURE(SessionFixture, TestMakeEntry2)
	{
		char            b[buffer_size];
		Entry<Session>* entry; 
		Entry<Session>* entry2; 

		CHECK((entry = Entry<Session>::Make(b, true, buffer_size-TAG_SIZE)) != NULL);
		CHECK(entry->IsFree() == true);
		CHECK(entry->get_size() == (buffer_size));
		CHECK(entry->get_payload_size() == (buffer_size-TAG_SIZE));

		entry2 = Entry<Session>::Split(session, entry, buffer_size-TAG_SIZE+1);
		CHECK(entry2==NULL);
		entry2 = Entry<Session>::Split(session, entry, 18);
		CHECK(entry2!=NULL);
		CHECK(entry2->IsFree() == true);
		CHECK(entry2->get_size() == (entry->get_size() - 18));
	}

}


SUITE(ContainersHashTablePage)
{
	TEST_FIXTURE(SessionFixture, TestInsert1)
	{
		char*         key1 = (char*) "key1";
		uint64_t      val;
		Page<Session> page;

		CHECK(page.Insert(session, key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
	}

	TEST_FIXTURE(SessionFixture, TestInsert2)
	{
		char*         key1 = (char*) "key1";
		char*         key2 = (char*) "key2";
		char*         key3 = (char*) "key3";
		uint64_t      val;
		Page<Session> page;

		CHECK(page.Insert(session, key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(session, key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(session, key3, strlen(key3)+1, 0xFB1) == 0);
		CHECK(page.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(session, key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
	}

	TEST_FIXTURE(SessionFixture, TestInsert3)
	{
		char*         key1 = (char*) "key1";
		char*         key2 = (char*) "key2";
		char          longkey3[PAGE_SIZE];
		int           longkey3_size = PAGE_SIZE - 18;
		uint64_t      val;
		Page<Session> page;

		CHECK(page.Insert(session, longkey3, longkey3_size, 0xFB1) == 0);
		CHECK(page.Search(session, longkey3, longkey3_size, &val) == 0);
		CHECK(val == 0xFB1);
	}


	TEST_FIXTURE(SessionFixture, TestInsert4)
	{
		char*         key1 = (char*) "key1";
		char*         key2 = (char*) "key2";
		char          longkey3[PAGE_SIZE];
		int           longkey3_size = PAGE_SIZE - 18;
		uint64_t      val;
		Page<Session> page;

		CHECK(page.Insert(session, key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(session, key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(session, longkey3, longkey3_size, 0xFB1) < 0);
		CHECK(page.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(session, key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(session, longkey3, longkey3_size, &val) < 0);
	}


	TEST_FIXTURE(SessionFixture, TestInsert5)
	{
		char*         key1 = (char*) "key1";
		char*         key2 = (char*) "key2";
		char          longkey3[PAGE_SIZE];
		int           longkey3_size = PAGE_SIZE - 18;
		uint64_t      val;
		Page<Session> page;

		CHECK(page.Insert(session, key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(session, key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(session, longkey3, longkey3_size, 0xFB1) < 0);
		CHECK(page.Search(session, longkey3, longkey3_size, &val) < 0);
		CHECK(page.Delete(session, key1, strlen(key1)+1) == 0);
		CHECK(page.Delete(session, key2, strlen(key2)+1) == 0);
		CHECK(page.Insert(session, longkey3, longkey3_size, 0xFB1) == 0);
		CHECK(page.Search(session, longkey3, longkey3_size, &val) == 0);
	}


	TEST_FIXTURE(SessionFixture, TestInsert6)
	{
		char*         key1 = (char*) "key1";
		char*         key2 = (char*) "key2";
		char*         key3 = (char*) "key3";
		char*         key4 = (char*) "key3";
		int           key1_size = (PAGE_SIZE-5*sizeof(uint64_t))/3;
		int           key2_size = (PAGE_SIZE-5*sizeof(uint64_t))/3;
		int           key3_size = (PAGE_SIZE-5*sizeof(uint64_t))/3;
		int           key4_size = 2*key1_size-10;
		uint64_t      val;
		Page<Session> page;

		CHECK(page.Insert(session, key1, key1_size, 0xCAFE) == 0);
		CHECK(page.Insert(session, key2, key2_size, 0xC1A) == 0);
		CHECK(page.Insert(session, key3, key3_size, 0xFB1) == 0);
		CHECK(page.Insert(session, key4, key4_size, 0xBEEF) < 0);
		CHECK(page.Search(session, key1, key1_size, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(session, key2, key2_size, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(session, key3, key3_size, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page.Delete(session, key2, key2_size) == 0);
		CHECK(page.Insert(session, key4, key4_size, 0xBEEF) < 0);
		CHECK(page.Insert(session, key2, key2_size, 0xB00) == 0);
		CHECK(page.Search(session, key2, key2_size, &val) == 0);
		CHECK(val == 0xB00);
		CHECK(page.Delete(session, key2, key2_size) == 0);
		CHECK(page.Insert(session, key4, key4_size, 0xBEEF) < 0);
		CHECK(page.Delete(session, key1, key2_size) == 0);
		CHECK(page.Insert(session, key4, key4_size, 0xBEEF) == 0);
	}



	TEST_FIXTURE(SessionFixture, TestDelete1)
	{
		char*         key1 = (char*) "key1";
		char*         key2 = (char*) "key2";
		char*         key3 = (char*) "key3";
		char*         key4 = (char*) "key4";
		uint64_t      val;
		Page<Session> page;

		CHECK(page.Insert(session, key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(session, key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(session, key3, strlen(key3)+1, 0xFB1) == 0);

		CHECK(page.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(session, key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);

		CHECK(page.Delete(session, key2, strlen(key2)+1) == 0);
		CHECK(page.Search(session, key2, strlen(key2)+1, &val) < 0);

		CHECK(page.Insert(session, key4, strlen(key4)+1, 0x18ACF9) == 0);

		CHECK(page.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(session, key2, strlen(key2)+1, &val) < 0);
		CHECK(page.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page.Search(session, key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0x18ACF9);
	}


	TEST_FIXTURE(SessionFixture, TestSplitHalf1)
	{
		char*         key1 = (char*) "key1_xxxx_";
		char*         key2 = (char*) "key2_xxxx_";
		char*         key3 = (char*) "key3_xxxx_";
		char*         key4 = (char*) "key4_xxxx_";
		uint64_t      val;
		Page<Session> page;
		Page<Session> newpage;

		CHECK(page.Insert(session, key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(session, key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(session, key3, strlen(key3)+1, 0xFB1) == 0);
		CHECK(page.Insert(session, key4, strlen(key4)+1, 0xFAFA) == 0);

		CHECK(page.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(session, key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page.Search(session, key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);

		CHECK(page.SplitHalf(session, &newpage) == 0);

		CHECK(page.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(session, key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(session, key3, strlen(key3)+1, &val) != 0);
		CHECK(page.Search(session, key4, strlen(key4)+1, &val) != 0);

		CHECK(newpage.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(newpage.Search(session, key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
	}


	TEST_FIXTURE(SessionFixture, TestSplitHalf2)
	{
		char*         key1 = (char*) "key1_xxxx_";
		char*         key2 = (char*) "key2_xxxx_";
		char*         key3 = (char*) "key3_xxxx_";
		char*         key4 = (char*) "key4_xxxx_";
		char*         key5 = (char*) "key5_xxxx_";
		uint64_t      val;
		Page<Session> page;
		Page<Session> newpage;

		CHECK(page.Insert(session, key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page.Insert(session, key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page.Insert(session, key3, strlen(key3)+1, 0xFB1) == 0);
		CHECK(page.Insert(session, key4, strlen(key4)+1, 0xFAFA) == 0);
		CHECK(page.Insert(session, key5, strlen(key5)+1, 0xBEEF) == 0);

		CHECK(page.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(session, key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page.Search(session, key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(page.Search(session, key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);

		CHECK(page.Delete(session, key2, strlen(key2)+1) == 0);

		CHECK(page.SplitHalf(session, &newpage) == 0);

		CHECK(page.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page.Search(session, key2, strlen(key2)+1, &val) != 0);
		CHECK(page.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(page.Search(session, key4, strlen(key4)+1, &val) != 0);
		CHECK(page.Search(session, key5, strlen(key5)+1, &val) != 0);

		CHECK(newpage.Search(session, key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(newpage.Search(session, key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);
	}


	TEST_FIXTURE(SessionFixture, TestMerge1)
	{
		char*         key1 = (char*) "key1_xxxx_";
		char*         key2 = (char*) "key2_xxxx_";
		char*         key3 = (char*) "key3_xxxx_";
		char*         key4 = (char*) "key4_xxxx_";
		char*         key5 = (char*) "key5_xxxx_";
		uint64_t      val;
		Page<Session> page1;
		Page<Session> page2;

		CHECK(page1.Insert(session, key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page1.Insert(session, key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page1.Insert(session, key3, strlen(key3)+1, 0xFB1) == 0);
		CHECK(page2.Insert(session, key4, strlen(key4)+1, 0xFAFA) == 0);
		CHECK(page2.Insert(session, key5, strlen(key5)+1, 0xBEEF) == 0);

		CHECK(page1.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page1.Search(session, key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page1.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page2.Search(session, key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(page2.Search(session, key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);

		CHECK(page1.Merge(session, &page2) == 0);

		CHECK(page1.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page1.Search(session, key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page1.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page1.Search(session, key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(page1.Search(session, key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);
	}


	TEST_FIXTURE(SessionFixture, TestMergeOverflow)
	{
		char*         key1 = (char*) "key1_0000_1111_2222";
		char*         key2 = (char*) "key2_0000_1111_2222";
		char*         key3 = (char*) "key3_0000_1111_2222";
		char*         key4 = (char*) "key4_0000_1111_2222";
		char*         key5 = (char*) "key5_0000_1111_2222";
		uint64_t      val;
		Page<Session> page1;
		Page<Session> page2;

		CHECK(page1.Insert(session, key1, strlen(key1)+1, 0xCAFE) == 0);
		CHECK(page1.Insert(session, key2, strlen(key2)+1, 0xC1A) == 0);
		CHECK(page1.Insert(session, key3, strlen(key3)+1, 0xFB1) == 0);
		CHECK(page2.Insert(session, key4, strlen(key4)+1, 0xFAFA) == 0);
		CHECK(page2.Insert(session, key5, strlen(key5)+1, 0xBEEF) == 0);

		CHECK(page1.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page1.Search(session, key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page1.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page2.Search(session, key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(page2.Search(session, key5, strlen(key5)+1, &val) == 0);
		CHECK(val == 0xBEEF);

		CHECK(page1.Merge(session, &page2) == -E_NOMEM);

		CHECK(page1.Search(session, key1, strlen(key1)+1, &val) == 0);
		CHECK(val == 0xCAFE);
		CHECK(page1.Search(session, key2, strlen(key2)+1, &val) == 0);
		CHECK(val == 0xC1A);
		CHECK(page1.Search(session, key3, strlen(key3)+1, &val) == 0);
		CHECK(val == 0xFB1);
		CHECK(page1.Search(session, key4, strlen(key4)+1, &val) == 0);
		CHECK(val == 0xFAFA);
		CHECK(page1.Search(session, key5, strlen(key5)+1, &val) != 0);
		CHECK(page2.Search(session, key5, strlen(key5)+1, &val) == 0);
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

class TestSession {

};

SUITE(ContainersHashTable)
{
	TEST_FIXTURE(SessionFixture, TestInsert)
	{
		int                                                        i;
		HashTable<Session>                                         ht;
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
			CHECK(ht.Insert(session, key.c_str(), strlen(key.c_str())+1, val) == 0);
		}

		for (it=kvmap.begin(); it != kvmap.end(); it++) {
			key = (*it).first;
			CHECK(ht.Search(session, key.c_str(), strlen(key.c_str())+1, &val) == 0);
			CHECK(val == (*it).second);
		}

	}
	
	struct triple {
		triple(std::string key, uint64_t val, bool exists)
			: key_(key),
			  val_(val),
			  exists_(exists)
		{ }

		std::string key_;
		uint64_t    val_;
		bool        exists_;
	};

	TEST_FIXTURE(SessionFixture, TestDelete)
	{
		int                                i;
		HashTable<Session>                 ht;
		std::vector<triple>                kvvec;
		std::vector<int>                   kvvec_todel;
		std::vector<triple>::iterator      it;
		std::vector<int>::iterator         it_vecint;
		std::string                        key;
		uint64_t                           val;
		int                                rv;

		srand(0);

		for (i=0; i<128; i++) {
			key=gen_rand_str(16, 16);
			kvvec.push_back(triple(key, i, true));
		}	

		for (it=kvvec.begin(); it != kvvec.end(); it++) {
			if ((*it).exists_) {
				key = (*it).key_;
				val = (*it).val_;
				CHECK(ht.Insert(session, key.c_str(), strlen(key.c_str())+1, val) == 0);
			}
		}

		kvvec_todel.push_back(1);
		kvvec_todel.push_back(6);
		kvvec_todel.push_back(17);
		kvvec_todel.push_back(53);
		kvvec_todel.push_back(112);
		for (it_vecint=kvvec_todel.begin(); it_vecint != kvvec_todel.end(); it_vecint++) {
			key = kvvec[(*it_vecint)].key_;
			kvvec[(*it_vecint)].exists_ = false;
			CHECK(ht.Delete(session, key.c_str(), strlen(key.c_str())+1) == 0);
		}

		for (it=kvvec.begin(); it != kvvec.end(); it++) {
			if ((*it).exists_) {
				key = (*it).key_;
				CHECK(ht.Search(session, key.c_str(), strlen(key.c_str())+1, &val) == 0);
				CHECK(val == (*it).val_);
			}
		}

	}
}
