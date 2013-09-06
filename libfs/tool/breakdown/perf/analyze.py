import sys

from pyctags import exuberant_ctags, ctags_file
from pyctags.harvesters import name_lookup_harvester, by_name_harvester
from pyctags import tag_entry
import os
import operator


def get_modlist(filename):
    entries = open(filename).readlines()
    modlist = []
    for e in entries:
        if e[0] == 'm':
            module_name = (e.split(',')[1]).strip()
            modlist.extend(module_name)
    return modlist


# Return inverse module maps:
#   sym2mod_map: symbol to module
#   file2mod_map: source code file to module
#
def get_modinvmap(filename):
    entries = open(filename).readlines()
    file2mod_map = {}
    sym2mod_map = {}
    module_name = None
    for e in entries:
        if e[0] == 'm':
            module_name = (e.split(',')[1]).strip()
            continue
        namestrip = e.strip()
        if len(namestrip) > 0:
            namelst = namestrip.split(',')
            name = namelst[1]
            if namelst[0] == 's':
                sym2mod_map[name] = module_name
            elif namelst[0] == 'f':
                file2mod_map[name] = module_name

    return sym2mod_map, file2mod_map


def get_symbol_samples(filename, user, kernel, threshold, cumulative_threshold):
    samples = open(filename).readlines()
    cumulative = 0
    total = 0
    symbol_samples = []
    for sample in samples:
        # ignore empty lines and lines that start with '#'
        if sample[0] == '#':
            continue
        sa = sample.split()
        if len(sa) == 0:
            continue
        perc = float(sa[0][:-1]) # ignore the '%' at the end
        num = int(sa[1])
        mode = sa[4]
        if mode == "[k]" and kernel == False:
            continue
        if mode != "[k]" and user == False:
            continue
        name = sa[5]
        cumulative = cumulative + perc
        if (perc < threshold):
            break
        if (cumulative > cumulative_threshold):
            break
        symbol_samples.append((name, num))
    return symbol_samples

def categorize_samples(sym2mod, file2mod, ctags, symbol_samples):
    samples_dict = {}
    for s in symbol_samples:
        sname = s[0]
        snum = s[1]
        if sname in sym2mod:
            modname = sym2mod[sname]
        elif sname in ctags:
            file = ctags[sname][0].file
            if file in file2mod:
                modname = file2mod[file]
            else:
                modname = file
        else:
            modname = sname
        if modname in samples_dict:
            samples_dict[modname] = samples_dict[modname] + snum
        else:
            samples_dict[modname] = snum
    return samples_dict 



def main(argv):
    all_symbol_samples = get_symbol_samples(argv[0], False, True, 0, 100)
    symbol_samples = get_symbol_samples(argv[0], False, True, 0.0, 100)
    sym2mod, file2mod = get_modinvmap(argv[1])
    names = name_lookup_harvester()
    by_name = by_name_harvester()
    tagfile = ctags_file(argv[2], harvesters=[names, by_name])
    ctags = by_name.get_data()
    module_samples = categorize_samples(sym2mod, file2mod, ctags, symbol_samples)
    total_samples = 0
    for s in all_symbol_samples:
        total_samples = total_samples + s[1]
    sorted_module_samples = sorted(module_samples.iteritems(), key=operator.itemgetter(1), reverse=True)
    total_perc = 0
    print 'NAME'.ljust(30), '%'
    for m in sorted_module_samples:
        perc = 100*float(m[1])/float(total_samples)
        total_perc = total_perc + perc
        print m[0].ljust(30), perc 
    print 'TOTAL'.ljust(30),total_perc

if __name__ == "__main__":
    main(sys.argv[1:])
