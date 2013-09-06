#ifndef _FILESET_H
#define _FILESET_H

#include <map>
#include <string>
#include <vector>


class FileSet {
public:
	class Index;
	class Entry;
	
	enum FileState {
		kNone = 0,
		kNotExists,
		kExists,
		kNumFileStates
	};

	FileSet(std::string path, int nfiles, int dirwidth_inner, int dirwidth_leaf, FileState initial_state);
	int fill(int nfiles, int dirwidth_inner, int dirwidth_leaf, FileState initial_state);
	int pick_file(FileState state, std::string* path, Entry** entryp);
	int set_state(Entry* entry, FileState state);
	
	void print();
	void create_files();
	
private:
	std::string  _path; // pathname prefix in fileset
	Index*       _index[kNumFileStates];
	unsigned int _seed;
};


class FileSet::Index {
	friend class FileSet;
public:
	Index();
	int remove(Entry* entry);
	int insert(Entry* entry);
	int pick_random(unsigned int* seed, Entry** entry);
	int find(const char* path, Entry** entry);

	void print();
private:
	std::map<std::string, Entry*> _map;
	std::vector<Entry*> _vec; // used to pick up an entry at random
};

class FileSet::Entry {
public:
	enum Type {
		kDir = 1,
		kFile
	};
	Entry(Entry::Type type, std::string name, int level, FileState initial_state);
	int create_file(std::string prefix);

	std::string _name;
	Type        _type;
	int         _loc; // location within the vector of entries 
	int         _level;
	FileState   _state;
};

#endif
