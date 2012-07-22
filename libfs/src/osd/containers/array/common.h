#ifndef __STAMNOS_OSD_ARRAY_CONTAINER_COMMON_H
#define __STAMNOS_OSD_ARRAY_CONTAINER_COMMON_H

#include "osd/main/common/obj.h"

namespace osd {
namespace containers {
namespace common {


#define ARRAY_CONTAINER_SIZE 2

template<typename T>
class ArrayContainer {
public:
template<typename Session>
class Object: public osd::cc::common::Object {
	typedef osd::containers::common::ByteContainer::Object<Session>  ByteContainer;

public:
	static Object* Make(Session* session, volatile char* b) {
		//TODO: Initialize
		return new ((void*) b) Object();
	}

	static Object* Load(osd::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}

	Object()
	{ 
		set_type(T_ARRAY_CONTAINER);
	}

	int Read(Session* session, int index, T* val);
	int Write(Session* session, int index, T val);
	int Size();

private:	
	T array_[ARRAY_CONTAINER_SIZE];
};


}; // class ArrayContainer



template<typename T>
template<typename Session>
int 
ArrayContainer<T>::Object<Session>::Read(Session* session, int index, T* val)
{
	*val = array_[index];

	return E_SUCCESS;
}


template<typename T>
template<typename Session>
int 
ArrayContainer<T>::Object<Session>::Write(Session* session, int index, T val)
{
	array_[index] = val;
	return E_SUCCESS;
}


template<typename T>
template<typename Session>
int 
ArrayContainer<T>::Object<Session>::Size()
{
	return ARRAY_CONTAINER_SIZE;
}


// Dan Bernstein's hash function
static unsigned long 
ArrayHash(const char *str)
{
	unsigned long hash = 5381;
	int c;
	char* tmp_str = const_cast<char*>(str);

	while (c = *tmp_str++) {
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

} // namespace common
} // namespace containers
} // namespace osd

#endif // __STAMNOS_OSD_ARRAY_CONTAINER_COMMON_H
