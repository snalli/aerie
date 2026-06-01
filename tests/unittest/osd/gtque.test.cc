#include "osd/main/common/gtque.h"
#include "osd/main/common/lock_protocol.h"
#include <gtest/gtest.h>
#include <pthread.h>

class MemberRecord
{
  public:
    typedef pthread_t id_t;
    typedef lock_protocol::Mode Mode;

    MemberRecord() : tid_(-1), mode_(lock_protocol::Mode(lock_protocol::Mode::NL))
    {
    }

    MemberRecord(id_t tid, Mode mode) : tid_(tid), mode_(mode)
    {
    }

    id_t id() const
    {
        return tid_;
    };
    Mode mode() const
    {
        return mode_;
    };
    void set_mode(Mode mode)
    {
        mode_ = mode;
    };

  private:
    id_t tid_;
    Mode mode_;
};

// Suite: GrantQueue
    struct CreateQueueFixture
    {
        CreateQueueFixture() : gtque(lock_protocol::Mode::CARDINALITY, lock_protocol::Mode::NL)
        {
        }

        GrantQueue<MemberRecord> gtque;
    };

    TEST_F(CreateQueueFixture, AddRemove)
    {
        MemberRecord mr(1, lock_protocol::Mode::XL);

        gtque.Add(mr);
        EXPECT_TRUE(gtque.Exists(1) == true);
        EXPECT_TRUE(gtque.Remove(1) == 0);
        EXPECT_TRUE(gtque.Exists(1) == false);
    }

    TEST_F(CreateQueueFixture, CanGrant1)
    {
        MemberRecord mr1(1, lock_protocol::Mode::XL);
        MemberRecord mr2(2, lock_protocol::Mode::XL);

        gtque.Add(mr1);
        EXPECT_TRUE(gtque.CanGrant(mr2.mode()) == false);
    }

    TEST_F(CreateQueueFixture, CanGrant2)
    {
        MemberRecord mr1(1, lock_protocol::Mode::XL);
        MemberRecord mr2(2, lock_protocol::Mode::XL);
        MemberRecord mr3(3, lock_protocol::Mode::SL);
        MemberRecord mr4(4, lock_protocol::Mode::SL);

        EXPECT_TRUE(gtque.Grant(mr1) == 0);
        EXPECT_TRUE(gtque.CanGrant(mr2.mode()) == false);
        EXPECT_TRUE(gtque.Remove(mr1.id()) == 0);
        EXPECT_TRUE(gtque.Grant(mr3) == 0);
        EXPECT_TRUE(gtque.Grant(mr4) == 0);
        EXPECT_TRUE(gtque.Remove(mr3.id()) == 0);
        EXPECT_TRUE(gtque.Grant(mr2) < 0);
        EXPECT_TRUE(gtque.Remove(mr4.id()) == 0);
        EXPECT_TRUE(gtque.Grant(mr2) == 0);
        EXPECT_TRUE(gtque.Exists(2) == true);
        EXPECT_TRUE(gtque.Grant(mr1) < 0);
    }

    TEST_F(CreateQueueFixture, CanGrant3)
    {
        MemberRecord mr1(1, lock_protocol::Mode::IX);
        MemberRecord mr2(2, lock_protocol::Mode::XL);
        MemberRecord mr3(3, lock_protocol::Mode::SL);
        MemberRecord mr4(4, lock_protocol::Mode::SL);
        MemberRecord mr5(5, lock_protocol::Mode::IS);
        MemberRecord mr6(6, lock_protocol::Mode::SR);
        MemberRecord mr7(7, lock_protocol::Mode::IXSL);

        EXPECT_TRUE(gtque.Grant(mr1) == 0);
        EXPECT_TRUE(gtque.Grant(mr3) == 0);
        EXPECT_TRUE(gtque.Grant(mr4) == 0);
        EXPECT_TRUE(gtque.Grant(mr2) < 0);
        EXPECT_TRUE(gtque.Grant(mr5) == 0);
        EXPECT_TRUE(gtque.Grant(mr6) < 0);
        EXPECT_TRUE(gtque.Grant(mr7) == 0);

        EXPECT_TRUE(gtque.Remove(1) == 0);
        EXPECT_TRUE(gtque.Remove(7) == 0);
        EXPECT_TRUE(gtque.Grant(mr6) == 0);

        EXPECT_TRUE(gtque.Remove(3) == 0);
        EXPECT_TRUE(gtque.Remove(4) == 0);

        EXPECT_TRUE(gtque.Exists(1) == false);
        EXPECT_TRUE(gtque.Exists(2) == false);
        EXPECT_TRUE(gtque.Exists(3) == false);
        EXPECT_TRUE(gtque.Exists(4) == false);
        EXPECT_TRUE(gtque.Exists(5) == true);
        EXPECT_TRUE(gtque.Exists(6) == true);
        EXPECT_TRUE(gtque.Exists(7) == false);

        EXPECT_TRUE(gtque.Remove(5) == 0);
        EXPECT_TRUE(gtque.Remove(6) == 0);
        EXPECT_TRUE(gtque.Exists(5) == false);
        EXPECT_TRUE(gtque.Exists(6) == false);

        EXPECT_TRUE(gtque.Grant(mr2) == 0);
        EXPECT_TRUE(gtque.Grant(mr3) < 0);
    }

    TEST_F(CreateQueueFixture, ConvertInPlace)
    {
        MemberRecord mr1(1, lock_protocol::Mode::IX);
        MemberRecord mr2(2, lock_protocol::Mode::XL);
        MemberRecord mr3(3, lock_protocol::Mode::SL);
        MemberRecord mr4(4, lock_protocol::Mode::SL);
        MemberRecord mr5(5, lock_protocol::Mode::IS);
        MemberRecord mr6(6, lock_protocol::Mode::SR);
        MemberRecord mr7(7, lock_protocol::Mode::IXSL);

        EXPECT_TRUE(gtque.Grant(mr1) == 0);
        EXPECT_TRUE(gtque.Grant(mr3) == 0);
        EXPECT_TRUE(gtque.ConvertInPlace(1, lock_protocol::Mode::IXSL) == 0);
        EXPECT_TRUE(gtque.ConvertInPlace(3, lock_protocol::Mode::XR) < 0);
    }

    TEST_F(CreateQueueFixture, PartialOrder)
    {
        MemberRecord mr1(1, lock_protocol::Mode::IS);
        MemberRecord mr2(2, lock_protocol::Mode::SL);
        MemberRecord mr3(3, lock_protocol::Mode::SR);
        MemberRecord mr4(4, lock_protocol::Mode::IX);
        MemberRecord mr5(5, lock_protocol::Mode::XL);
        MemberRecord mr6(6, lock_protocol::Mode::XR);
        MemberRecord mr7(7, lock_protocol::Mode::IXSL);

        EXPECT_TRUE(gtque.Grant(mr1) == 0);
        EXPECT_TRUE(gtque.Grant(mr3) == 0);
        EXPECT_TRUE(gtque.PartialOrder(lock_protocol::Mode::SL) == 0);
        EXPECT_TRUE(gtque.PartialOrder(lock_protocol::Mode::NL) < 0);
        EXPECT_TRUE(gtque.PartialOrder(lock_protocol::Mode::XR) > 0);
        EXPECT_TRUE(gtque.PartialOrder(lock_protocol::Mode::IX) == 0);
    }
