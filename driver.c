/*
 * Title: Dynamic Memory Allocator 
 * Author: Christian Wills <cwills.dev@gmail.com>
 * License: GPLv2 (see COPYING)
 * File: driver.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "malloc.h"

int num_mallocs = 0;
int num_frees = 0;

/*
void* malloc(size_t sz){
    //void *(*libc_malloc)(size_t) = dlsym(RTLD_NEXT, "malloc");
	num_mallocs++;
    return my_malloc(sz);
}

void free(void *p){
    //void (*libc_free)(void*) = dlsym(RTLD_NEXT, "free");
    num_frees++;
    my_free(p);
}
*/
int main(void){
	long heap_brk_before = (long) sbrk(0);
	int i;
	char *ptrs[100];

	char *ptr1, *ptr2, *ptr3, *ptr4;

	ptr1 = malloc(100);
	ptr2 = malloc(100);
	ptr3 = malloc(100);
	ptr4 = malloc(100);

	memset(ptr1, 'c', 100);
	memset(ptr2, 'c', 100);
	memset(ptr3, 'c', 100);
	memset(ptr4, 'c', 100);

	ptr1 = realloc(ptr1, 150);
	ptr2 = realloc(ptr2, 150);
	ptr3 = realloc(ptr3, 150);
	ptr4 = realloc(ptr4, 150);
	
	memset(ptr1, 'c', 150);
	memset(ptr2, 'c', 150);
	memset(ptr3, 'c', 150);
	memset(ptr4, 'c', 150);
	
	

	free(ptr1);
	free(ptr2);
	free(ptr3);
	free(ptr4);
	
	ptr1 = malloc(80);
	ptr2 = malloc(80);
	ptr3 = malloc(80);
	ptr4 = malloc(80);

	memset(ptr1, 'c', 80);
	memset(ptr2, 'c', 80);
	memset(ptr3, 'c', 80);
	memset(ptr4, 'c', 80);

	ptr1 = realloc(ptr1, 96);
	ptr2 = realloc(ptr2, 96);
	ptr3 = realloc(ptr3, 96);
	ptr4 = realloc(ptr4, 96);
	
	memset(ptr1, 'c', 96);
	memset(ptr2, 'c', 96);
	memset(ptr3, 'c', 96);
	memset(ptr4, 'c', 96);
	
	
	free(ptr1);
	free(ptr2);
	free(ptr3);
	free(ptr4);

	printf("ptr1: %ld, ptr2: %ld, ptr3: %ld, ptr4: %ld\n", ptr1 - 48, ptr2 - 48, ptr3 - 48, ptr4 - 48);
	print_heap_chunks();
	printf("\n");

	print_free_list();

	exit(1);



	for(i=0; i < 100; i++){
		char *buf;
		int bytes = rand() % 8192;
//		printf("mallocing %d bytes\n",bytes);
		buf = malloc(bytes);
//		printf("%ld\n", (long int) buf % 8);
		ptrs[i] = buf;
	}

	for(i=0; i < 100; i++){
		//if(i != 53)
		free(ptrs[i]);
	}
	
	/*	
	for(i=99; i>=0 ; i--){
		free(ptrs[i]);
	}
*/
	printf("num_mallocs = %d, num_frees = %d\n", num_mallocs, num_frees);	
	printf("brk_before = %d, brk_after = %d, heap_size %d \n", heap_brk_before, (long) sbrk(0), (long) sbrk(0) - heap_brk_before );

	for(i=0; i < 100; i++){
		char *buf;
		int bytes = rand() % 8192;
		//printf("mallocing %d bytes\n",bytes);
		buf = malloc(bytes);
		//free(buf);
		ptrs[i] = buf;
	}
	
	for(i=0; i < 100; i++){
		//if(i != 78)
		free(ptrs[i]);
	}
/*
	char *buf;
	if( !(buf = malloc(2*1024*1024*1024))){
		struct rlimit mylim;
		printf("2gb malloc failed!\n");
		getrlimit(RLIMIT_DATA, &mylim);
		printf("RLIMIT_AS soft: %llu RLIMIT_AS hard: %llu\n",mylim.rlim_cur, mylim.rlim_max ); 
	}
*/	
	char *buf2;
	if( !(buf2 = malloc(1000000000))){
		printf("1gb malloc failed!\n");
	}
	
	char *buf4;
	if(! (buf4 = realloc(buf2, 100000001))){
		printf("realloc failed\n");
	}

	if(! (buf4 = realloc(buf2, 500))){
		printf("realloc failed\n");
	}

	free(buf2);
	/*
	char *buf3;
	if( !(buf3 = malloc(1000000000))){
		printf("1gb malloc failed!\n");
	}
*/
	printf("brk_before = %ld, brk_after = %ld, heap_size %ld \n", heap_brk_before, (long) sbrk(0), (long) sbrk(0) - heap_brk_before );
	print_free_list();
	return 0;
}
