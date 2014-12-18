#ifndef _CORKSCREW_SYMBOL_TABLE_H
#define _CORKSCREW_SYMBOL_TABLE_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		uintptr_t start;
		uintptr_t end;
		char *name;
	} symbol_t;

	typedef struct {
		symbol_t *symbols;
		size_t num_symbols;
	} symbol_table_t;

/*
 * Loads a symbol table from a given file.
 * Returns NULL on error.
 */
	symbol_table_t *load_symbol_table(const char *filename);

/*
 * Frees a symbol table.
 */
	void free_symbol_table(symbol_table_t * table);

/*
 * Finds a symbol associated with an address in the symbol table.
 * Returns NULL if not found.
 */
	const symbol_t *find_symbol(const symbol_table_t * table, uintptr_t addr);

#ifdef __cplusplus
}
#endif
#endif				// _CORKSCREW_SYMBOL_TABLE_H
