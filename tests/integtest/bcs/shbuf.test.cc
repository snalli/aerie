#include "ipc.fixture.h"
#include "pxfs/client/client_i.h"
#include "test/integration/bcs/test_protocol.h"
#include "testfw/integrationtest.h"
#include <stdio.h>
#include <stdlib.h>

using namespace client;

SUITE(IPC)
{
    TEST_FIXTURE(IPCFixture, SharedBuffer)
    {
    }
}
