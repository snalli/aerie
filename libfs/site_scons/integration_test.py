import os, subprocess, string, re, sys, signal
import Queue
import testfw
from xml.etree.ElementTree import ElementTree
import xml
import testfw.integration_test


def addIntegrationTest(env, integration_test):
    env.Append(INTEGRATION_TESTS = [integration_test])


def runIntegrationTests(source, target, env):
    testfw.integration_test.setAlarmHandler()
    scheduler = testfw.integration_test.SchedulerInstance()
    timeout_itests = []
    failed_itests = []
    num_tests = 0
    num_failed_tests = 0
    num_timeouts = 0
    num_failures = 0
    for itest in env['INTEGRATION_TESTS']:
        # if a specific test is asked then run just that
        # otherwise run any tests that match the filter
        if env['TEST_NAME']:
            if not itest.matchName(env['TEST_NAME']):
                continue
        else:
            if not itest.matchFilter(env['TEST_FILTER']):
                continue
        num_tests = num_tests + 1
        print itest.name
        ret = itest.run(scheduler, env['TEST_STDOUT'], env['TEST_EXTRA_ARGS'], env['TEST_ATTACH_GDB'])
        if ret == testfw.integration_test.IntegrationTest.TIMEOUT:
            num_timeouts=num_timeouts+1
            timeout_itests.append(itest)
        elif ret == testfw.integration_test.IntegrationTest.FAILURE:
            failed_itests.append(itest)
            num_failed_tests = num_failed_tests+1
            # count the number of failures of the failed test
            for test in itest.test_results:
                if len(test[3]) > 0:
                    num_failures = num_failures + len(test[3])

    # print a summary report
    if num_failed_tests > 0 or num_timeouts > 0:
        print "FAILURE:", num_failed_tests, 'out of', num_tests, 'integration tests failed (', num_failures, 'failures, ', num_timeouts, 'timeouts).'
        if len(failed_itests) > 0:
            print 'Failed tests:'
            for i, itest in enumerate(failed_itests):
                print '(%03d) FAILED:' % i, itest.name
                for result in itest.test_results:
                    if len(test[3]) > 0:
                        for failure in test[3]:
                            print "      ERROR:  Failure in", test[0] + ':' + test[1], failure
        if len(timeout_itests) > 0:
            print 'Timeout tests:'
            for i, itest in enumerate(timeout_itests):
                print '(%d) TIMEOUT: ' % i, itest.name
    else:
        print "Success:", num_tests, 'tests passed.'
        open(str(target[0]),'w').write("PASSED\n")

    # print any buffered output (stdout, stderr)
    if env['TEST_STDOUT'] == 'deferred' or env['TEST_STDERR'] == 'deferred':
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
