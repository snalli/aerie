import os, subprocess, string, re, sys, signal
import sysv_ipc
import threading
import tempfile
import Queue
import time

# The scheduler executes tasks in the order defined by the task predecessor
# dependencies, which form a data flow graph.


# TODO: StrandManager and Scheduler manage a single schedule at a time and 
# create strands, processes, events under that single schedule. To start
# a new schedule one has to basically clear the active schedule and then
# reuse it. 

STRANDMGR_MSGQUE_KEY=42
STRANDMGR_SEM_KEY_BASE=142

# some helper functions follow

def split_tuple(str):
    m = re.search("(.*):(.*)", str)
    if m:
        return (m.group(1), m.group(2))
    else:
        return (None, None)

def split_triple(str):
    m = re.search("(.+):(.+):(.+)", str)
    if m:
        return (m.group(1), m.group(2), m.group(3))
    else:
        return (None, None, None)

def get_strand_tag(event_tag):
    m = re.search("(.+:.+):(.+)", event_tag)
    if m:
        return m.group(1)
    return None
 
# A rendezvous is a point designated for meeting by several strands
#
# implementation note: we allow multiple dynamic instances of a rendezvous.
# there are two challenges:
# 1) ensure that a strand that doesn't block lets others know that it 
#    arrived at the rendezvous. we use the strand_arrivals dictionary
#    to keep track of the arrival counts per strand.
# 2) know who is waiting at each dynamic instant. we could keep a wait list
#    per each instant but we don't need to as a single instance suffices.
#    this is based on the observation that someone that didn't wait in a 
#    an instant will not wait in the next instant either. 
class RendezvousPoint:
    def __init__(self, strand_action_pair_list):
        self.lock = threading.Lock() 
        self.strand_arrivals = {}
        self.strand_action = {}
        for s in strand_action_pair_list:
            self.strand_arrivals[s[0].tag] = 0
            self.strand_action[s[0].tag] = s[1] # actions are 'block' or 'noblock'
        self.wait_list = []

    def trigger(self, strand):
        self.lock.acquire()
        if strand.tag in self.strand_action:
            self.strand_arrivals[strand.tag] += 1 
            action = self.strand_action[strand.tag]
        else:
            NameError("Unknown strand hit rendezvous point")
        if action == 'block':
            self.wait_list.append(strand)
        else:
            strand.semaphore.release()
        # before I move on I check whether all others have arrived at
        # the rendezvous point and if they had then I wake all waiters
        # including myself (I should have already put myself in the list)
        unblock = True
        for stag, arcnt in self.strand_arrivals.iteritems():
            if arcnt == 0:
                unblock = False
        if unblock:
            for stag, arcnt in self.strand_arrivals.iteritems():
                self.strand_arrivals[stag] -= 1
            for strand in self.wait_list:
                strand.semaphore.release()
            self.wait_list = []
        self.lock.release()
 

# a strand represents a control flow entity within a process which
# we don't directly create but the process does
class Strand:
    def __init__(self, manager, msgque_key, semaphore, sem_key, tag, args):
        self.manager = manager
        self.tag = tag
        self.msgque_key = msgque_key
        self.sem_key = sem_key
        self.semaphore = semaphore


def StrandManagerInstance():
    if not StrandManager.instance:
        StrandManager.instance = StrandManager()
    return StrandManager.instance

# StrandManager is a daemon so use this class as a singleton 
class StrandManager(threading.Thread):
    instance = None
    def __init__(self):
        self.clearSysVIPC()
        self.lock = threading.Lock()
        threading.Thread.__init__(self)
        self.daemon = True
        self.msgque_key = STRANDMGR_MSGQUE_KEY
        self.msgque = sysv_ipc.MessageQueue(STRANDMGR_MSGQUE_KEY, sysv_ipc.IPC_CREX)
        self.sem_key_base = STRANDMGR_SEM_KEY_BASE
        self.sem_key_next = self.sem_key_base
        self.semaphore_list = []
        self.running = True
        self.tag2strand = {}
        self.tag2rendezvous = {}
    
    def __del__(self):
        if self.msgque:
            self.msgque.remove()

    def clearSysVIPC(self):
        msgque = sysv_ipc.MessageQueue(STRANDMGR_MSGQUE_KEY, sysv_ipc.IPC_CREAT)
        msgque.remove()
        # create a bunch of semaphores and destroy them to ensure semaphores named by
        # keys used by the manager don't exist.
        for i in range(20):
            semaphore = sysv_ipc.Semaphore(STRANDMGR_SEM_KEY_BASE+i, sysv_ipc.IPC_CREAT)
            semaphore.remove()

    def run(self):
        while self.running:
            m = self.msgque.receive(True)
            if m[1] == 1: # event trigger 
                event_tag = m[0]
                strand_tag = get_strand_tag(event_tag)
                strand = None
                if self.tag2strand.has_key(strand_tag):
                    strand = self.tag2strand[strand_tag]
                if not strand:
                    continue
                rendezvous = None
                if self.tag2rendezvous.has_key(event_tag):
                    rendezvous = self.tag2rendezvous[event_tag]
                if rendezvous:
                    rendezvous.trigger(strand)
                else:
                    strand.semaphore.release()

    def stop(self):
        self.running = False
        self.msgque.send("STOP") # send a message to unblock the queue

    def createStrand(self, StrandConstructor, tag, args):
        self.lock.acquire()
        semaphore = sysv_ipc.Semaphore(self.sem_key_next, sysv_ipc.IPC_CREX)
        strand = StrandConstructor(self, self.msgque_key, semaphore, self.sem_key_next, tag, args)
        self.sem_key_next = self.sem_key_next + 1
        self.semaphore_list.append(semaphore)
        self.tag2strand[tag] = strand
        self.lock.release()
        return strand
    
    def clear(self):
        self.tag2strand.clear()
        self.tag2rendezvous.clear()
        for s in self.semaphore_list:
            s.remove()
        self.semaphore_list = []
        self.sem_key_next = self.sem_key_base

    def createRendezvousPoint(self, event_list):
        strand_list = []
        event_tag_list = []
        for event in event_list:
            m = re.search("(.+:.+:.+):(.*)", event)
            if m:
                event_tag = m.group(1)
                event_action = m.group(2)
            else:
                m = re.search("(.+:.+:.+)", event)
                if not m:
                    raise NameError("Non well formatted event in rendezvous")
                event_tag = m.group(1)
                event_action = 'noblock'
            event_tag_list.append(event_tag)
            strand_tag = get_strand_tag(event_tag)
            if self.tag2strand.has_key(strand_tag): 
                strand_list.append((self.tag2strand[strand_tag], event_action))
            else:
                return 

        # create rendezvous 
        rendezvous = RendezvousPoint(strand_list)
        for e in event_tag_list:
            self.tag2rendezvous[e] = rendezvous

        return rendezvous
 
# A task represents a group of strands. A task is executed by the scheduler. 
class Task:
    CREATED = 1  # the task descriptor has been created but task cannot yet run
                 # as its predecessor tasks have to finish
    READY = 2    # predecessor tasks finished execution. task is ready to run.

    def __init__(self, tag, cmd, cmd_args, preds, daemon, vargs):
        self.tag = tag
        self.osenv = {}
        self.succ_list = []
        self.process = None
        self.cmd = cmd
        self.args = cmd_args
        self.daemon = daemon
        self.out_file = None
        # predecessors and status
        if type(preds) is list:
            if len(self.pred_list) == 0:
                self.pred_list = None
                self.status = Task.READY
            else:
                self.pred_list = preds
                self.status = Task.CREATED
        else:
            if preds:
                self.pred_list = [preds]
                self.status = Task.CREATED
            else:
                self.pred_list = None
                self.status = Task.READY
        if self.pred_list:
            for pred in self.pred_list:
                pred.succ_list.append(self)
        self.osenv['DEBUG_IDENTIFIER'] = self.tag
    
    def ready(self):
        return self.status == Task.READY

    def removePred(self, pred):
        if self.pred_list:
            self.pred_list.remove(pred)
        if len(self.pred_list) == 0:
            self.status = Task.READY

    def run(self, output='deferred', attach_gdb = False):
        osenv = os.environ 
        osenv.update(self.osenv)
        if output=='none' or output == 'deferred':
            self.out_file = tempfile.TemporaryFile()
            cmd = [self.cmd] + self.args
        elif output == 'multi-term':
            bashcmd = [self.cmd] + self.args
            cmd = ['gnome-terminal', '--disable-factory', '--geometry=140x15+0-400', '-x','bash']
            #cmd = ['gnome-terminal', '--geometry=140x15+0-400', '-x','bash']
            if attach_gdb:
                cmd = cmd + ['-c', 'gdb -args %s; read' % string.join(bashcmd, ' ')]
            else:
                cmd = cmd + ['-c', '%s; read' % string.join(bashcmd, ' ')]
        elif output == 'single-term':
            cmd = [self.cmd] + self.args
        self.process = subprocess.Popen(cmd, shell=False,
                                        stdin=subprocess.PIPE,
                                        stdout=self.out_file,
                                        stderr=self.out_file,
                                        close_fds=False, env=osenv)
    def kill(self):
        try:
            os.kill(self.process.pid, 9)
        except OSError:
            # no such process
            pass


def SchedulerInstance():
    if not Scheduler.instance:
        Scheduler.instance = Scheduler()
    return Scheduler.instance

# use this class as a singleton otherwise strandmanager cannot be a daemon 
# which complicates initialization of SysV semaphores and message queues
class Scheduler:
    instance = None
    NONE = 1
    RUNNING = 2
    SUCCESS = 3 # finished successfully
    FAILURE = 4 # finished abnormally
    TIMEOUT = 5 # timed out
    def __init__(self, timeout = 60):
        self.status = Scheduler.NONE
        self.name = 'test'
        self.timeout = timeout
        self.lock = threading.Lock()
        self.rq = Queue.Queue()
        self.cv = threading.Condition(self.lock)
        self.sm = StrandManagerInstance()
        self.sm.start()
        self.ready_queue = Queue.Queue()
        self.pid2task = {}
        self.task_list = []

    def taskFinishedCallback(self, task):
        for succ in task.succ_list:
            succ.removePred(task)
            if succ.ready():
                self.ready_queue.put(succ)
        if (task.out_file):
            task.out_file.seek(0)

    # run all tasks till all non-daemon tasks complete.
    def run(self, output = 'deferred', attach_gdb = False):
        wait_list = []
        nowait_list = []
        self.status = Scheduler.RUNNING
        if output == 'deferred' or output == 'none':
            signal.alarm(self.timeout)
        
        try:
            while not self.ready_queue.empty() or len(wait_list):
                # execure all tasks in the ready queue
                while not self.ready_queue.empty():
                    task = self.ready_queue.get()
                    task.run(output, attach_gdb)
                    if task.process == None:
                        self.taskFinishedCallback(task)
                    else:
                        self.pid2task[task.process.pid] = task
                        if task.daemon == False:
                            wait_list.append(task)
                        else:
                            nowait_list.append(task), task.tag, task.succ_list
                # check if there is at least one task we need to wait for
                if len(wait_list):
                    (pid, process_status) = os.wait()
                    finished_task = self.pid2task[pid]
                    finished_task.process_status = process_status
                    rv = finished_task.process_status >> 8
                    if finished_task in wait_list:
                        self.taskFinishedCallback(finished_task)
                        wait_list.remove(finished_task)
                    if finished_task in nowait_list:
                        nowait_list.remove(finished_task)
            signal.alarm(0)
            self.status = Scheduler.SUCCESS
        except OSError:
            self.status = Scheduler.TIMEOUT
        except None:
            self.status = Scheduler.FAILURE
            print "CRITICAL FAILURE"
        # now kill the remaining processes
        for task in wait_list + nowait_list:
            task.kill()
            (pid, process_status) = os.wait()
            task.process_status = process_status

    def clear(self):
        self.sm.clear()
        self.pid2task.clear()
        self.task_list = []
        self.status = Scheduler.NONE

    def createStrand(self, StrandConstructor, tag, *args):
        return self.sm.createStrand(StrandConstructor, tag, args)

    def createRendezvousPoint(self, event_list):
        return self.sm.createRendezvousPoint(event_list)

    def createTask(self, TaskConstructor, tag, cmd, cmd_args, preds, daemon, *args):
        task = TaskConstructor(tag, cmd, cmd_args, preds, daemon, args)
        self.task_list.append(task)
        if task.status == Task.READY:
            self.ready_queue.put(task)
        return task            

def alarmHandler(signum, frame):
    pass

def setAlarmHandler():
    signal.signal(signal.SIGALRM, alarmHandler)
