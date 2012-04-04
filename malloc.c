#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "list.h"
#include "malloc.h"

typedef struct {
	size_t size;
	struct list_head free_list;
} malloc_chunk_t;

/// Even when malloc(0) is called, at minimum the a pointer to the following number of bytes is returned
#define MIN_MAL_SIZE 8

/// The smallest chunk that can be created by a malloc call
#define MIN_SIZE (sizeof(malloc_chunk_t) + MIN_MAL_SIZE)

/// Linked list of free memmory chunks
struct list_head *free_list = NULL; 

void print_free_list(void){
	struct list_head *pos;
	
	list_for_each(pos, free_list){
		malloc_chunk_t *chunk_ptr;

		chunk_ptr = list_entry(pos, malloc_chunk_t, free_list);
		printf("chunk: size = %d\n", chunk_ptr->size);
	}
}

/**
 * sys_malloc - Increase the heap size and create a new chunk to fullfill the request
 * @size: size of requested memmory in bytes
 */
static void *sys_malloc(size_t size){
	size_t new_chunk_size;
	malloc_chunk_t *new_chunk_ptr;
	void *res_ptr;

	if(size < MIN_MAL_SIZE){
		size = MIN_MAL_SIZE;
	}

	new_chunk_size = sizeof(malloc_chunk_t) + size;
	
	/* Increase heap size */
	if( (new_chunk_ptr = sbrk(new_chunk_size)) < 0){
		return NULL;
	}
	
	/* Setup chunk metadata */
	new_chunk_ptr->size = size;
	INIT_LIST_HEAD(&(new_chunk_ptr->free_list));

	res_ptr = (void *)((char *) new_chunk_ptr + sizeof(malloc_chunk_t));
}

/**
 * my_malloc - Custom malloc() that implements the "worst fit" algo.
 * @size: size of requested memmory in bytes
 */ 
void *my_malloc(size_t size){
	// run worst fit algo
	
	// No suitable free chunks
	return (void *) sys_malloc(size);
}

void my_free(void *ptr){
	malloc_chunk_t *chunk_ptr;

	chunk_ptr = (malloc_chunk_t *) ((char *) ptr - sizeof(malloc_chunk_t));
	
	if(free_list == NULL){	
		free_list = &(chunk_ptr->free_list);
	}
	else {
		list_add(&(chunk_ptr->free_list), free_list);	
	}
	
	return;
}


