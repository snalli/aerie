#include "fileset.h"
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <queue>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string.h>

/*
 * Removes the last file or directory name from a pathname.
 * Basically removes characters from the end of the path by
 * setting them to \0 until a forward slash '/' is
 * encountered. It also removes the forward slash.
 */
static char *
trunc_dirname(char *dir)
{
	char *s = dir + strlen(dir);

	while (s != dir) {
		int c = *s;

		*s = 0;
		if (c == '/')
			break;
		s--;
	}
	return (dir);
}


/*
 * Creates multiple nested directories as required by the
 * supplied path. Starts at the end of the path, creating
 * a list of directories to mkdir, up to the root of the
 * path, then mkdirs them one at a time from the root on down.
 */
int
mkdir_r(std::string path, int mode)
{
        char tmp[256];
        char *p = NULL;
        size_t len;
 
        strcpy(tmp, path.c_str());
        len = strlen(tmp);
        if(tmp[len - 1] == '/') {
                tmp[len - 1] = 0;
	}
        for(p = tmp + 1; *p; p++) {
                if(*p == '/') {
                        *p = 0;
                        mkdir(tmp, mode);
                        *p = '/';
                }
	}
        mkdir(tmp, mode);
	return 0;
}



uint64_t rand64(unsigned int* seed)
{
	uint64_t val;
	uint16_t* tmp = reinterpret_cast<uint16_t*>(&val);
	tmp[0] = (uint16_t) ::rand_r(seed);
	tmp[1] = (uint16_t) ::rand_r(seed);
	tmp[2] = (uint16_t) ::rand_r(seed);
	tmp[3] = (uint16_t) ::rand_r(seed);
	return val;
}

FileSet::FileSet(std::string path, int nfiles, int dirwidth_inner, int dirwidth_leaf, FileState initial_state)
{
	for (int i=0; i<kNumFileStates; i++) {
		_index[i] = new Index();
	}
	_path = path;
	_seed = 1021;
	fill(nfiles, dirwidth_inner, dirwidth_leaf, initial_state);
}


int FileSet::fill(int nfiles, int dirwidth_inner, int dirwidth_leaf, FileState initial_state)
{
	FileSet::Index* fidx = _index[initial_state];
	Entry* cur_entry = new Entry(Entry::kDir, "/", 0, initial_state);
	std::queue<Entry*> bfs;
	bfs.push(cur_entry);
	int depth = ceil(log(nfiles/dirwidth_leaf) / log(dirwidth_inner));
	int files_count = 0;
	while (bfs.size()) {
		cur_entry = bfs.front();
		bfs.pop();
		if (cur_entry->_level == depth) {
			// create leaf files
			for (int i=0; i<dirwidth_leaf; i++) {
				std::stringstream ss;
				ss << cur_entry->_name;
				ss << std::setfill('0') << std::setw(10) << i;
				Entry* entry = new Entry(Entry::kFile, ss.str(), cur_entry->_level + 1, initial_state);
				fidx->insert(entry);
				files_count++;
			}
			if (files_count >= nfiles) {
				break;	
			}
		} else {
			// create inner directories
			for (int i=0; i<dirwidth_inner; i++) {
				std::stringstream ss;
				ss << cur_entry->_name;
				ss << std::setfill('0') << std::setw(10) << i;
				ss << "/";
				Entry* entry = new Entry(Entry::kDir, ss.str(), cur_entry->_level+1, initial_state);
				fidx->insert(entry);
				bfs.push(entry);
			}
		}
	}
	return 0;
}

int FileSet::pick_file(FileState state, std::string* path, Entry** entryp)
{	
	Entry* entry;

	do {
		_index[state]->pick_random(&_seed, &entry);
	} while (entry->_type != Entry::kFile);

	*path = _path + entry->_name;
	if (entryp) {
		*entryp = entry;
	}
	return 0;
}


int FileSet::set_state(Entry* entry, FileState state)
{
	_index[entry->_state]->remove(entry);
	_index[state]->insert(entry);
	entry->_state = state;
}

void FileSet::print()
{
	_index[kExists]->print(); 
}

void FileSet::create_files()
{
	std::map<std::string, Entry*>::iterator itr;
	std::map<std::string, Entry*>::iterator next;
	for (itr = next = _index[kNotExists]->_map.begin(); itr != _index[kNotExists]->_map.end(); itr = next) {
		next++; // set_state below erases the element so we need to 
		        // advance the iterator before removal
		Entry* entry = itr->second;
		entry->create_file(_path);
		set_state(entry, kExists);
	}
}

FileSet::Index::Index()
{ }

int FileSet::Index::insert(Entry* entry)
{
	_map.insert(std::pair<std::string, Entry*>(entry->_name, entry));
	entry->_loc = _vec.size();
	_vec.push_back(entry);
	return 0;
}


int FileSet::Index::remove(Entry* entry)
{
	Entry* last_entry = _vec.back();
	_map.erase(entry->_name);
	last_entry->_loc = entry->_loc;
	_vec[entry->_loc] = last_entry;
	_vec.pop_back();
	return 0;
}


int FileSet::Index::pick_random(unsigned int* seed, Entry** entry)
{
	uint64_t i = rand64(seed) % _vec.size();
	*entry = _vec[i];
	return 0;
}


int FileSet::Index::find(const char* path, Entry** entry)
{
	std::map<std::string, Entry*>::iterator it;
	it = _map.find(path);
	*entry = it->second;
	return 0;
}


void FileSet::Index::print()
{
	std::map<std::string, Entry*>::iterator itr;
	for (itr = _map.begin(); itr != _map.end(); itr++) {
		std::cout << itr->second->_name << std::endl;
	}
}

FileSet::Entry::Entry(Entry::Type type, std::string name, int level, FileState initial_state)
	: _type(type),
	  _name(name),
	  _level(level),
	  _state(initial_state)
{ }

int FileSet::Entry::create_file(std::string prefix)
{
	if (_type == kDir) {
		mkdir_r(prefix + _name, S_IRWXU);
	} else {
		std::string fullname = prefix + _name; 
		int fd = open(fullname.c_str(), O_CREAT | O_WRONLY, S_IRWXU);
		close(fd);
	}
	
}
