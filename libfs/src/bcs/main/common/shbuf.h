#ifndef __STAMNOS_BCS_COMMON_SHARED_BUFFER_H	
#define __STAMNOS_BCS_COMMON_SHARED_BUFFER_H	

class SharedBufferDescriptor {
public:
	SharedBufferDescriptor()
	{ }

	SharedBufferDescriptor(std::string path, size_t size)
		: path_(path),
		  size_(size)
	{ }

	std::string  path_;
	unsigned int size_;
};

#endif // __STAMNOS_BCS_COMMON_SHARED_BUFFER_H	
