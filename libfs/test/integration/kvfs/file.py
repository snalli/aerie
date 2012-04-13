#
# FILE TESTS
#


import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    #
    # FILE TESTS (SINGLE CLIENT)
    #
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'KVFSFile:TestPut',
        init_script = os.path.join(parent_dir, 'test/integration/kvfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'KVFSFile:TestPut')])
        },
        rendezvous = []
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'KVFSFile:TestPutSync',
        init_script = os.path.join(parent_dir, 'test/integration/kvfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'KVFSFile:TestPutSync')])
        },
        rendezvous = []
    ))
    
    #
    # FILE TESTS (CONCURRENT CLIENTS)
    #
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'KVFSFile:TestPutGet',
        init_script = os.path.join(parent_dir, 'test/integration/kvfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'KVFSFile:TestPut')]),
            'C2': ( testProgram, [('T1', 'KVFSFile:TestGet')])
        },
        rendezvous = [('C1:T1:E2:block', 'C2:T1:E1:block'), 
                      ('C1:T1:E3:block', 'C2:T1:E3:block')]
    ))
