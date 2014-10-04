//=========================================================================
// errors.h
// List of error codes used by the network-layer code
//=========================================================================

#ifndef __ERRORS_H
#define __ERRORS_H

#define ERROR_CODES(code) \
	code(OK)       /* no error                      */ \
	code(ERROR)    /* unknown command/generic error */ \
	code(ENOSYS)   /* operation not implemented     */ \
	code(EINVAL)   /* invalid argument(s)           */ \
	code(ENOMEM)   /* not enough memory/buffers     */ \
	code(ENONET)   /* network is down               */ \
	code(ENOWAY)   /* no route to destination       */ \
	code(ENODEV)   /* interface not connected       */ \
	code(ERANGE)   /* too many hops                 */ \
	code(EPROTO)   /* protocol error                */ \
	code(EMSGSIZE) /* message too long              */ \
	code(EBUSY)    /* device or resource busy       */ \
	code(EIO)      /* I/O error                     */ \
	code(ETIMEOUT) /* I/O timeout                   */ \

#define LIST(x) x,
enum error_codes { ERROR_CODES(LIST) ELAST } ;
#undef LIST

#ifdef __cplusplus
#include <string>
std::string error_name (int x);
#endif

#endif
