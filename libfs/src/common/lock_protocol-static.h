// contains static initializations
// must be included by one .cc file per binary only

#ifndef _LOCK_PROTOCOL_STATIC_H_AFH190
#define _LOCK_PROTOCOL_STATIC_H_AFH190

#include "common/lock_protocol.h"


// compatibilities among access modes
bool lock_protocol::Mode::compatibility_table_[][lock_protocol::Mode::CARDINALITY] = {
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

//FIXME: it should be IX < IXSL < XL according to the table above. double check compatibilities

// partial order lattice:
//
//             +--> SL --+     +--> IXSL -- XL --+   
//             |         |     |                 |   
// NL --> IS --+         +-->--+                 +--> XR
//             |         |     |                 |   
//             +--> IX --+     +-----> SR -------+   


// restrictiveness (or severity) of each mode. higher severity means
// more restrictive. 
// IX and IXSL are partially ordered but have the same
// severity in the table below. Treat them as special case
int lock_protocol::Mode::severity_table_[] = {
	/* NL   */  0,
	/* SL   */  2,
	/* SR   */  3,
	/* IS   */  1,
	/* IX   */  2,
	/* XL   */  3,
	/* XR   */  4,
	/* IXSL */  3
};

// for each mode, the next most restrictive mode based on the lattice above
lock_protocol::Mode::Enum lock_protocol::Mode::successor_table_[] = {
	/* NL   */  lock_protocol::Mode::IS,
	/* SL   */  lock_protocol::Mode::SR, // WARNING: SL has multiple successors
	/* SR   */  lock_protocol::Mode::XR,
	/* IS   */  lock_protocol::Mode::SL,
	/* IX   */  lock_protocol::Mode::IXSL,
	/* XL   */  lock_protocol::Mode::XR,
	/* XR   */  lock_protocol::Mode::XR,
	/* IXSL */  lock_protocol::Mode::XR
};

// for each mode, the least severe recursive mode that covers this mode
lock_protocol::Mode::Enum lock_protocol::Mode::least_recursive_mode_table_[] = {
	/* NL   */  lock_protocol::Mode::SR,
	/* SL   */  lock_protocol::Mode::SR,
	/* SR   */  lock_protocol::Mode::SR,
	/* IS   */  lock_protocol::Mode::SR,
	/* IX   */  lock_protocol::Mode::XR,
	/* XL   */  lock_protocol::Mode::XR,
	/* XR   */  lock_protocol::Mode::XR,
	/* IXSL */  lock_protocol::Mode::XR
};


// Hierarchical Locks

uint32_t lock_protocol::Mode::hierarchy_rule_bitmaps_[] = {
	/* NL   */  BITMAP_SET(lock_protocol::Mode::NL) | 
	            BITMAP_SET(lock_protocol::Mode::SL) |
	            BITMAP_SET(lock_protocol::Mode::SR) |
	            BITMAP_SET(lock_protocol::Mode::IS) | 
	            BITMAP_SET(lock_protocol::Mode::IX) | 
	            BITMAP_SET(lock_protocol::Mode::XL) | 
	            BITMAP_SET(lock_protocol::Mode::XR) | 
	            BITMAP_SET(lock_protocol::Mode::IXSL),

	/* SL   */  BITMAP_SET(lock_protocol::Mode::IS) | 
	            BITMAP_SET(lock_protocol::Mode::SR) |
	            BITMAP_SET(lock_protocol::Mode::IX) |
	            BITMAP_SET(lock_protocol::Mode::XR),

	/* SR   */  BITMAP_SET(lock_protocol::Mode::IS) | 
	            BITMAP_SET(lock_protocol::Mode::IX) |
	            BITMAP_SET(lock_protocol::Mode::SR),

	/* IS   */  BITMAP_SET(lock_protocol::Mode::IS) | 
	            BITMAP_SET(lock_protocol::Mode::SR),
	
	/* IX   */  BITMAP_SET(lock_protocol::Mode::IS) | 
	            BITMAP_SET(lock_protocol::Mode::IX),

	/* XL   */  BITMAP_SET(lock_protocol::Mode::IX) | 
	            BITMAP_SET(lock_protocol::Mode::XR) |
	            BITMAP_SET(lock_protocol::Mode::IXSL),
	
	/* XR   */  BITMAP_SET(lock_protocol::Mode::IX) |
	            BITMAP_SET(lock_protocol::Mode::IXSL) | 
	            BITMAP_SET(lock_protocol::Mode::XR),

	/* IXSL */  BITMAP_SET(lock_protocol::Mode::IX) | 
	            BITMAP_SET(lock_protocol::Mode::IXSL) |
	            BITMAP_SET(lock_protocol::Mode::XR)
};


uint32_t lock_protocol::Mode::recursive_rule_bitmaps_[] = {
	/* NL   */ BITMAP_SET(lock_protocol::Mode::NL) |
	           BITMAP_SET(lock_protocol::Mode::SR) |
	           BITMAP_SET(lock_protocol::Mode::XR),

	/* SL   */ BITMAP_SET(lock_protocol::Mode::SR) |
	           BITMAP_SET(lock_protocol::Mode::XR),

	/* SR   */ BITMAP_SET(lock_protocol::Mode::SR),

	/* IS   */ BITMAP_SET(lock_protocol::Mode::SR) |
	           BITMAP_SET(lock_protocol::Mode::XR),
	
	/* IX   */ BITMAP_SET(lock_protocol::Mode::XR),

	/* XL   */ BITMAP_SET(lock_protocol::Mode::XR),

	/* XR   */ BITMAP_SET(lock_protocol::Mode::XR),
	
	/* IXSL */ BITMAP_SET(lock_protocol::Mode::XR)
};




std::string lock_protocol::Mode::mode2str_table_[] = { 
	"NL", "SL", "SR", "IS", "IX", "XL", "XR", "IXSL"
};




#endif /* _LOCK_PROTOCOL_STATIC_H_AFH190 */
