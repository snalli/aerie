import fnmatch
import os
from pwd import getpwuid
from grp import getgrgid


from stat import *

def protbits2str(protbits):
    protstr=[]
    for i in range(0,9):
        protstr.append('-')
    if protbits & S_IRUSR:
        protstr[0] = 'r'
    if protbits & S_IWUSR:
        protstr[1] = 'w'
    if protbits & S_IXUSR:
        protstr[2] = 'x'
    if protbits & S_IRGRP:
        protstr[3] = 'r'
    if protbits & S_IWGRP:
        protstr[4] = 'w'
    if protbits & S_IXGRP:
        protstr[5] = 'x'
    if protbits & S_IROTH:
        protstr[6] = 'r'
    if protbits & S_IWOTH:
        protstr[7] = 'w'
    if protbits & S_IXOTH:
        protstr[8] = 'x'
    return "".join(protstr)

def print_acl(fcount, acl_map):
    acl_len = 0
    print 
    print fcount, "files walked"
    for acl in acl_map.keys():
        acl_len = acl_len + 1
    print acl_len, "ACLs found"
    print "protbits", "user", "group", "acl", "#files"
    for acl in acl_map:
        print protbits2str(acl[0]), getpwuid(acl[1])[0], getgrgid(acl[2])[0], acl, acl_map[acl]

def walk_acl(rootPath):
    acl_map = {}
    fcount = 0
    for root, dirs, files in os.walk(rootPath):
        for filename in files:
            fcount = fcount + 1
            if fcount % 10000 == 0:
                print_acl(fcount, acl_map)
            pathname = os.path.join(root, filename)
            try:
                stat = os.stat(pathname)
                mode = stat[ST_MODE]
                uid = stat[ST_UID]
                gid = stat[ST_GID]
                protection_bits = mode & (S_IRWXU | S_IRWXG | S_IRWXO)
                if (protection_bits, uid, gid) not in acl_map:
                    acl_map[(protection_bits, uid, gid)] = 1
                else:
                    acl_map[(protection_bits, uid, gid)] = acl_map[(protection_bits, uid, gid)] + 1

            except:
                continue

    print_acl(fcount, acl_map)

walk_acl('/')
