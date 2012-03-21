#ifndef __STAMNOS_BCS_COMMON_SHARED_BUFFER_H	
#define __STAMNOS_BCS_COMMON_SHARED_BUFFER_H	

class SharedBufferDescriptor {
public:
	SharedBufferDescriptor()
	{ }

	SharedBufferDescriptor(int id, std::string path, size_t size)
		: id_(id),
		  path_(path),
		  size_(size)
	{ }

	int          id_;   // capability: identifier private to a client
	std::string  path_;
	unsigned int size_;
};

#endif // __STAMNOS_BCS_COMMON_SHARED_BUFFER_H	
