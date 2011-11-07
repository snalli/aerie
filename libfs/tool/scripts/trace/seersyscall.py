# parses a SEER syscall entry

import string
import sys
import re
import os
import collections
import bisect
import util


# parses a syscall entry
class Syscall:

    SYS_OPEN    = 'open'
    SYS_CLOSE   = 'close'
    SYS_RENAME  = 'rename'
    SYS_CHDIR   = 'chdir'
    SYS_MKDIR   = 'mkdir'
    SYS_RMDIR   = 'rmdir'
    SYS_CREAT   = 'creat'
    SYS_LINK    = 'link'
    SYS_UNLINK  = 'unlink'
    SYS_FORK    = 'fork'
    SYS_EXIT    = 'exit'
    
    O_RDWR = 2
    O_WRONLY = 1
    O_RDONLY = 0

    flags = {
        'O_RDWR': O_RDWR,
        'O_WRONLY': O_WRONLY,
        'O_RDONLY': O_RDONLY,
        'O_CREAT': O_CREAT,
        'O_TRUNC': O_TRUNC
    }

    def __init__(self):
        pass


    # factory: parses a string and creates a Syscall object
    @staticmethod
    def Parse(line):
        m = re.match("([0-9]+) UID ([0-9]+) PID ([0-9]+) (.+) ([A-Z]{1}) ([0-9]+.[0-9]+) ([a-z]+)\((.+)\) = ([-0-9]+)(.*)", line)
        if not m:
            m = re.match("[0-9]+ RESTART", line)
            if m:
                return None
            else:
                NameError("Unknown SEER entry")
        syscall_obj = Syscall()
        syscall_obj.offset = m.group(1)
        syscall_obj.uid = int(m.group(2))
        syscall_obj.pid = int(m.group(3))
        syscall_obj.program = m.group(4)
        syscall_obj.flag = m.group(5)
        syscall_obj.ts = util.TimeStamp.get_ts2(m.group(6) + ':0')
        syscall_obj.identifier = m.group(7)
        args = m.group(8)
        args = string.split(args, ',')
        # do some preprocessing of the argument list
        args = map(lambda x: string.strip(x), args) # strip whitespace
        args = map(lambda x: string.strip(x, '"'), args) # strip double quote
        if syscall_obj.identifier == Syscall.SYS_OPEN:
            flags = args[1].split('|')

#TODO
            print flags 

            raise NameError("THIS IS THE POINT WHERE YOU LEFT")


        syscall_obj.args = args
        syscall_obj.exit = int(m.group(9))
        if syscall_obj.exit >= 0:
            syscall_obj.success = True
        else:
            syscall_obj.success = False
        return syscall_obj

    def srcPath(self):
        pathname = self.args[0]
        (pathname_dir, pathname_file) = os.path.split(pathname)

        # seer format does not provide inode numbers so we use pathname to 
        # identify the dir/file (accepting the fact of false negatives due
        # to aliasing)
        dir = (pathname_dir, pathname_dir)
        file = (pathname_file, pathname_file)
        return (dir, file)

    def dstPath():
        pathname = self.args[1]
        (pathname_dir, pathname_file) = os.path.split(pathname)

        # seer format does not provide inode numbers so we use pathname to 
        # identify the dir/file (accepting the fact of false negatives due
        # to aliasing)
        dir = (pathname_dir, pathname_dir)
        file = (pathname_file, pathname_file)
        return (dir, file)
    
