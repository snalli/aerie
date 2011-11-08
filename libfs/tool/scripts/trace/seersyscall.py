# parses a SEER syscall entry

import string
import sys
import re
import os
import collections
import bisect
import util


# FIXME: replace os.join with our join. the problem is that 
# os.join does not normalize path: dots may stay present. 
# for example: os.join("/A/B", "./C") gives /A/B/./C
# if we use the pathname to identify a file then /A/B/.C/ should be the same as /A/B/C

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
    
    O_RDWR    = 0x2
    O_WRONLY  = 0x1
    O_RDONLY  = 0x0
    O_CREAT   = 0x0100
    O_TRUNC   = 0x1000
    O_APPEND  = 0x2000

    open_flags = {
        'O_RDWR': O_RDWR,
        'O_WRONLY': O_WRONLY,
        'O_RDONLY': O_RDONLY,
        'O_CREAT': O_CREAT,
        'O_TRUNC': O_TRUNC,
        'O_APPEND': O_APPEND
    }

    def __init__(self):
        pass


    # factory: parses a string and creates a Syscall object
    @staticmethod
    def Parse(line):
        m = re.match("([0-9]+) UID ([0-9]+) PID ([0-9]+) (.+) ([A-Z]{1}) ([0-9]+.[0-9]+) ([0-9a-z]+)\((.+)\) = ([-0-9]+)(.*)", line)
        if not m:
            m = re.match("[0-9]+ RESTART", line)
            if m:
                return None
            else:
                print line
                m = re.match("([0-9]+) UID ([0-9]+) PID ([0-9]+) (.+) ([A-Z]{1}) ([0-9]+.[0-9]+)", line)
                print m.group(7)
                raise NameError("Unknown SEER entry: %s" % line)
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
            flags_int = 0
            for f in flags:
                if f in Syscall.open_flags:
                    flags_int |= Syscall.open_flags[f]
            args[1] = flags_int       
        syscall_obj.args = args
        syscall_obj.exit = int(m.group(9))
        if syscall_obj.exit >= 0:
            syscall_obj.success = True
        else:
            syscall_obj.success = False
        return syscall_obj

    def __Path(self, pathname):
        (pathname_dir, pathname_file) = os.path.split(pathname)
        # seer format does not provide inode numbers so we use pathname to 
        # identify the dir/file (accepting the fact of false negatives due
        # to aliasing)
        dir = (pathname_dir, pathname_dir)
        file = (pathname_file, pathname_file)
        return (dir, file)

    def srcPath(self):
        pathname = self.args[0]
        return self.__Path(pathname)

    def srcAbsPath(self, cwd=None):
        pathname = self.args[0]
        # make path absolute
        if pathname[0] != '/': 
            if cwd == None:
                return (None, None)
            pathname = os.path.join(cwd, pathname)
        return self.__Path(pathname)

    def dstPath(self):
        pathname = self.args[1]
        return self.__Path(pathname)

    def dstAbsPath(self, cwd=None):
        pathname = self.args[1]
        # make path absolute
        if pathname[0] != '/': 
            if cwd == None:
                return (None, None)
            pathname = os.path.join(cwd, pathname)
        return self.__Path(pathname)

   
