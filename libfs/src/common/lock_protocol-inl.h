#ifndef _LOCK_PROTOCOL_INLINE_H_AKL156
#define _LOCK_PROTOCOL_INLINE_H_AKL156


inline lock_protocol::Mode::Set::Enum operator|(lock_protocol::Mode::Enum a, lock_protocol::Mode::Enum b)
{
	return static_cast<lock_protocol::Mode::Set::Enum>((1 << a) | (1 <<b ));
}


inline lock_protocol::Mode::Set::Iterator 
lock_protocol::Mode::Set::begin()
{
	return Iterator(*this);
}


inline lock_protocol::Mode::Set::Iterator 
lock_protocol::Mode::Set::end()
{
	return Iterator();
}


inline bool 
lock_protocol::Mode::Set::Compatible(lock_protocol::Mode mode, 
                                     lock_protocol::Mode::Set mode_set)
{
	Iterator itr;

	for (itr = mode_set.begin(); itr != mode_set.end(); itr++) {
		if (!lock_protocol::Mode::Compatible((*itr), mode)) { 
			return false;
		}
	}
	return true;
}


inline int 
lock_protocol::Mode::Set::PartialOrder(lock_protocol::Mode mode, 
						               lock_protocol::Mode::Set mode_set) 
{
	int      r;
	bool     init_po = false;
	int      po;
	Iterator itr;

	for (itr = mode_set.begin(); itr != mode_set.end(); itr++) {
		r = lock_protocol::Mode::PartialOrder(mode, (*itr));
		if (init_po == false) {
			init_po = true;
			po = r;
		} else {
			if (r != po) {
				po = 0;
				return po;
			}
		} 
	}
	return po;
}


inline std::string 
lock_protocol::Mode::Set::String()
{
	Iterator    itr;
	std::string str;
	bool        cat_separator = false;

	for (itr = begin(); itr != end(); itr++) {
		str += (cat_separator) ? "|" : ""; 
		str += (*itr).String(); 
		cat_separator = true;
	}
	return str;
}

/// \brief Retuns the most severe mode found in the set, which is compatible
/// with mode compatible_mode.
inline lock_protocol::Mode
lock_protocol::Mode::Set::MostSevere(lock_protocol::Mode compatible_mode)
{
	lock_protocol::Mode::Set::Iterator itr;
	lock_protocol::Mode                mode;

	for (itr = begin(); itr != end(); itr++) {
		if ((*itr) > mode && compatible_mode.Compatible(*itr)) {
			mode = *itr;
		}
	}
	
	return mode;
}


#endif /* _LOCK_PROTOCOL_INLINE_H_AKL156 */
