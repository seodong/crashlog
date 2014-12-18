#ifndef _CORKSCREW_BACKTRACE_HELPER_H
#define _CORKSCREW_BACKTRACE_HELPER_H

#include <sys/types.h>
#include <stdbool.h>
#include "backtrace/backtrace.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Add a program counter to a backtrace if it will fit.
 * Returns the newly added frame, or NULL if none.
 */
	backtrace_frame_t *add_backtrace_entry(uintptr_t pc,
					       backtrace_frame_t * backtrace,
					       size_t ignore_depth, size_t max_depth,
					       size_t * ignored_frames, size_t * returned_frames);

#ifdef __cplusplus
}
#endif
#endif				// _CORKSCREW_BACKTRACE_HELPER_H
