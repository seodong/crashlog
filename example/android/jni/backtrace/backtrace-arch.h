/* Architecture dependent functions. */

#ifndef _CORKSCREW_BACKTRACE_ARCH_H
#define _CORKSCREW_BACKTRACE_ARCH_H

#include "backtrace.h"

#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Rewind the program counter by one instruction. */
	uintptr_t rewind_pc_arch(const memory_t * memory, uintptr_t pc);

	ssize_t unwind_backtrace_signal_arch(siginfo_t * siginfo, void *sigcontext,
					     const map_info_t * map_info_list,
					     backtrace_frame_t * backtrace, size_t ignore_depth, size_t max_depth);

#ifdef __cplusplus
}
#endif
#endif				// _CORKSCREW_BACKTRACE_ARCH_H
