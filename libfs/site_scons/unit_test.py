import os, subprocess, string, re
from xml.etree.ElementTree import ElementTree
import xml
import re

def addUnitTestFilter(env, path, test_filter):
    args = []
    osenv = ({})
    suite = None
    test = None
    if test_filter:
        m = re.search("suite=([A-Za-z0-9]*)", test_filter)
        if m:
            suite = m.group(1)    
        m = re.search("test=([A-Za-z0-9]*)", test_filter)
        if m:
            test = m.group(1)
    if suite:
        args.append('-T,-suite=%s' % (suite))
    if test:
        args.append('-T,-test=%s' % (test))
    env.Append(UNIT_TEST_CMDS = [(osenv, path, args)])

 
def addUnitTestList(env, path, suite, *tests):
    if tests == ():
        osenv = ({})
        args = ['-T,-suite=%s' % (suite, utest)]
        env.Append(UNIT_TEST_CMDS = [(osenv, path, args)])
    else:
        for utest in tests[0]:
            osenv = ({})
            args = ['-T,-suite=%s,-test=%s' % (suite, utest)]
            env.Append(UNIT_TEST_CMDS = [(osenv, path, args)])


def addUnitTest(env, path, suite, test):
    addUnitTestList(env, path, suite, [test])


def addUnitTestSuite(env, path, suite):
    addUnitTestList(env, path, suite)


def runUnitTests(source, target, env, verbose=False):
    results = []
    if (env['TEST_STDOUT'] != 'none' and env['TEST_STDOUT'] != 'deferred') or \
       (env['TEST_STDERR'] != 'none' and env['TEST_STDERR'] == 'deferred'):
        stdout_file = None
        stderr_file = None
    else:
        stdout_file = subprocess.PIPE
        stderr_file = subprocess.PIPE
    for test in env['UNIT_TEST_CMDS']:
        osenv = os.environ 
        osenv.update(test[0])
        path = test[1]
        args = test[2]
        if stdout_file or stderr_file:
            args += ['-T,-deferred']
        utest = subprocess.Popen([path] + args, shell=False,
                                 stdin=subprocess.PIPE,
                                 stdout=stdout_file,
                                 stderr=stderr_file, close_fds=True, env=osenv)
        ret = os.waitpid(utest.pid, 0)
        if stdout_file or stderr_file:
            lines = utest.stderr.readlines()
            if len(lines) > 0 and re.search("xml", lines[0]):
                xmlout = string.join(lines)
                tree = xml.etree.ElementTree.XML(xmlout)
                test_list = []
                for child in tree:
                    failure_list = []
                    for failure in child:
                        failure_list.append(failure.attrib["message"])
                    test_list.append((child.attrib["suite"], child.attrib["name"], child.attrib["time"], failure_list))
                results.extend(test_list)
            else:
                # Something really bad happen; fail immediately 
                print "FAILURE:", ret[1]
                for line in utest.stderr.readlines():
                    print line,
                return 
        
    if stdout_file or stderr_file:
        num_tests = 0
        num_failed_tests = 0
        num_failures = 0
        for test in results:
            num_tests = num_tests + 1
            if len(test[3]) > 0:
                num_failed_tests = num_failed_tests+1
                num_failures = num_failures+len(test[3])
                for failure in test[3]:
                    print "ERROR: Failure in", test[0] + ':' + test[1], failure
        if num_failed_tests > 0:
            print "FAILURE:", num_failed_tests, 'out of', num_tests, 'tests failed (', num_failures, 'failures).'
        else:
            print "Success:", num_tests, 'tests passed.'
            open(str(target[0]),'w').write("PASSED\n")
