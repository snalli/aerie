import sys
import re
import os
import collections
import bisect
import util
import processmap

O_RDWR = 2
O_WRONLY = 1
O_RDONLY = 0


# file instance 
class File:
    def __init__(self, ts, pid, fd, inode, rdwr, active=False):
        self.ts = ts
        self.pid = pid
        self.fd = fd
        self.inode = inode
        self.rdwr = rdwr
        self.active = active



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
        self.concurrent_rr_accesses = 0
        self.concurrent_rw_ww_accesses = 0 # RW (read-write) and WW (write-write)
        self.histogram_seq_rr_accesses = util.Histogram(width)
        self.histogram_seq_rw_ww_accesses = util.Histogram(width)
    
    def __str__(self):
        s = "total accesses = " + str(self.total_accesses) + ", "
        s += "concurrent accesses = " 
        s += "(RR=" + str(self.concurrent_rr_accesses) + ", " 
        s += "RW/WW=" + str(self.concurrent_rw_ww_accesses) + "), " 
        s += "RR accesses histogram = " + self.histogram_seq_rr_accesses.__str__()
        s += "RW/WW accesses histogram = " + self.histogram_seq_rw_ww_accesses.__str__()
        return s


# tracks accesses on files (inodes)
class FileSpace:
    def __init__(self, process_map, inode_space, window_size):
        self.window = window_size
        self.inode_space = inode_space
        self.process_map = process_map
        self.inode2file_map = Inode2FileMap()
        self.inode2stat_dict = collections.defaultdict(InodeStatistic)
        self.accesses_ordby_ts = [] # contains tuples (ts, file)

    # SEER traces appear to have a flaw where some open() system calls
    # return 0 as the file descriptor instead of the true file descriptor number.
    # We try to fix such cases by mapping an unknown file descriptor to the
    # last open system call that returned 0. To guard against valid 0 file 
    # descriptors we consider 0 file descriptors that have not been used a later
    # system call.
    def RemapFile(self, pid, new_fd):
        old_fd = 0
        self.process_map.RemapFileDescriptor(pid, 0, new_fd)

    def DropOlder(self, ts):
        del_elem = []
        for access in self.accesses_ordby_ts:
            access_ts = access[0]
            if util.TimeStamp.diff_ts_usec(ts, access_ts) > 0:
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
    def CheckOverlap(self, ts, pid, inode, rdwr, count_sequential_once=True):
        files = self.inode2file_map.Find(inode)
        stat = self.inode2stat_dict[inode]
        # first, check for concurrent accesses (access inode while someone 
        # else has it open (active=true))
        for f in files:
            if f.pid != pid:
                if f.active == True:
                    if f.rdwr == O_RDONLY and rdwr == O_RDONLY:
                        stat.concurrent_rr_accesses += 1
                    else:
                        stat.concurrent_rw_ww_accesses += 1
                    self.inode2stat_dict[inode] = stat
                    return
        # no concurrent access, check for sequential conflicting accesses
        if count_sequential_once:
            most_recent = self.inode2file_map.FindMostRecent(inode, ts)
            if most_recent and most_recent.pid != pid:
                time_diff = util.TimeStamp.diff_ts_usec(ts, most_recent.ts)
                if time_diff < self.window:
                    if most_recent.rdwr == O_RDONLY and rdwr == O_RDONLY:
                        stat.histogram_seq_rr_accesses.Incr(time_diff, 1)
                    else:
                        stat.histogram_seq_rw_ww_accesses.Incr(time_diff, 1)
                    self.inode2stat_dict[inode] = stat
        else:
            for f in files:
                if f.pid != pid:
                    time_diff = util.TimeStamp.diff_ts_usec(ts, f.ts)
                    if time_diff < self.window:
                        if f.rdwr == O_RDONLY and rdwr == O_RDONLY:
                            stat.histogram_seq_rr_accesses.Incr(time_diff, 1)
                        else:
                            stat.histogram_seq_rw_ww_accesses.Incr(time_diff, 1)
                        self.inode2stat_dict[inode] = stat
                        break
        
    def Open(self, ts, pid, fd, inode, rdwr):
        f = File(ts, pid, fd, inode, rdwr, active=True)
        self.process_map.InsertFile(f)
        self.inode2file_map.Update(f)
        stat = self.inode2stat_dict[inode]
        stat.total_accesses = stat.total_accesses + 1

    def Dup(self, ts, old_pid, old_fd, new_pid, new_fd):
        oldf = self.process_map.FindFile(old_pid, old_fd)
        if oldf:
            newf = File(ts, new_pid, new_fd, oldf.inode, oldf.rdwr, active=True)
            self.process_map.InsertFile(newf)
            self.inode2file_map.Update(newf)
            stat = self.inode2stat_dict[oldf.inode]
            stat.total_accesses = stat.total_accesses + 1

    def Fork(self, ts, ppid, pid):
        files = self.process_map.FindFile(ppid)
        if files:
            for fd, f in files.iteritems():
                self.Dup(ts, ppid, fd, pid, fd)

    def Close(self, ts, pid, fd):
        f = self.process_map.FindFile(pid, fd)
        if f:
            self.process_map.RemoveFile(f)
            f.ts = ts
            f.active = False
            self.accesses_ordby_ts.append((ts, f))
        else:
            self.RemapFile(pid, fd)

    def Read(self, ts, pid, fd):
        f = self.process_map.FindFile(pid, fd)
        if not f:
            self.RemapFile(pid, fd)

    def Write(self, ts, pid, fd):
        f = self.process_map.FindFile(pid, fd)
        if not f:
            self.RemapFile(pid, fd)

    def CloseFiles(self, ts, pid):
        files = self.process_map.FindFile(pid)
        if files:
            for fd, f in files.iteritems():
                f.ts = ts
                self.accesses_ordby_ts.append((ts, f))
        self.process_map.RemovePid(pid)

    def Access(self, ts, pid, inode, rdwr):
        f = File(ts, pid, 0, inode, rdwr)
        self.inode2file_map.Update(f)
        self.accesses_ordby_ts.append((ts, f))
        stat = self.inode2stat_dict[inode]
        stat.total_accesses = stat.total_accesses + 1

    # synonyms (i.e. pathnames that map to the same inode) are reported once using
    # one of the possible pahtnames
    def Report(self):
        total_accesses = 0
        concurrent_rr_accesses = 0
        concurrent_rw_ww_accesses = 0
        histogram_seq_rr_accesses = util.Histogram()
        histogram_seq_rw_ww_accesses = util.Histogram()
        map = self.inode_space.Inode2Path()
        print 'FILESPACE REPORT'
        for k, inode_stat in self.inode2stat_dict.iteritems():
            total_accesses += inode_stat.total_accesses
            concurrent_rr_accesses += inode_stat.concurrent_rr_accesses
            concurrent_rw_ww_accesses += inode_stat.concurrent_rw_ww_accesses
            histogram_seq_rr_accesses.Merge(inode_stat.histogram_seq_rr_accesses)
            histogram_seq_rw_ww_accesses.Merge(inode_stat.histogram_seq_rw_ww_accesses)
            if k in map:
                print "%s: %s" % (map[k], inode_stat)

        print 'SUMMARY'
        print 'total file accesses      = ', total_accesses
        print 'concurrent file accesses = ', concurrent_rr_accesses + concurrent_rw_ww_accesses
        print 'concurrent RR file accesses = ', concurrent_rr_accesses
        print 'concurrent RW/WW file accesses = ', concurrent_rw_ww_accesses
        print 'seq RR file accesses histogram    = ', histogram_seq_rr_accesses
        print 'seq RW/WW file accesses histogram    = ', histogram_seq_rw_ww_accesses

