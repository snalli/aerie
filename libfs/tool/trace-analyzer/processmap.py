
# process state and info (cwd, open files)
# we build state and info incrementally so it may not be complete 
class Process:
    def __init__(self, pid):
        self.pid = pid
        self.cwd = None
        self.files = {} # fd to File
        pass

    def SetParent(self, parent_process):
        if not self.cwd:
            self.cwd = parent_process.cwd

    def InsertFile(self, file):
        fd = file.fd
        self.files[fd] = file

    def RemoveFile(self, file):
        fd = file.fd 
        if fd in self.files:
            del self.files[fd]


    def Files(self):
        return self.files

    def File(self, fd):
        if not self.files.has_key(fd):
            return None
        return self.files[fd]


# keeps track a map of known processes (PID to process)
class ProcessMap:
    def __init__(self):
        self.dict = {} # PID to process
        pass
    
    def Process(self, pid, create=False):
        if pid in self.dict:
            return self.dict[pid]
        else:
            if create:
                self.dict[pid] = Process(pid)
                return self.dict[pid]
            else:
                return None

    def RemovePid(self, pid):
        if pid in self.dict:
            del self.dict[pid]

    def SetParent(self, child_pid, parent_pid):
        child_process = self.dict[child_pid]
        parent_process = self.dict[parent_pid]
        child_process.SetParent(parent_process)
    
    def __find(self, pid, fd):
        process = self.Process(pid, False)
        if not process:
            return None
        else:
            return process.File(fd)


    def FindFile(self, pid, fd=None):
        if fd == None:
            process = self.Process(pid, False)
            if process:
                return process.Files()
            else:
                return None
        else:
            return self.__find(pid, fd)

    def InsertFile(self, file):
        pid = file.pid
        fd = file.fd
        process = self.Process(pid, True)
        process.InsertFile(file)

    def RemoveFile(self, file):
        pid = file.pid
        process = self.Process(pid, create=False)
        if process:
            process.RemoveFile(file)
            


class PidFd2FileMap:
    def __init__(self):
        self.dict = {}

    def Remove(self, pid, fd):
        f = self.__find(pid, fd)
        if f:
            del f
