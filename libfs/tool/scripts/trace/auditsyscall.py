# parses an auditctl syscall entry

import sys
import re
import os
import collections
import bisect
import util


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

def str2tuple(str):
    list = str.split('=')
    return (list[0], list[1])

def get_cwd(str):
    m = re.search("cwd=\"(.+)\"", str)
    return m.group(1)
 
def log_type(str):
    m = re.match("type=([A-Z]+)", str)
    return m.group(1)


# parses a syscall entry
class Syscall:
    # System call numbers have to agree with the ones in 
    # /usr/include/asm/unistd_64.h

    SYS_OPEN    = 2
    SYS_CLOSE   = 3
    SYS_RENAME  = 82 
    SYS_MKDIR   = 83  # todo
    SYS_RMDIR   = 84  # todo
    SYS_CREAT   = 85  # todo
    SYS_LINK    = 86  # todo
    SYS_UNLINK  = 87  
    SYS_FORK    = 0   # todo
    SYS_EXIT    = 0   # todo

    def __init__(self, tuple_list):
        self.ts = util.TimeStamp.get_ts(tuple_list[1][1])
        self.identifier = int(tuple_list[3][1]) # identifier is a number
        if tuple_list[4][1] == 'no'
            self.success = False
        else:
            self.success = True
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

    # factory: parses a string and creates a Syscall object
    @staticmethod
    def Parse(lines, i):
        if log_type(lines[i]) != "SYSCALL":
            return None
        syscall_list = Syscall.__parse_syscall_line(lines[i])
        syscall_obj = Syscall(syscall_list)
        if not syscall_obj.success:
            return None
        path_list = []
        cwd_found = False
        path_found = False
        if syscall_obj.items > 0:
            items = syscall_obj.items
            for l in lines[i:]:
                ts = util.TimeStamp.get_ts(l)
                if util.TimeStamp.cmp_ts(ts, syscall_obj.ts) != 0:
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

    def srcPath(self):
        if len(path) == 0:
            return (None, None)
        if self.identifier == Syscall.SYS_OPEN:
            if (len(syscall.path) == 1):
                pathname = os.path.join(syscall.cwd, syscall.path[0][0])
                inode = syscall.path[0][1]
            else:
                pathname = os.path.join(syscall.cwd, syscall.path[0][0], syscall.path[1][0])
                inode = syscall.path[1][1]
            (pathname_dir, pathname_file) = os.path.split(pathname)
            src_dir = (pathname_dir, 0) # FIXME: how to get the inode of the directory???
            src_file = (pathname_file, inode)
        else:
            src_dir = self.path[0]
            src_file = self.path[2]
        return (src_dir, src_file)

    def dstPath():
        dst_dir = self.path[1]
        dst_file = self.path[3]
        return (dst_dir, dst_file)
