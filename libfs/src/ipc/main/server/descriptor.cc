#include <stdio.h>

class Descriptor {
public:
	
	void* descriptor(int n) {
		return descriptor_tbl[n];
	}

protected:
	void** descriptor_tbl; 
};


template<class T>
class BaseDescriptor {
public:
	static T* Descriptor(Descriptor* dsc) {
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
int BaseDescriptor<T>::descriptor_typeid_ = 0;

class Storage: public BaseDescriptor<Storage> {
public:
	int id;
	int cap;
	
};

class Keeper: public BaseDescriptor<Keeper> {
public:
	int id;
	int cap;
	
};



template<class A, class B>
class DescriptorTemplate: public Descriptor {
public:
	DescriptorTemplate() {
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



main() 
{
	DescriptorTemplate<Storage, Keeper> dsc;
	Storage* storage_dsc = Storage::Descriptor(&dsc);
	storage_dsc->cap = 1;
}
