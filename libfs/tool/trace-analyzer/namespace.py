import sys
import re
import os
import collections
import bisect
import util

# todo: rename

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
        if not self.accesses.has_key(pid):
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
            time_diff = util.TimeStamp.diff_ts(ts, other_ts)
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
 
    # accesses each name component of a full path name and returns
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
 

