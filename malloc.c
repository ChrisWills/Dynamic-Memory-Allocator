/*
 * Title: Dynamic Memory Allocator 
 * Author: Christian Wills <cwills.dev@gmail.com>
 * License: GPLv2 (see COPYING)
 * File: malloc.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "list.h"
#include "malloc.h"

/// Fullfill all requests with the given byte alignment
#define BYTE_ALIGNMENT 8

/// Even when malloc(0) is called, at minimum the a pointer to the following number of bytes is returned
#define MIN_MAL_SIZE 8

/// Minimum brk increase in bytes
#define MIN_BRK_INCREASE 8192

/// Minimum brk decrease in bytes
#define MIN_BRK_DECREASE 8192

/// Memory chunk metadata structure
typedef struct {
	size_t prev_size;
	size_t size; 					// (mem requested + padding) + this struct overhead
	struct list_head free_list;
	short int used;					// used flag - TODO merge this into a bit field inside size
} malloc_chunk_t;

/// PAD to add to malloc_chunk_t struct so that mem returned to user is aligned
#define PAD_SIZE (BYTE_ALIGNMENT - (sizeof(malloc_chunk_t) % BYTE_ALIGNMENT))

/// Given a request size, returns the minimum chunk size to fullfill the request and maintain alignment
#define CALC_CHUNK_SIZE(size) (size + (BYTE_ALIGNMENT - (size % BYTE_ALIGNMENT)) + sizeof(malloc_chunk_t) + PAD_SIZE) 

/// Smallest chunk possible
#define MIN_CHUNK_SIZE CALC_CHUNK_SIZE(MIN_MAL_SIZE)

/// converts a pointer to a chunk to a pointer to the memmory it contains 
#define chunk2mem(chunk) (void *)(((char *) chunk) + sizeof(malloc_chunk_t) + PAD_SIZE)

/// converts a pointer to memmory to a pointer to the malloc_chunk_t that represents it
#define mem2chunk(mem) 	(malloc_chunk_t *)(((char *) mem) - sizeof(malloc_chunk_t) - PAD_SIZE)

/// Linked list of free memmory chunks
static struct list_head free_list = LIST_HEAD_INIT(free_list); 

/// Start of the heap, set on first call to malloc()
static malloc_chunk_t *heap_head = NULL;

/// Last chunk on the heap
static malloc_chunk_t *heap_tail = NULL;

/// Internal functions
static void resize_chunk(malloc_chunk_t *target_chunk, size_t size);
static void *use_free_chunk(malloc_chunk_t *target_chunk, size_t size);
static void *sys_malloc(size_t size);
static malloc_chunk_t *get_worst_fit_chunk(size_t size);
static void merge_adjacent(malloc_chunk_t *target_chunk);
static void shrink_brk(void);

#ifdef MALLOC_DEBUG
/**
 * print_free_list - Prints out "chunk <size>\n" for each chunk in the free list. Used for debugging only.
 */
void print_free_list(void){
	malloc_chunk_t *cur_chunk;
	printf("FREE LIST\n");
	printf("addr of free_list: %lu\n", &free_list);
	printf("sizeof(malloc_chunk_t) = %lu\n", sizeof(malloc_chunk_t));
	int list_len = 0;
	list_for_each_entry(cur_chunk, &free_list, free_list){
		printf("chunk: size = %ld, prev_size: %ld, used: %d, self: %ld next: %ld, prev: %ld\n", (long int) cur_chunk->size, cur_chunk->prev_size, cur_chunk->used, cur_chunk, cur_chunk->free_list.next, cur_chunk->free_list.prev);
		list_len++;
	}
	printf("list_len: %d\n", list_len);
}
/**
 * print_heap_chunks
 *
 */
void print_heap_chunks(void){
	malloc_chunk_t *cur_chunk;

	if(!heap_head){
		printf("NO HEAP YET!\n");
		return;
	}
	
	printf("HEAP CHUNKS\n");
	printf("addr of free_list: %lu\n", &free_list);
	cur_chunk = heap_head;
	size_t prev_size = 0;

	while(cur_chunk != heap_tail){
		if(prev_size != cur_chunk->prev_size){
			printf("PREV_SIZE INCORRECT!!!\n");
			exit(1);
		}
		prev_size = cur_chunk->size;
		printf("chunk: size = %ld, prev_size: %ld, used: %d, self: %ld next: %ld, prev: %ld\n", (long int) cur_chunk->size, cur_chunk->prev_size, cur_chunk->used, cur_chunk, cur_chunk->free_list.next, cur_chunk->free_list.prev);
		cur_chunk = (malloc_chunk_t *) (((char *)cur_chunk) + cur_chunk->size);	
	}

	printf("chunk: size = %ld, prev_size: %ld, used: %d, self: %ld next: %ld, prev: %ld\n", (long int) cur_chunk->size, cur_chunk->prev_size, cur_chunk->used, cur_chunk, cur_chunk->free_list.next, cur_chunk->free_list.prev);

}
#endif

/**
 * resize_chunk - shrink @target_chunk to minimal size to fullfill @size memory request
 *                and create new free chunk in the remaining space and add it to free list.
 * @target_chunk - chunk to split
 * @size - memory request to fullfill (target_chunk's new size will be CALC_CHUNK_SIZE(size))
 */
static void resize_chunk(malloc_chunk_t *target_chunk, size_t size){
		size_t new_free_chunk_size;
		malloc_chunk_t *after_new_free_chunk;
		malloc_chunk_t *new_free_chunk;

		new_free_chunk_size = target_chunk->size - CALC_CHUNK_SIZE(size);
		target_chunk->size = CALC_CHUNK_SIZE(size);
		new_free_chunk = (malloc_chunk_t *) (((char *)target_chunk) + target_chunk->size);

		if(target_chunk == heap_tail){
			heap_tail = new_free_chunk;
		}
		
		new_free_chunk->prev_size = target_chunk->size;
		new_free_chunk->size = new_free_chunk_size;
		new_free_chunk->used = false;   
		list_add(&(new_free_chunk->free_list), &free_list);	
		
		if(new_free_chunk != heap_tail){
			after_new_free_chunk = (malloc_chunk_t *)((char *)new_free_chunk + new_free_chunk->size);
			after_new_free_chunk->prev_size = new_free_chunk->size;
		}
		return;
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

	if(target_chunk->size < (CALC_CHUNK_SIZE(size))){
		return NULL;
	}

	new_free_chunk_size = target_chunk->size - CALC_CHUNK_SIZE(size);

	// If chunk is big enough split it and add unused portion back to free_list
	if(new_free_chunk_size >= MIN_CHUNK_SIZE){
		resize_chunk(target_chunk, size);
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

	// Increase heap size
	if( (new_chunk_ptr = (malloc_chunk_t *) sbrk(brk_increase)) == (void *) -1){
		return NULL;
	}

	// Setup chunk metadata 
	new_chunk_ptr->size = brk_increase;
	new_chunk_ptr->used = false;
	
	// First call to malloc(), set heap_head and heap_tail for later calls
	if(heap_head == NULL){
		heap_head = new_chunk_ptr;
		new_chunk_ptr->prev_size = 0;
		heap_tail = new_chunk_ptr;
	}
	else {
		new_chunk_ptr->prev_size = heap_tail->size;
		heap_tail = new_chunk_ptr;
	}

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
	
	// Find largest chunk that can service request, if it exists
	list_for_each_entry(cur_chunk, &free_list, free_list){
		if(cur_chunk->size >= min_chunk_size && cur_chunk->size > worst_fit_size){
			worst_fit_size = cur_chunk->size;
			worst_fit_chunk = cur_chunk;
		}
	}
	
	// If we found a suitable chunk, remove from free_list and return it
	if(worst_fit_chunk != NULL){
		__list_del_entry(&(worst_fit_chunk->free_list));
		worst_fit_chunk->used = true;
	}

	return worst_fit_chunk;
}

/**
 * merge_adjacent - Merge current free()'ed chunk with adjacent free chunks if any.
 * @target_chunk: free()'ed chunk with which to attempt merger
 */
static void merge_adjacent(malloc_chunk_t *target_chunk){
	malloc_chunk_t *prev_chunk;
	malloc_chunk_t *next_chunk;
	malloc_chunk_t *next_next_chunk;

	if(target_chunk == NULL){
		return;
	}

	// Target is the only free chunk - nothing to do
	if( (target_chunk == heap_head) && (target_chunk == heap_tail)){
		return;
	}
	
	// Chunk is not at the end of heap space, so there is def. a chunk following it
	if(target_chunk != heap_tail){
		next_chunk = (malloc_chunk_t *)((char *)target_chunk + target_chunk->size);
		
		// If next chunk is free merge with target
		if(!next_chunk->used){
			__list_del_entry(&(next_chunk->free_list));
			target_chunk->size += next_chunk->size;
			if(next_chunk == heap_tail){
				heap_tail = target_chunk;
			}
			else{
				next_next_chunk = (malloc_chunk_t *)((char *)target_chunk + target_chunk->size);
				next_next_chunk->prev_size = target_chunk->size;
			}
		}
	}
	// Chunk is not at the beginning of heap space, so there is def. a chunk preceeding it
	if(target_chunk != heap_head){
		prev_chunk = (malloc_chunk_t *)(((char *)target_chunk) - target_chunk->prev_size);
		if(!prev_chunk->used){
			__list_del_entry(&(target_chunk->free_list));
			prev_chunk->size += target_chunk->size;
			if(target_chunk == heap_tail){
				heap_tail = prev_chunk;
			}
			else {
				next_next_chunk = (malloc_chunk_t *)(((char *)prev_chunk) + prev_chunk->size);
				next_next_chunk->prev_size = prev_chunk->size;
			}
		}
	}

	return;
}

/*
 * shrink_brk - Merge contiguous tail free chunks and if the resulting chunk is > MIN_BRK_DECREASE,
 *				delete the large free chunk and shrink the heap.
 */
static void shrink_brk(void){
	malloc_chunk_t *prev_chunk = (malloc_chunk_t *) ((char *)heap_tail - heap_tail->prev_size);
	size_t shrink_counter = 0;

	if(heap_tail->used){
		return;
	}
	
	shrink_counter += heap_tail->size;

	while(!prev_chunk->used){
		shrink_counter += prev_chunk->size;
		__list_del_entry(&(heap_tail->free_list));
		prev_chunk->size += heap_tail->size;
		heap_tail = prev_chunk;
		if(heap_tail->prev_size == 0){
			break;
		}
		prev_chunk = (malloc_chunk_t *) (((char *) heap_tail) - heap_tail->prev_size);
	}
	
	if(shrink_counter >= MIN_BRK_DECREASE){
		__list_del_entry(&(heap_tail->free_list));
		
		if(heap_tail == heap_head){
			heap_tail = NULL;
			heap_head = NULL;
		}
		else {
			heap_tail = (malloc_chunk_t *) (((char *)heap_tail) - heap_tail->prev_size);
		}
		
		sbrk(-1*shrink_counter);
	}
	
	return;
}

/**
 * malloc - Custom malloc() that implements the "worst fit" algo.
 * @size: size of requested memmory in bytes
 */ 
void *malloc(size_t size){
	malloc_chunk_t *worst_fit_chunk;

	// Check request in bounds
	if(size < MIN_MAL_SIZE){
		size = MIN_MAL_SIZE;
	}
	
	// Pad size to maintain byte alignment
	size += (BYTE_ALIGNMENT - (size % BYTE_ALIGNMENT));

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
 * free -	Custom free() that works with the above custom malloc().
 *          Double free()s are detected but invalid pointers are not 
 *          and result in undefined (aka very bad) behavior.
 * @ptr: pointer to the memory block that was malloc()'ed.
 */
void free(void *ptr){
	malloc_chunk_t *target_chunk;

	if(ptr == NULL){
		return;
	}
	
	target_chunk = mem2chunk(ptr);

#ifdef MALLOC_DETECT_DOUBLE_FREE
	if(!target_chunk->used){
			fprintf(stderr, "ERROR in free(): double-free detected\n");
			exit(1);
			return;
	}
#endif
	target_chunk->used = false;
	list_add(&(target_chunk->free_list), &free_list);	

	merge_adjacent(target_chunk);

	shrink_brk();

	return;
}

void *calloc(size_t nmemb, size_t size){
	size_t tot_mem = nmemb * size;
	void *mem;
	
	if(tot_mem < MIN_MAL_SIZE){
		tot_mem = MIN_MAL_SIZE;
	}
	
	if( (mem = malloc(tot_mem)) != NULL){
		memset(mem, '\0', tot_mem);
		return mem; 
	}
	else {
		return NULL;
	}
}

void *realloc(void *ptr, size_t size){
	malloc_chunk_t *target_chunk;
	size_t new_chunk_size;
	void *mem;
	
	if(ptr == NULL){
		return malloc(size);
	}
	
	if(size == 0){
		free(ptr);
		return NULL;
	}

	if(size < MIN_MAL_SIZE){
		size = MIN_MAL_SIZE;
	}

	target_chunk = mem2chunk(ptr);
	new_chunk_size = CALC_CHUNK_SIZE(size);

	if(target_chunk->size >= (new_chunk_size + MIN_CHUNK_SIZE)){
		// Shrink chunk and free extra space
		resize_chunk(target_chunk, size);
		return chunk2mem(target_chunk);
	}
	else if(target_chunk->size > new_chunk_size && target_chunk->size < (new_chunk_size + MIN_CHUNK_SIZE)){
		// Enough space in current chunk, do nothing
		return chunk2mem(target_chunk);
	}
	else {
		// Need a new larger chunk
		void *new_mem = malloc(size);
	
		if(new_mem == NULL){
			return NULL;
		}
	
		memcpy(new_mem, ptr, target_chunk->size - sizeof(malloc_chunk_t) - PAD_SIZE);

		free(ptr);

		return new_mem;
	}
}
