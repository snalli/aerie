#include <set>
#include "common/lock_protocol.h"
#include "tool/testfw/unittest.h"

SUITE(LockProtocolMode)
{
	TEST(TestAssignment)
	{
		lock_protocol::Mode mode0 = lock_protocol::Mode::NL;
		lock_protocol::Mode mode1 = lock_protocol::Mode::XL;
		lock_protocol::Mode mode2;
		lock_protocol::Mode mode3;

		mode2 = mode0;
		mode3 = mode1;

		CHECK(mode0 == mode2);
		CHECK(mode1 == mode3);

	}

	TEST(TestEquality1)
	{
		lock_protocol::Mode mode0 = lock_protocol::Mode::NL;
		lock_protocol::Mode mode1 = lock_protocol::Mode::SL;
		lock_protocol::Mode mode2 = lock_protocol::Mode::SL;
		lock_protocol::Mode mode3 = lock_protocol::Mode::XL;

       	CHECK(mode0 != mode1);
       	CHECK(mode1 == mode2);
       	CHECK(mode2 != mode3);
       	CHECK(mode3 == mode3);
	}

	TEST(TestSuccessor1)
	{
		CHECK(lock_protocol::Mode(lock_protocol::Mode::XR) == lock_protocol::Mode(lock_protocol::Mode::XL).Successor());
		CHECK(lock_protocol::Mode(lock_protocol::Mode::XR) == lock_protocol::Mode::Successor(lock_protocol::Mode::XL));
	}

	TEST(TestPartialOrder1)
	{
		CHECK(lock_protocol::Mode::PartialOrder(lock_protocol::Mode::XR, lock_protocol::Mode::XL) > 0 );
		CHECK(lock_protocol::Mode::PartialOrder(lock_protocol::Mode::IS, lock_protocol::Mode::XL) < 0);
		CHECK(lock_protocol::Mode::PartialOrder(lock_protocol::Mode::IXSL, lock_protocol::Mode::XL) < 0);
		
		CHECK(lock_protocol::Mode(lock_protocol::Mode::IXSL) < lock_protocol::Mode(lock_protocol::Mode::XR));
		CHECK(lock_protocol::Mode(lock_protocol::Mode::IX) < lock_protocol::Mode(lock_protocol::Mode::IXSL));
		CHECK(lock_protocol::Mode(lock_protocol::Mode::XL) > lock_protocol::Mode(lock_protocol::Mode::SL));
	}

	TEST(TestSupremum1)
	{
		CHECK(lock_protocol::Mode::Supremum(lock_protocol::Mode(lock_protocol::Mode::IXSL), lock_protocol::Mode(lock_protocol::Mode::XL)) 
		      == lock_protocol::Mode::XL);
	}

	TEST(TestSupremum2)
	{
		CHECK(lock_protocol::Mode::Supremum(lock_protocol::Mode(lock_protocol::Mode::IXSL), lock_protocol::Mode(lock_protocol::Mode::SL)) 
		      == lock_protocol::Mode::IXSL);
	}

	TEST(TestSet)
	{
		CHECK(lock_protocol::Mode::Set::NL == lock_protocol::Mode::Set(lock_protocol::Mode::NL).value());
		CHECK(lock_protocol::Mode::Set::SL != lock_protocol::Mode::Set(lock_protocol::Mode::NL).value());
		CHECK(lock_protocol::Mode::Set::XL == lock_protocol::Mode::Set(lock_protocol::Mode::XL).value());
		CHECK((lock_protocol::Mode::XL | lock_protocol::Mode::SL) == 
		      (lock_protocol::Mode::Set::XL | lock_protocol::Mode::Set::SL));
		CHECK((lock_protocol::Mode::Set(lock_protocol::Mode::XL) | lock_protocol::Mode::Set(lock_protocol::Mode::SL)).value() == 
		      (lock_protocol::Mode::Set::XL | lock_protocol::Mode::Set::SL));
	}

	TEST(TestSetIterator1)
	{
		lock_protocol::Mode::Set            mode_set;
		lock_protocol::Mode::Set::Iterator  itr;
		std::set<lock_protocol::Mode::Enum> stl_set;

		for (itr = mode_set.begin(); itr != mode_set.end(); itr++) {
			CHECK(stl_set.erase(static_cast<lock_protocol::Mode::Enum>((*itr).value())) == 1);
		}

		mode_set.Insert(lock_protocol::Mode::NL);
		mode_set.Insert(lock_protocol::Mode::SL);
		stl_set.insert(lock_protocol::Mode::NL);
		stl_set.insert(lock_protocol::Mode::SL);

		for (itr = mode_set.begin(); itr != mode_set.end(); itr++) {
			CHECK(stl_set.erase(static_cast<lock_protocol::Mode::Enum>((*itr).value())) == 1);
		}
		CHECK(stl_set.size() == 0); // for-loop removed all entries
	}

	TEST(TestSetIterator2)
	{
		lock_protocol::Mode::Set            mode_set;
		lock_protocol::Mode::Set::Iterator  itr;
		std::set<lock_protocol::Mode::Enum> stl_set;

		mode_set.Insert(lock_protocol::Mode::XL);
		mode_set.Insert(lock_protocol::Mode::SL);
		stl_set.insert(lock_protocol::Mode::XL);
		stl_set.insert(lock_protocol::Mode::SL);

		for (itr = mode_set.begin(); itr != mode_set.end(); itr++) {
			CHECK(stl_set.erase(static_cast<lock_protocol::Mode::Enum>((*itr).value())) == 1);
		}
		CHECK(stl_set.size() == 0); // for-loop removed all entries
	}

	TEST(TestSetIterator3)
	{
		lock_protocol::Mode::Set            mode_set;
		lock_protocol::Mode::Set::Iterator  itr;
		std::set<lock_protocol::Mode::Enum> stl_set;

		mode_set.Insert(lock_protocol::Mode::IXSL);
		stl_set.insert(lock_protocol::Mode::IXSL);

		for (itr = mode_set.begin(); itr != mode_set.end(); itr++) {
			CHECK(stl_set.erase(static_cast<lock_protocol::Mode::Enum>((*itr).value())) == 1);
		}
		CHECK(stl_set.size() == 0); // for-loop removed all entries
	}

	TEST(TestMostSevere)
	{
		lock_protocol::Mode::Set   mode_set;

		mode_set.Insert(lock_protocol::Mode::IXSL);
		CHECK(mode_set.Exists(lock_protocol::Mode::IXSL));
		mode_set.Insert(lock_protocol::Mode::XR);
		CHECK(mode_set.Exists(lock_protocol::Mode::XR));
		CHECK(mode_set.MostSevere(lock_protocol::Mode::NL) == lock_protocol::Mode(lock_protocol::Mode::XR));
		CHECK(mode_set.MostSevere(lock_protocol::Mode::IX) == lock_protocol::Mode(lock_protocol::Mode::IXSL));
		CHECK(mode_set.MostSevere(lock_protocol::Mode::SR) == lock_protocol::Mode(lock_protocol::Mode::NL));
		CHECK(mode_set.MostSevere(lock_protocol::Mode::SL) == lock_protocol::Mode(lock_protocol::Mode::IXSL));
		
		mode_set.Clear();
		CHECK(mode_set.MostSevere(lock_protocol::Mode::NL) == lock_protocol::Mode(lock_protocol::Mode::NL));
		mode_set.Insert(lock_protocol::Mode::XR);
		CHECK(mode_set.Exists(lock_protocol::Mode::XR));
		CHECK(mode_set.MostSevere(lock_protocol::Mode::NL) == lock_protocol::Mode(lock_protocol::Mode::XR));
		
		mode_set.Clear();
		mode_set.Insert(lock_protocol::Mode::SL);
		CHECK(mode_set.Exists(lock_protocol::Mode::SL));
		mode_set.Insert(lock_protocol::Mode::IX);
		CHECK(mode_set.Exists(lock_protocol::Mode::IX));
		mode_set.Insert(lock_protocol::Mode::XR);
		CHECK(mode_set.Exists(lock_protocol::Mode::XR));
		CHECK(mode_set.MostSevere(lock_protocol::Mode::NL) == lock_protocol::Mode(lock_protocol::Mode::XR));
		CHECK(mode_set.MostSevere(lock_protocol::Mode::SL) == lock_protocol::Mode(lock_protocol::Mode::SL));
		CHECK(mode_set.MostSevere(lock_protocol::Mode::SR) == lock_protocol::Mode(lock_protocol::Mode::SL));
	}
}
