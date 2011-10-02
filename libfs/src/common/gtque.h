#ifndef _GRANT_QUEUE_H_AJK191
#define _GRANT_QUEUE_H_AJK191

#include <iostream>
#include <stdint.h>
#include <map>
#include <vector>

// We assume mode==0 is non-locked so we remove members set at that mode

template <class MemberType>
class GrantQueue {
public:
	typedef typename std::map<int, MemberType>::iterator iterator;
	GrantQueue(int);
	bool Exists(typename MemberType::id_t);
	bool CanGrant(typename MemberType::Mode mode);
	int ConvertInPlace(typename MemberType::id_t id, typename MemberType::Mode new_mode);
	void Add(const MemberType& cr);
	void Remove(typename MemberType::id_t id);
	int PartialOrder(typename MemberType::Mode mode);
	bool IsModeSet(typename MemberType::Mode mode) { return (mode_union_.IsSet(mode));}
	int  Size() { return members_.size(); }
	int  Empty() { return members_.empty(); }
	void Print(std::ostream&);
	MemberType* Find(typename MemberType::id_t);

	iterator begin() { return members_.begin(); }
	iterator end() { return members_.end(); }

private:
	bool CanConvertInPlace(typename MemberType::id_t, typename MemberType::Mode new_mode);

	std::map<typename MemberType::id_t, MemberType>  members_;
	typename MemberType::Mode::Bitmap                mode_union_;
	std::vector<uint8_t>                             mode_cnt_;
};


template <class MemberType>
GrantQueue<MemberType>::GrantQueue(int nmode)
	: mode_union_(0)
{
	mode_cnt_.resize(nmode);
	for (int i=0; i<nmode; i++) {
		mode_cnt_[i] = 0;
	}
}


template <class MemberType>
bool
GrantQueue<MemberType>::Exists(typename MemberType::id_t id) {
    return (members_.find(id) != members_.end());
}


template <class MemberType>
MemberType*
GrantQueue<MemberType>::Find(typename MemberType::id_t id)
{
	typename std::map<typename MemberType::id_t, MemberType>::iterator itr;

	if ((itr = members_.find(id)) != members_.end()) {
		return &(*itr).second;
	} 
	return NULL;
}


// mode less-than         union(grant_queue): returns -1
// mode greater-than      union(grant_queue): returns 1
// mode not-ordered-with  union(grant_queue): returns 0
template <class MemberType>
int
GrantQueue<MemberType>::PartialOrder(typename MemberType::Mode mode)
{
	return MemberType::Mode::Bitmap::PartialOrder(mode, mode_union_);
}


template <class MemberType>
bool
GrantQueue<MemberType>::CanGrant(typename MemberType::Mode mode)
{
	return MemberType::Mode::Bitmap::Compatible(mode, mode_union_);
}


template <class MemberType>
void
GrantQueue<MemberType>::Add(const MemberType& cr)
{
	assert(Exists(cr.id()) == false); // existing member should use convert
	members_[cr.id()] = cr;
	mode_cnt_[cr.mode().value()]++;
	mode_union_ |= 1 << cr.mode();
}


template <class MemberType>
bool
GrantQueue<MemberType>::CanConvertInPlace(typename MemberType::id_t id, 
                                          typename MemberType::Mode new_mode)
{
	typename MemberType::Mode::Bitmap mode_union = mode_union_;
	typename MemberType::Mode         member_mode;

	assert(Exists(id) == true);
	member_mode = members_[id].mode();

	// if the mode bit flag is set because of member id then 
	// exclude the mode this member holds the lock at
	if ((mode_cnt_[member_mode.value()]-1==0)) {
		mode_union.Remove(member_mode);
	}
	
	return MemberType::Mode::Bitmap::Compatible(new_mode, mode_union);
}


// if new_mode is 0 then conversion removes member
template <class MemberType>
int
GrantQueue<MemberType>::ConvertInPlace(typename MemberType::id_t id, 
                                       typename MemberType::Mode new_mode)
{	
	if (Exists(id) == false) {
		return -1;
	}
	if (CanConvertInPlace(id, new_mode) == false) {
		return -1;
	}

	MemberType& mb = members_[id];
	assert(mode_cnt_[mb.mode().value()]>0);
	if (--mode_cnt_[mb.mode().value()] == 0) {
		mode_union_.Remove(mb.mode());
	}
	if (new_mode != 0) {
		mb.set_mode(new_mode);
		mode_cnt_[mb.mode().value()]++;
		mode_union_.Insert(mb.mode());
	} else {
		members_.erase(id);
	}
	return 0;
}


template <class MemberType>
void
GrantQueue<MemberType>::Remove(typename MemberType::id_t id)
{
	ConvertInPlace(id, 0);
}


template <class MemberType>
void
GrantQueue<MemberType>::Print(std::ostream& out)
{
	typename std::map<typename MemberType::id_t, MemberType>::iterator   itr_icr;

	out << "MEMBERS: " << members_.size() << std::endl;
	for (itr_icr = members_.begin(); 
	     itr_icr != members_.end(); itr_icr++) 
	{
		MemberType& h = (*itr_icr).second;
		out << "clt=" << h.id() << ", mode=" << "(" << h.mode() << ")" << std::endl; 
	}
}


#endif /* _GRANT_QUEUE_H_AJK191 */
