#
# OPTIMISTIC READS (TRANSACTION) TESTS
#

import testfw
import os 

def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'STM:Test',
        init_script = os.path.join(parent_dir, 'test/integration/ssa/init-obj.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'STM:TestPessimisticUpdate')]),
            'C2': ( testProgram, [('T1', 'STM:TestOptimisticRead')])
        },
        rendezvous = [
                      ('C1:T1:AfterMapObjects:block', 'C2:T1:BeforeMapObjects:block'),
                      ('C1:T1:BeforeUnlock:block', 'C2:T1:AfterFind:block'),
                      ('C1:T1:End:block', 'C2:T1:End:block'),
        ]
    ))
