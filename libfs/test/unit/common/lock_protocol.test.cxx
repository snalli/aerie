#include "common/lock_protocol.h"
#include "tool/testfw/unittest.h"


SUITE(LockProtocolMode)
{
	TEST(TestEquality1)
	{
		lock_protocol::MODE mode0 = lock_protocol::MODE::NL;
		lock_protocol::MODE mode1 = lock_protocol::MODE::SL;
		lock_protocol::MODE mode2 = lock_protocol::MODE::SL;
		lock_protocol::MODE mode3 = lock_protocol::MODE::XL;

       	CHECK(mode0 != mode1);
       	CHECK(mode1 == mode2);
       	CHECK(mode2 != mode3);
       	CHECK(mode3 == mode3);
	}

	TEST(TestSuccessor1)
	{
		CHECK(lock_protocol::MODE(lock_protocol::MODE::XR) == lock_protocol::MODE(lock_protocol::MODE::XL).Successor());
		CHECK(lock_protocol::MODE(lock_protocol::MODE::XR) == lock_protocol::MODE::Successor(lock_protocol::MODE::XL));
	}

	TEST(TestPartialOrder1)
	{
		CHECK(lock_protocol::MODE::PartialOrder(lock_protocol::MODE::XR, lock_protocol::MODE::XL) > 0 );
		CHECK(lock_protocol::MODE::PartialOrder(lock_protocol::MODE::IS, lock_protocol::MODE::XL) < 0);
		CHECK(lock_protocol::MODE::PartialOrder(lock_protocol::MODE::IXSL, lock_protocol::MODE::XL) == 0);
		
		CHECK(lock_protocol::MODE(lock_protocol::MODE::IXSL) < lock_protocol::MODE(lock_protocol::MODE::XR));
		CHECK(lock_protocol::MODE(lock_protocol::MODE::IX) < lock_protocol::MODE(lock_protocol::MODE::IXSL));
		CHECK(lock_protocol::MODE(lock_protocol::MODE::XL) > lock_protocol::MODE(lock_protocol::MODE::SL));
	}

	TEST(TestSupremum1)
	{
		CHECK(lock_protocol::MODE::Supremum(lock_protocol::MODE(lock_protocol::MODE::IXSL), lock_protocol::MODE(lock_protocol::MODE::XL)) 
		      == lock_protocol::MODE::XR);
	}

	TEST(TestBitmap)
	{
		CHECK(lock_protocol::MODE::Bitmap::NL == lock_protocol::MODE::Bitmap(lock_protocol::MODE::NL).value());
		CHECK(lock_protocol::MODE::Bitmap::SL != lock_protocol::MODE::Bitmap(lock_protocol::MODE::NL).value());
		CHECK(lock_protocol::MODE::Bitmap::XL == lock_protocol::MODE::Bitmap(lock_protocol::MODE::XL).value());
		CHECK((lock_protocol::MODE::XL | lock_protocol::MODE::SL) == 
		      (lock_protocol::MODE::Bitmap::XL | lock_protocol::MODE::Bitmap::SL));
		CHECK((lock_protocol::MODE::Bitmap(lock_protocol::MODE::XL) | lock_protocol::MODE::Bitmap(lock_protocol::MODE::SL)).value() == 
		      (lock_protocol::MODE::Bitmap::XL | lock_protocol::MODE::Bitmap::SL));
	}
}
