import os, subprocess, string, re, sys, signal
import Queue
import testfw
from xml.etree.ElementTree import ElementTree
import xml
import testfw.integration_test


def addIntegrationTest(env, integration_test):
    env.Append(INTEGRATION_TESTS = [integration_test])



def runIntegrationTests(source, target, env):
    all_results = []
    testfw.integration_test.setAlarmHandler()
    timeout_itests = []
    failed_itests = []
    for itest in env['INTEGRATION_TESTS']:
        if not re.match(env['TEST_FILTER'], itest.suite+itest.name):
            continue
        itest.run(env['TEST_STDOUT'], env['TEST_STDERR'], env['TEST_EXTRA_ARGS'])
        itest_results = []
        if itest.timed_out == True:
            all_results.append(['TIMEOUT'])
            timeout_itests.append(itest)
            continue
        itest_has_failure = False
        for test_name, test in itest.tests_graph.iteritems():
            if test_name == "__PreTest" or test_name == "__PostTest":
                continue
            if test.p is not None:
                # if stderr is set to interactive mode then we can't parse 
                # stderr output as there is nothing in the buffer to parse
                # BUG: we want to be able to parse stderr in interactive mode
                if env['TEST_STDERR'] == 'interactive':
                    continue
                test.stderr_file.seek(0)
                lines = test.stderr_file.readlines()
                if len(lines) > 0:
                    for line in lines:
                        if re.search("xml", line):
                            xmlout = line
                            tree = xml.etree.ElementTree.XML(xmlout)
                            test_list = []
                            for child in tree:
                                failure_list = []
                                for failure in child:
                                    failure_list.append(failure.attrib["message"])
                                    itest_has_failure = True
                                test_list.append((child.attrib["suite"], child.attrib["name"], child.attrib["time"], failure_list))
                            itest_results.extend(test_list)
                else:
                    # if the process is not a process we killed (signal 9) then
                    # something really bad happen, fail immediately 
                    if test.status and test.status != 9:
                        print "TEST UBNORMAL TERMINATION: ", test_name, " status=", test.status  >> 8
                        failed_itests.append(itest)
                        if test.p.stderr:
                            print test.p.stderr.readlines()
                        return
        if itest_has_failure:
            failed_itests.append(itest)

        all_results.append(itest_results)

    num_tests = 0
    num_failed_tests = 0
    num_failures = 0
    num_timeouts = 0
    for itest_result in all_results:
        num_tests = num_tests + 1
        test_failed = False
        for test in itest_result:
            if test == 'TIMEOUT':
                test_failed = True
                print "ERROR: Integration test is timed out."
                num_timeouts=num_timeouts+1
                break
            if len(test[3]) > 0:
                test_failed = True
                num_failures = num_failures+len(test[3])
                for failure in test[3]:
                    print "ERROR: Failure in", test[0] + ':' + test[1], failure
        if test_failed == True:
            num_failed_tests = num_failed_tests+1
    if num_failed_tests > 0:
        print "FAILURE:", num_failed_tests, 'out of', num_tests, 'integration tests failed (', num_failures, 'failures, ', num_timeouts, 'timeouts).'

    else:
        print "Success:", num_tests, 'tests passed.'
        open(str(target[0]),'w').write("PASSED\n")

    # print any buffered output (stdout, stderr)
    if env['TEST_STDOUT'] == 'buffered' or env['TEST_STDERR'] == 'buffered':
        print "\nBUFFERED STDOUT & STDERR OUTPUT:"
        print "================================"
        for itest in env['INTEGRATION_TESTS']:
            for test_name, test in itest.tests_graph.iteritems():
                if test_name == "__PreTest" or test_name == "__PostTest":
                    continue
                if test.p:
                    print test_name, test.cmd, string.join(test.args)
                    if env['TEST_STDOUT'] == 'buffered':
                        test.stdout_file.seek(0)
                        for line in test.stdout_file.readlines():
                            print '\tSTDOUT >',line,
                    if env['TEST_STDERR'] == 'buffered':
                        test.stderr_file.seek(0)
                        for line in test.stderr_file.readlines():
                            print '\tSTDERR >',line,
