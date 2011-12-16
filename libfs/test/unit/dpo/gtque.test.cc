#include <pthread.h>
#include "dpo/base/common/lock_protocol.h"
#include "dpo/base/common/gtque.h"
#include "tool/testfw/unittest.h"


class MemberRecord {
public:
	typedef pthread_t            id_t;
	typedef lock_protocol::Mode  Mode;

	MemberRecord()
		: tid_(-1), 
		  mode_(lock_protocol::Mode(lock_protocol::Mode::NL))
	{ }

	MemberRecord(id_t tid, Mode mode)
		: tid_(tid), 
		  mode_(mode)
	{ }

	id_t id() const { return tid_; };
	Mode mode() const { return mode_; };
	void set_mode(Mode mode) { mode_ = mode; };

private:
	id_t tid_;
	Mode mode_;
};


SUITE(GrantQueue)
{
	struct CreateQueueFixture 
	{
		CreateQueueFixture() 
			: gtque(lock_protocol::Mode::CARDINALITY, lock_protocol::Mode::NL)
		{ }
		
		GrantQueue<MemberRecord> gtque;

	};

	TEST_FIXTURE(CreateQueueFixture, AddRemove)
	{
		MemberRecord mr(1, lock_protocol::Mode::XL);

		gtque.Add(mr);
		CHECK(gtque.Exists(1) == true);
		CHECK(gtque.Remove(1) == 0);
		CHECK(gtque.Exists(1) == false);
	}

	TEST_FIXTURE(CreateQueueFixture, CanGrant1)
	{
		MemberRecord mr1(1, lock_protocol::Mode::XL);
		MemberRecord mr2(2, lock_protocol::Mode::XL);

		gtque.Add(mr1);
		CHECK(gtque.CanGrant(mr2.mode()) == false);
	}

	TEST_FIXTURE(CreateQueueFixture, CanGrant2)
	{
		MemberRecord mr1(1, lock_protocol::Mode::XL);
		MemberRecord mr2(2, lock_protocol::Mode::XL);
		MemberRecord mr3(3, lock_protocol::Mode::SL);
		MemberRecord mr4(4, lock_protocol::Mode::SL);

		CHECK(gtque.Grant(mr1) == 0);
		CHECK(gtque.CanGrant(mr2.mode()) == false);
		CHECK(gtque.Remove(mr1.id()) == 0);
		CHECK(gtque.Grant(mr3) == 0);
		CHECK(gtque.Grant(mr4) == 0);
		CHECK(gtque.Remove(mr3.id()) == 0);
		CHECK(gtque.Grant(mr2) < 0);
		CHECK(gtque.Remove(mr4.id()) == 0);
		CHECK(gtque.Grant(mr2) == 0);
		CHECK(gtque.Exists(2) == true);
		CHECK(gtque.Grant(mr1) < 0);
	}

	TEST_FIXTURE(CreateQueueFixture, CanGrant3)
	{
		MemberRecord mr1(1, lock_protocol::Mode::IX);
		MemberRecord mr2(2, lock_protocol::Mode::XL);
		MemberRecord mr3(3, lock_protocol::Mode::SL);
		MemberRecord mr4(4, lock_protocol::Mode::SL);
		MemberRecord mr5(5, lock_protocol::Mode::IS);
		MemberRecord mr6(6, lock_protocol::Mode::SR);
		MemberRecord mr7(7, lock_protocol::Mode::IXSL);

		CHECK(gtque.Grant(mr1) == 0);
		CHECK(gtque.Grant(mr3) == 0);
		CHECK(gtque.Grant(mr4) == 0);
		CHECK(gtque.Grant(mr2) < 0);
		CHECK(gtque.Grant(mr5) == 0);
		CHECK(gtque.Grant(mr6) < 0);
		CHECK(gtque.Grant(mr7) == 0);

		CHECK(gtque.Remove(1) == 0);
		CHECK(gtque.Remove(7) == 0);
		CHECK(gtque.Grant(mr6) == 0);

		CHECK(gtque.Remove(3) == 0);
		CHECK(gtque.Remove(4) == 0);

		CHECK(gtque.Exists(1) == false);
		CHECK(gtque.Exists(2) == false);
		CHECK(gtque.Exists(3) == false);
		CHECK(gtque.Exists(4) == false);
		CHECK(gtque.Exists(5) == true);
		CHECK(gtque.Exists(6) == true);
		CHECK(gtque.Exists(7) == false);

		CHECK(gtque.Remove(5) == 0);
		CHECK(gtque.Remove(6) == 0);
		CHECK(gtque.Exists(5) == false);
		CHECK(gtque.Exists(6) == false);

		CHECK(gtque.Grant(mr2) == 0);
		CHECK(gtque.Grant(mr3) < 0);
	}

	TEST_FIXTURE(CreateQueueFixture, ConvertInPlace)
	{
		MemberRecord mr1(1, lock_protocol::Mode::IX);
		MemberRecord mr2(2, lock_protocol::Mode::XL);
		MemberRecord mr3(3, lock_protocol::Mode::SL);
		MemberRecord mr4(4, lock_protocol::Mode::SL);
		MemberRecord mr5(5, lock_protocol::Mode::IS);
		MemberRecord mr6(6, lock_protocol::Mode::SR);
		MemberRecord mr7(7, lock_protocol::Mode::IXSL);

		CHECK(gtque.Grant(mr1) == 0);
		CHECK(gtque.Grant(mr3) == 0);
		CHECK(gtque.ConvertInPlace(1, lock_protocol::Mode::IXSL) == 0);
		CHECK(gtque.ConvertInPlace(3, lock_protocol::Mode::XR) < 0);
	}
	

	TEST_FIXTURE(CreateQueueFixture, PartialOrder)
	{
		MemberRecord mr1(1, lock_protocol::Mode::IS);
		MemberRecord mr2(2, lock_protocol::Mode::SL);
		MemberRecord mr3(3, lock_protocol::Mode::SR);
		MemberRecord mr4(4, lock_protocol::Mode::IX);
		MemberRecord mr5(5, lock_protocol::Mode::XL);
		MemberRecord mr6(6, lock_protocol::Mode::XR);
		MemberRecord mr7(7, lock_protocol::Mode::IXSL);

		CHECK(gtque.Grant(mr1) == 0);
		CHECK(gtque.Grant(mr3) == 0);
		CHECK(gtque.PartialOrder(lock_protocol::Mode::SL) == 0);
		CHECK(gtque.PartialOrder(lock_protocol::Mode::NL) < 0);
		CHECK(gtque.PartialOrder(lock_protocol::Mode::XR) > 0);
		CHECK(gtque.PartialOrder(lock_protocol::Mode::IX) == 0);
	}
	
}
