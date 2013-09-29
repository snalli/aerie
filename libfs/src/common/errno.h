#ifndef __STAMNOS_ERRNO_H
#define __STAMNOS_ERRNO_H

#ifdef __DEFINE_ERRNO
# error "__DEFINE_ERRNO previously defined"
#endif

/*
 * Define error codes and error messages here
 */
#define __DEFINE_ERRNO(ACTION)                                               \
    ACTION(E_SUCCESS, "Success")                                             \
    ACTION(E_ERROR, "Generic error")                                         \
    ACTION(E_NOMEM, "No memory")                                             \
    ACTION(E_EXIST, "Name already exists")                                   \
    ACTION(E_NOENT, "Name does not exist")                                   \
    ACTION(E_INVAL, "Invalid argument")                                      \
    ACTION(E_KVFS, "File belongs to a kernel filesystem")                    \
    ACTION(E_BUSY, "Resource busy")                                          \
    ACTION(E_NOTEMPTY, "Not empty")                                          \
    ACTION(E_IPC, "Interprocess communication error")                        \
    ACTION(E_UNKNOWNFS, "Unknown file system type")                          \
    ACTION(E_ERRNO, "Standard C library error; check errno for details")     \
    ACTION(E_VRFY, "Verification error")                                     \
    ACTION(E_PROT, "Protection error")                                       \


#ifdef __ENUM_MEMBER
# error "__ENUM_MEMBER previously defined"
#endif

#define __ENUM_MEMBER(name, str)  name,

enum {
	__DEFINE_ERRNO(__ENUM_MEMBER)
	E_MAXERRNO
};

#undef __ENUM_MEMBER /* don't polute the macro namespace */

#ifdef __ERRNO_STRING
# error "__ERRNO_STRING previously defined"
#endif

#define __ERRNO_STRING(name, str) str,


static const char* 
ErrorToString(int err) {
	static const char* errstr[] = {
		__DEFINE_ERRNO(__ERRNO_STRING)
		"Unknown error code"
	};
	if (err >= 0 && err < E_MAXERRNO) {
		return errstr[err];
	}
	return errstr[E_MAXERRNO];
}

#undef __ERRNO_STRING /* don't polute the macro namespace */
#undef __DEFINE_ERRNO /* don't polute the macro namespace */

#endif /* __STAMNOS_ERRNO_H */
