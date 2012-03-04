#
# OBJECT/CONTAINER TESTS
#


import testfw
import os 


def addIntegrationTests(env, root_dir, testProgram, serverProgram):
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'Object:Test',
        init_script = os.path.join(root_dir, 'test/integration/dpo/init-obj.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'Object:Test')]),
            'C2': ( testProgram, [('T1', 'Object:Test')])
        },
        rendezvous = [
                      ('C1:T1:AfterMapObjects:block', 'C2:T1:BeforeMapObjects:block'),
                      ('C1:T1:AfterLock:block', 'C2:T1:BeforeLock:block'),
                      ('C1:T1:End:block', 'C2:T1:End:block'),
        ]
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'ContainersNameContainer:Test',
        init_script = os.path.join(root_dir, 'test/integration/dpo/init-obj.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'ContainersNameContainer:Test')]),
            'C2': ( testProgram, [('T1', 'ContainersNameContainer:Test')])
        },
        rendezvous = [
                      ('C1:T1:AfterMapObjects:block', 'C2:T1:BeforeMapObjects:block'),
                      ('C1:T1:AfterLock:block', 'C2:T1:BeforeLock:block'),
                      ('C1:T1:End:block', 'C2:T1:End:block'),
        ]
    ))
