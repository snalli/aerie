#ifndef _INODE_H_JAK129
#define _INODE_H_JAK129

#include <stdint.h>
#include <stdio.h>

namespace client {

typedef uint64_t InodeNumber;

class SuperBlock;

//FIXME: this class should be abstract class
class Inode {
public:
	virtual int Init(InodeNumber ino) = 0;
	virtual int Open(char* path, int flags) = 0;
	virtual int Write(char* src, uint64_t off, uint64_t n) = 0;
	virtual int Read(char* dst, uint64_t off, uint64_t n) = 0;
	virtual int Lookup(char* name, Inode** inode) = 0;
	virtual int LookupFast(char* name, Inode* inode) = 0;
	virtual int Insert(char* name, Inode* inode) = 0;
	virtual int Link(char* name, Inode* inode, bool overwrite) = 0;

	virtual client::SuperBlock* GetSuperBlock() = 0;
	virtual void SetSuperBlock(client::SuperBlock* sb) = 0;

	virtual InodeNumber GetInodeNumber() { return ino_; };
	virtual void SetInodeNumber(InodeNumber ino) { ino_ = ino; };

protected:
	InodeNumber ino_;
};

/*

class InodeImmutable: public Inode {
public:
	
	static InodeImmutable* ino2obj(uint64_t ino) {
		return reinterpret_cast<InodeImmutable*> (ino);
	}

private:


};


*/

} // namespace client


#endif /* _INODE_H_JAK129 */
