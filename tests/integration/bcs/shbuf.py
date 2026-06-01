#
# SHARED BUFFER TESTS
#

import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    #
    # SINGLE CLIENT
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
