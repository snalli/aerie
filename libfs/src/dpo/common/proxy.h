#ifndef __STAMNOS_DPO_COMMON_OBJECT_PROXY_H
#define __STAMNOS_DPO_COMMON_OBJECT_PROXY_H

namespace dpo {

namespace common {

// just an empty class that serves the base class type
class ObjectProxy {

};


template<class Subject>
class ObjectProxyTemplate: public ObjectProxy {
public:
	Subject* subject() { return subject_; }
	void set_subject(Subject* subject) { subject_ = subject; }

protected:
	Subject* subject_;
};

} // namespace common

} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
