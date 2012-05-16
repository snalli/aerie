#include "fileset.h"

int
FileSet::Create(const char* root, int dirwidth, int dirdepth, int dirfanout, FileSet* fileset)
{
	std::string     filename;
	char            buf[1024];
	
	if (dirdepth == 0) {
		return 0;
	}

	for (int k = 0; k < dirwidth; k++) {
		std::stringstream ss_filename;
		ss_filename << std::string(root);
		sprintf(buf, "F%07d", k);
		ss_filename << "/" << std::string(buf);
		fileset->vec_.push_back(ss_filename.str());
	}

	for (int d=0; d < dirfanout; d++) {
		std::stringstream ss_dirname;
		ss_dirname << std::string(root);
		sprintf(buf, "D%07d", d);
		ss_dirname << "/" << std::string(buf);
		Create(ss_dirname.str().c_str(), dirwidth, dirdepth-1, dirfanout, fileset);
	}

	pthread_mutex_init(&fileset->mutex_, NULL);

	return 0;
}


// Copy n filenames to dst fileset (randomly)
int
FileSet::Copy(int n, FileSet* dst) 
{
	std::string filename;

	pthread_mutex_lock(&mutex_);
	pthread_mutex_lock(&dst->mutex_);
	for (int i=0; i<n; i++) {
		dst->Put(filename);
	}
	pthread_mutex_unlock(&mutex_);
	pthread_mutex_unlock(&dst->mutex_);
	return 0;
}


// Move n filenames to dst fileset (randomly)
int
FileSet::MoveRandom(int n, FileSet* dst) 
{
	std::string filename;

	pthread_mutex_lock(&mutex_);
	pthread_mutex_lock(&dst->mutex_);
	for (int i=0; i<n; i++) {
		GetRandom(&filename, true);
		dst->Put(filename, true);
	}
	pthread_mutex_unlock(&mutex_);
	pthread_mutex_unlock(&dst->mutex_);
	return 0;
}


void
FileSet::Print()
{
	std::vector<std::string>::iterator itr;

	for (itr = vec_.begin(); itr != vec_.end(); itr++) {
		std::cout << *itr << std::endl;
	}
}


