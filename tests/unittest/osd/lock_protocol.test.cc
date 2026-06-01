#include "osd/main/common/lock_protocol.h"
#include <gtest/gtest.h>
#include <set>

// Suite: LockProtocolMode
    TEST(LockProtocolMode, TestAssignment)
    {
        lock_protocol::Mode mode0 = lock_protocol::Mode::NL;
        lock_protocol::Mode mode1 = lock_protocol::Mode::XL;
        lock_protocol::Mode mode2;
        lock_protocol::Mode mode3;

        mode2 = mode0;
        mode3 = mode1;

        EXPECT_TRUE(mode0 == mode2);
        EXPECT_TRUE(mode1 == mode3);
    }

    TEST(LockProtocolMode, TestEquality1)
    {
        lock_protocol::Mode mode0 = lock_protocol::Mode::NL;
        lock_protocol::Mode mode1 = lock_protocol::Mode::SL;
        lock_protocol::Mode mode2 = lock_protocol::Mode::SL;
        lock_protocol::Mode mode3 = lock_protocol::Mode::XL;

        EXPECT_TRUE(mode0 != mode1);
        EXPECT_TRUE(mode1 == mode2);
        EXPECT_TRUE(mode2 != mode3);
        EXPECT_TRUE(mode3 == mode3);
    }

    TEST(LockProtocolMode, TestSuccessor1)
    {
        EXPECT_TRUE(lock_protocol::Mode(lock_protocol::Mode::XR) ==
              lock_protocol::Mode(lock_protocol::Mode::XL).Successor());
        EXPECT_TRUE(lock_protocol::Mode(lock_protocol::Mode::XR) ==
              lock_protocol::Mode::Successor(lock_protocol::Mode::XL));
    }

    TEST(LockProtocolMode, TestPartialOrder1)
    {
        EXPECT_TRUE(lock_protocol::Mode::PartialOrder(lock_protocol::Mode::XR, lock_protocol::Mode::XL) >
              0);
        EXPECT_TRUE(lock_protocol::Mode::PartialOrder(lock_protocol::Mode::IS, lock_protocol::Mode::XL) <
              0);
        EXPECT_TRUE(lock_protocol::Mode::PartialOrder(lock_protocol::Mode::IXSL,
                                                lock_protocol::Mode::XL) < 0);

        EXPECT_TRUE(lock_protocol::Mode(lock_protocol::Mode::IXSL) <
              lock_protocol::Mode(lock_protocol::Mode::XR));
        EXPECT_TRUE(lock_protocol::Mode(lock_protocol::Mode::IX) <
              lock_protocol::Mode(lock_protocol::Mode::IXSL));
        EXPECT_TRUE(lock_protocol::Mode(lock_protocol::Mode::XL) >
              lock_protocol::Mode(lock_protocol::Mode::SL));
    }

    TEST(LockProtocolMode, TestSupremum1)
    {
        EXPECT_TRUE(lock_protocol::Mode::Supremum(lock_protocol::Mode(lock_protocol::Mode::IXSL),
                                            lock_protocol::Mode(lock_protocol::Mode::XL)) ==
              lock_protocol::Mode::XL);
    }

    TEST(LockProtocolMode, TestSupremum2)
    {
        EXPECT_TRUE(lock_protocol::Mode::Supremum(lock_protocol::Mode(lock_protocol::Mode::IXSL),
                                            lock_protocol::Mode(lock_protocol::Mode::SL)) ==
              lock_protocol::Mode::IXSL);
    }

    TEST(LockProtocolMode, TestSet)
    {
        EXPECT_TRUE(lock_protocol::Mode::Set::NL ==
              lock_protocol::Mode::Set(lock_protocol::Mode::NL).value());
        EXPECT_TRUE(lock_protocol::Mode::Set::SL !=
              lock_protocol::Mode::Set(lock_protocol::Mode::NL).value());
        EXPECT_TRUE(lock_protocol::Mode::Set::XL ==
              lock_protocol::Mode::Set(lock_protocol::Mode::XL).value());
        EXPECT_TRUE((lock_protocol::Mode::XL | lock_protocol::Mode::SL) ==
              (lock_protocol::Mode::Set::XL | lock_protocol::Mode::Set::SL));
        EXPECT_TRUE((lock_protocol::Mode::Set(lock_protocol::Mode::XL) |
               lock_protocol::Mode::Set(lock_protocol::Mode::SL))
                  .value() == (lock_protocol::Mode::Set::XL | lock_protocol::Mode::Set::SL));
    }

    TEST(LockProtocolMode, TestSetIterator1)
    {
        lock_protocol::Mode::Set mode_set;
        lock_protocol::Mode::Set::Iterator itr;
        std::set<lock_protocol::Mode::Enum> stl_set;

        for (itr = mode_set.begin(); itr != mode_set.end(); itr++)
        {
            EXPECT_TRUE(stl_set.erase(static_cast<lock_protocol::Mode::Enum>((*itr).value())) == 1);
        }

        mode_set.Insert(lock_protocol::Mode::NL);
        mode_set.Insert(lock_protocol::Mode::SL);
        stl_set.insert(lock_protocol::Mode::NL);
        stl_set.insert(lock_protocol::Mode::SL);

        for (itr = mode_set.begin(); itr != mode_set.end(); itr++)
        {
            EXPECT_TRUE(stl_set.erase(static_cast<lock_protocol::Mode::Enum>((*itr).value())) == 1);
        }
        EXPECT_TRUE(stl_set.size() == 0); // for-loop removed all entries
    }

    TEST(LockProtocolMode, TestSetIterator2)
    {
        lock_protocol::Mode::Set mode_set;
        lock_protocol::Mode::Set::Iterator itr;
        std::set<lock_protocol::Mode::Enum> stl_set;

        mode_set.Insert(lock_protocol::Mode::XL);
        mode_set.Insert(lock_protocol::Mode::SL);
        stl_set.insert(lock_protocol::Mode::XL);
        stl_set.insert(lock_protocol::Mode::SL);

        for (itr = mode_set.begin(); itr != mode_set.end(); itr++)
        {
            EXPECT_TRUE(stl_set.erase(static_cast<lock_protocol::Mode::Enum>((*itr).value())) == 1);
        }
        EXPECT_TRUE(stl_set.size() == 0); // for-loop removed all entries
    }

    TEST(LockProtocolMode, TestSetIterator3)
    {
        lock_protocol::Mode::Set mode_set;
        lock_protocol::Mode::Set::Iterator itr;
        std::set<lock_protocol::Mode::Enum> stl_set;

        mode_set.Insert(lock_protocol::Mode::IXSL);
        stl_set.insert(lock_protocol::Mode::IXSL);

        for (itr = mode_set.begin(); itr != mode_set.end(); itr++)
        {
            EXPECT_TRUE(stl_set.erase(static_cast<lock_protocol::Mode::Enum>((*itr).value())) == 1);
        }
        EXPECT_TRUE(stl_set.size() == 0); // for-loop removed all entries
    }

    TEST(LockProtocolMode, TestMostSevere)
    {
        lock_protocol::Mode::Set mode_set;

        mode_set.Insert(lock_protocol::Mode::IXSL);
        EXPECT_TRUE(mode_set.Exists(lock_protocol::Mode::IXSL));
        mode_set.Insert(lock_protocol::Mode::XR);
        EXPECT_TRUE(mode_set.Exists(lock_protocol::Mode::XR));
        EXPECT_TRUE(mode_set.MostSevere(lock_protocol::Mode::NL) ==
              lock_protocol::Mode(lock_protocol::Mode::XR));
        EXPECT_TRUE(mode_set.MostSevere(lock_protocol::Mode::IX) ==
              lock_protocol::Mode(lock_protocol::Mode::IXSL));
        EXPECT_TRUE(mode_set.MostSevere(lock_protocol::Mode::SR) ==
              lock_protocol::Mode(lock_protocol::Mode::NL));
        EXPECT_TRUE(mode_set.MostSevere(lock_protocol::Mode::SL) ==
              lock_protocol::Mode(lock_protocol::Mode::IXSL));

        mode_set.Clear();
        EXPECT_TRUE(mode_set.MostSevere(lock_protocol::Mode::NL) ==
              lock_protocol::Mode(lock_protocol::Mode::NL));
        mode_set.Insert(lock_protocol::Mode::XR);
        EXPECT_TRUE(mode_set.Exists(lock_protocol::Mode::XR));
        EXPECT_TRUE(mode_set.MostSevere(lock_protocol::Mode::NL) ==
              lock_protocol::Mode(lock_protocol::Mode::XR));

        mode_set.Clear();
        mode_set.Insert(lock_protocol::Mode::SL);
        EXPECT_TRUE(mode_set.Exists(lock_protocol::Mode::SL));
        mode_set.Insert(lock_protocol::Mode::IX);
        EXPECT_TRUE(mode_set.Exists(lock_protocol::Mode::IX));
        mode_set.Insert(lock_protocol::Mode::XR);
        EXPECT_TRUE(mode_set.Exists(lock_protocol::Mode::XR));
        EXPECT_TRUE(mode_set.MostSevere(lock_protocol::Mode::NL) ==
              lock_protocol::Mode(lock_protocol::Mode::XR));
        EXPECT_TRUE(mode_set.MostSevere(lock_protocol::Mode::SL) ==
              lock_protocol::Mode(lock_protocol::Mode::SL));
        EXPECT_TRUE(mode_set.MostSevere(lock_protocol::Mode::SR) ==
              lock_protocol::Mode(lock_protocol::Mode::SL));
    }
