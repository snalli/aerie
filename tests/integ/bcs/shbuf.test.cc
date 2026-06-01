#include "ipc.fixture.h"
#include "pxfs/client/client_i.h"
#include "test/integration/bcs/test_protocol.h"
// TODO: port to modern test framework (was testfw/unittest++)
#include <stdio.h>
#include <stdlib.h>

using namespace client;

SUITE(IPC)
{
    TEST_FIXTURE(IPCFixture, SharedBuffer)
    {
    }
}
