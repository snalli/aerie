#ifndef _INODE_H_JAK129
#define _INODE_H_JAK129

#include <stdint.h>
#include <stdio.h>

namespace client {

class SuperBlock;

//FIXME: this class should be abstract class
class Inode {
public:
	virtual int Init(uint64_t ino) = 0;
	virtual int Open(char* path, int flags) = 0;
	virtual int Lookup(char* name, Inode** inode) = 0;
	virtual int LookupFast(char* name, Inode* inode) = 0;
	virtual int Insert(char* name, Inode* inode) = 0;
	virtual int Link(char* name, Inode* inode, bool overwrite) = 0;

	virtual client::SuperBlock* GetSuperBlock() = 0;
	virtual void SetSuperBlock(client::SuperBlock* sb) = 0;

	virtual uint64_t GetInodeNumber() { return ino_; };
	virtual void SetInodeNumber(uint64_t ino) { ino_ = ino; };

protected:
	uint64_t ino_;
};

/*

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
};


*/

} // namespace client


#endif /* _INODE_H_JAK129 */
