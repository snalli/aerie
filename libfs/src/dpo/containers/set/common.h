#ifndef __STAMNOS_DPO_SET_CONTAINER_COMMON_H
#define __STAMNOS_DPO_SET_CONTAINER_COMMON_H

#include "dpo/main/common/obj.h"
#include "dpo/containers/byte/common.h"

namespace dpo {
namespace containers {
namespace common {

template<typename T>
class SetContainer {
public:
template<typename Session>
class Object: public dpo::cc::common::Object {
	typedef dpo::containers::common::ByteContainer::Object<Session>  ByteContainer;

public:
	static Object* Make(Session* session, volatile char* b) {
		//TODO: Initialize
		return new ((void*) b) Object();
	}

	static Object* Load(dpo::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}

	Object()
	{ 
		set_type(T_SET_CONTAINER);
		size_ = 0;
	}

	int Insert(Session* session, T val);
	int Read(Session* session, int pos, T* val);
	int Size();

private:	
	ByteContainer byte_container_;
	int size_;
};


}; // class SetContainer


template<typename T>
template<typename Session>
int 
SetContainer<T>::Object<Session>::Insert(Session* session, T val)
{
	uint64_t size = size_++;
	char* c = (char*) &val;
/*
	printf("INSERT: c[0]=%x\n", c[0]);
	printf("INSERT: c[1]=%x\n", c[1]);
	printf("INSERT: c[2]=%x\n", c[2]);
	printf("INSERT: c[3]=%x\n", c[3]);
*/
	return byte_container_.Write(session, c, size*sizeof(T), sizeof(T));
}


template<typename T>
template<typename Session>
int 
SetContainer<T>::Object<Session>::Read(Session* session, int pos, T* val)
{
	char c[8];
	int ret = byte_container_.Read(session, c, pos*sizeof(T), sizeof(T));
	//return byte_container_.Read(session, (char*)val, pos*sizeof(T), sizeof(T));
	
	printf("READ: c[0]=%x\n", c[0]);
	printf("READ: c[1]=%x\n", c[1]);
	printf("READ: c[2]=%x\n", c[2]);
	printf("READ: c[3]=%x\n", c[3]);
	printf("READ: c[4]=%x\n", c[4]);
	printf("READ: c[5]=%x\n", c[5]);
	printf("READ: c[6]=%x\n", c[6]);
	printf("READ: c[7]=%x\n", c[7]);
	memcpy(val, c, sizeof(T));
	return ret;
}


template<typename T>
template<typename Session>
int 
SetContainer<T>::Object<Session>::Size()
{
	return byte_container_.Size() / sizeof(T);
}




} // namespace common
} // namespace containers
} // namespace dpo

#endif // __STAMNOS_DPO_SET_CONTAINER_COMMON_H
