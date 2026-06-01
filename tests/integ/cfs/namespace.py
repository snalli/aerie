#
# NAMESPACE TESTS
#

import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    #
    # NAMESPACE TESTS (SINGLE CLIENT)
    #
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'CFS:TestMkfs',
        init_script = os.path.join(parent_dir, 'test/integration/cfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'CFS_Namespace:TestMkfs')])
        },
        rendezvous = []
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'CFS:TestMountSingle',
        init_script = os.path.join(parent_dir, 'test/integration/cfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'CFS_Namespace:TestMount')])
        },
        rendezvous = []
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'CFS:TestMkfsMkdir',
        init_script = os.path.join(parent_dir, 'test/integration/cfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'CFS_Namespace:TestMkfsMkdir')])
        },
        rendezvous = []
    ))
    
    #
    # NAMESPACE TESTS (CONCURRENT CLIENTS)
    #

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'CFS:TestMkdirRmdirConcurrent',
        init_script = os.path.join(parent_dir, 'test/integration/cfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'CFS_Namespace:TestMkfsMkdir')]),
            'C2': ( testProgram, [('T1', 'CFS_Namespace:TestRmdir')])
        },
        rendezvous = [('C1:T1:E2:block', 'C2:T1:E1:block'), 
                      ('C1:T1:E3:block', 'C2:T1:E3:block')]
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'CFS:TestMkdirChdirConcurrent',
        init_script = os.path.join(parent_dir, 'test/integration/cfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'CFS_Namespace:TestMkfsMkdir')]),
            'C2': ( testProgram, [('T1', 'CFS_Namespace:TestChdir')])
        },
        rendezvous = [('C1:T1:E2:block', 'C2:T1:E1:block'), 
                      ('C1:T1:E3:block', 'C2:T1:E3:block')]
    ))
