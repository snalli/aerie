import sys
import re
import os
import collections
import bisect
import analyzer
import auditanalyzer


def main(argv):
    al = analyzer.Analyzer()
    audit_log = argv[0];
    af = open(audit_log, 'r')
    al = af.readlines()
    pl = []
    if len(argv) > 1:
        process_log = argv[1];
        pf = open(process_log, 'r')
        pl = pf.readlines()
    for i in range(len(al)):
        syscall_obj = auditanalyzer.Syscall.Parse(al, i)
        al.Syscall(syscall_obj)
        if syscall_obj:
            al.KillProcessOlderThan(pl, syscall_obj.ts)

    al.Report()


if __name__ == "__main__":
    main(sys.argv[1:])
