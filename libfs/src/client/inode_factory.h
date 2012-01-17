#ifndef __STAMNOS_CLIENT_INODE_FACTORY_H
#define __STAMNOS_CLIENT_INODE_FACTORY_H

namespace client {

class InodeFactory {
public:
	virtual int Make() = 0;
	virtual int Load() = 0;

private:

};

} // namespace client

#endif // __STAMNOS_CLIENT_INODE_FACTORY_H
