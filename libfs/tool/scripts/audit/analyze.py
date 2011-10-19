import sys
import re
import os
import collections
import bisect


# TODO
# track dup2 system call
# track fork => inherit file descriptors
# diffentiate between shared accesses (R) and conflicting accesses (RW, WW)
# identify process names that contribute to sharing

# System call numbers have to agree with the ones in 
# /usr/include/asm/unistd_64.h

FILESPACE_WINDOW = 1000000
NAMESPACE_WINDOW = 1000000

SYS_OPEN    = 2
SYS_CLOSE   = 3
SYS_RENAME  = 82 
SYS_MKDIR   = 83  # todo
SYS_RMDIR   = 84  # todo
SYS_CREAT   = 85  # todo
SYS_LINK    = 86  # todo
SYS_UNLINK  = 87  

O_RDWR = 2
O_WRONLY = 1
O_RDONLY = 0


def get_name(str):
    m = re.search("name=\"(.+)\"", str)
    if m:
        return m.group(1)
    else:
        return None

def get_inode(str):
    m = re.search("inode=([0-9]+)", str)
    if m:
        return m.group(1)
    else:
        return None

class TimeStamp:
    @staticmethod
    def get_ts(str):
        m = re.search("audit\(([0-9]+)\.([0-9]+):([0-9]+)\)", str)
        ts = (int(m.group(1)), int(m.group(2)), int(m.group(3)))
        return ts    
    
    @staticmethod
    def cmp_ts(ts1, ts2):
        if (ts1[0] == ts2[0]):
            if (ts1[1] == ts2[1]):
                i = 2
            else:
                i = 1
        else:
            i = 0

        if (ts1[i] == ts2[i]):
            return 0
        if (ts1[i] > ts2[i]):
            return 1
        if (ts1[i] < ts2[i]):
            return -1
    
    @staticmethod
    def diff_ts(ts1, ts2):
        return ts1[0] - ts2[0]


class Histogram:
    def __init__(self, width=1000):
        self.histogram = {}
        self.width = width

    def __str__(self):
        return self.histogram.__str__()

    def Incr(self, key, val):
        key = (key / self.width) * self.width
        oldval = 0
        if not self.histogram.has_key(key):
            oldval = self.histogram[key] = 0
        else:
            oldval = self.histogram[key]
        self.histogram[key] = oldval + val


class NameNode:
    def __init__(self, name):
        self.accesses = {} # pid      -> (ts, access_type)
        self.children = {} # str_name -> NameNode
        self.access_histogram = Histogram()
        self.name = name

    def Access(self, ts, pid, type):
        self.accesses[pid] = (ts, type)
    
    # accesses name under this node
    def AccessName(self, ts, pid, name, type):
        cur_node = self.Child(name)
        cur_node.Access(ts, pid, type)
 
    def Remove(self, ts, pid):
        if not self.children.has_key(pid):
            return
        del self.accesses[pid] 

    def Child(self, name):
        if self.children.has_key(name):
            node = self.children[name]
        else:
            node = self.children[name] = NameNode(name)
        return node

    # check if overlap with accesses falling in the window [ts-window, ts] of node
    def CheckOverlap(self, ts, pid, window_size):
        for other_pid, other_val in self.accesses.iteritems():
            other_ts = other_val[0]
            time_diff = TimeStamp.diff_ts(ts, other_ts)
            if other_pid != pid and time_diff < window_size:
                self.access_histogram.Incr(time_diff, 1)
 

# Issues: 
#  - Symbolic links
#  - how to resolve a .. (parent link) appearing in a pathname?
class NameSpace:
    def __init__(self, window_size):
        self.window = window_size
        self.root = NameNode('$ROOT')
        self.accesses_ordby_ts = [] # contains tuples (ts, pid, pathname)
        pass

    @staticmethod
    def DFS(start, path):
        abspath = path + [start.name]
        print "/".join(abspath), start.access_histogram.histogram
        for key, val in start.children.iteritems():
            NameSpace.DFS(val, abspath)


    def List(self):
        path = []
        NameSpace.DFS(self.root, path)

    # drop accesses that are older than ts
    def DropOlder(self, ts):
        del_elem = []
        for access in self.accesses_ordby_ts:
            access_ts = access[0]
            if TimeStamp.diff_ts(ts, access_ts) > 0:
                del_elem.append(access)
            else:
                break
        self.accesses_ordby_ts = self.accesses_ordby_ts[len(del_elem):]
        for d in del_elem:
            ts = d[0]
            pid = d[1]
            pathname = d[2]
            cur_node = self.root
            for name in pathname.split('/'):
                if name != '':
                    cur_node = cur_node.Child(name)
                cur_node.Remove(ts, pid)
                 
    def DropOlderWindow(self, ts):
        self.DropOlder((ts[0] - self.window, ts[1], ts[2]))
    
       
    def CheckOverlap(self, ts, pid, pathname):
        cur_node = self.root
        for name in pathname.split('/'):
            if name != '':
                cur_node = cur_node.Child(name)
            # check if overlap with most recent accesses of node
            cur_node.CheckOverlap(ts, pid, self.window)
    
    # accesses name under a node
    def AccessName(self, node, ts, pid, name, type):
        cur_node = node.Child(name)
        cur_node.Access(ts, pid, type)
        cur_node.CheckOverlap(ts, pid, self.window)
 
    # access each name component of a full path name and returns
    # last NameNode
    def AccessPath(self, ts, pid, pathname, type, parent_type):
        cur_node = self.root
        pathname_split = pathname.split('/')
        pathname_split_len = len(pathname_split)
        for i, name in enumerate(pathname_split):
            if name != '':
                cur_node = cur_node.Child(name)
            if i == pathname_split_len - 1: # last component
                cur_node.Access(ts, pid, type)
            elif i == len(pathname_split)-2: # second-to-last component, a.k.a parent of last
                cur_node.Access(ts, pid, parent_type)
            else:
                cur_node.Access(ts, pid, O_RDONLY)
        self.accesses_ordby_ts.append((ts, pid, pathname))
        self.CheckOverlap(ts, pid, pathname)
        return cur_node
 

# parses a syscall entry
class Syscall:
    def __init__(self, tuple_list):
        self.ts = TimeStamp.get_ts(tuple_list[1][1])
        self.number = int(tuple_list[3][1])
        self.success = tuple_list[4][1]
        self.exit = tuple_list[5][1]
        self.args = [long(tuple_list[6][1], 16)]
        self.args.append(long(tuple_list[7][1], 16)) 
        self.args.append(long(tuple_list[8][1], 16)) 
        self.args.append(long(tuple_list[9][1], 16)) 
        self.items = int(tuple_list[10][1])
        self.ppid = int(tuple_list[11][1])
        self.pid = int(tuple_list[12][1])
        self.uid = int(tuple_list[14][1])
        self.path = []
        self.cwd = ""

    @staticmethod
    def __parse_syscall_line(str):
        list = str.split(' ')
        tl = []
        for l in list:
            t = str2tuple(l)
            if t[1][0] == '\"':
                t = (t[0], t[1].strip('\"'))
            tl.append(t)
        return tl

    # creates a Syscall object
    @staticmethod
    def Parse(lines, i):
        if log_type(lines[i]) != "SYSCALL":
            return None
        syscall_list = Syscall.__parse_syscall_line(lines[i])
        syscall_obj = Syscall(syscall_list)
        if syscall_obj.success == 'no':
            return None
        path_list = []
        cwd_found = False
        path_found = False
        if syscall_obj.items > 0:
            items = syscall_obj.items
            for l in lines[i:]:
                ts = TimeStamp.get_ts(l)
                if TimeStamp.cmp_ts(ts, syscall_obj.ts) != 0:
                    continue
                if log_type(l) == 'CWD':
                    syscall_obj.cwd = get_cwd(l)
                    cwd_found = True
                if log_type(l) == 'PATH':
                    name = get_name(l)
                    inode_str = get_inode(l)
                    if name != None and inode_str != None:
                        path_list.append((name, int(inode_str)))
                    items = items - 1 
                    if items == 0:
                        path_found = True
                if cwd_found == True and path_found == True:
                    break
        syscall_obj.path.extend(path_list)
        return syscall_obj


def log_type(str):
    m = re.match("type=([A-Z]+)", str)
    return m.group(1)

def str2tuple(str):
    list = str.split('=')
    return (list[0], list[1])

def get_cwd(str):
    m = re.search("cwd=\"(.+)\"", str)
    return m.group(1)
 

# file instance 
class File:
    def __init__(self, ts, pid, fd, inode, rdwr, active=False):
        self.ts = ts
        self.pid = pid
        self.fd = fd
        self.inode = inode
        self.rdwr = rdwr
        self.active = active


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


class Inode2FileMap:
    def __init__(self):
        self.dict = {}

    # updates mapping inode, pid --> file
    # updates come in timestamp order so we keep the N most recent 
    # ones per pid
    def Update(self, file, N=1):
        inode = file.inode
        pid = file.pid
        if not self.dict.has_key(inode):
            self.dict[inode] = {}
        if not self.dict[inode].has_key(pid):
            self.dict[inode][pid] = []
        head = self.dict[inode][pid][:N]
        self.dict[inode][pid] = [file] + head

    def Find(self, inode):
        if not self.dict.has_key(inode):
            return None
        files = []
        for p, f in self.dict[inode].iteritems():
            files.extend(f)
        return files
    
    def FindMostRecent(self, inode, ts):
        most_recent = None
        files = self.Find(inode)
        if files:
            for f in files:
                if f.ts < ts:
                    if most_recent:
                        if most_recent.ts < f.ts :
                            most_recent = f
                    else:
                        most_recent = f
        return most_recent
        
    def Remove(self, inode):
        files = self.Find(inode)
        if files:
            del self.dict[inode]
            
    def Remove(self, file):
        inode = file.inode
        pid = file.pid
        if self.dict.has_key(inode):
            if self.dict[inode].has_key(pid):
                try:
                    self.dict[inode][pid].remove(file)
                except ValueError:
                    pass



class InodeStatistic:
    def __init__(self, width=10000):
        self.total_accesses = 0
        self.concurrent_accesses = 0
        self.seq_accesses_histogram = Histogram(width)
    
    def __str__(self):
        s = "total accesses = " + str(self.total_accesses) + ", "
        s = s + "concurrent accesses = " + str(self.concurrent_accesses) + ", "
        s = s + "accesses histogram = " + self.seq_accesses_histogram.__str__()
        return s

# tracks accesses on files (inodes)
class FileSpace:
    def __init__(self, window_size):
        self.window = window_size
        self.pidfd2file_map = PidFd2FileMap()
        self.inode2file_map = Inode2FileMap()
        self.inode2stat_dict = collections.defaultdict(InodeStatistic)
        self.accesses_ordby_ts = [] # contains tuples (ts, file)

    def DropOlder(self, ts):
        del_elem = []
        for access in self.accesses_ordby_ts:
            access_ts = access[0]
            if TimeStamp.diff_ts(ts, access_ts) > 0:
                del_elem.append(access)
            else:
                break
        self.accesses_ordby_ts = self.accesses_ordby_ts[len(del_elem):]
        for d in del_elem:
            self.inode2file_map.Remove(d[1])
    
    def DropOlderWindow(self, ts):
        self.DropOlder((ts[0] - self.window, ts[1], ts[2]))

    # An access may overlap with multiple accesses from other processes
    #   -- we count one overlap
    # A process may make multiple accesses to an inode previously accessed by 
    # another process
    #   -- we count them as single conflicting accesses (as they are sequential)
    #      for example:
    #           P1       P2       P3
    #           |        |        |
    #           X 0      |        |
    #           |        X 1      |
    #           |        |        X 2
    #           |        |        |
    #           |        |        X 3
    #
    #      X 2 and X 3 are counted as a single access 
    #
    def CheckOverlap(self, ts, pid, inode, count_sequential_once=True):
        files = self.inode2file_map.Find(inode)
        stat = self.inode2stat_dict[inode]
        # first, check for concurrent accesses (access inode while someone else has it open)
        for f in files:
            if f.pid != pid:
                if f.ts == (0, 0, 0): 
                    stat.concurrent_accesses = stat.concurrent_accesses + 1
                    self.inode2stat_dict[inode] = stat
                    return
        # no concurrent access, check for sequential conflicting accesses
        if count_sequential_once:
            most_recent = self.inode2file_map.FindMostRecent(inode, ts)
            if most_recent and most_recent.pid != pid:
                time_diff = TimeStamp.diff_ts(ts, f.ts)
                if time_diff < self.window:
                    stat.seq_accesses_histogram.Incr(time_diff, 1)
                    self.inode2stat_dict[inode] = stat
        else:
            for f in files:
                if f.pid != pid:
                    time_diff = TimeStamp.diff_ts(ts, f.ts)
                    if time_diff < self.window:
                        stat.seq_accesses_histogram.Incr(time_diff, 1)
                        self.inode2stat_dict[inode] = stat
                        break
        
    def Open(self, ts, pid, fd, inode, rdwr):
        f = File(ts, pid, fd, inode, rdwr, active=True)
        self.pidfd2file_map.Insert(f)
        self.inode2file_map.Update(f)
        stat = self.inode2stat_dict[inode]
        stat.total_accesses = stat.total_accesses + 1

    def Close(self, ts, pid, fd):
        f = self.pidfd2file_map.Find(pid, fd)
        if f:
            self.pidfd2file_map.Remove(f)
            f.ts = ts
            f.active = False
            self.accesses_ordby_ts.append((ts, f))

    def ClosePid(self, ts, pid):
        files = self.pidfd2file_map.Find(pid)
        if files:
            for fd, f in files.iteritems():
                f.ts = ts
                self.accesses_ordby_ts.append((ts, f))
        self.pidfd2file_map.RemovePid(pid)

    def Access(self, ts, pid, inode, rdwr):
        f = File(ts, pid, 0, inode, rdwr)
        self.inode2file_map.Update(f)
        self.accesses_ordby_ts.append((ts, f))
        stat = self.inode2stat_dict[inode]
        stat.total_accesses = stat.total_accesses + 1

    def Report(self):
        for k,v in self.inode2stat_dict.iteritems():
            print "inode %10d: %s" % (k, v)


class Environment:
    def __init__(self):
        self.filespace = FileSpace(FILESPACE_WINDOW)
        #self.filespace = None
        #self.namespace = NameSpace(NAMESPACE_WINDOW)
        self.namespace = None

    def ComputeSyscallOpen(self, syscall):
        if (len(syscall.path) == 0):
            return
        fd = int(syscall.exit)
        if (len(syscall.path) == 1):
            pathname = os.path.join(syscall.cwd, syscall.path[0][0])
            inode = syscall.path[0][1]
        else:
            pathname = os.path.join(syscall.cwd, syscall.path[0][0], syscall.path[1][0])
            inode = syscall.path[1][1]
        pid = syscall.pid
        ts = syscall.ts
        rdwr = syscall.args[1] & (O_RDWR | O_WRONLY)
        if self.filespace:
            self.filespace.Open(ts, pid, fd, inode, rdwr)
            self.filespace.CheckOverlap(ts, pid, inode)
            self.filespace.DropOlderWindow(ts)
        if self.namespace:
            self.namespace.AccessPath(ts, pid, pathname, rdwr, O_RDONLY)
            self.namespace.DropOlderWindow(ts)

    def ComputeSyscallClose(self, syscall):
        ts = syscall.ts
        pid = syscall.pid
        fd = int(syscall.args[0])
        if self.filespace:
            f = self.filespace.Close(ts, pid, fd)

    def ComputeSyscallRename(self, syscall):
        ts = syscall.ts
        pid = syscall.pid
        src_dir = syscall.path[0]
        dst_dir = syscall.path[1]
        src_file = syscall.path[2]
        dst_file = syscall.path[3]
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

    def ComputeSyscallUnlink(self, syscall):
        ts = syscall.ts
        pid = syscall.pid
        target_dir = syscall.path[0]
        target_file = syscall.path[1]
        if self.filespace:
            f = self.filespace.Access(ts, pid, target_dir[1], O_RDWR)
            f = self.filespace.Access(ts, pid, target_file[1], O_RDWR)
            self.filespace.CheckOverlap(ts, pid, target_dir[1])
            self.filespace.CheckOverlap(ts, pid, target_file[1])
            self.filespace.DropOlderWindow(ts)
        if self.namespace:
            node = self.namespace.AccessPath(ts, pid, target_dir[0], O_RDWR, O_RDONLY)
            self.namespace.AccessName(node, ts, pid, target_file[0], O_RDWR)
            self.namespace.DropOlderWindow(ts)

    def ComputeSyscall(self, syscall):
        if syscall == None:
            return 
        options = {
            SYS_OPEN: self.ComputeSyscallOpen,
            SYS_CLOSE: self.ComputeSyscallClose,
            SYS_RENAME: self.ComputeSyscallRename,
            SYS_UNLINK: self.ComputeSyscallUnlink
        }
        if options.has_key(syscall.number):
            options[syscall.number](syscall)

    def KillProcessOlderThan(self, process_log_lines, ts):
        if not process_log_lines:
            return
        next = True
        while(next and process_log_lines):
            str = process_log_lines[0]    
            m = re.search("([0-9]+).([0-9]+) ([0-9]+)", str)
            process_ts = (int(m.group(1)), int(m.group(2)), 0)
            pid = int(m.group(3))
            if TimeStamp.cmp_ts(ts, process_ts):
                process_log_lines.pop(0)
                self.filespace.ClosePid(ts, pid)
            else:    
                next = False



def main(argv):
    env = Environment()
    audit_log = argv[0];
    af = open(audit_log, 'r')
    al = af.readlines()
    pl = []
    if len(argv) > 1:
        process_log = argv[1];
        pf = open(process_log, 'r')
        pl = pf.readlines()
    for i in range(len(al)):
        syscall_obj = Syscall.Parse(al, i)
        env.ComputeSyscall(syscall_obj)
        if syscall_obj:
            env.KillProcessOlderThan(pl, syscall_obj.ts)

    if env.filespace:
        env.filespace.Report()
    if env.namespace:
        env.namespace.List()        


if __name__ == "__main__":
    main(sys.argv[1:])
