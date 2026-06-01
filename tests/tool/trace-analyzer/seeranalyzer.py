# ANALYZER FRONT END: 
# parses a SEER syscall entry for further analysis by the analyzer backend

import string
import sys
import re
import os
import collections
import bisect
import util

# inode that identifies a file
# SEER does not provide with the inode number of each file so we need to build 
# a unique inode to identify each file across path-name changes

class Inode:
    def __init__(self, name):
        self.children = {} # str_name -> Inode

    def Unlink(self, name):
        if not self.children.has_key(name):
            return
        del self.children[name]
    
    def Link(self, old_parent_inode, old_name, new_name):
        if not old_parent_inode.children.has_key(old_name):
            return
        inode = old_parent_inode.children[old_name]
        self.children['foo'] = inode
    
    def Rename(self, old_name, new_parent_inode, new_name):
        if not self.children.has_key(old_name):
            return
        inode = self.children[old_name]
        new_parent_inode.children[new_name] = inode
        del self.children[old_name]

    def Child(self, name, create):
        if self.children.has_key(name):
            node = self.children[name]
        else:
            if create:
                node = self.children[name] = Inode(name)
            else:
                node = None
        return node



# tracks paths to inodes
class InodeSpace:
    def __init__(self):
        self.root = Inode('$ROOT')

    # accesses each name component of a full path name and shadows it with an
    # inode
    # returns the inode objects corresponding to the last and second to last 
    # component of the pathname
    def Lookup(self, pathname, create=True):
        parent_last_inode = None
        last_inode = None
        inode = self.root
        pathname_split = pathname.split('/')
        pathname_split_len = len(pathname_split)
        for i, name in enumerate(pathname_split):
            # check '.'; don't need to check for '..' as it doesn't appear
            # in a normalized pathname
            if name == '.':
                inode = inode
            if name != '':
                inode = inode.Child(name, create)
            if i == pathname_split_len - 1: # last component
                last_inode = inode
            elif i == len(pathname_split)-2: # second-to-last component, a.k.a parent of last
                parent_last_inode = inode

        return (parent_last_inode, last_inode)
       

    @staticmethod
    def DFS(start, path, dict):
        for key, val in start.children.iteritems():
            dict[val] = path+'/'+key
            InodeSpace.DFS(val, path+'/'+key, dict)

    def Print(self):
        dict = {}
        InodeSpace.DFS(self.root, '', dict)

    def Inode2Path(self):
        dict = {}
        InodeSpace.DFS(self.root, '', dict)
        return dict


    # pathname must be absolute path 
    def PathTuple(self, pathname, inode_dir, inode_file):
        (pathname_dir, pathname_file) = os.path.split(pathname)
        dir = (pathname_dir, inode_dir)
        file = (pathname_file, inode_file)
        return (dir, file)

    def SyscallOpen(self, syscall):
        pathname = util.absPath(syscall.cwd, syscall.args[0])
        (parent_inode, inode) = self.Lookup(pathname)
        path_tuple = self.PathTuple(pathname, parent_inode, inode)
        syscall.paths.append(path_tuple)

    def SyscallRename(self, syscall):
        old_path = util.absPath(syscall.cwd, syscall.args[0])
        new_path = util.absPath(syscall.cwd, syscall.args[1])
        (old_path_dir, old_path_file) = os.path.split(old_path)
        (new_path_dir, new_path_file) = os.path.split(new_path)
        (old_parent_inode, inode) = self.Lookup(old_path)
        (new_parent_inode, tmp_inode) = self.Lookup(new_path)
        if not old_parent_inode:
            return
        old_parent_inode.Rename(old_path_file, new_parent_inode, new_path_file)
        path_tuple = self.PathTuple(old_path, old_parent_inode, inode)
        syscall.paths.append(path_tuple)
        path_tuple = self.PathTuple(new_path, new_parent_inode, inode)
        syscall.paths.append(path_tuple)

    def SyscallLink(self, syscall):
        old_path = util.absPath(syscall.cwd, syscall.args[0])
        new_path = util.absPath(syscall.cwd, syscall.args[1])
        (old_path_dir, old_path_file) = os.path.split(old_path)
        (new_path_dir, new_path_file) = os.path.split(new_path)
        (old_parent_inode, inode) = self.Lookup(old_path)
        (new_parent_inode, tmp_inode) = self.Lookup(new_path)
        new_parent_inode.Link(old_parent_inode, old_path_file, new_path_file)
        path_tuple = self.PathTuple(old_path, old_parent_inode, inode)
        syscall.paths.append(path_tuple)
        path_tuple = self.PathTuple(new_path, new_parent_inode, inode)
        syscall.paths.append(path_tuple)

    def SyscallUnlink(self, syscall):
        path = util.absPath(syscall.cwd, syscall.args[0])
        (path_dir, path_file) = os.path.split(path)
        (parent_inode, inode) = self.Lookup(path)
        if not parent_inode or not inode:
            return
        parent_inode.Unlink(path_file)
        path_tuple = self.PathTuple(path, parent_inode, inode)
        syscall.paths.append(path_tuple)

    def Update(self, syscall):
        options = {
            Syscall.SYS_OPEN: self.SyscallOpen,
            Syscall.SYS_RENAME: self.SyscallRename,
            Syscall.SYS_LINK: self.SyscallLink,
            Syscall.SYS_UNLINK: self.SyscallUnlink,
        }
        if options.has_key(syscall.identifier):
            options[syscall.identifier](syscall)
    

# parses a syscall entry
class Syscall:

    SYS_OPEN    = 'open'
    SYS_CLOSE   = 'close'
    SYS_READ    = 'read'
    SYS_WRITE    = 'write'
    SYS_RENAME  = 'rename'
    SYS_CHDIR   = 'chdir'
    SYS_MKDIR   = 'mkdir'
    SYS_RMDIR   = 'rmdir'
    SYS_CREAT   = 'creat'
    SYS_LINK    = 'link'
    SYS_UNLINK  = 'unlink'
    SYS_FORK    = 'fork'
    SYS_EXIT    = 'exit'
    SYS_DUP     = 'dup'
    SYS_DUP2    = 'dup2'
    
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
    def Parse(analyzer, line):
        m = re.match("([0-9]+) UID ([0-9]+) PID ([0-9]+) (.+) ([A-Z]{1}) ([0-9]+.[0-9]+) ([0-9a-z]+)\((.+)\) = ([-0-9]+)(.*)", line)
        if not m:
            m = re.match("[0-9]+ RESTART", line)
            if m:
                return None
            else:
                # ERROR: Unknown SEER entry format
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
        syscall_obj.paths = []
        process = analyzer.process_map.Process(syscall_obj.pid, True)
        syscall_obj.cwd = process.cwd  # cwd before executing the system call
        if syscall_obj.exit >= 0:
            analyzer.inode_space.Update(syscall_obj)
            syscall_obj.success = True
        else:
            syscall_obj.success = False
        return syscall_obj

    def srcPath(self):
        if len(self.paths) > 0:
            return self.paths[0]
        else:
            return ((None, None), (None, None))

    def dstPath(self):
        if len(self.paths) > 1:
            return self.paths[1]
        else:
            return ((None, None), (None, None))

class SeerAnalyzer:
    syscall_class = Syscall
    inode_space = InodeSpace()
    
