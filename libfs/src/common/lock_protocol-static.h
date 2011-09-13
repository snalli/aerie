// contains static initializations
// must be included by one .cc file per binary only

#ifndef _LOCK_PROTOCOL_STATIC_H_AFH190
#define _LOCK_PROTOCOL_STATIC_H_AFH190

#include "common/lock_protocol.h"

bool lock_protocol::Mode::compatibility_table[8][8] = {
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


int lock_protocol::Mode::severity_table[8] = {
	/* NL   */  0,
	/* SL   */  2,
	/* SR   */  3,
	/* IS   */  1,
	/* IX   */  2,
	/* XL   */  4,
	/* XR   */  5,
	/* IXSL */  2
};

std::string lock_protocol::Mode::mode2str_table[] = { 
	"NL", "SL", "SR", "IS", "IX", "XL", "XR", "IXSL"
};

#endif /* _LOCK_PROTOCOL_STATIC_H_AFH190 */
