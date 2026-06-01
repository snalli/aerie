#include "common/errno.h"
#include "osd/containers/name/container.h"
#include "fixture/session.fixture.h"
#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>

using namespace osd::containers::common;

// Suite: ContainersNameContainer
    TEST_F(SessionFixture, TestInsertLookup)
    {
        osd::common::ObjectId oid;
        volatile char* buffer = (volatile char*) malloc(sizeof(NameContainer::Object<Session>));
        NameContainer::Object<Session>* name_container =
            NameContainer::Object<Session>::Make(session, buffer);

        EXPECT_TRUE(name_container->Insert(session, ".", osd::common::ObjectId(1)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "..", osd::common::ObjectId(2)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "foo", osd::common::ObjectId(3)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "bar", osd::common::ObjectId(4)) == 0);
        EXPECT_TRUE(name_container->Find(session, ".", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(1));
        EXPECT_TRUE(name_container->Find(session, "..", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(2));
        EXPECT_TRUE(name_container->Find(session, "foo", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(3));
        EXPECT_TRUE(name_container->Find(session, "bar", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(4));
    }

    TEST_F(SessionFixture, TestEraseDot)
    {
        osd::common::ObjectId oid;
        volatile char* buffer = (volatile char*) malloc(sizeof(NameContainer::Object<Session>));
        NameContainer::Object<Session>* name_container =
            NameContainer::Object<Session>::Make(session, buffer);

        EXPECT_TRUE(name_container->Insert(session, ".", osd::common::ObjectId(2)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "..", osd::common::ObjectId(1)) == 0);

        EXPECT_TRUE(name_container->Find(session, ".", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(2));
        EXPECT_TRUE(name_container->Find(session, "..", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(1));

        EXPECT_TRUE(name_container->Erase(session, ".") == 0);
        EXPECT_TRUE(name_container->Find(session, ".", &oid) == -E_EXIST);

        EXPECT_TRUE(name_container->Erase(session, "..") == 0);
        EXPECT_TRUE(name_container->Find(session, "..", &oid) == -E_EXIST);
    }

    TEST_F(SessionFixture, TestErase)
    {
        osd::common::ObjectId oid;
        volatile char* buffer = (volatile char*) malloc(sizeof(NameContainer::Object<Session>));
        NameContainer::Object<Session>* name_container =
            NameContainer::Object<Session>::Make(session, buffer);

        EXPECT_TRUE(name_container->Insert(session, ".", osd::common::ObjectId(1)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "..", osd::common::ObjectId(2)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "foo", osd::common::ObjectId(3)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "bar", osd::common::ObjectId(4)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "doc", osd::common::ObjectId(12)) == 0);

        EXPECT_TRUE(name_container->Find(session, ".", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(1));
        EXPECT_TRUE(name_container->Find(session, "..", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(2));
        EXPECT_TRUE(name_container->Find(session, "foo", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(3));
        EXPECT_TRUE(name_container->Find(session, "bar", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(4));
        EXPECT_TRUE(name_container->Find(session, "doc", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(12));

        EXPECT_TRUE(name_container->Erase(session, "foo") == 0);
        EXPECT_TRUE(name_container->Find(session, "foo", &oid) != 0);

        EXPECT_TRUE(name_container->Erase(session, "bar") == 0);
        EXPECT_TRUE(name_container->Find(session, "bar", &oid) != 0);

        EXPECT_TRUE(name_container->Insert(session, "foo", 13) == 0);
        EXPECT_TRUE(name_container->Find(session, "foo", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(13));
        EXPECT_TRUE(name_container->Insert(session, "bar", 14) == 0);
        EXPECT_TRUE(name_container->Find(session, "bar", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(14));

        EXPECT_TRUE(name_container->Erase(session, "doc") == 0);
        EXPECT_TRUE(name_container->Find(session, "doc", &oid) != 0);

        EXPECT_TRUE(name_container->Erase(session, "foo") == 0);
        EXPECT_TRUE(name_container->Find(session, "foo", &oid) != 0);
        EXPECT_TRUE(name_container->Erase(session, "bar") == 0);
        EXPECT_TRUE(name_container->Find(session, "bar", &oid) != 0);
    }

    TEST_F(SessionFixture, TestInsertOverwrite)
    {
        osd::common::ObjectId oid;
        volatile char* buffer = (volatile char*) malloc(sizeof(NameContainer::Object<Session>));
        NameContainer::Object<Session>* name_container =
            NameContainer::Object<Session>::Make(session, buffer);

        EXPECT_TRUE(name_container->Insert(session, ".", osd::common::ObjectId(1)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "..", osd::common::ObjectId(2)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "foo", osd::common::ObjectId(3)) == 0);
        EXPECT_TRUE(name_container->Insert(session, "bar", osd::common::ObjectId(4)) == 0);

        EXPECT_TRUE(name_container->Find(session, ".", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(1));
        EXPECT_TRUE(name_container->Find(session, "..", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(2));
        EXPECT_TRUE(name_container->Find(session, "foo", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(3));
        EXPECT_TRUE(name_container->Find(session, "bar", &oid) == 0);
        EXPECT_TRUE(oid == osd::common::ObjectId(4));

        EXPECT_TRUE(name_container->Insert(session, "foo", osd::common::ObjectId(13)) != 0);
    }
