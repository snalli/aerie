#
# FILE TESTS
#


import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    #
    # FILE TESTS (CONCURRENT CLIENTS)
    #

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'CFSFile:TestCreateOpenConcurrent',
        init_script = os.path.join(parent_dir, 'test/integration/cfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'CFSFile:TestCreate')]),
            'C2': ( testProgram, [('T1', 'CFSFile:TestOpen')])
        },
        rendezvous = [('C1:T1:E2:block', 'C2:T1:E1:block'), 
                      ('C1:T1:E3:block', 'C2:T1:E3:block')]
    ))


