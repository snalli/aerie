#include "osd/containers/map/openhashtable.h"
#include "common/errno.h"
#include <gtest/gtest.h>
#include "fixture/session.fixture.h"
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

struct Key
{
    Key()
    {
        c[0] = '\0';
    }

    Key(const char* str)
    {
        strcpy(c, str);
    }

    char c[32];

    bool operator==(const Key& other)
    {
        return strcmp(c, other.c) == 0;
    }
};

class Traits
{
  public:
    static uint hash(const char* key)
    {
        uint hash = 2166136261;
        for (const char* s = key; *s; s++)
            hash = (16777619 * hash) ^ (*s);
        return hash;
    };

    static uint hash(const Key& key)
    {
        uint hash = 2166136261;
        for (const char* s = key.c; *s; s++)
            hash = (16777619 * hash) ^ (*s);
        return hash;
    };

    static bool isEmpty(Key& key)
    {
        return (key.c[0] == '\0') ? true : false;
    }
};

static std::string gen_rand_str(const int minlen, const int maxlen)
{
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";
    int len;
    char s[128];

    assert(maxlen < 128);

    len = minlen + rand() % (maxlen - minlen + 1);

    for (int i = 0; i < len; ++i)
    {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;

    return std::string(s);
}

typedef OpenHashTable<Session, Key, uint64_t, Traits> OpenHT;

// Suite: ContainersOpenHashTable
    TEST_F(SessionFixture, TestInsert)
    {
        int i;
        OpenHT ht(0.8);
        std::map<std::string, uint64_t> kvmap;
        std::map<std::string, uint64_t>::iterator it;
        std::pair<std::map<std::string, uint64_t>::iterator, bool> ret;
        Key key;
        std::string key_str;
        uint64_t val;
        int rv;

        srand(0);

        for (i = 0; i < 128; i++)
        {
            key_str = gen_rand_str(16, 16);
            kvmap.insert(std::pair<std::string, uint64_t>(key_str, i));
        }
        for (it = kvmap.begin(); it != kvmap.end(); it++)
        {
            key = Key(((*it).first).c_str());
            val = (*it).second;
            EXPECT_TRUE(ht.Insert(session, key, val) == 0);
        }
        for (it = kvmap.begin(); it != kvmap.end(); it++)
        {
            key = Key(((*it).first).c_str());
            EXPECT_TRUE(ht.Lookup(session, key, &val) == 0);
            EXPECT_TRUE(val == (*it).second);
        }
    }

    struct triple
    {
        triple(std::string key, uint64_t val, bool exists) : key_(key), val_(val), exists_(exists)
        {
        }

        std::string key_;
        uint64_t val_;
        bool exists_;
    };

    TEST_F(SessionFixture, TestDelete)
    {
        int i;
        OpenHT ht(0.8);
        std::vector<triple> kvvec;
        std::vector<int> kvvec_todel;
        std::vector<triple>::iterator it;
        std::vector<int>::iterator it_vecint;
        Key key;
        std::string key_str;
        uint64_t val;
        int rv;

        srand(0);

        for (i = 0; i < 128; i++)
        {
            key_str = gen_rand_str(16, 16);
            kvvec.push_back(triple(key_str, i, true));
        }

        for (it = kvvec.begin(); it != kvvec.end(); it++)
        {
            if ((*it).exists_)
            {
                key = Key(((*it).key_).c_str());
                val = (*it).val_;
                EXPECT_TRUE(ht.Insert(session, key, val) == 0);
            }
        }

        kvvec_todel.push_back(1);
        kvvec_todel.push_back(6);
        kvvec_todel.push_back(17);
        kvvec_todel.push_back(53);
        kvvec_todel.push_back(112);
#if 0
		for (it_vecint=kvvec_todel.begin(); it_vecint != kvvec_todel.end(); it_vecint++) {
			key = Key(kvvec[(*it_vecint)].key_.c_str());
			kvvec[(*it_vecint)].exists_ = false;
			EXPECT_TRUE(ht.Remove(session, key) == 0);
		}

		for (it=kvvec.begin(); it != kvvec.end(); it++) {
			key = Key(((*it).key_).c_str());
			if ((*it).exists_) {
				EXPECT_TRUE(ht.Lookup(session, key, &val) == 0);
				EXPECT_TRUE(val == (*it).val_);
			} else {
				EXPECT_TRUE(ht.Lookup(session, key, &val) != 0);
			}
		}
#endif
    }
