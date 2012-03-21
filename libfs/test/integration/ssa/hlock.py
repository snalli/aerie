#
# HIERARCHICAL LOCK TESTS
#

import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'SSA_HLock:TestLockUnlockSingleClient',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'HLock:TestLockIXLockXRUnlock')])
        },
        rendezvous = [
        ]
    ))


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'SSA_HLock:TestLockUnlockConcurrentClient1',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'HLock:TestLockIXLockXLUnlock')]),
            'C2': ( testProgram, [('T1', 'HLock:TestLockIXLockXLUnlock')])
        },
        rendezvous = [
                      ('C1:T1:E1:block', 'C2:T1:E1:block'),
                      ('C1:T1:E2:block', 'C2:T1:E2:block')
        ]
    ))


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'SSA_HLock:TestLockUnlockConcurrentClient2',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'HLock:TestLockISLockSLUnlock')]),
            'C2': ( testProgram, [('T1', 'HLock:TestLockISLockSLUnlock')])
        },
        rendezvous = [
                      ('C1:T1:E1:block', 'C2:T1:E1:block'),
                      ('C1:T1:E2:block', 'C2:T1:E2:block'),
                      ('C1:T1:E3:block', 'C2:T1:E3:block')
        ]
    ))


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'SSA_HLock:TestDeepLockUnlockConcurrentClient',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'HLock:TestLockIXLockIXLockXLUnlockAll')]),
            'C2': ( testProgram, [('T1', 'HLock:TestLockXR')])
        },
        rendezvous = [
                      ('C1:T1:E5:block', 'C2:T1:E1:block'),
                      ('C1:T1:E6:block', 'C2:T1:E2:block')
        ]
    ))

