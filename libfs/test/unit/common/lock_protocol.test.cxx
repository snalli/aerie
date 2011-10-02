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
		CHECK(lock_protocol::Mode::PartialOrder(lock_protocol::Mode::IXSL, lock_protocol::Mode::XL) == 0);
		
		CHECK(lock_protocol::Mode(lock_protocol::Mode::IXSL) < lock_protocol::Mode(lock_protocol::Mode::XR));
		CHECK(lock_protocol::Mode(lock_protocol::Mode::IX) < lock_protocol::Mode(lock_protocol::Mode::IXSL));
		CHECK(lock_protocol::Mode(lock_protocol::Mode::XL) > lock_protocol::Mode(lock_protocol::Mode::SL));
	}

	TEST(TestSupremum1)
	{
		CHECK(lock_protocol::Mode::Supremum(lock_protocol::Mode(lock_protocol::Mode::IXSL), lock_protocol::Mode(lock_protocol::Mode::XL)) 
		      == lock_protocol::Mode::XR);
	}

	TEST(TestBitmap)
	{
		CHECK(lock_protocol::Mode::Bitmap::NL == lock_protocol::Mode::Bitmap(lock_protocol::Mode::NL).value());
		CHECK(lock_protocol::Mode::Bitmap::SL != lock_protocol::Mode::Bitmap(lock_protocol::Mode::NL).value());
		CHECK(lock_protocol::Mode::Bitmap::XL == lock_protocol::Mode::Bitmap(lock_protocol::Mode::XL).value());
		CHECK((lock_protocol::Mode::XL | lock_protocol::Mode::SL) == 
		      (lock_protocol::Mode::Bitmap::XL | lock_protocol::Mode::Bitmap::SL));
		CHECK((lock_protocol::Mode::Bitmap(lock_protocol::Mode::XL) | lock_protocol::Mode::Bitmap(lock_protocol::Mode::SL)).value() == 
		      (lock_protocol::Mode::Bitmap::XL | lock_protocol::Mode::Bitmap::SL));
	}
}
