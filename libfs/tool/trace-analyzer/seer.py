import sys
import re
import os
import string
import collections
import bisect
import analyzer
import seeranalyzer


def main(argv):

    al = analyzer.Analyzer(seeranalyzer.SeerAnalyzer)
    audit_log = argv[0];
    af = open(audit_log, 'r')
    lines = af.readlines()
    nlines = len(lines)
    for i, l in enumerate(lines):
        if len(l) == 0: # empty line; continue with the next one
            continue
        # there are cases when objdump may break an entry into two lines.
        # identify such cases and concatenate the two lines into a single one
        if re.search("[0-9]+ UID", lines[i]) and i+1 < nlines \
          and not re.search("[0-9]+ UID", lines[i+1]):
            line = string.strip(lines[i]) + string.strip(lines[i+1])
            lines[i+1] = '' # consume the second line so that we don't parse it again
        else:
            line = lines[i]
        print line
        syscall_obj = seeranalyzer.Syscall.Parse(al, line)
        if syscall_obj and syscall_obj.success:
            al.Syscall(syscall_obj)
    al.Report()


if __name__ == "__main__":
    main(sys.argv[1:])
