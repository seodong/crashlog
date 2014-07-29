#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif				/* _GNU_SOURCE */

#include <stdint.h>
#include <time.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <inttypes.h>
#include <linux/stacktrace.h>
#include <sys/mman.h>

#include <sys/ptrace.h>
#include <asm/sigcontext.h>
#include "backtrace/properties.h"

#include "backtrace/backtrace.h"
#include "backtrace/symbol_table.h"

/* Machine context at the time a signal was raised. */
typedef struct ucontext {
	uint32_t uc_flags;
	struct ucontext *uc_link;
	stack_t uc_stack;
	struct sigcontext uc_mcontext;
	uint32_t uc_sigmask;
} ucontext_t;

/* Unwind state. */
typedef struct {
	uint32_t gregs[16];
} unwind_state_t;

#include "sig_segv.h"

#include <android/log.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "SnackSO_Call", __VA_ARGS__);

extern char* gExternalStoragePath;
extern char gAppVersion[];

int property_get(const char *key, char *value, const char *default_value)
{
	int len;

	len = __system_property_get(key, value);
	if (len > 0) {
		return len;
	}

	if (default_value) {
		len = strlen(default_value);
		memcpy(value, default_value, len + 1);
	}
	return len;
}

void crashlog(const char *fmt, ...)
{
	FILE *fp;
	char fname[300] = { 0, };
	char log_path[256];
	va_list ap;

	if (!gExternalStoragePath) {
		LOGD("crashlog path not defined\n");
		return;
	}

	snprintf(log_path, sizeof(log_path) - 1, "%s", gExternalStoragePath);

	if (access(log_path, F_OK) != 0)
		mkdir(log_path, 0777);

	snprintf(fname, sizeof(fname) - 1, "%s/crash.txt", log_path);

	/* TODO : fname directory check */
	fp = fopen(fname, "a");
	if (!fp)
		return;

	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);

	fflush(fp);

	fclose(fp);
}

static void dump_build_info(void)
{
	char fingerprint[PROPERTY_VALUE_MAX];
	property_get("ro.build.fingerprint", fingerprint, "unknown");
	crashlog("Build fingerprint: '%s'\n", fingerprint);
}

static int signal_has_address(int sig)
{
	switch (sig) {
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
		return 1;
	default:
		return 0;
	}
}

static const char *get_signame(int sig)
{
	switch (sig) {
	case SIGILL:
		return "SIGILL";
	case SIGABRT:
		return "SIGABRT";
	case SIGBUS:
		return "SIGBUS";
	case SIGFPE:
		return "SIGFPE";
	case SIGSEGV:
		return "SIGSEGV";
	case SIGPIPE:
		return "SIGPIPE";
#ifdef SIGSTKFLT
	case SIGSTKFLT:
		return "SIGSTKFLT";
#endif
	case SIGSTOP:
		return "SIGSTOP";
	default:
		return "?";
	}
}

static const char *get_sigcode(int signo, int code)
{
	// Try the signal-specific codes...
	switch (signo) {
	case SIGILL:
		switch (code) {
		case ILL_ILLOPC:
			return "ILL_ILLOPC";
		case ILL_ILLOPN:
			return "ILL_ILLOPN";
		case ILL_ILLADR:
			return "ILL_ILLADR";
		case ILL_ILLTRP:
			return "ILL_ILLTRP";
		case ILL_PRVOPC:
			return "ILL_PRVOPC";
		case ILL_PRVREG:
			return "ILL_PRVREG";
		case ILL_COPROC:
			return "ILL_COPROC";
		case ILL_BADSTK:
			return "ILL_BADSTK";
		}
		break;
	case SIGBUS:
		switch (code) {
		case BUS_ADRALN:
			return "BUS_ADRALN";
		case BUS_ADRERR:
			return "BUS_ADRERR";
		case BUS_OBJERR:
			return "BUS_OBJERR";
		}
		break;
	case SIGFPE:
		switch (code) {
		case FPE_INTDIV:
			return "FPE_INTDIV";
		case FPE_INTOVF:
			return "FPE_INTOVF";
		case FPE_FLTDIV:
			return "FPE_FLTDIV";
		case FPE_FLTOVF:
			return "FPE_FLTOVF";
		case FPE_FLTUND:
			return "FPE_FLTUND";
		case FPE_FLTRES:
			return "FPE_FLTRES";
		case FPE_FLTINV:
			return "FPE_FLTINV";
		case FPE_FLTSUB:
			return "FPE_FLTSUB";
		}
		break;
	case SIGSEGV:
		switch (code) {
		case SEGV_MAPERR:
			return "SEGV_MAPERR";
		case SEGV_ACCERR:
			return "SEGV_ACCERR";
		}
		break;
	case SIGTRAP:
		switch (code) {
		case TRAP_BRKPT:
			return "TRAP_BRKPT";
		case TRAP_TRACE:
			return "TRAP_TRACE";
		}
		break;
	}
	// Then the other codes...
	switch (code) {
	case SI_USER:
		return "SI_USER";
#if defined(SI_KERNEL)
	case SI_KERNEL:
		return "SI_KERNEL";
#endif
	case SI_QUEUE:
		return "SI_QUEUE";
	case SI_TIMER:
		return "SI_TIMER";
	case SI_MESGQ:
		return "SI_MESGQ";
	case SI_ASYNCIO:
		return "SI_ASYNCIO";
#if defined(SI_SIGIO)
	case SI_SIGIO:
		return "SI_SIGIO";
#endif
#if defined(SI_TKILL)
	case SI_TKILL:
		return "SI_TKILL";
#endif
	}
	// Then give up...
	return "?";
}

static void dump_fault_addr(pid_t tid, int sig, siginfo_t * si)
{
	if (signal_has_address(sig)) {
		crashlog("signal %d (%s), code %d (%s), fault addr %0*" PRIxPTR "\n",
			 sig, get_signame(sig), si->si_code, get_sigcode(sig, si->si_code),
			 sizeof(uintptr_t) * 2, (uintptr_t) (si->si_addr));
	} else {
		crashlog("signal %d (%s), code %d (%s), fault addr --------\n",
			 sig, get_signame(sig), si->si_code, get_sigcode(sig, si->si_code));
	}
}

void dump_registers(pid_t tid, unwind_state_t * r)
{
	crashlog("    r0 %08x  r1 %08x  r2 %08x  r3 %08x\n",
		 (uint32_t) (r->gregs[0]), (uint32_t) (r->gregs[1]), (uint32_t) (r->gregs[2]), (uint32_t) (r->gregs[3]));
	crashlog("    r4 %08x  r5 %08x  r6 %08x  r7 %08x\n",
		 (uint32_t) (r->gregs[4]), (uint32_t) (r->gregs[5]), (uint32_t) (r->gregs[6]), (uint32_t) (r->gregs[7]));
	crashlog("    r8 %08x  r9 %08x  sl %08x  fp %08x\n",
		 (uint32_t) (r->gregs[8]), (uint32_t) (r->gregs[9]), (uint32_t) (r->gregs[10]), (uint32_t) (r->gregs[11]));
	crashlog("    ip %08x  sp %08x  lr %08x  pc %08x\n",
		 (uint32_t) (r->gregs[12]), (uint32_t) (r->gregs[13]), (uint32_t) (r->gregs[14]), (uint32_t) (r->gregs[15]));

}

static void dump_thread_info(pid_t pid, pid_t tid, int scope_flags)
{
	char path[128];
	char threadnamebuf[255];
	char *threadname = NULL;
	FILE *fp;

	snprintf(path, sizeof(path) - 1, "/proc/%d/comm", tid);
	if ((fp = fopen(path, "r"))) {
		threadname = fgets(threadnamebuf, sizeof(threadnamebuf), fp);
		fclose(fp);
		if (threadname) {
			size_t len = strlen(threadname);
			if (len && threadname[len - 1] == '\n') {
				threadname[len - 1] = '\0';
			}
		}
	} else {
		crashlog("%s: can not file open fail : %s\n", __func__, path);
	}

	if (scope_flags) {
		char procnamebuf[255];
		char *procname = NULL;

		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path) - 1, "/proc/%d/cmdline", pid);
		if ((fp = fopen(path, "r"))) {
			procname = fgets(procnamebuf, sizeof(procnamebuf), fp);
			fclose(fp);
		} else {
			crashlog("%s: can not file open fail : %s\n", __func__, path);
		}

		crashlog("pid: %d, tid: %d, name: %s  >>> %s <<<\n", pid, tid, threadname ? threadname : "UNKNOWN",
			 procname ? procname : "UNKNOWN");
	} else {
		crashlog("pid: %d, tid: %d, name: %s\n", pid, tid, threadname ? threadname : "UNKNOWN");
	}
}

typedef struct backtrace_map_info {
	struct backtrace_map_info *next;
	uintptr_t start;
	uintptr_t end;
	int is_readable;
	int is_writable;
	int is_executable;
	char name[];
} backtrace_map_info_t;

typedef struct {
	size_t num;		/* The current fame number. */
	uintptr_t pc;		/* The absolute pc. */
	uintptr_t sp;		/* The top of the stack. */
	size_t stack_size;	/* The size of the stack, zero indicate an unknown stack size. */
	const char *map_name;	/* The name of the map to which this pc belongs, NULL indicates the pc doesn't belong to a known map. */
	uintptr_t map_offset;	/* pc relative to the start of the map, only valid if map_name is not NULL. */
	char *func_name;	/* The function name associated with this pc, NULL if not found. */
	uintptr_t func_offset;	/* pc relative to the start of the function, only valid if func_name is not NULL. */
} backtrace_frame_data_t;

#define STACK_WORDS 16
int dump_stack(size_t frame_count, backtrace_frame_t * frames)
{
	size_t i;
	size_t first = 0, last;

	for (i = 0; i < (size_t) frame_count; ++i) {
		if (frames[i].stack_top) {
			if (!first) {
				first = i + 1;
			}
			last = i;
		}
	}
	if (!first) {
		return;
	}
	first--;

	uintptr_t sp = frames[first].stack_top - STACK_WORDS * sizeof(uint32_t);

	dump_stack_segment(frames, &sp, STACK_WORDS, -1);
	for (i = first; i <= last; i++) {
		backtrace_frame_t *frame = (backtrace_frame_t *) & frames[i];

		if (sp != frame->stack_top) {
			crashlog("         ........  ........\n");
			sp = frame->stack_top;
		}
		if (i - first == 5) {
			break;
		}
		if (i == last) {
			dump_stack_segment(frames, &sp, STACK_WORDS, i);
			if (sp < frame->stack_top + frame->stack_size) {
				crashlog("         ........  ........\n");
			}
		} else {
			size_t words = frame->stack_size / sizeof(uint32_t);
			if (words == 0) {
				words = 1;
			} else if (words > STACK_WORDS) {
				words = STACK_WORDS;
			}
			dump_stack_segment(frames, &sp, words, i);
		}
	}
	return 0;
}

static ssize_t signal_backtrace(siginfo_t * siginfo, void *sigcontext)
{
	const size_t MAX_DEPTH = 32;
	backtrace_frame_t *frames = (backtrace_frame_t *) malloc(sizeof(backtrace_frame_t) * MAX_DEPTH);

	map_info_t *map_info = acquire_my_map_info_list();

	ssize_t frame_count = unwind_backtrace_signal_arch(siginfo, sigcontext, map_info, frames, 0, MAX_DEPTH);

	backtrace_symbol_t *backtrace_symbols = (backtrace_symbol_t *) malloc(sizeof(backtrace_symbol_t) * frame_count);
	get_backtrace_symbols(frames, frame_count, backtrace_symbols);
	release_my_map_info_list(map_info);

	size_t i;

	if (frame_count) {
		crashlog("\nbacktrace: %ld\n", (size_t) frame_count);

		for (i = 0; i < (size_t) frame_count; ++i) {
			char line[MAX_BACKTRACE_LINE_LENGTH];
			format_backtrace_line(i, &frames[i], &backtrace_symbols[i], line, MAX_BACKTRACE_LINE_LENGTH);
			if (backtrace_symbols[i].symbol_name != NULL) {
				// get_backtrace_symbols found the symbol's name with dladdr(3).
				crashlog("  %s\n", line);
			} else {
				// We don't have a symbol. Maybe this is a static symbol, and
				// we can look it up?
				symbol_table_t *symbols = NULL;
				if (backtrace_symbols[i].map_name != NULL) {
					symbols = load_symbol_table(backtrace_symbols[i].map_name);
				}
				const symbol_t *symbol = NULL;
				if (symbols != NULL) {
					symbol = find_symbol(symbols, frames[i].absolute_pc);
				}
				if (symbol != NULL) {
					int offset = frames[i].absolute_pc - symbol->start;
					crashlog("  %s (%s%+d)\n", line, symbol->name, offset);
				} else {
					crashlog("  %s\n", line);
				}
				free_symbol_table(symbols);
			}
		}

		crashlog("\nstack:\n");
		dump_stack(frame_count, frames);
	}

	free_backtrace_symbols(backtrace_symbols, frame_count);
	free(backtrace_symbols);
	free(frames);
	return frame_count;
}

static void dump_time_info(void)
{
	struct tm *ntm;
	time_t now;
	char tbuf[512];

	now = time(NULL);
	ntm = localtime(&now);

	strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S %Z", ntm);
	crashlog("Time : %s\n", tbuf);
}

static void signal_segv(int signum, siginfo_t * info, void *ptr)
{
	pid_t pid, tid;
	unwind_state_t state;
	ucontext_t *uc = (ucontext_t *) ptr;

	signal(signum, SIG_DFL);

	state.gregs[0] = uc->uc_mcontext.arm_r0;
	state.gregs[1] = uc->uc_mcontext.arm_r1;
	state.gregs[2] = uc->uc_mcontext.arm_r2;
	state.gregs[3] = uc->uc_mcontext.arm_r3;
	state.gregs[4] = uc->uc_mcontext.arm_r4;
	state.gregs[5] = uc->uc_mcontext.arm_r5;
	state.gregs[6] = uc->uc_mcontext.arm_r6;
	state.gregs[7] = uc->uc_mcontext.arm_r7;
	state.gregs[8] = uc->uc_mcontext.arm_r8;
	state.gregs[9] = uc->uc_mcontext.arm_r9;
	state.gregs[10] = uc->uc_mcontext.arm_r10;
	state.gregs[11] = uc->uc_mcontext.arm_fp;
	state.gregs[12] = uc->uc_mcontext.arm_ip;
	state.gregs[13] = uc->uc_mcontext.arm_sp;
	state.gregs[14] = uc->uc_mcontext.arm_lr;
	state.gregs[15] = uc->uc_mcontext.arm_pc;

	pid = getpid();
	tid = gettid();

	crashlog("*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\n");

	dump_time_info();
	dump_build_info();
	dump_thread_info(pid, tid, 1);
	dump_fault_addr(tid, signum, info);
	dump_registers(tid, &state);

	signal_backtrace(info, ptr);

	exit(-1);

	return;

}


/* INIT Function */

int setup_sigsegv(void)
{
	struct sigaction action;

	memset(&action, 0, sizeof(action));
	action.sa_sigaction = signal_segv;
	if (sigemptyset(&action.sa_mask) < 0)
		return -1;

	action.sa_flags = (SA_SIGINFO | SA_RESTART);
#if 0
	/* modify seodong 
	 * SA_ONSTACK: 설정시에 signal stack을 따로 쓰기 때문에 signal stack이 작아서 backtrace를 정상적으로 할수 없다.
	 * */
	action.sa_flags |= SA_ONSTACK;
#endif

	sigaction(SIGABRT, &action, NULL);
	sigaction(SIGBUS, &action, NULL);
	sigaction(SIGFPE, &action, NULL);
	sigaction(SIGILL, &action, NULL);
	sigaction(SIGPIPE, &action, NULL);
	sigaction(SIGSEGV, &action, NULL);
#if defined(SIGSTKFLT)
	sigaction(SIGSTKFLT, &action, NULL);
#endif
	sigaction(SIGTRAP, &action, NULL);
	return 0;
}
