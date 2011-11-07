import sys
import re
import os
import collections
import bisect
import util
import processmap

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
        self.concurrent_accesses = 0
        self.seq_accesses_histogram = Histogram(width)
    
    def __str__(self):
        s = "total accesses = " + str(self.total_accesses) + ", "
        s = s + "concurrent accesses = " + str(self.concurrent_accesses) + ", "
        s = s + "accesses histogram = " + self.seq_accesses_histogram.__str__()
        return s


# tracks accesses on files (inodes)
class FileSpace:
    def __init__(self, process_map, window_size):
        self.window = window_size
        self.process_map = process_map
        self.pidfd2file_map = PidFd2FileMap()
        self.inode2file_map = Inode2FileMap()
        self.inode2stat_dict = collections.defaultdict(InodeStatistic)
        self.accesses_ordby_ts = [] # contains tuples (ts, file)

    def DropOlder(self, ts):
        del_elem = []
        for access in self.accesses_ordby_ts:
            access_ts = access[0]
            if util.TimeStamp.diff_ts(ts, access_ts) > 0:
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
                time_diff = util.TimeStamp.diff_ts(ts, f.ts)
                if time_diff < self.window:
                    stat.seq_accesses_histogram.Incr(time_diff, 1)
                    self.inode2stat_dict[inode] = stat
        else:
            for f in files:
                if f.pid != pid:
                    time_diff = util.TimeStamp.diff_ts(ts, f.ts)
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


