import os, subprocess, string, re, sys, signal
import Queue

class Test:
    def __init__(self, name, cmd, args, osenv, wait_for_me):
        self.name = name
        self.osenv = osenv
        self.cmd = cmd
        self.args = args
        self.wait_for_me = wait_for_me
        self.succ_list = []
        self.p = None

    def run(self):
        osenv = os.environ 
        osenv.update(self.osenv)
        if (self.cmd == ''):
            return None
        print self.cmd, self.args
        self.p = subprocess.Popen([self.cmd] + self.args, shell=False,
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE,
                                 close_fds=True, env=osenv)
        print 'Run', self.name, self.p.pid

    def kill(self):
        try:
            os.kill(self.p.pid, 9)
        except OSError:
            # no such process
            pass


class IntegrationTest:
    def __init__(self, osenv):
        self.pid2test = {}
        self.osenv = {}
        pre_test = Test('__PreTest', '', [''], {}, True)
        post_test = Test('__PreTest', '', [''], {}, True)
        self.tests_graph = {}
        self.tests_graph[pre_test.name] = pre_test
        self.tests_graph[post_test.name] = post_test
        self.timeout = 0

    def setPreTest(self, cmd, args, osenv):
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
        test_node = Test(name, cmd, args, osenv, wait_for_me)
        self.tests_graph[name] = test_node
        for pred_name in pred_list:
            if self.tests_graph.has_key(pred_name):
                pred_node = self.tests_graph[pred_name]
                pred_node.succ_list.append(test_node)

    def run(self):
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
                    test.run()
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
                    if finished_test in wait_list:
                        rv = status >> 8
                        print "Done", finished_test.name, pid, rv
                        print finished_test.p.stdout.readlines()
                        for succ in finished_test.succ_list:
                            ready_queue.put(succ)
                        wait_list.remove(finished_test)
            signal.alarm(0)
        except OSError:
            print "TIMEOUT"
        except None:
            print "CRITICAL FAILURE"
            
        # now kill all the remaining processes 
        for test in wait_list + nowait_list:
            test.kill()
            print test.p.stdout.readlines()

def alarmHandler(signum, frame):
    pass

def main(argv):
    integration_test = IntegrationTest({})
    integration_test.setPreTest('ls', [''], {})
    integration_test.addTest(None, 'S', 'ls', ['-t '], {}, False)
    integration_test.addTest(None, 'C1', './test', [''], {})
    integration_test.addTest(None, 'C2', 'ls', [''], {})
    integration_test.run()


if __name__ == "__main__":
    signal.signal(signal.SIGALRM, alarmHandler)
    main(sys.argv[1:])
