#include "backtrace/backtrace-helper.h"

backtrace_frame_t *add_backtrace_entry(uintptr_t pc, backtrace_frame_t * backtrace,
				       size_t ignore_depth, size_t max_depth, size_t * ignored_frames, size_t * returned_frames)
{
	if (*ignored_frames < ignore_depth) {
		*ignored_frames += 1;
		return NULL;
	}
	if (*returned_frames >= max_depth) {
		return NULL;
	}
	backtrace_frame_t *frame = &backtrace[*returned_frames];
	frame->absolute_pc = pc;
	frame->stack_top = 0;
	frame->stack_size = 0;
	*returned_frames += 1;
	return frame;
}
