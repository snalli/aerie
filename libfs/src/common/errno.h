#ifndef __STAMNOS_ERRNO_H
#define __STAMNOS_ERRNO_H

enum {
	E_SUCCESS = 0,
	E_NOMEM = 1,
	E_EXIST,
	E_NOENT,
	E_INVAL,
	E_KVFS,
	E_BUSY,
	E_NOTEMPTY,
	E_IPC,
	E_ERRNO     // caller should check errno variable for the error returned 
};

#endif /* __STAMNOS_ERRNO_H */
