#
# STAT TESTS
#


import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    #
    # CONCURRENT CLIENTS
    #

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSStat:File',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSStat:File')]),
        },
        rendezvous = [('C1:T1:E2:block', 'C2:T1:E1:block')]
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSStat:Dir',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSStat:Dir')]),
        },
        rendezvous = [('C1:T1:E2:block', 'C2:T1:E1:block')]
    ))


    
    #
    # CONCURRENT CLIENTS
    #

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSStat:TestCreateOpenConcurrent',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSFile:TestCreate')]),
            'C2': ( testProgram, [('T1', 'MFSFile:TestOpen')])
        },
        rendezvous = [('C1:T1:E2:block', 'C2:T1:E1:block'), 
                      ('C1:T1:E3:block', 'C2:T1:E3:block')]
    ))


