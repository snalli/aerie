import os, subprocess, string, re, sys, signal
import tempfile
import Queue
from xml.etree.ElementTree import ElementTree
import xml
from scheduler import *

    
class TestStrand(Strand):
    def __init__(self, manager, msgque_key, semaphore, sem_key, tag, args):
        Strand.__init__(self, manager, msgque_key, semaphore, sem_key, tag, args)
        suite_test = args[0]
        (suite, test) = split_tuple(suite_test)
        self.suite = suite
        self.test = test
        self.args = ["-T,-thread,-tag=%s,-msg=%d,-sem=%d,-suite=%s,-test=%s" % (tag, self.msgque_key, self.sem_key, suite, test)]
 

class TestTask(Task):
    def __init__(self, tag, cmd, cmd_args, preds, daemon, varargs):
        strands = varargs[0]
        Task.__init__(self, tag, cmd, cmd_args, preds, daemon, varargs)
        # strands
        if type(strands) is list:
            self.strand_list = strands
        else:
            self.strand_list = [strands]
        self.osenv['DEBUG_IDENTIFIER'] = self.tag
        self.args = self.args + ['-T,-filter=exact']
        for s in self.strand_list:
            self.args = self.args + s.args

    def run(self, output='deferred'):
        if output=='none' or output=='deferred':
            self.args = self.args + ['-T,-deferred']
        Task.run(self, output)


# this describes a schedule of an integration test
class IntegrationTest:
    SUCCESS = 0
    FAILURE = Scheduler.FAILURE
    TIMEOUT = Scheduler.TIMEOUT
    
    class Thread:
        def __init__(self, tag, suite, test):
            self.tag = tag
            self.suite = suite
            self.test = test

        def __str__(self):
            return "%s %s:%s" % (self.tag, self.suite, self.test)

    class Process:
        def __init__(self, tag, cmd, args):
            self.tag = tag
            self.cmd = cmd
            self.args = args
            self.threads = []

        def addThread(self, tag, suite, test):
            thread = IntegrationTest.Thread(tag, suite, test)
            self.threads.append(thread)

        def __str__(self):
            str = "%s: %s %s\n" % (self.tag, self.cmd, string.join(self.args, ' '))
            for t in self.threads:
                str = str + "\t%s\n" % (t.__str__())
            return str

    class RendezvousPoint:
        def __init__(self, events):
            self.events = events

        def __str__(self):
            str = ""
            for e in self.events:
                str = str + "%s " % e
            return str

    def __init__(self, name, testfw, server, clients, rendezvous):
        (name_suite, name_test) = split_tuple(name)
        if not name_suite or not name_test:
            raise NameError('Test name %s is not well formatted. Please use this format:  <SUITE>:<TEST>' % name)
        self.name = name
        testfw_program_abspath = os.path.join(os.getcwd(), str(testfw[0]))
        self.testfw = IntegrationTest.Process('TESTFW', testfw_program_abspath, ['-T,-init'])
        server_program_abspath = os.path.join(os.getcwd(), str(server[0]))
        self.server = IntegrationTest.Process('S', server_program_abspath, ['-p', '10000'])
        self.clients = []
        for c, cv in clients.iteritems():
            client_tag = c
            client_program = cv[0]
            client_program_abspath = os.path.join(os.getcwd(), str(client_program[0]))
            client = IntegrationTest.Process(client_tag, client_program_abspath, ['-T,-host=10000'])
            for t in cv[1]:
                (suite, test) = split_tuple(t[1])
                client.addThread("%s:%s" % (client_tag, t[0]), suite, test)
            self.clients.append(client)
        self.rendezvous = []
        for r in rendezvous:
            self.rendezvous.append(IntegrationTest.RendezvousPoint(r))
        self.test_results = []

    def __str__(self):
        str = 'TASKS\n'
        str = str + self.server.__str__()
        for c in self.clients:
            str = str + c.__str__()
        str = str + 'EVENTS\n'
        for i, e in enumerate(self.rendezvous):
            str = str + "(%d) %s\n" % (i, e.__str__())
        return str

    def matchFilter(self, filter):
        (suite_filter, test_filter) = split_tuple(filter)
        (suite, test) = split_tuple(self.name)
        if re.match(suite_filter, suite) and re.match(test_filter, test):
            return True
        return False

    def run(self, scheduler, output='deferred', extra_cmd_args=''):
        args = extra_cmd_args.split()
        scheduler.clear()
        # testfw init
        testfw_task = scheduler.createTask(Task, self.server.tag, self.testfw.cmd, self.testfw.args, None, False)
        # server
        scheduler.createTask(Task, self.server.tag, self.server.cmd, self.server.args + args, testfw_task, True)
        # clients
        for c in self.clients:
            strands = []
            for t in c.threads:
                strands.append(scheduler.createStrand(TestStrand, t.tag, "%s:%s" % (t.suite, t.test)))
            scheduler.createTask(TestTask, c.tag, c.cmd, c.args + args, testfw_task, False, strands) # None should be the Init test
        # rendezvous points
        for r in self.rendezvous:
            scheduler.createRendezvousPoint(r.events)
        scheduler.run(output)
        itest_has_failure = False
        if scheduler.status == Scheduler.SUCCESS:
            # analyze output to find whether tests had any CHECKS failed
            self.test_results = []
            for t in scheduler.task_list:
                if t.process_status > 0 and t.process_status != 9:
                    # ubnormal termination caused by unknown reason
                    itest_has_failure = True
                    continue
                if t.out_file:
                    t.out_file.seek(0)
                    lines = t.out_file.readlines()
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
                                self.test_results.extend(test_list)
            if itest_has_failure:
                return IntegrationTest.FAILURE
            else:
                return IntegrationTest.SUCCESS
        else:
            return Scheduler.status
