#include "backtrace-arch.h"
#include "backtrace-helper.h"
#include "backtrace/map_info.h"
#include "backtrace/symbol_table.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <pthread.h>
#include <unwind.h>

#include <dlfcn.h>

#define ALOGV(...)   ((void)0)

void init_memory(memory_t * memory, const map_info_t * map_info_list)
{
	memory->tid = -1;
	memory->map_info_list = map_info_list;
}

void init_memory_ptrace(memory_t * memory, pid_t tid)
{
	memory->tid = tid;
	memory->map_info_list = NULL;
}

bool try_get_word(const memory_t * memory, uintptr_t ptr, uint32_t * out_value)
{
	ALOGV("try_get_word: reading word at %p", (void *)ptr);
	if (ptr & 3) {
		ALOGV("try_get_word: invalid pointer %p", (void *)ptr);
		*out_value = 0xffffffffL;
		return false;
	}
	if (memory->tid < 0) {
		if (!is_readable_map(memory->map_info_list, ptr)) {
			ALOGV("try_get_word: pointer %p not in a readable map", (void *)ptr);
			*out_value = 0xffffffffL;
			return false;
		}
		*out_value = *(uint32_t *) ptr;
		return true;
	} else {
#if defined(__APPLE__)
		ALOGV("no ptrace on Mac OS");
		return false;
#else
		// ptrace() returns -1 and sets errno when the operation fails.
		// To disambiguate -1 from a valid result, we clear errno beforehand.
		errno = 0;
		*out_value = ptrace(PTRACE_PEEKTEXT, memory->tid, (void *)ptr, NULL);
		if (*out_value == 0xffffffffL && errno) {
			ALOGV("try_get_word: invalid pointer 0x%08x reading from tid %d, "
			      "ptrace() errno=%d", ptr, memory->tid, errno);
			return false;
		}
		return true;
#endif
	}
}

static void init_backtrace_symbol(backtrace_symbol_t * symbol, uintptr_t pc)
{
	symbol->relative_pc = pc;
	symbol->relative_symbol_addr = 0;
	symbol->map_name = NULL;
	symbol->symbol_name = NULL;
	symbol->demangled_name = NULL;
}

extern char *__cxa_demangle(const char *mangled, char *buf, size_t * len, int *status);

char *demangle_symbol_name(const char *name)
{
	return __cxa_demangle(name, 0, 0, 0);
}

void get_backtrace_symbols(const backtrace_frame_t * backtrace, size_t frames, backtrace_symbol_t * backtrace_symbols)
{
	map_info_t *milist = acquire_my_map_info_list();
	size_t i;
	for (i = 0; i < frames; i++) {
		const backtrace_frame_t *frame = &backtrace[i];
		backtrace_symbol_t *symbol = &backtrace_symbols[i];
		init_backtrace_symbol(symbol, frame->absolute_pc);

		const map_info_t *mi = find_map_info(milist, frame->absolute_pc);
		if (mi) {
			symbol->relative_pc = frame->absolute_pc - mi->start;
			if (mi->name[0]) {
				symbol->map_name = strdup(mi->name);
			}
			Dl_info info;
			if (dladdr((const void *)frame->absolute_pc, &info) && info.dli_sname) {
				symbol->relative_symbol_addr = (uintptr_t) info.dli_saddr - (uintptr_t) info.dli_fbase;
				symbol->symbol_name = strdup(info.dli_sname);
				symbol->demangled_name = demangle_symbol_name(symbol->symbol_name);
			}
		}
	}
	release_my_map_info_list(milist);
}

void free_backtrace_symbols(backtrace_symbol_t * backtrace_symbols, size_t frames)
{
	size_t i;
	for (i = 0; i < frames; i++) {
		backtrace_symbol_t *symbol = &backtrace_symbols[i];
		free(symbol->map_name);
		free(symbol->symbol_name);
		free(symbol->demangled_name);
		init_backtrace_symbol(symbol, 0);
	}
}

void format_backtrace_line(unsigned frameNumber, const backtrace_frame_t * frame __attribute__ ((unused)),
			   const backtrace_symbol_t * symbol, char *buffer, size_t bufferSize)
{
	const char *mapName = symbol->map_name ? symbol->map_name : "<unknown>";
	const char *symbolName = symbol->demangled_name ? symbol->demangled_name : symbol->symbol_name;
	int fieldWidth = (bufferSize - 80) / 2;
	if (symbolName) {
		uint32_t pc_offset = symbol->relative_pc - symbol->relative_symbol_addr;
		if (pc_offset) {
			snprintf(buffer, bufferSize, "#%02u  pc %08x  %.*s (%.*s+%u)",
				 frameNumber, (unsigned int)symbol->relative_pc,
				 fieldWidth, mapName, fieldWidth, symbolName, pc_offset);
		} else {
			snprintf(buffer, bufferSize, "#%02u  pc %08x  %.*s (%.*s)",
				 frameNumber, (unsigned int)symbol->relative_pc, fieldWidth, mapName, fieldWidth, symbolName);
		}
	} else {
		snprintf(buffer, bufferSize, "#%02u  pc %08x  %.*s",
			 frameNumber, (unsigned int)symbol->relative_pc, fieldWidth, mapName);
	}
}

static bool ReadWord(uintptr_t ptr, uint32_t * out_value, map_info_t * milist)
{
	if (ptr & 3) {
		ALOGV("try_get_word: invalid pointer %p", (void *)ptr);
		*out_value = 0xffffffffL;
		return false;
	}

	if (is_readable_map(milist, ptr)) {
		*out_value = *(uint32_t *) ptr;
		return true;
	} else {
		*out_value = 0xffffffffL;
		return false;
	}
}

const char *GetFunctionNameRaw(const map_info_t * milist, uintptr_t pc, uintptr_t * offset)
{
	*offset = 0;
	Dl_info info;
	char *name = NULL;
	const map_info_t *map_info = find_map_info(milist, pc);
	if (map_info) {
		if (dladdr((const void *)pc, &info)) {
			if (info.dli_sname) {
				*offset = pc - map_info->start - (uintptr_t) info.dli_saddr + (uintptr_t) info.dli_fbase;
				return info.dli_sname;
			}
		} else {
			symbol_table_t *symbol_table = load_symbol_table(map_info->name);
			if (symbol_table) {
				const symbol_t *elf_symbol = find_symbol(symbol_table, pc - map_info->start);
				if (elf_symbol) {
					name = elf_symbol->name;
					*offset = pc - map_info->start - elf_symbol->start;
				} else if ((elf_symbol = find_symbol(symbol_table, pc)) != NULL) {
					name = elf_symbol->name;
					*offset = pc - elf_symbol->start;
				}
				free_symbol_table(symbol_table);
				return name;
			}
		}
	}
	return NULL;
}

const char *GetFunctionName(const map_info_t * milist, uintptr_t pc, uintptr_t * offset)
{
	char *func_name = (char *)GetFunctionNameRaw(milist, pc, offset);
	char *name;
	if (func_name) {
		name = demangle_symbol_name(func_name);
		if (name) {
			func_name = name;
		}
	}
	return func_name;
}

void dump_stack_segment(backtrace_frame_t * frames, uintptr_t * sp, size_t words, int label, map_info_t * milists)
{
	size_t i;
	map_info_t *milist = acquire_my_map_info_list();
	for (i = 0; i < words; i++) {
		uint32_t stack_content;
		char *map_name = NULL;

		if (!ReadWord(*sp, &stack_content, milist)) {
			break;
		}

		map_name = (char *)GetMapName(milist, stack_content);
		if (!map_name)
			map_name = "";

		uintptr_t offset = 0;
		char *func_name = NULL;
		func_name = (char *)GetFunctionName(milist, stack_content, &offset);

		if (func_name) {
			if (!i && label >= 0) {
				if (offset) {
					crashlog("    #%02d  %08x  %08x  %s (%s+%u)\n",
						 label, *sp, stack_content, map_name, func_name, offset);
				} else {
					crashlog("    #%02d  %08x  %08x  %s (%s)\n", label, *sp, stack_content, map_name,
						 func_name);
				}
			} else {
				if (offset) {
					crashlog("         %08x  %08x  %s (%s+%u)\n",
						 *sp, stack_content, map_name, func_name, offset);
				} else {
					crashlog("         %08x  %08x  %s (%s)\n", *sp, stack_content, map_name, func_name);
				}
			}
		} else {
			if (!i && label >= 0) {
				crashlog("    #%02d  %08x  %08x  %s\n", label, *sp, stack_content, map_name);
			} else {
				crashlog("         %08x  %08x  %s\n", *sp, stack_content, map_name);
			}
		}

		*sp += sizeof(uint32_t);
	}
	release_my_map_info_list(milist);
}
