#include "common/bitset.h"
#include "tool/testfw/unittest.h"

// bitpos contains the positions of the bits that are set
bool CompareBitSet(DynamicBitSet* bset, std::set<int> bitpos)
{
	std::vector<int>::iterator iter;
	
	for (int i=0; i<bset->Size(); i++) {
		if (bset->IsSet(i) && bitpos.find(i) == bitpos.end()) {
			return false;
		} else {
			bitpos.erase(i);
		}
	}
	return bitpos.size() == 0;
}

SUITE(BitSet)
{
	TEST(TestSet)
	{
		char           buf[512];
		std::set<int>  bitpos;
		DynamicBitSet* bset = DynamicBitSet::Make(buf, 64);
		
		bset->Set(1);
		bitpos.clear();
		bitpos.insert(1);
		CHECK(CompareBitSet(bset, bitpos) == true);
	
		bset->Reset();
		bset->Set(0);
		bset->Set(63);
		bitpos.clear();
		bitpos.insert(0);
		bitpos.insert(63);
		CHECK(CompareBitSet(bset, bitpos) == true);

		bset->Reset();
		bset->Set(0);
		bset->Set(7);
		bset->Set(63);
		bitpos.clear();
		bitpos.insert(0);
		bitpos.insert(7);
		bitpos.insert(63);
		CHECK(CompareBitSet(bset, bitpos) == true);
	}

	TEST(TestOperatorIndex)
	{
		char           buf[512];
		std::set<int>  bitpos;
		DynamicBitSet* bset = DynamicBitSet::Make(buf, 64);
		
		(*bset)[4] = true;
		CHECK(bset->IsSet(4) == true);
		CHECK((*bset)[4] == true);
	
		bitpos.clear();
		bitpos.insert(4);
		CHECK(CompareBitSet(bset, bitpos) == true);
	}

}
