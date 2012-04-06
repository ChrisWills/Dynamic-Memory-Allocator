/*
 * Title: Dynamic Memory Allocator 
 * Author: Christian Wills <cwills.dev@gmail.com>
 * License: GPLv2 (see COPYING)
 * File: malloc.h
 */

/*
	** Compile time options **
	
	Option						Default_Value			Description
	--------------------------------------------------------------------------------------------
	MALLOC_DEBUG				NOT DEFINED				Enables debugging functions when defined
	MALLOC_DETECT_DOUBLE_FREE	NOT_DEFINED				Enabled double free detection when defined at the expense of free() runtime performance

 */

#include <stddef.h>

void *malloc(size_t size);

void free(void *ptr);

#ifdef MALLOC_DEBUG
void print_free_list(void);
#endif
