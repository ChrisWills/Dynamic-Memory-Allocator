/*
	** Compile time options **
	
	Option						Default_Value			Description
	--------------------------------------------------------------------------------------------
	MALLOC_DEBUG				NOT DEFINED				Enables debugging functions when defined
	MALLOC_DETECT_DOUBLE_FREE	NOT_DEFINED				Enabled double free detection when defined at the expense of free() runtime performance

 */

#include <stddef.h>

void *my_malloc(size_t size);

void my_free(void *ptr);

#ifdef MALLOC_DEBUG
void print_free_list(void);
#endif
