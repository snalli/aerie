import sys
import re
import os
import collections
import bisect
import analyzer
import seersyscall


def main(argv):
    al = analyzer.Analyzer(seersyscall.Syscall)
    audit_log = argv[0];
    af = open(audit_log, 'r')
    for l in af.readlines():
        syscall_obj = seersyscall.Syscall.Parse(l)
        if syscall_obj and syscall_obj.success:
            al.Syscall(syscall_obj)

    if al.filespace:
        al.filespace.Report()
    if al.namespace:
        al.namespace.List()        


if __name__ == "__main__":
    main(sys.argv[1:])
