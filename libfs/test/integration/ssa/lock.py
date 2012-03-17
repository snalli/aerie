#
# BASE LOCK TESTS
#

import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestLockUnlockSingle',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestLockUnlock')])
        },
        rendezvous = []
    ))


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestLockUnlockConcurrent1',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestLockUnlock')]),
            'C2': ( testProgram, [('T1', 'Lock:TestLockUnlock')])
        },
        rendezvous = [('C1:T1:E2', 'C2:T1:E1'), 
                      ('C2:T1:E3', 'C1:T1:E3')]
    ))


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestLockUnlockMultipleTimesConcurrent1',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestLockUnlockMultipleTimes')]),
            'C2': ( testProgram, [('T1', 'Lock:TestLockUnlockMultipleTimes')])
        },
        rendezvous = [('C1:T1:E1:block', 'C2:T1:E1:block')]
    ))


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestLockUnlockMultipleTimesConcurrent2',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestLockUnlockMultipleTimes')]),
            'C2': ( testProgram, [('T1', 'Lock:TestLockUnlockMultipleTimes')])
        },
        rendezvous = [
                      ('C1:T1:E1:block', 'C2:T1:E1:block'),
                      ('C1:T1:E3:block', 'C2:T1:E2:block'),
                      ('C1:T1:E4:block', 'C2:T1:E4:block')
        
        ]
    ))


    # checks that a client that grabs a cached lock is serialized
    # with respect to another client trying to acquire the same lock
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestLockUnlockMultipleTimesConcurrent3',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestLockUnlockTwoTimes')]),
            'C2': ( testProgram, [('T1', 'Lock:TestLockUnlock')])
        },
        rendezvous = [
                      ('C1:T1:E3:block', 'C2:T1:E1:block'),
                      ('C1:T1:E4:block', 'C2:T1:E3:block')
        ]
    ))


    # asynchronous conversion
    # a client acquires a lock in XL and then tries to convert it in SL
    # a second client tries to acquire the lock in SL between the two events 
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestLockConvert1',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestLockXLConvertSL')]),
            'C2': ( testProgram, [('T1', 'Lock:TestLockSL')])
        },
        rendezvous = [
                      ('C1:T1:E2:block', 'C2:T1:E1:block'),
                      ('C1:T1:E3:block', 'C2:T1:E2:block')
        ]
    ))


    # synchronous conversion
    # a client acquires a lock in XL and then tries to convert it in SL
    # a second client tries to acquire the lock in SL between the two events 
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestLockConvert2',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestLockXLConvertSLsynchronous')]),
            'C2': ( testProgram, [('T1', 'Lock:TestLockSL')])
        },
        rendezvous = [
                      ('C1:T1:E2:block', 'C2:T1:E1:block'),
                      ('C1:T1:E3:block', 'C2:T1:E2:block')
        ]
    ))


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestSharedLockUnlockConcurrentClients1',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestSharedLockUnlock')]),
            'C2': ( testProgram, [('T1', 'Lock:TestSharedLockUnlock')])
        },
        rendezvous = [
                      ('C1:T1:E2:block', 'C2:T1:E2:block')
        ]
    ))


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestSharedLockUnlockConcurrentClients1',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestSharedLockXLUnlockLockSLUnlock')]),
            'C2': ( testProgram, [('T1', 'Lock:TestSharedLockXLUnlockLockSLUnlock')])
        },
        rendezvous = [
                      ('C1:T1:E3:block', 'C2:T1:E3:block'),
                      ('C1:T1:E4:block', 'C2:T1:E4:block')
        ]
    ))


    # cancel request. deadlock scenario: two clients deadlock. one of the clients 
    # sends a cancel request to break the deadlock  
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestLockCancel1',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestLockALockB'), ('T2', 'Lock:TestCancelLock')]),
            'C2': ( testProgram, [('T1', 'Lock:TestLockBLockA')])
        },
        rendezvous = [
                      ('C1:T1:E2:block', 'C2:T1:E2:block'),
                      ('C1:T1:E3:block', 'C1:T2:E1:block')
        ]
    ))


    # cancel request. non-deadlock scenario (false positive): client thinks has 
    # deadlocked and cancels the request. false positive.
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Lock:TestLockCancel2',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Lock:TestLockALockB'), ('T2', 'Lock:TestCancelLock')]),
            'C2': ( testProgram, [('T1', 'Lock:TestLockBUnlockB')])
        },
        rendezvous = [
                      ('C1:T1:E2:block', 'C2:T1:E2:block'),
                      ('C1:T1:E3:block', 'C1:T2:E1:block'), # C1:T2 waits till C1:T1 starts the request
                      ('C1:T2:E2:block', 'C2:T1:E3:block'), # C1:T2 makes sure C2:T1 has the lock 
                                                            # before starting the cancel request
                      ('C1:T2:E3:block', 'C2:T1:E4:block')  # C2:T1 releases the lock only after C1:T2
                                                            # is done with the cancel request
        ]
    ))



