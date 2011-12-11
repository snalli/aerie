#ifndef __STAMNOS_DPO_COMMON_OBJECT_PROXY_H
#define __STAMNOS_DPO_COMMON_OBJECT_PROXY_H

#include "dpo/common/obj.h"

namespace dpo {

namespace common {

// just an empty class that serves the base class type
class ObjectProxy {
public:
	dpo::common::ObjectId oid() {
		return subject_->oid();
	}
protected:
	dpo::common::Object* subject_;
};


template<class Subject>
class ObjectProxyTemplate: public ObjectProxy {
public:
	Subject* subject() { return static_cast<Subject*>(subject_); }
	void set_subject(Subject* subject) { subject_ = subject; }
};

} // namespace common

} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
