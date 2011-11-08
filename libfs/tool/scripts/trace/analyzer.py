import sys
import re
import os
import collections
import bisect
import filespace
import namespace
import processmap
import util


# TODO
# track dup2 system call
# track fork => inherit file descriptors
# diffentiate between shared accesses (R) and conflicting accesses (RW, WW)
# identify process names that contribute to sharing


FILESPACE_WINDOW = 1000000
NAMESPACE_WINDOW = 1000000

O_RDWR = 2
O_WRONLY = 1
O_RDONLY = 0


class Analyzer:
    def __init__(self, syscall_class):
        self.syscall_class = syscall_class
        self.process_map = processmap.ProcessMap()
        #self.filespace = filespace.FileSpace(self.process_map, FILESPACE_WINDOW)
        self.filespace = None
        #self.namespace = namespace.NameSpace(NAMESPACE_WINDOW)
        self.namespace = None

    def SyscallOpen(self, syscall):
        fd = int(syscall.exit)
        pid = syscall.pid
        process = self.process_map.Process(pid)
        (src_dir, src_file) = syscall.srcAbsPath(process.cwd)
        print src_dir, src_file
        if not src_dir or not src_file:
            return
        pathname = os.path.join(src_dir[0], src_file[0])
        inode = src_file[1]
        ts = syscall.ts
        rdwr = syscall.args[1] & (O_RDWR | O_WRONLY)
        if self.filespace:
            self.filespace.Open(ts, pid, fd, inode, rdwr)
            self.filespace.CheckOverlap(ts, pid, inode)
            self.filespace.DropOlderWindow(ts)
        if self.namespace:
            self.namespace.AccessPath(ts, pid, pathname, rdwr, O_RDONLY)
            self.namespace.DropOlderWindow(ts)

    def SyscallClose(self, syscall):
        ts = syscall.ts
        pid = syscall.pid
        fd = int(syscall.args[0])
        if self.filespace:
            f = self.filespace.Close(ts, pid, fd)

    def SyscallRename(self, syscall):
        ts = syscall.ts
        pid = syscall.pid
        (src_dir, src_file) = syscall.srcAbsPath()
        (dst_dir, dst_file) = syscall.dstAbsPath()
        if not src_dir or not src_file or not dst_dir or not dst_file:
            return
        if self.filespace:
            f = self.filespace.Access(ts, pid, src_dir[1], O_RDWR)
            if src_dir[1] != dst_dir[1]:
                f = self.filespace.Access(ts, pid, dst_dir[1], O_RDWR)
            f = self.filespace.Access(ts, pid, src_file[1], O_RDWR)
            self.filespace.CheckOverlap(ts, pid, src_dir[1])
            self.filespace.CheckOverlap(ts, pid, dst_dir[1])
            self.filespace.CheckOverlap(ts, pid, src_file[1])
            self.filespace.DropOlderWindow(ts)
        if self.namespace:
            node = self.namespace.AccessPath(ts, pid, src_dir[0], O_RDWR, O_RDONLY)
            self.namespace.AccessName(node, ts, pid, src_file[0], O_RDWR)
            if (src_dir[0] != dst_dir[0]):
                node = self.namespace.AccessPath(ts, pid, dst_dir[0], O_RDWR, O_RDONLY)
            self.namespace.AccessName(node, ts, pid, dst_file[0], O_RDWR)
            self.namespace.DropOlderWindow(ts)

    def SyscallUnlink(self, syscall):
        ts = syscall.ts
        pid = syscall.pid
        (src_dir, src_file) = syscall.srcAbsPath()
        if not src_dir or not src_file:
            return
        if self.filespace:
            f = self.filespace.Access(ts, pid, src_dir[1], O_RDWR)
            f = self.filespace.Access(ts, pid, src_file[1], O_RDWR)
            self.filespace.CheckOverlap(ts, pid, src_dir[1])
            self.filespace.CheckOverlap(ts, pid, src_file[1])
            self.filespace.DropOlderWindow(ts)
        if self.namespace:
            node = self.namespace.AccessPath(ts, pid, src_dir[0], O_RDWR, O_RDONLY)
            self.namespace.AccessName(node, ts, pid, src_file[0], O_RDWR)
            self.namespace.DropOlderWindow(ts)

    def SyscallChdir(self, syscall):
        pid = syscall.pid
        process = self.process_map.Process(pid)
        (src_dir, src_file) = syscall.srcPath()
        cwd = os.path.join(src_dir[0], src_file[0])
        process.cwd = cwd

    def SyscallFork(self, syscall):
        pid = syscall.pid
        process = self.process_map.Process(pid)
        ppid = int(syscall.args[0])
        print ppid, 'forks', pid
        self.process_map.SetParent(pid, ppid)
        pass

    def KillProcessOlderThan(self, process_log_lines, ts):
        if not process_log_lines:
            return
        next = True
        while(next and process_log_lines):
            str = process_log_lines[0]    
            m = re.search("([0-9]+).([0-9]+) ([0-9]+)", str)
            process_ts = (int(m.group(1)), int(m.group(2)), 0)
            pid = int(m.group(3))
            if util.TimeStamp.cmp_ts(ts, process_ts):
                process_log_lines.pop(0)
                self.filespace.ClosePid(ts, pid)
            else:    
                next = False

    def SyscallExit(self, syscall):
        print 'SyscallExit'
        pass

    def Syscall(self, syscall):
        if syscall == None:
            return 
        options = {
            self.syscall_class.SYS_OPEN: self.SyscallOpen,
            self.syscall_class.SYS_CLOSE: self.SyscallClose,
            self.syscall_class.SYS_RENAME: self.SyscallRename,
            self.syscall_class.SYS_UNLINK: self.SyscallUnlink,
            self.syscall_class.SYS_CHDIR: self.SyscallChdir,
            self.syscall_class.SYS_FORK: self.SyscallFork,
            self.syscall_class.SYS_EXIT: self.SyscallExit
        }
        if options.has_key(syscall.identifier):
            options[syscall.identifier](syscall)


