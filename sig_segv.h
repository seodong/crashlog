#ifndef __SIGSEGV_
#define __SIGSEGV_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <signal.h>

	int setup_sigsegv(void);
	const char *lookup_symbol(const void *addr, unsigned *offset, char *name, size_t bufSize);
	void crashlog(const char *fmt, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __SIGSEGV_ */
