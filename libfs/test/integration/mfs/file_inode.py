#
# FILE INODE TESTS
#


import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    #
    # FILE INODE TESTS (SINGLE CLIENT)
    #

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSFileInode:TestWriteReadSingle1',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSFileInode:TestWrite1')])
        },
        rendezvous = []
    ))

    
    #
    # FILE INODE TESTS (MULTIPLE CLIENTS)
    #
    
    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSFileInode:TestWriteReadConcurrent1',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSFileInode:TestWrite1')]),
            'C2': ( testProgram, [('T1', 'MFSFileInode:TestRead1')])
        },
        rendezvous = [
                      ('C1:T1:AfterMapObjects:block', 'C2:T1:BeforeMapObjects:block'),
                      ('C1:T1:AfterLock:block', 'C2:T1:BeforeLock:block'),
                      ('C1:T1:End:block', 'C2:T1:End:block'),
        ]
    ))
