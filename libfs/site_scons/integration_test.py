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
        itest.run(env['TEST_STDOUT'], env['TEST_STDERR'])
        itest_results = []
        if itest.timed_out == True:
            all_results.append(['TIMEOUT'])
            timeout_itests.append(itest)
            continue
        itest_has_failure = False
        for test_name, test in itest.tests_graph.iteritems():
            if test.p is not None:
                lines = test.p.stderr.readlines()
                if len(lines) > 0 and re.search("xml", lines[0]):
                    xmlout = string.join(lines)
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
                    # if the process is not a process we killed then
                    # something really bad happen, fail immediately 
                    if test.status and test.status != 9:
                        print "FAILURE:"
                        failed_itests.append(itest)
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

        # print any buffered output (stdout, stderr)
        if env['TEST_STDOUT'] == 'buffered' or env['TEST_STDERR'] == 'buffered':
            print "\nBUFFERED STDOUT & STDERR OUTPUT:"
            print "================================"
            for failed_itest in failed_itests+timeout_itests:
                for test_name, test in failed_itest.tests_graph.iteritems():
                    if test.p:
                        print test_name, test.cmd, string.join(test.args)
                        if env['TEST_STDOUT'] == 'buffered':
                            for line in test.p.stdout.readlines():
                                print '\tSTDOUT >',line,
                        if env['TEST_STDERR'] == 'buffered':
                            for line in test.p.stderr.readlines():
                                print '\tSTDERR >',line,

    else:
        print "Success:", num_tests, 'tests passed.'
        open(str(target[0]),'w').write("PASSED\n")
