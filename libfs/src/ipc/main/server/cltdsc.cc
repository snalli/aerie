#include <stdio.h>

class AbstractClientDescriptorConstructor {
public:
    virtual void operator()(void* mem) = 0;
};


struct ClientDescriptor {
public:
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

protected:
	static int payload_size_;
	static int registered_dsc_cnt_;
	static int offset_[16];
	static AbstractClientDescriptorConstructor* constructors_[16];
};

int ClientDescriptor::payload_size_ = 0;
int ClientDescriptor::registered_dsc_cnt_ = 0;
int ClientDescriptor::offset_[16];
AbstractClientDescriptorConstructor* ClientDescriptor::constructors_[16];


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




struct StorageDescriptor: public ClientDescriptorTemplate<StorageDescriptor> {
public:
	StorageDescriptor() {
		printf("StorageDescriptor: CONSTRUCTOR: %p\n", this);
	}
	int id;
	int cap;
	
};

struct LockDescriptor: public ClientDescriptorTemplate<LockDescriptor> {
public:
	int id;
	int cap;
	int cap2;
	
};



main() 
{
	StorageDescriptor::Register();
	LockDescriptor::Register();

	ClientDescriptor* dsc = ClientDescriptor::Create();
	printf("dsc=%p\n", dsc);
	dsc = ClientDescriptor::Create();
	printf("dsc=%p\n", dsc);

	StorageDescriptor* sdsc = StorageDescriptor::Descriptor(dsc);
	printf("sdsc=%p\n", sdsc);

}
