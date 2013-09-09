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
        e = e.strip()
        # ignore empty lines
        if len(e) == 0:
           continue 
        # ignore anything after a # character
        char_pound = e.find('#')
	if char_pound == 0:
           continue
        if char_pound > 0:
           e = e[0:char_pound-1]
        if e[0] == 'm':
            module_name = (e.split(',')[1]).strip()
            continue
        elif e[0] == 's':
            name = (e.split(',')[1]).strip()
            sym2mod_map[name] = module_name
        elif e[0] == 'f':
            name = (e.split(',')[1]).strip()
            file2mod_map[name] = module_name

    return sym2mod_map, file2mod_map


def get_symbol_samples(filename):
    samples = open(filename).readlines()
    user_symbol_samples = []
    kernel_symbol_samples = []
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
        name = sa[5]
        if mode == "[k]":
            kernel_symbol_samples.append((name, num))
        else:
            user_symbol_samples.append((name, num))
    return user_symbol_samples, kernel_symbol_samples


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


def print_symbol(name, perc_abs, perc_rel):
    print name.ljust(30), 
    if type (perc_abs) == int or type (perc_abs) == float:
        perc_abs = str(perc_abs)
    print perc_abs.ljust(20),
    if type (perc_rel) == int or type (perc_rel) == float:
        perc_rel = str(perc_rel)
    print perc_rel.ljust(20)


def main(argv):
    user_symbol_samples, kernel_symbol_samples = get_symbol_samples(argv[0])
    sym2mod, file2mod = get_modinvmap(argv[1])
    names = name_lookup_harvester()
    by_name = by_name_harvester()
    tagfile = ctags_file(argv[2], harvesters=[names, by_name])
    ctags = by_name.get_data()
    user_total_samples = 0
    for s in user_symbol_samples:
        user_total_samples = user_total_samples + s[1]
    kernel_total_samples = 0
    for s in kernel_symbol_samples:
        kernel_total_samples = kernel_total_samples + s[1]
    total_samples = user_total_samples + kernel_total_samples
    kernel_module_samples = categorize_samples(sym2mod, file2mod, ctags, kernel_symbol_samples)
    kernel_sorted_module_samples = sorted(kernel_module_samples.iteritems(), key=operator.itemgetter(1), reverse=True)
    if total_samples == 0:
	print 'No samples...'
	return
    print_symbol('NAME', '%', '% (relative)')
    print_symbol('USER', 100*float(user_total_samples)/float(total_samples), 100)
    print_symbol('KERNEL', 100*float(kernel_total_samples)/float(total_samples), 100)
    for m in kernel_sorted_module_samples:
        perc_abs = 100*float(m[1])/float(total_samples)
        perc_rel = 100*float(m[1])/float(kernel_total_samples)
        print_symbol(m[0], perc_abs, perc_rel)

if __name__ == "__main__":
    main(sys.argv[1:])
