#
# NAMESPACE TESTS
#

import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFS:TestMkfs',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Namespace:TestMkfs')])
        },
        rendezvous = []
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFS:TestMountSingle',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Namespace:TestMount')])
        },
        rendezvous = []
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFS:TestMkfsMountConcurrent',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Namespace:TestMkfs')]),
            'C2': ( testProgram, [('T1', 'Namespace:TestMount')])
        },
        rendezvous = [('C1:T1:MkfsAfter:block', 'C2:T1:MountBefore:block')]
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFS:TestMkdirRmdirConcurrent',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Namespace:TestMkfsMkdir')]),
            'C2': ( testProgram, [('T1', 'Namespace:TestRmdir')])
        },
        rendezvous = [('C1:T1:E2:block', 'C2:T1:E1:block'), 
                      ('C1:T1:E3:block', 'C2:T1:E3:block')]
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFS:TestMkdirChdirConcurrent',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Namespace:TestMkfsMkdir')]),
            'C2': ( testProgram, [('T1', 'Namespace:TestChdir')])
        },
        rendezvous = [('C1:T1:E2:block', 'C2:T1:E1:block'), 
                      ('C1:T1:E3:block', 'C2:T1:E3:block')]
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFS:TestRename',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Namespace:TestCreateCheckRenamed')]),
            'C2': ( testProgram, [('T1', 'Namespace:TestRename')])
        },
        rendezvous = [('C1:T1:E2:block', 'C2:T1:E1:block'), 
                      ('C1:T1:E3:block', 'C2:T1:E2:block'),
                      ('C1:T1:EEND:block', 'C2:T1:EEND:block')]
    ))
