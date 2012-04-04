#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "list.h"
#include "malloc.h"

/// memmory chunk metadata structure
typedef struct {
	size_t size; 					// mem requested + this struct overhead
	struct list_head free_list;
} malloc_chunk_t;

/// Even when malloc(0) is called, at minimum the a pointer to the following number of bytes is returned
#define MIN_MAL_SIZE 8

/// Minimum brk increase in bytes
#define MIN_BRK_INCREASE 8192

/// The smallest chunk that can be created by a malloc call
#define MIN_SIZE (sizeof(malloc_chunk_t) + MIN_MAL_SIZE)

/// Linked list of free memmory chunks
static struct list_head free_list = LIST_HEAD_INIT(free_list); 

/**
 * print_free_list - Prints out "chunk <size>\n" for each chunk in the free list. Used for debugging only.
 */
void print_free_list(void){
	struct list_head *pos;
	malloc_chunk_t *cur_chunk;

	printf("sizeof(malloc_chunk_t) = %d\n", sizeof(malloc_chunk_t));

	list_for_each_entry(cur_chunk, &free_list, free_list){
		printf("chunk: size = %d\n", cur_chunk->size);
	}
}

/**
 * use_free_chunk - Given a free_chunk of adequate size, fullfill the size request with that chunk.
 * 					Split the chunk and put the unused portion back in the free_list if possible.
 * @free_chunk: adequate sized free chunk from free list
 * @size: size of request
 */
static void *use_free_chunk(malloc_chunk_t *target_chunk, size_t size){
	size_t new_free_chunk_size;
	malloc_chunk_t *new_free_chunk;

	if(target_chunk == NULL){
		return NULL;
	}

	if(target_chunk->size < (size + sizeof(malloc_chunk_t))){
		return NULL;
	}

	new_free_chunk_size = target_chunk->size - (size + sizeof(malloc_chunk_t));

	// If chunk is big enough split it and add unused portion back to free_list
	if(new_free_chunk_size >= (MIN_MAL_SIZE + sizeof(malloc_chunk_t))){
		target_chunk->size = size + sizeof(malloc_chunk_t);
		new_free_chunk = (malloc_chunk_t *) ((char *)target_chunk + target_chunk->size);
		new_free_chunk->size = new_free_chunk_size;
		list_add(&(new_free_chunk->free_list), &free_list);	
	}

	return (void *) ((char *) target_chunk + sizeof(malloc_chunk_t));
}

/**
 * sys_malloc - Increase the heap size and create a new chunk to fullfill the request
 * @size: size of requested memmory in bytes
 */
static void *sys_malloc(size_t size){
	size_t new_chunk_size;
	malloc_chunk_t *new_chunk_ptr;
	void *res_ptr;
	size_t brk_increase;

	new_chunk_size = sizeof(malloc_chunk_t) + size;

	if(new_chunk_size <= MIN_BRK_INCREASE){
		brk_increase = MIN_BRK_INCREASE;
	}
	else{
		brk_increase = new_chunk_size;
	}

	/* Increase heap size */
	if( (new_chunk_ptr = (malloc_chunk_t *) sbrk(brk_increase)) < 0){
		return NULL;
	}

	/* Setup chunk metadata */
	new_chunk_ptr->size = brk_increase;
	//list_add(&(new_chunk_ptr->free_list), &free_list);	
	
	return (void *) use_free_chunk(new_chunk_ptr, size);
}

static malloc_chunk_t *get_worst_fit_chunk(size_t size){
	size_t min_chunk_size = size + sizeof(malloc_chunk_t);
	size_t worst_fit_size = 0;
	malloc_chunk_t *worst_fit_chunk = NULL;
	malloc_chunk_t *cur_chunk;

	list_for_each_entry(cur_chunk, &free_list, free_list){
		if(cur_chunk->size >= min_chunk_size && cur_chunk->size > worst_fit_size){
			worst_fit_size = cur_chunk->size;
			worst_fit_chunk = cur_chunk;
		}
	}

	if(worst_fit_chunk != NULL){
		__list_del_entry(&(worst_fit_chunk->free_list));
	}

	return worst_fit_chunk;
}




/**
 * my_malloc - Custom malloc() that implements the "worst fit" algo.
 * @size: size of requested memmory in bytes
 */ 
void *my_malloc(size_t size){
	malloc_chunk_t *worst_fit_chunk;
	
	// Check request in bounds
	if(size < MIN_MAL_SIZE){
		size = MIN_MAL_SIZE;
	}

	printf("malloc input size = %d\n", size);

	// Try to find a free chunk to fullfill request
	if( (worst_fit_chunk = get_worst_fit_chunk(size)) == NULL){
 		// No free chunks work, increase brk, return ptr to new mem
		return (void *) sys_malloc(size);
	}
	else {
 		// Found a free chunk, use it to fullfill request, split if possible 	
		return (void *) use_free_chunk(worst_fit_chunk, size);
	}
}

/**
 * my_free - Custom free() that works with the above custom malloc().
 * @ptr: pointer to the memory block that was malloc()'ed.
 */
void my_free(void *ptr){
	malloc_chunk_t *chunk_ptr = (malloc_chunk_t *) ((char *) ptr - sizeof(malloc_chunk_t));

	list_add(&(chunk_ptr->free_list), &free_list);	

	return;
}


