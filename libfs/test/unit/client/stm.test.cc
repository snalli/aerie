#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "common/types.h"
#include "client.fixture.h"
#include "client/stm.h"

/*
class TestObject {
public:
	TestObject(TimeStamp ts)
		: ts_(ts)
	{ }

	void set_ts(TimeStamp ts) {
		ts_ = ts;
	}

	TimeStamp ts() { return ts_; }

private:
	TimeStamp ts_;
};
*/

SUITE(STM)
{
	using namespace client;

	TEST(NullTransaction)
	{
		STM_BEGIN()
		/* do nothing */
		STM_END()
	}

	TEST(AbortTransaction1)
	{
		int val = 0;

		STM_BEGIN()
		/* do nothing */
		STM_ABORT()
		val = 1; /* control flow doesn't reach this statement */
		STM_END()

		CHECK(val == 0);
	}

/*
	TEST(MultipleRead)
	{
		OptReadSet<TestObject> read_set;
		TestObject             obj1(1);
		TestObject             obj2(2);
		TestObject             obj3(1);
		
		read_set.Reset();
		CHECK(read_set.Read(&obj1) == 0);
		obj1.set_ts(2);
		CHECK(read_set.Validate() == false);

		
	}
*/
}
