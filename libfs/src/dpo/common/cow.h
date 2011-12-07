#ifndef _COW_STAMNOS_H_KAL190
#define _COW_STAMNOS_H_KAL190

namespace cow {

template<class Proxy, class Subject>
class ObjectProxy {
public:
	Subject* subject() { return subject_; }
	void setSubject(Subject* subject) { subject_ = subject; }
private:
	Subject* subject_;
};


} // namespace::cow

#endif // _COW_STAMNOS_H_KAL190
