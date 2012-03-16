#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "common/types.h"
#include "ssa/main/client/stm.h"
#include "unit/fixture/client.fixture.h"

#if 0
class TestObject: public dcc::stm::Object {
public:

};

class TestObjectProxy: public client::stm::ObjectProxy<TestObjectProxy, TestObject> {
public:
	
};


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


	TEST(RollbackTransaction1)
	{
		int retries = 0;
		int val = 0;

		STM_BEGIN()
		if (retries++>1) {
			goto done;
		}
		/* do nothing */
		stm::Transaction* tx = stm::Self();
		tx->Rollback(stm::ABORT_VALIDATE);
		val = 1; /* control flow doesn't reach this statement */
		STM_END()

	done:
		CHECK(retries == 3);
		CHECK(val == 0);
	}
	
	TEST(SingleRead)
	{
		int             retries=0;
		TestObject      obj1;
		TestObjectProxy proxy1;

		proxy1.setSubject(&obj1);
		
		STM_BEGIN()
		if (retries++>0) {
			goto done;
		}
		proxy1.xOpenRO();
		STM_ABORT_IF_INVALID()
		proxy1.xOpenRO();
		obj1.xSetVersion(1);  // we emulate someone else's write/commit
		STM_ABORT_IF_INVALID() // this must abort transaction
		STM_END()

	done:	
		CHECK(retries == 2); // make sure we aborted once
		CHECK(obj1.xVersion() == 1);
	}
}

#endif 
