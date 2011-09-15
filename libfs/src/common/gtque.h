#ifndef _GRANT_QUEUE_H_AJK191
#define _GRANT_QUEUE_H_AJK191

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
	bool CanGrant(int);
	int ConvertInPlace(typename MemberType::id_t, int);
	void Add(const MemberType&);
	void Remove(typename MemberType::id_t);
	int PartialOrder(int mode);
	bool IsModeSet(int mode) { return (mode_union_ & (1 << mode));}
	int  Size() { return members_.size(); }
	int  Empty() { return members_.empty(); }
	void Print(std::ostream);
	MemberType& Get(int);

	iterator begin() { return members_.begin(); }
	iterator end() { return members_.end(); }

private:
	bool IsModeCompatible(int, int);
	bool CanConvertInPlace(typename MemberType::id_t, int);

	std::map<typename MemberType::id_t, MemberType>  members_;
	uint32_t                                         mode_union_;
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
MemberType&
GrantQueue<MemberType>::Get(int clt)
{
	return members_[clt];
}


template <class MemberType>
bool
GrantQueue<MemberType>::IsModeCompatible(int mode, int exclude_mode)
{
	int val = mode_union_;
	int m;

	while (val) {
		m = __builtin_ctz(val); 
		val &= ~(1 << m);
		if (m == exclude_mode) {
			continue;
		}
		if (!MemberType::Mode::compatibility_table[m][mode]) { 
			return false;
		}
	}
	return true;
}

// mode less-than         union(grant_queue): returns -1
// mode greater-than      union(grant_queue): returns 1
// mode not-ordered-with  union(grant_queue): returns 0
template <class MemberType>
int
GrantQueue<MemberType>::PartialOrder(int mode)
{
	int val = mode_union_;
	int severity = 0;
	int m;
	int po = -1;
	int r;

	while (val) {
		m = __builtin_ctz(val); 
		val &= ~(1 << m);
		if ((r = MemberType::Mode::PartialOrder(mode, m)) > po) {
			po = r;
		}
	}
	return severity;
}


template <class MemberType>
bool
GrantQueue<MemberType>::CanGrant(int mode)
{
	return IsModeCompatible(mode, -1);
}


template <class MemberType>
void
GrantQueue<MemberType>::Add(const MemberType& cr)
{
	assert(Exists(cr.id()) == false); // existing member should use convert
	members_[cr.id()] = cr;
	mode_cnt_[cr.mode()]++;
	mode_union_ |= 1 << cr.mode();
}


template <class MemberType>
bool
GrantQueue<MemberType>::CanConvertInPlace(typename MemberType::id_t id, int new_mode)
{
	int val = mode_union_;
	int member_mode;
	int exclude_mode = -1;

	assert(Exists(id)==true);
	member_mode = members_[id].mode();

	// if the mode bit flag is set because of member id then 
	// exclude the mode this member holds the lock at
	exclude_mode = (mode_cnt_[member_mode]-1==0) ? member_mode: -1;

	return IsModeCompatible(new_mode, exclude_mode);
}


// if new_mode is 0 then conversion removes member
template <class MemberType>
int
GrantQueue<MemberType>::ConvertInPlace(typename MemberType::id_t id, int new_mode)
{
	if (Exists(id) == false) {
		return -1;
	}
	if (CanConvertInPlace(id, new_mode) == false) {
		return -1;
	}

	MemberType& mb = members_[id];
	assert(mode_cnt_[mb.mode()]>0);
	if (--mode_cnt_[mb.mode()] == 0) {
		mode_union_ &= ~(1 << mb.mode());
	}
	if (new_mode != 0) {
		mb.set_mode(new_mode);
		mode_cnt_[mb.mode()]++;
		mode_union_ |= 1 << mb.mode();
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
GrantQueue<MemberType>::Print(std::ostream out)
{
	typename std::map<int, MemberType>::iterator   itr_icr;

	out << "MEMBERS" << std::endl;
	for (itr_icr = members_.begin(); 
	     itr_icr != members_.end(); itr_icr++) 
	{
		MemberType& h = (*itr_icr).second;
		out << "clt=" << h.id() << ", mode=" << "(" << h.mode() << ")" << std::endl; 
	}
}


#endif /* _GRANT_QUEUE_H_AJK191 */
