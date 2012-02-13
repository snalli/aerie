#ifndef __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H
#define __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H

#include "ipc/backend/rpc.h"

namespace server {

/*
template<class C>
ClientDescriptor* ClientDescriptorAllocator()
{
	new C

}
*/


class ClientDescriptor {
public:
	ClientDescriptor(int clt, rpcc* cl);
	rpcc* rpc() { return rpc_; }
	
	
	void* descriptor(int n) {
		return descriptor_tbl[n];
	}

protected:
	rpcc*  rpc_;
	int    clt_;
	void** descriptor_tbl; 
};


template<class T>
class ClientDescriptorTemplate {
public:
	static T* ClientDescriptor(ClientDescriptor* dsc) {
		return reinterpret_cast<T*>(dsc->descriptor(descriptor_typeid_));
	}
	
	static void set_descriptor_typeid(int id) {
		descriptor_typeid_ = id;
	}

	static int descriptor_typeid() {
		return descriptor_typeid_;
	}
private:	
	static int descriptor_typeid_;		
};

template<class T>
int ClientDescriptorType<T>::descriptor_typeid_ = 0;


template<class A>
class ClientDescriptorTemplate1: public ClientDescriptor {
public:
	ClientDescriptorTemplate1() {
		descriptor_tbl = descriptors;
		A::set_descriptor_typeid(1);
		descriptors[A::descriptor_typeid()] = &a;
	}

private:	
	void* descriptors[1];
	A a;
};



template<class A, class B>
class ClientDescriptorTemplate2: public ClientDescriptor {
public:
	ClientDescriptorTemplate2() {
		descriptor_tbl = descriptors;
		A::set_descriptor_typeid(1);
		B::set_descriptor_typeid(2);
		descriptors[A::descriptor_typeid()] = &a;
		descriptors[B::descriptor_typeid()] = &b;
	}

private:	
	void* descriptors[2];
	A a;
	B b;
};





} // namespace server

#endif // __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H
