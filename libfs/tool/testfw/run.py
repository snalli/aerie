import os, subprocess, string, re, sys, signal
import threading
import tempfile
import Queue
import time

class myoutput(object):
    def __init__(self):
        self.fd = 1
        pass
    def fileno(self):
        return self.fd
    def write(self,string):
        print "TST", string


class Task:
    def __init__(self, name, pred_list):
        self.name = name
        self.pred_list = pred_list

    def run(self):
        print self.name


class Scheduler:
    def __init__(self):
        self.name = 'test'
        self.lock = threading.Lock()
        self.rq = Queue.Queue()
        self.cv = threading.Condition(self.lock)

    def listen_events(i):
        pass

    def reap_finished_process(self, i):
        time.sleep(1)
        self.cv.acquire()
        self.rq.put(None)
        self.cv.notify()
        self.cv.release()
        pass


    def start(self, root_task):
        t = threading.Thread(target=self.reap_finished_process, args=(self, ))
        t.start()

        self.rq.put(root_task)
        self.cv.acquire()
        while self.rq.empty():
            self.cv.wait()
        task = self.rq.get()    
        print task
        self.cv.release()

        t.join()
        #    while not ready_queue.empty():
        #        # execure all tests in the ready queue
        #        task = ready_queue.get()
        #        task.run()
        

def alarmHandler(signum, frame):
    pass

def setAlarmHandler():
    signal.signal(signal.SIGALRM, alarmHandler)


s = Scheduler()
s.start(None)
