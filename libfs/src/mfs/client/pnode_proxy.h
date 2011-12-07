#ifndef _PNODE_PROXY_H_KLA190

class PnodeProxy {
public:
	virtual int Init(client::Session* session, InodeNumber ino) = 0;
	virtual int Write(client::Session* session, char* src, uint64_t off, uint64_t n) = 0;
	virtual int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) = 0;
	virtual int Lookup(client::Session* session, const char* name, InodeNumber* ino) = 0;
	virtual int Link(client::Session* session, const char* name, InodeNumber ino, bool overwrite) = 0;
	virtual int Unlink(client::Session* session, const char* name) = 0;
	virtual int Publish(client::Session* session) = 0;
};

#endif
