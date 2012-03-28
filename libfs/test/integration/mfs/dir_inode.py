#
# DIRECTORY INODE TESTS
#
import testfw
import os 


def addIntegrationTests(env, parent_dir, testProgram, serverProgram):
    #
    # DIRECTORY INODE TESTS (SINGLE CLIENT)
    #


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSDirInode:TestLink',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSDirInode:TestLink')])
        },
        rendezvous = []
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSDirInode:TestUnlink',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSDirInode:TestUnlink')])
        },
        rendezvous = []
    ))

    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSDirInode:TestFileWrite',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSDirInode:TestFileWrite')])
        },
        rendezvous = []
    ))


    #
    # DIRECTORY INODE TESTS (MULTIPLE CLIENTS)
    #


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSDirInode:TestLinkConcurrent1',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSDirInode:TestLink1_publisher')]),
            'C2': ( testProgram, [('T1', 'MFSDirInode:TestLink1_consumer')])
        },
        rendezvous = [
                      ('C1:T1:AfterMapObjects:block', 'C2:T1:BeforeMapObjects:block'),
                      ('C1:T1:AfterLock:block', 'C2:T1:BeforeLock:block'),
                      ('C1:T1:End:block', 'C2:T1:End:block'),
        ]
    ))


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSDirInode:TestLinkConcurrent2',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSDirInode:TestLink2_publisher')]),
            'C2': ( testProgram, [('T1', 'MFSDirInode:TestLink2_consumer')])
        },
        rendezvous = [
                      ('C1:T1:AfterMapObjects:block', 'C2:T1:BeforeMapObjects:block'),
                      ('C1:T1:AfterLock:block', 'C2:T1:BeforeLock:block'),
                      ('C1:T1:End:block', 'C2:T1:End:block'),
        ]
    ))


    env.addIntegrationTest(testfw.integration_test.IntegrationTest(
        name = 'MFSDirInode:TestUnlinkConcurrent1',
        init_script = os.path.join(parent_dir, 'test/integration/mfs/init.sh'),
        testfw = testProgram, server = serverProgram,
        clients = { 
            'C1': ( testProgram, [('T1', 'MFSDirInode:TestUnlink1_publisher')]),
            'C2': ( testProgram, [('T1', 'MFSDirInode:TestUnlink1_consumer')])
        },
        rendezvous = [
                      ('C1:T1:AfterMapObjects:block', 'C2:T1:BeforeMapObjects:block'),
                      ('C1:T1:AfterLock:block', 'C2:T1:BeforeLock:block'),
                      ('C1:T1:End:block', 'C2:T1:End:block'),
        ]
    ))


