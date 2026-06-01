import sys
import re
import os

# FIXME: replace os.join with our join. the problem is that 
# os.join does not normalize path: dots may stay present. 
# for example: os.join("/A/B", "./C") gives /A/B/./C
# if we use the pathname to identify a file then /A/B/.C/ should be the same as /A/B/C

def absPath(cwd, path):
    if cwd and path[0] != '/':
        return os.path.join(cwd, path)
    else:
        return path

class TimeStamp:
    @staticmethod
    def get_ts(str):
        m = re.search("audit\(([0-9]+)\.([0-9]+):([0-9]+)\)", str)
        ts = (int(m.group(1)), int(m.group(2)), int(m.group(3)))
        return ts    

    @staticmethod
    def get_ts2(str):
        m = re.search("([0-9]+)\.([0-9]+):([0-9]+)", str)
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

    @staticmethod
    def diff_ts_usec(ts1, ts2):
        return (ts1[0] - ts2[0])*1000000 + ts1[1] - ts2[1]

class Histogram:
    def __init__(self, width=10000):
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
    
    def Merge(self, other):
        for k, v in other.histogram.iteritems():
            if k in self.histogram:
                self.histogram[k] += v
            else:
                self.histogram[k] = v

