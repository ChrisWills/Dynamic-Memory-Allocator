#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "list.h"
#include "malloc.h"

/// Fullfill all requests with the given byte alignment
#define BYTE_ALIGNMENT 8

/// Even when malloc(0) is called, at minimum the a pointer to the following number of bytes is returned
#define MIN_MAL_SIZE 8

/// Minimum brk increase in bytes
#define MIN_BRK_INCREASE 8192

/// memmory chunk metadata structure
typedef struct {
	size_t size; 					// mem requested + this struct overhead
	struct list_head free_list;
	short int used;
} malloc_chunk_t;

// PAD to add to malloc_chunk_t struct so that mem returned to user is aligned
#define PAD_SIZE (BYTE_ALIGNMENT - (sizeof(malloc_chunk_t) % BYTE_ALIGNMENT))

// Given a request size, returns the minimum chunk size to fullfill the request and maintain alignment
#define CALC_CHUNK_SIZE(size) (size + (BYTE_ALIGNMENT - (size % BYTE_ALIGNMENT)) + sizeof(malloc_chunk_t) + PAD_SIZE) 

// Smallest chunk possible
#define MIN_CHUNK_SIZE CALC_CHUNK_SIZE(MIN_MAL_SIZE)

// converts a pointer to a chunk to a pointer to the memmory it contains 
#define chunk2mem(chunk) (void *)(((char *) chunk) + sizeof(malloc_chunk_t) + PAD_SIZE)

// converts a pointer to memmory to a pointer to the malloc_chunk_t that represents it
#define mem2chunk(mem) 	(malloc_chunk_t *)(((char *) mem) - sizeof(malloc_chunk_t) - PAD_SIZE)

/// Linked list of free memmory chunks
static struct list_head free_list = LIST_HEAD_INIT(free_list); 

/// Start of the heap, set on first call to malloc()
static void *heap_start = NULL;


#ifdef MALLOC_DEBUG
/**
 * print_free_list - Prints out "chunk <size>\n" for each chunk in the free list. Used for debugging only.
 */
void print_free_list(void){
	malloc_chunk_t *cur_chunk;

	printf("sizeof(malloc_chunk_t) = %lu\n", sizeof(malloc_chunk_t));
	int list_len = 0;
	list_for_each_entry(cur_chunk, &free_list, free_list){
		printf("chunk: size = %ld, used: %d, self: %ld next: %ld, prev: %ld\n", (long int) cur_chunk->size, cur_chunk->used, cur_chunk, cur_chunk->free_list.next, cur_chunk->free_list.prev);
		list_len++;
	}
	printf("list_len: %d\n", list_len);
}
#endif

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

	if(target_chunk->size < (CALC_CHUNK_SIZE(size))){
		return NULL;
	}

	new_free_chunk_size = target_chunk->size - CALC_CHUNK_SIZE(size);

	// If chunk is big enough split it and add unused portion back to free_list
	if(new_free_chunk_size >= MIN_CHUNK_SIZE){
		target_chunk->size = CALC_CHUNK_SIZE(size);
		new_free_chunk = (malloc_chunk_t *) ((char *)target_chunk + target_chunk->size);
		new_free_chunk->size = new_free_chunk_size;
		new_free_chunk->used = false;   
		list_add(&(new_free_chunk->free_list), &free_list);	
	}

	target_chunk->used = true;
	return chunk2mem(target_chunk);
}

/**
 * sys_malloc - Increase the heap size and create a new chunk to fullfill the request
 * @size: size of requested memmory in bytes
 */
static void *sys_malloc(size_t size){
	size_t new_chunk_size;
	malloc_chunk_t *new_chunk_ptr;
	size_t brk_increase;

	new_chunk_size = CALC_CHUNK_SIZE(size);

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
	
	return (void *) use_free_chunk(new_chunk_ptr, size);
}

/**
 * get_worst_fit_chunk - Find the largest chunk that is at least large enough to fullfill @size request.
 * 						 If no chunk is found, return NULL.
 * @size: size of memmory request
 */
static malloc_chunk_t *get_worst_fit_chunk(size_t size){
	size_t min_chunk_size = CALC_CHUNK_SIZE(size);
	size_t worst_fit_size = 0;
	malloc_chunk_t *worst_fit_chunk = NULL;
	malloc_chunk_t *cur_chunk;
	
	// find largest chunk that can service request, if it exists
	list_for_each_entry(cur_chunk, &free_list, free_list){
		if(cur_chunk->size >= min_chunk_size && cur_chunk->size > worst_fit_size){
			worst_fit_size = cur_chunk->size;
			worst_fit_chunk = cur_chunk;
		}
	}
	
	// if we found a suitable chunk, remove from free_list and return it
	if(worst_fit_chunk != NULL){
		__list_del_entry(&(worst_fit_chunk->free_list));
	}

	// return NULL if no suitable chunk was found
	return worst_fit_chunk;
}

/**
 * merge_adjacent - Merge current free()'ed chunk with adjacent free chunks if any.
 * @target_chunk: free()'ed chunk with which to attempt merger
 * FIXME: currently can only merge with chunks following target in heap
 */
void merge_adjacent(malloc_chunk_t *target_chunk){
	malloc_chunk_t *prev_chunk;
	malloc_chunk_t *next_chunk;
	bool target_is_first;
	bool target_is_last;

	if(target_chunk == NULL){
		return;
	}

	target_is_first = (target_chunk == heap_start); 
	target_is_last = (((char *)target_chunk + target_chunk->size) == sbrk(0));

	// target is the only free chunk - nothing to do
	if(target_is_first && target_is_last){
		return;
	}
	
	// chunk is not at the end of heap space, so there is def. a chunk following it
	if(!target_is_last){
		next_chunk = (malloc_chunk_t *)((char *)target_chunk + target_chunk->size);
		
		// if next chunk is free merge with target
		if(next_chunk->used != true){
			__list_del_entry(&(next_chunk->free_list));
			target_chunk->size += next_chunk->size;
		}
	}
	
	/*
	if(!target_is_first){
		// TODO - malloc_chunk_t needs size of previous chunk for this to work.	
	}
	*/

	return;
}


/**
 * my_malloc - Custom malloc() that implements the "worst fit" algo.
 * @size: size of requested memmory in bytes
 */ 
void *my_malloc(size_t size){
	int sz;
	malloc_chunk_t *worst_fit_chunk;
	
	// set heap start for later
	if(heap_start == NULL){
		heap_start = sbrk(0);
	}

	// Check request in bounds
	if(size < MIN_MAL_SIZE){
		sz = MIN_MAL_SIZE;
	}
	else {
		sz = size;
	}
	
	// Pad size to maintain byte alignment
	sz += (BYTE_ALIGNMENT - (sz % BYTE_ALIGNMENT));

	// Try to find a free chunk to fullfill request
	if( (worst_fit_chunk = get_worst_fit_chunk(sz)) == NULL){
 		// No free chunks work, increase brk, return ptr to new mem
		return (void *) sys_malloc(sz);
	}
	else {
 		// Found a free chunk, use it to fullfill request, split if possible 	
		return (void *) use_free_chunk(worst_fit_chunk, sz);
	}
}

/**
 * my_free -	Custom free() that works with the above custom malloc().
 *				Double free()s are detected but invalid pointers are not 
 *				and result in undefined (aka very bad) behavior.
 * @ptr: pointer to the memory block that was malloc()'ed.
 */
void my_free(void *ptr){
	malloc_chunk_t *target_chunk;
	malloc_chunk_t *cur_chunk;

	if(ptr == NULL){
		return;
	}
	
	target_chunk = mem2chunk(ptr);

#ifdef MALLOC_DETECT_DOUBLE_FREE
	if(target_chunk->used == false){
			fprintf(stderr, "ERROR in my_free(): double-free detected\n");
			exit(1);
			return;
	}
#endif
	target_chunk->used = false;
	list_add(&(target_chunk->free_list), &free_list);	

	merge_adjacent(target_chunk);

	return;
}


