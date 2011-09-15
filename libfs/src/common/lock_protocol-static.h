// contains static initializations
// must be included by one .cc file per binary only

#ifndef _LOCK_PROTOCOL_STATIC_H_AFH190
#define _LOCK_PROTOCOL_STATIC_H_AFH190

#include "common/lock_protocol.h"

// compatibilities among access modes
bool lock_protocol::Mode::compatibility_table[][lock_protocol::Mode::CARDINALITY] = {
	/*             NL,   SL,    SR,    IS,    IX,    XL,    XR,    IXSL   */
	/* NL    */ {  true, true,  true,  true,  true,  true,  true,  true },
	/* SL    */ {  true, true,  true,  true,  true,  false, false, true },
	/* SR    */ {  true, true,  true,  true,  false, false, false, false },
	/* IS    */ {  true, true,  true,  true,  true,  true,  false, true },
	/* IX    */ {  true, true,  false, true,  true,  true,  false, true },
	/* XL    */ {  true, false, false, true,  true,  false, false, false },
	/* XR    */ {  true, false, false, false, false, false, false, false },
	/* IXSL  */ {  true, true,  false, true,  true,  false, false, true }
};


// partial order lattice:
//
//                    +--> IX -- IXSL --+   
//                    |                 |   
// NL --> IS -- SL -->+-----> XL -------+--> XR
//                    |                 |   
//                    +-----> SR -------+   

// restrictiveness (or severity) of each mode. higher severity means
// more restrictive. 
// IX and IXSL are partially ordered but have the same
// severity in the table below. Treat them as special case
int lock_protocol::Mode::severity_table[] = {
	/* NL   */  0,
	/* SL   */  2,
	/* SR   */  3,
	/* IS   */  1,
	/* IX   */  3,
	/* XL   */  3,
	/* XR   */  4,
	/* IXSL */  3
};

// for each mode, the next most restrictive mode based on the lattice above
int lock_protocol::Mode::successor_table[] = {
	/* NL   */  lock_protocol::IS,
	/* SL   */  lock_protocol::SR, // WARNING: SL has multiple successors
	/* SR   */  lock_protocol::XR,
	/* IS   */  lock_protocol::SL,
	/* IX   */  lock_protocol::IXSL,
	/* XL   */  lock_protocol::XR,
	/* XR   */  lock_protocol::XR,
	/* IXSL */  lock_protocol::XR
};


std::string lock_protocol::Mode::mode2str_table[] = { 
	"NL", "SL", "SR", "IS", "IX", "XL", "XR", "IXSL"
};

#endif /* _LOCK_PROTOCOL_STATIC_H_AFH190 */
