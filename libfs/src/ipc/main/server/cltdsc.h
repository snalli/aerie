#ifndef __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H
#define __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H

#include "ipc/backend/rpc.h"
#include "ipc/main/common/macros.h"

namespace server {

class AbstractClientDescriptorConstructor {
public:
    virtual void operator()(void* mem) = 0;
};


struct ClientDescriptor {
public:
	ClientDescriptor(int clt, rpcc* rpccl)
		: clt_(clt),
		  rpcc_(rpccl)
	{ }
	
	void* descriptor(int n) {
		return (void*) (((char*) this) + offset_[n]);
	}

	static int Register(int size, AbstractClientDescriptorConstructor* constructor) {
		int r = registered_dsc_cnt_++;
		constructors_[r] = constructor;
		offset_[r] = sizeof(ClientDescriptor) + payload_size_;
		payload_size_ += size;
		return r;
	}

	static ClientDescriptor* Create() {
		int obj_size = sizeof(ClientDescriptor) + payload_size_;
		char* obj = new char[obj_size];
		for (int i = 0; i < registered_dsc_cnt_; i++) {
			(*constructors_[i])((void*) (obj + offset_[i]));
		}
		return reinterpret_cast<ClientDescriptor*>(obj);
	}

	RPC_CALL(rpcc_, rpcc::to_max)

protected:
	rpcc*      rpcc_;
	int        clt_;
	
	static int payload_size_;
	static int registered_dsc_cnt_;
	static int offset_[16];
	static AbstractClientDescriptorConstructor* constructors_[16];
};


template<typename T>
struct ClientDescriptorTemplate {
public:
	class ClientDescriptorConstructor: public AbstractClientDescriptorConstructor {
	public:
		void operator()(void* mem) {
			new(mem) T();
		}
	};

	inline void* operator new(size_t nbytes, void* ptr) {
		return ptr;
	}

	static T* Descriptor(ClientDescriptor* dsc) {
		return reinterpret_cast<T*>(dsc->descriptor(descriptor_typeid_));
	}
	
	static void Register() {
		if (!registered_) {
			AbstractClientDescriptorConstructor* constructor = new ClientDescriptorConstructor();
			descriptor_typeid_ = ClientDescriptor::Register(sizeof(T), constructor);
		}
	}

	static void set_descriptor_typeid(int id) {
		descriptor_typeid_ = id;
	}

	static int descriptor_typeid() {
		return descriptor_typeid_;
	}

private:	
	static int descriptor_typeid_;
	static bool registered_;
};


template<typename T>
int ClientDescriptorTemplate<T>::descriptor_typeid_ = 0;

template<typename T>
bool ClientDescriptorTemplate<T>::registered_ = false;





} // namespace server

#endif // __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H
