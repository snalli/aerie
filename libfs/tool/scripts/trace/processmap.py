
# process state and info (cwd, open files)
# we build state and info incrementally so it may not be complete 
class Process:
    def __init__(self, pid):
        self.pid = pid
        self.cwd = None
        pass

    def SetParent(self, parent_process):
        if not self.cwd:
            self.cwd = parent_process.cwd

# keeps track a map of known processes (PID to process)
class ProcessMap:
    def __init__(self):
        self.dict = {} # PID to process
        pass
    
    def Process(self, pid):
        if pid in self.dict:
            return self.dict[pid]
        else:
            self.dict[pid] = Process(pid)
            return self.dict[pid]

    def Remove(self, pid):
        if pid in self.dict:
            del self.dict[pid]

    def SetParent(self, child_pid, parent_pid):
        child_process = self.dict[child_pid]
        parent_process = self.dict[parent_pid]
        child_process.SetParent(parent_process)


class PidFd2FileMap:
    def __init__(self):
        self.dict = {}

    def __find(self, pid, fd):
        if not self.dict.has_key(pid):
            return None
        files = self.dict[pid]
        if not files.has_key(fd):
            return None
        f = files[fd]
        return f

    def Insert(self, file):
        pid = file.pid
        fd = file.fd
        if not self.dict.has_key(pid):
            self.dict[pid] = {}
        files = self.dict[pid]
        files[fd] = file

    def Find(self, pid, fd=None):
        if fd == None:
            if not self.dict.has_key(pid):
                return None
            return self.dict[pid]
        else:
            return self.__find(pid, fd)

    def Remove(self, pid, fd):
        f = self.__find(pid, fd)
        if f:
            del f

    def RemovePid(self, pid):
        if not self.dict.has_key(pid):
            return None
        files = self.dict[pid]
        files.clear()
        del self.dict[pid]
    
    def Remove(self, file):
        pid = file.pid
        fd = file.fd
        f = self.__find(pid, fd)
        if f:
            del self.dict[pid][fd]


