import os, subprocess, string, re, sys, signal
import tempfile
import Queue


class myoutput(object):
    def __init__(self):
        self.fd = 1
        pass
    def fileno(self):
        return self.fd
    def write(self,string):
        print "TST", string


class Test:
    def __init__(self, name, cmd, args, osenv, wait_for_me):
        self.name = name
        self.osenv = osenv
        self.cmd = cmd
        self.args = args
        self.wait_for_me = wait_for_me
        self.succ_list = []
        self.p = None
        self.status = 0
        self.stdout_file = None
        self.stderr_file = None

    def run(self, stdout_, stderr_, extra_args=''):
        osenv = os.environ 
        osenv.update(self.osenv)
        osenv['DEBUG_IDENTIFIER'] = self.name
        if (self.cmd == ''):
            return None
        if type(self.args) == type(''):
            self.args = self.args.split()
        if stdout_ == 'none' or stdout_ == 'buffered':
            self.stdout_file = tempfile.TemporaryFile()
        if stderr_ == 'none' or stderr_ == 'buffered':
            self.stderr_file = tempfile.TemporaryFile()
        self.args.append('-T,-tag=%s' % self.name)
        if stderr_ == 'interactive':
            self.args.append('-T,-interactive')
        self.p = subprocess.Popen([self.cmd] + self.args + extra_args.split(), shell=False,
                                 stdin=subprocess.PIPE,
                                 stdout=self.stdout_file,
                                 stderr=self.stderr_file,
                                 close_fds=False, env=osenv)

    def kill(self):
        try:
            os.kill(self.p.pid, 9)
        except OSError:
            # no such process
            pass


class IntegrationTest:
    def __init__(self, suite, name, timeout=60, osenv={}):
        self.suite = suite
        self.name = name
        self.pid2test = {}
        self.osenv = osenv
        pre_test = Test('__PreTest', '', '', {}, True)
        post_test = Test('__PostTest', '', '', {}, True)
        self.tests_graph = {}
        self.tests_graph[pre_test.name] = pre_test
        self.tests_graph[post_test.name] = post_test
        self.timeout = timeout
        self.timed_out = False

    def setPreTest(self, cmd, args, osenv):
        if type(args) == type(''):
            args = args.split()
        pre_test = self.tests_graph['__PreTest']
        pre_test.cmd = cmd
        pre_test.args = args
        pre_test.osenv = osenv

    def setPostTest(self, cmd, args, osenv):
        pass

    def addTest(self, pred_list, name, cmd, args, osenv, wait_for_me=True):
        if pred_list == None or len(pred_list) == 0:
            pred_list = ['__PreTest']
        if type(pred_list) == type(''):
            pred_list = [pred_list]
        if type(args) == type(''):
            args = args.split()
        test_node = Test(name, cmd, args, osenv, wait_for_me)
        self.tests_graph[name] = test_node
        for pred_name in pred_list:
            if self.tests_graph.has_key(pred_name):
                pred_node = self.tests_graph[pred_name]
                pred_node.succ_list.append(test_node)

    def run(self, stdout_='buffered', stderr_='buffered', extra_args=''):
        ready_list = []
        wait_list = []
        nowait_list = []

        ready_queue = Queue.Queue()
        ready_queue.put(self.tests_graph['__PreTest'])
        signal.alarm(self.timeout)
        try:
            while not ready_queue.empty() or len(wait_list):
                # execure all tests in the ready queue
                while not ready_queue.empty():
                    test = ready_queue.get()
                    test.run(stdout_, stderr_, extra_args)
                    if test.p == None:
                        for succ in test.succ_list:
                            ready_queue.put(succ)
                        continue
                    self.pid2test[test.p.pid] = test
                    if test.wait_for_me:
                        wait_list.append(test)
                    else:
                        nowait_list.append(test)
                        for succ in test.succ_list:
                            ready_queue.put(succ)
                # check if there is at least one test we need to wait for
                if len(wait_list):
                    (pid, status) = os.wait()
                    finished_test = self.pid2test[pid]
                    finished_test.status = status
                    rv = finished_test.status >> 8
                    if finished_test in wait_list:
                        for succ in finished_test.succ_list:
                            ready_queue.put(succ)
                        wait_list.remove(finished_test)
                    if finished_test in nowait_list:
                        nowait_list.remove(finished_test)
            signal.alarm(0)
        except OSError:
            self.timed_out = True
        except None:
            print "CRITICAL FAILURE"
            
        # now kill all the remaining processes 
        for test in wait_list + nowait_list:
            test.kill()
            (pid, status) = os.wait()
            test.status = status

def alarmHandler(signum, frame):
    pass

def setAlarmHandler():
    signal.signal(signal.SIGALRM, alarmHandler)
