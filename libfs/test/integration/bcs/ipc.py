#
# INTERPROCESS COMMUNICATION TESTS
#

import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'IPC:TestServerIsAlive',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'IPC:TestServerIsAlive')])
        },
        rendezvous = []
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'IPC:TestAdd',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'IPC:TestAdd')])
        },
        rendezvous = []
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'IPC:TestEcho',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'IPC:TestEcho')])
        },
        rendezvous = []
    ))

    #
    # CONCURRENT CLIENTS
    #

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'IPC:TestSum',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'IPC:TestSum')]),
            'C2': ( testProgram, [('T1', 'IPC:TestSum')])
        },
        rendezvous = []
    ))

    #
    # SHARED BUFFER TESTS (SINGLE CLIENT)
    # 

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'IPC:SharedBuffer',
        init_script = os.path.join(parent_dir, 'test/integration/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'IPC:SharedBuffer')])
        },
        rendezvous = []
    ))
