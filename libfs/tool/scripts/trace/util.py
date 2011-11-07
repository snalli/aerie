import sys
import re
import os


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
