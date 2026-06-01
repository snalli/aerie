#include "common/bitset.h"
#include <gtest/gtest.h>

// bitpos contains the positions of the bits that are set
bool CompareBitSet(DynamicBitSet* bset, std::set<int> bitpos)
{
    std::vector<int>::iterator iter;

    for (size_t i = 0; i < (size_t)bset->Size(); i++)
    {
        if (bset->IsSet(i) && bitpos.find(i) == bitpos.end())
        {
            return false;
        }
        else
        {
            bitpos.erase(i);
        }
    }
    return bitpos.size() == 0;
}

// Suite: BitSet
    TEST(BitSet, TestSet)
    {
        char buf[512];
        std::set<int> bitpos;
        DynamicBitSet* bset = DynamicBitSet::Make(buf, 64);

        bset->Set(1);
        bitpos.clear();
        bitpos.insert(1);
        EXPECT_TRUE(CompareBitSet(bset, bitpos) == true);

        bset->Reset();
        bset->Set(0);
        bset->Set(63);
        bitpos.clear();
        bitpos.insert(0);
        bitpos.insert(63);
        EXPECT_TRUE(CompareBitSet(bset, bitpos) == true);

        bset->Reset();
        bset->Set(0);
        bset->Set(7);
        bset->Set(63);
        bitpos.clear();
        bitpos.insert(0);
        bitpos.insert(7);
        bitpos.insert(63);
        EXPECT_TRUE(CompareBitSet(bset, bitpos) == true);
    }

    TEST(BitSet, TestOperatorIndex)
    {
        char buf[512];
        std::set<int> bitpos;
        DynamicBitSet* bset = DynamicBitSet::Make(buf, 64);

        (*bset)[4] = true;
        EXPECT_TRUE(bset->IsSet(4) == true);
        EXPECT_TRUE((*bset)[4] == true);

        bitpos.clear();
        bitpos.insert(4);
        EXPECT_TRUE(CompareBitSet(bset, bitpos) == true);
    }
