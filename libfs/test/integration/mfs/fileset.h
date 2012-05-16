#ifndef __STAMNOS_TEST_FILESET_H
#define __STAMNOS_TEST_FILESET_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>


class FileSet {
public:
	typedef std::vector<std::string>::iterator iterator;

	static int Create(const char* root, int dirwidth, int dirdepth, int dirfanout, FileSet* fileset);
	int MoveRandom(int n, FileSet* dstfileset);
	int Copy(int n, FileSet* dst);

	int GetRandomInternal(std::string* str, bool erase, bool has_lock)
	{
		if (!has_lock) {
			pthread_mutex_lock(&mutex_);
		}
		int k = rand() % vec_.size();
		*str = vec_[k];
		if (erase) {
			vec_.erase(vec_.begin() + k);
		}
		if (!has_lock) {
			pthread_mutex_unlock(&mutex_);
		}
		return 0;
	}
	
	int GetRandom(std::string* str, bool has_lock = false)
	{
		return GetRandomInternal(str, false, has_lock);
	}


	int EraseRandom(std::string* str, bool has_lock = false)
	{
		return GetRandomInternal(str, true, has_lock);
	}


	void Put(std::string filename, bool has_lock = false)
	{
		if (!has_lock) {
			pthread_mutex_lock(&mutex_);
		}
		vec_.push_back(filename);
		if (!has_lock) {
			pthread_mutex_unlock(&mutex_);
		}
	}

	void Print();
	
	int Size() { return vec_.size(); }

	iterator begin() { return vec_.begin(); }	
	iterator end() { return vec_.end(); }	

private:
	std::vector<std::string> vec_;
	pthread_mutex_t          mutex_;
};


inline std::string ExtractDirectory(const std::string& path)
{
	return path.substr(0, path.find_last_of('/') + 1);
}

inline std::string ExtractFilename(const std::string& path)
{
	return path.substr(path.find_last_of('/') + 1);
}

#endif // __STAMNOS_TEST_FILESET_H
