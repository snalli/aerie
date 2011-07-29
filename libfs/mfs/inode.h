#ifndef _INODE_H_JAK129
#define _INODE_H_JAK129

#include <stdint.h>


//FIXME: this class should be abstract class
class Inode {
public:
	virtual int Init(uint64_t ino) { return 0; };
	virtual int Lookup(char* name) { return 0; };
	//FIXME: virtual int LookupOptimistic(char* name) { return 0; };
	virtual int Insert(char* name, Inode* inode) { return 0; };
private:


};


class InodeImmutable: public Inode {
public:
	
	static InodeImmutable* ino2obj(uint64_t ino) {
		return reinterpret_cast<InodeImmutable*> (ino);
	}

private:


};


class InodeMutable: public Inode {
public:
	
	

protected:
	uint64_t ino_;
};



#endif /* _INODE_H_JAK129 */
