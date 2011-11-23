#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "common/types.h"
#include "client.fixture.h"
#include "client/optreadset.h"

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


SUITE(ClientOptReadSet)
{
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

}
